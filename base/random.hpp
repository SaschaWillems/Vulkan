/*
* Basic C++11 random number helper
*
* Copyright (C) 2024 by Bradley Austin Davis
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/
#pragma once
#include <random>
#include <cmath>
#include <glm/glm.hpp>

namespace vkx {

	constexpr float M_TAU = static_cast<float>(M_PI * 2.0f);

	class Random {
		std::default_random_engine gen;


	public:
		Random(uint32_t seed = std::random_device{}())
			: gen{ seed } {};

		void seed(uint32_t seed = std::random_device{}()) {
			gen = std::default_random_engine{ seed };
		}

		float real(float range = 1.0f) { return real(0.0f, range); }

		float real(float min, float max) {
			std::uniform_real_distribution<float> dst{ min, max };
			return dst(gen);
		}

		float radian() { return real(M_TAU); }

		float degree() { return real(360.0f); }

		bool boolean() {
			std::uniform_int_distribution<unsigned int> dst{ 0, 1 };
			return dst(gen) == 1;
		}

		glm::vec3 color() { return glm::vec3(real(), real(), real()); }

		glm::vec2 polar() {
			float theta = radian();
			float phi = acos(real(-1.0, 1.0));
			return glm::vec2(phi, theta);
		}

		glm::vec3 sphere(const glm::vec3& scale = glm::vec3{ 1.0f }) {
			const auto phiTheta = polar();
			const auto& phi = phiTheta.x;
			const auto& theta = phiTheta.y;
			const auto sinPhi = sin(phi);
			return glm::vec3{ sinPhi* cos(theta), sinPhi* sin(theta), cos(phi) } *scale;
		}

	};
}