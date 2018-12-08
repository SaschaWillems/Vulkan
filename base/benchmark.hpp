/*
* Benchmark class
*
* Copyright (C) 2016-2017 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include <vector>
#include <string>
#include <algorithm>
#include <limits>
#include <functional>
#include <chrono>
#include <iomanip>

namespace vks
{
	class Benchmark {
	private:
		FILE *stream;
		VkPhysicalDeviceProperties deviceProps;
	public:
		bool active = false;
		bool outputFrameTimes = false;
		uint32_t warmup = 1;
		uint32_t duration = 10;
		std::vector<double> frameTimes;
		std::string filename = "";

		double runtime = 0.0;
		uint32_t frameCount = 0;

		void run(std::function<void()> renderFunc, VkPhysicalDeviceProperties deviceProps) {
			active = true;
			this->deviceProps = deviceProps;
#if defined(_WIN32)
			AttachConsole(ATTACH_PARENT_PROCESS);
			freopen_s(&stream, "CONOUT$", "w+", stdout);
			freopen_s(&stream, "CONOUT$", "w+", stderr);
#endif
			std::cout << std::fixed << std::setprecision(3);

			// Warm up phase to get more stable frame rates
			{
				double tMeasured = 0.0;
				while (tMeasured < (warmup * 1000)) {
					auto tStart = std::chrono::high_resolution_clock::now();
					renderFunc();
					auto tDiff = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - tStart).count();
					tMeasured += tDiff;
				};
			}

			// Benchmark phase
			{
				while (runtime < (duration * 1000.0)) {
					auto tStart = std::chrono::high_resolution_clock::now();
					renderFunc();
					auto tDiff = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - tStart).count();
					runtime += tDiff;
					frameTimes.push_back(tDiff);
					frameCount++;
				};
				std::cout << "Benchmark finished" << std::endl;
				std::cout << "device : " << deviceProps.deviceName << " (driver version: " << deviceProps.driverVersion << ")" << std::endl;
				std::cout << "runtime: " << (runtime / 1000.0) << std::endl;
				std::cout << "frames : " << frameCount << std::endl;
				std::cout << "fps    : " << frameCount / (runtime / 1000.0) << std::endl;
			}
		}

		void saveResults() {
			std::ofstream result(filename, std::ios::out);
			if (result.is_open()) {
				result << std::fixed << std::setprecision(4);

				result << "device,driverversion,duration (ms),frames,fps" << std::endl;
				result << deviceProps.deviceName << "," << deviceProps.driverVersion << "," << runtime << "," << frameCount << "," << frameCount / (runtime / 1000.0) << std::endl;

				if (outputFrameTimes) {
					result << std::endl << "frame,ms" << std::endl;
					for (size_t i = 0; i < frameTimes.size(); i++) {
						result << i << "," << frameTimes[i] << std::endl;
					}
					double tMin = *std::min_element(frameTimes.begin(), frameTimes.end());
					double tMax = *std::max_element(frameTimes.begin(), frameTimes.end());
					double tAvg = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0) / (double)frameTimes.size();
					std::cout << "best   : " << (1000.0 / tMin) << " fps (" << tMin << " ms)" << std::endl;
					std::cout << "worst  : " << (1000.0 / tMax) << " fps (" << tMax << " ms)" << std::endl;
					std::cout << "avg    : " << (1000.0 / tAvg) << " fps (" << tAvg << " ms)" << std::endl;
					std::cout << std::endl;
				}

				result.flush();
#if defined(_WIN32)
				FreeConsole();
#endif
			}
		}
	};
}