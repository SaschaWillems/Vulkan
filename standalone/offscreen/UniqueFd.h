#pragma once
#include <unistd.h>
#include <utility>

class UniqueFd {
public:
    explicit UniqueFd(int fd = -1) noexcept : fd_(fd) {}
    ~UniqueFd() { if (fd_ >= 0) (void)::close(fd_); }
    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;
    UniqueFd(UniqueFd&& other) noexcept : fd_(other.Release()) {}
    UniqueFd& operator=(UniqueFd&& other) noexcept {
        if (this != &other) { UniqueFd old(Release()); fd_ = other.Release(); }
        return *this;
    }
    int Get() const noexcept { return fd_; }
    int Release() noexcept { return std::exchange(fd_, -1); }
private:
    int fd_;
};
