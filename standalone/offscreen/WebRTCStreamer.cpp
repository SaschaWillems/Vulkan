#include "WebRTCStreamer.h"

#include <nlohmann/json.hpp>
#include <rtc/rtc.hpp>

#include <algorithm>
#include <atomic>
#include <deque>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <stdexcept>
#include <utility>

using Json = nlohmann::json;

bool ApplyControlMessage(CameraState& camera, std::string_view message, RenderControls* render) {
    if (message.size() > 512) return false;
    const auto value = Json::parse(message, nullptr, false);
    if (!value.is_object() || !value.contains("type") || !value["type"].is_string()) return false;
    if (value["type"] == "rotate" || value["type"] == "pan") {
        if (!value.contains("dx") || !value["dx"].is_number() ||
            !value.contains("dy") || !value["dy"].is_number()) return false;
        return value["type"] == "pan" ? camera.Pan(value["dx"].get<double>(), value["dy"].get<double>()) :
            camera.Rotate(value["dx"].get<double>(), value["dy"].get<double>());
    }
    if (value["type"] == "zoom") {
        if (!value.contains("delta") || !value["delta"].is_number()) return false;
        return camera.Zoom(value["delta"].get<double>());
    }
    if (value["type"] == "set_render_mode") {
        if (!render || !value.contains("mode") || !value["mode"].is_string()) return false;
        const auto mode = ParseRenderMode(value["mode"].get<std::string>());
        return mode && render->Set(*mode);
    }
    if (value["type"] == "reset") { camera.Reset(); return true; }
    return false;
}

namespace {
constexpr size_t maxClients = 4;
constexpr size_t maxSignalingMessages = 128;

struct Session : std::enable_shared_from_this<Session> {
    std::shared_ptr<rtc::WebSocket> socket;
    std::shared_ptr<rtc::PeerConnection> peer;
    std::shared_ptr<rtc::Track> video;
    std::shared_ptr<rtc::DataChannel> controls;
    std::atomic<bool> closed{false};
    std::atomic<bool> keyframe{true};
    std::mutex mutex;
    std::deque<std::string> messages;
    std::vector<rtc::Candidate> pendingCandidates;
    bool answered = false;
    const std::chrono::steady_clock::time_point created = std::chrono::steady_clock::now();

    ~Session() {
        // Callbacks use weak ownership; closing never runs under the inbox mutex.
        if (socket) socket->resetCallbacks();
        if (controls) controls->resetCallbacks();
        if (video) video->resetCallbacks();
        if (peer) peer->resetCallbacks();
        try { if (peer) peer->close(); } catch (...) {}
        try { if (socket) socket->close(); } catch (...) {}
    }

    void Signal(const Json& message) noexcept {
        try {
            if (socket->isOpen()) socket->send(message.dump());
        } catch (...) { closed.store(true); }
    }

    void SendCamera(const CameraState& camera) noexcept {
        try {
            const auto pose = camera.Load();
            if (controls->isOpen() && controls->bufferedAmount() < 4096)
                controls->send(Json{{"type", "camera"}, {"yaw", pose.yaw},
                    {"pitch", pose.pitch}, {"distance", pose.distance},
                    {"target", {pose.target.x,pose.target.y,pose.target.z}}}.dump());
        } catch (...) { closed.store(true); }
    }

    void SendRenderMode(const RenderControls& render, bool accepted = true) noexcept {
        try {
            if (controls->isOpen()) controls->send(Json{{"type","render_mode"},
                {"mode",RenderModeName(render.Load().mode)}, {"accepted",accepted},
                {"rayQueries",render.SupportsRayQueries()}, {"samples",render.samples.load()},
                {"reason",accepted ? "" : "Mode unsupported or invalid; hardware Vulkan ray queries are required"}}.dump());
        } catch (...) { closed.store(true); }
    }
    void AttachSocket() {
        const std::weak_ptr<Session> weak = shared_from_this();
        socket->onClosed([weak] { if (auto self = weak.lock()) self->closed.store(true); });
        socket->onError([weak](std::string error) {
            std::cerr << "Signaling: " << error << '\n';
            if (auto self = weak.lock()) self->closed.store(true);
        });
        socket->onMessage([weak](rtc::message_variant data) {
            if (auto self = weak.lock()) {
                try {
                    if (auto* text = std::get_if<std::string>(&data)) {
                        std::lock_guard lock(self->mutex);
                        if (text->size() > 65536 || self->messages.size() >= maxSignalingMessages)
                            self->closed.store(true);
                        else self->messages.push_back(std::move(*text));
                    }
                } catch (...) { self->closed.store(true); }
            }
        });
    }

