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
	public:
		bool active = false;
		uint32_t framesPerIteration = 1000;
		uint32_t iterations = 10;
		std::vector<double> iterationTimes;
		std::string filename = "benchmarkresults.csv";

		void run(std::function<void()> renderFunc) {
			active = true;
#if defined(_WIN32)
			AttachConsole(ATTACH_PARENT_PROCESS);
			FILE *stream;
			freopen_s(&stream, "CONOUT$", "w+", stdout);
			freopen_s(&stream, "CONOUT$", "w+", stderr);
#endif
			iterationTimes.resize(iterations);
			for (uint32_t i = 0; i < iterations; i++) {
				for (uint32_t f = 0; f < framesPerIteration; f++) {
					auto tStart = std::chrono::high_resolution_clock::now();
					renderFunc();
					auto tEnd = std::chrono::high_resolution_clock::now();
					auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
					double frameTime = (double)tDiff / 1000.0;
					iterationTimes[i] += frameTime;
				}
			}
		}

		void saveResults(std::string appinfo, std::string deviceinfo) {
			std::ofstream result(filename, std::ios::out);
			if (result.is_open()) {

				double tMin = *std::min_element(iterationTimes.begin(), iterationTimes.end());
				double tMax = *std::max_element(iterationTimes.begin(), iterationTimes.end());
				double tAvg = std::accumulate(iterationTimes.begin(), iterationTimes.end(), 0.0) / iterations;

				result << std::fixed << std::setprecision(3);
				result << appinfo << std::endl;
				result << deviceinfo << std::endl;
				result << ",iterations,time(ms),fps" << std::endl;;
				for (size_t i = 0; i < iterationTimes.size(); i++) {
					result << "," << i << "," << iterationTimes[i] << "," << (framesPerIteration / iterationTimes[i]) << std::endl;
				}
				result << ",summary" << std::endl;
				result << ",,time(ms),fps" << std::endl;
				result << ",min," << tMin << "," << (1000.0 / tMin) << std::endl;
				result << ",max," << tMax << "," << (1000.0 / tMax) << std::endl;
				result << ",avg," << tAvg << "," << (1000.0 / tAvg) << std::endl;

				// Output averages to stdout
				std::cout << std::fixed << std::setprecision(3);
				std::cout << "best : " << (1000.0 / tMin) << " fps (" << tMin << " ms)" << std::endl;
				std::cout << "worst: " << (1000.0 / tMax) << " fps (" << tMax << " ms)" << std::endl;
				std::cout << "avg  : " << (1000.0 / tAvg) << " fps (" << tAvg << " ms)" << std::endl;
				std::cout << std::flush;
			}
		}
	};
}