    void Start(const rtc::Configuration& configuration, const std::shared_ptr<CameraState>& camera,
               const WebRTCStreamer::Options& options) {
        if (peer) throw std::runtime_error("Session already started");
        peer = std::make_shared<rtc::PeerConnection>(configuration);
        const std::weak_ptr<Session> weak = shared_from_this();
        peer->onStateChange([weak](rtc::PeerConnection::State state) {
            if (auto self = weak.lock()) {
                if (state == rtc::PeerConnection::State::Failed || state == rtc::PeerConnection::State::Closed)
                    self->closed.store(true);
            }
        });
        peer->onLocalDescription([weak](rtc::Description description) {
            if (auto self = weak.lock()) {
                try { self->Signal({{"type", description.typeString()}, {"sdp", std::string(description)}}); }
                catch (...) { self->closed.store(true); }
            }
        });
        peer->onLocalCandidate([weak](rtc::Candidate candidate) {
            if (auto self = weak.lock()) {
                try { self->Signal({{"type", "candidate"}, {"candidate", std::string(candidate)}, {"mid", candidate.mid()}}); }
                catch (...) { self->closed.store(true); }
            }
        });
        rtc::Description::Video description("video", rtc::Description::Direction::SendOnly);
        if (options.codec == VideoCodec::AV1) {
            description.addAV1Codec(96, "profile=0;level-idx=17;tier=0");
        } else {
            std::ostringstream profile;
            profile << "profile-level-id=42e0" << std::hex << std::setfill('0') << std::setw(2)
                    << options.h264Level << ";packetization-mode=1;level-asymmetry-allowed=1";
            description.addH264Codec(96, profile.str());
        }
        description.addSSRC(42, "offscreen", "offscreen-stream", "offscreen-video");
        video = peer->addTrack(description);
        auto config = std::make_shared<rtc::RtpPacketizationConfig>(42, "offscreen", 96,
                                                                  rtc::H264RtpPacketizer::ClockRate);
        std::shared_ptr<rtc::RtpPacketizer> packetizer;
        if (options.codec == VideoCodec::AV1)
            packetizer = std::make_shared<rtc::AV1RtpPacketizer>(rtc::AV1RtpPacketizer::Packetization::TemporalUnit, config);
        else packetizer = std::make_shared<rtc::H264RtpPacketizer>(rtc::NalUnit::Separator::StartSequence, config);
        packetizer->addToChain(std::make_shared<rtc::RtcpSrReporter>(config));
        packetizer->addToChain(std::make_shared<rtc::RtcpNackResponder>());
        packetizer->addToChain(std::make_shared<rtc::PliHandler>([weak] {
            if (auto self = weak.lock()) self->keyframe.store(true);
        }));
        video->setMediaHandler(packetizer);
        video->onOpen([weak] { if (auto self = weak.lock()) self->keyframe.store(true); });
        controls = peer->createDataChannel("controls");
        const auto render = options.renderControls;
        controls->onOpen([weak, camera, render] {
            if (auto self = weak.lock()) { self->SendCamera(*camera); self->SendRenderMode(*render); }
        });
        controls->onMessage([weak, camera, render](rtc::message_variant data) {
            if (auto self = weak.lock()) {
                try {
                    const auto* text = std::get_if<std::string>(&data);
                    if (!text || text->size() > 512) return;
                    const bool accepted = ApplyControlMessage(*camera, *text, render.get());
                    const auto message = Json::parse(*text,nullptr,false);
                    if (message.is_object() && message.value("type",Json{}) == "set_render_mode") {
                        self->SendRenderMode(*render,accepted);
                        return;
                    }
                    if (accepted) self->SendCamera(*camera);
                } catch (...) { self->closed.store(true); }
            }
        });
        peer->setLocalDescription(rtc::Description::Type::Offer);
    }

    void Process(const rtc::Configuration& configuration, const std::shared_ptr<CameraState>& camera,
                 const WebRTCStreamer::Options& options) {
        std::deque<std::string> incoming;
        {
            std::lock_guard lock(mutex);
            incoming.swap(messages);
        }
        for (const auto& text : incoming) {
            const auto message = Json::parse(text);
            const auto type = message.at("type").get<std::string>();
            if (type == "start") {
                Start(configuration, camera, options);
            } else if (type == "answer" && peer && !answered) {
                peer->setRemoteDescription(rtc::Description(message.at("sdp").get<std::string>(), "answer"));
                answered = true;
                for (const auto& candidate : pendingCandidates) peer->addRemoteCandidate(candidate);
                pendingCandidates.clear();
            } else if (type == "candidate" && peer) {
                const auto candidate = message.at("candidate").get<std::string>();
                if (candidate.empty()) continue; // End-of-candidates needs no rtc::Candidate.
                rtc::Candidate ice(candidate, message.at("mid").get<std::string>());
                if (answered) peer->addRemoteCandidate(ice);
                else if (pendingCandidates.size() < 64) pendingCandidates.push_back(std::move(ice));
                else throw std::runtime_error("Too many pending ICE candidates");
            } else throw std::runtime_error("Unexpected signaling message");
        }
        if ((!video || !video->isOpen()) && std::chrono::steady_clock::now() - created > std::chrono::seconds(20))
            closed.store(true);
    }
};

struct AcceptQueue {
    std::mutex mutex;
    bool stopped = false;
    std::deque<std::shared_ptr<rtc::WebSocket>> sockets;
};
} // namespace

struct WebRTCStreamer::Impl {
    std::shared_ptr<CameraState> camera;
    rtc::Configuration configuration;
    Options options;
    std::shared_ptr<AcceptQueue> incoming = std::make_shared<AcceptQueue>();
    std::unique_ptr<rtc::WebSocketServer> server;
    std::vector<std::shared_ptr<Session>> sessions;

    Impl(std::shared_ptr<CameraState> state, const Options& settings) : camera(std::move(state)), options(settings) {
        if (!camera) throw std::invalid_argument("WebRTCStreamer requires a camera");
        if (!options.renderControls) throw std::invalid_argument("WebRTCStreamer requires render controls");
        configuration.disableAutoNegotiation = true;
        configuration.maxMessageSize = 4096;
        for (const auto& url : options.iceServers) configuration.iceServers.emplace_back(url);
        rtc::WebSocketServer::Configuration ws;
        ws.port = options.port;
        ws.bindAddress = options.bindAddress;
        ws.maxMessageSize = 65536;
        ws.connectionTimeout = std::chrono::seconds(10);
        server = std::make_unique<rtc::WebSocketServer>(ws);
        const std::weak_ptr<AcceptQueue> weak = incoming;
        server->onClient([weak](std::shared_ptr<rtc::WebSocket> socket) {
            try {
                if (auto queue = weak.lock()) {
                    std::lock_guard lock(queue->mutex);
                    if (!queue->stopped && queue->sockets.size() < maxClients) {
                        queue->sockets.push_back(std::move(socket));
                        return;
                    }
                }
                socket->close();
            } catch (...) { /* A rejected socket's destructor closes it. */ }
        });
    }

    ~Impl() {
        std::deque<std::shared_ptr<rtc::WebSocket>> pending;
        {
            std::lock_guard lock(incoming->mutex);
            incoming->stopped = true;
            pending.swap(incoming->sockets);
        }
        server->stop();
        server.reset();
        for (auto& socket : pending) { try { socket->close(); } catch (...) {} }
        sessions.clear();
    }
};

WebRTCStreamer::WebRTCStreamer(std::shared_ptr<CameraState> camera, const Options& options)
    : impl_(std::make_unique<Impl>(std::move(camera), options)) {}
WebRTCStreamer::~WebRTCStreamer() = default;
uint16_t WebRTCStreamer::Port() const { return impl_->server->port(); }

void WebRTCStreamer::Poll() {
    auto& state = *impl_;
    std::erase_if(state.sessions, [](const auto& session) { return session->closed.load(); });
    std::deque<std::shared_ptr<rtc::WebSocket>> pending;
    {
        std::lock_guard lock(state.incoming->mutex);
        pending.swap(state.incoming->sockets);
    }
    for (auto& socket : pending) {
        if (state.sessions.size() >= maxClients) { socket->close(); continue; }
        auto session = std::make_shared<Session>();
        session->socket = std::move(socket);
        session->AttachSocket();
        state.sessions.push_back(std::move(session));
    }
    for (const auto& session : state.sessions) {
        try { session->Process(state.configuration, state.camera, state.options); }
        catch (const std::exception& error) {
            std::cerr << "Closing peer: " << error.what() << '\n';
            session->Signal({{"type", "error"}, {"message", "Invalid signaling or negotiation failed"}});
            session->closed.store(true);
        }
    }
}

bool WebRTCStreamer::HasViewers() const {
    return std::any_of(impl_->sessions.begin(), impl_->sessions.end(), [](const auto& session) {
        return !session->closed.load() && session->video && session->video->isOpen();
    });
}

bool WebRTCStreamer::ConsumeKeyFrameRequest() {
    bool needed = false;
    for (const auto& session : impl_->sessions) {
        if (session->video && session->video->isOpen()) needed |= session->keyframe.exchange(false);
    }
    return needed;
}

void WebRTCStreamer::SendFrame(std::span<const uint8_t> encoded, std::chrono::duration<double> captureTime) {
    if (encoded.empty()) return;
    for (const auto& session : impl_->sessions) {
        if (session->closed.load() || !session->video || !session->video->isOpen()) continue;
        try {
            // libdatachannel converts capture time to its randomized 90 kHz RTP timestamp base.
            session->video->sendFrame(reinterpret_cast<const rtc::byte*>(encoded.data()), encoded.size(),
                                      rtc::FrameInfo(captureTime));
        } catch (const std::exception& error) {
            std::cerr << "Video send: " << error.what() << '\n';
            session->closed.store(true);
        }
    }
}
