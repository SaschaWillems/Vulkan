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
	public:
		bool active = false;
		uint32_t framesPerIteration = 1000;
		uint32_t iterationCount = 10;
		//std::vector<double> iterationTimes;
		struct Iteration {
			std::vector<double> frameTimes;
		};
		std::vector<Iteration> iterations;
		std::string filename = "benchmarkresults.csv";

		void run(std::function<void()> renderFunc) {
			active = true;
#if defined(_WIN32)
			AttachConsole(ATTACH_PARENT_PROCESS);
			freopen_s(&stream, "CONOUT$", "w+", stdout);
			freopen_s(&stream, "CONOUT$", "w+", stderr);
#endif
			// "Warm up" run to get more stable frame rates
			for (uint32_t f = 0; f < framesPerIteration; f++) {
				renderFunc();
			}

			iterations.resize(iterationCount);
			for (uint32_t i = 0; i < iterationCount; i++) {
				iterations[i].frameTimes.resize(framesPerIteration);
				for (uint32_t f = 0; f < framesPerIteration; f++) {
					auto tStart = std::chrono::high_resolution_clock::now();
					renderFunc();
					auto tEnd = std::chrono::high_resolution_clock::now();
					auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
					iterations[i].frameTimes[f] = tDiff;
				}
			}
		}

		void saveResults(std::string appinfo, std::string deviceinfo) {
			std::ofstream result(filename, std::ios::out);
			if (result.is_open()) {
				double tMinAll = std::numeric_limits<double>::max();
				double tMaxAll = std::numeric_limits<double>::min();
				double tAvgAll = 0.0;

				result << std::fixed << std::setprecision(4);
				result << "iteration,min(ms),max(ms),avg(ms),min(fps),max(fps),avg(fps)" << std::endl;
				uint32_t index = 0;
				for (auto iteration : iterations) {
					double tMin = *std::min_element(iteration.frameTimes.begin(), iteration.frameTimes.end());
					double tMax = *std::max_element(iteration.frameTimes.begin(), iteration.frameTimes.end());
					double tAvg = std::accumulate(iteration.frameTimes.begin(), iteration.frameTimes.end(), 0.0) / framesPerIteration;
					if (tMin < tMinAll) {
						tMinAll = tMin;
					}
					if (tMax > tMaxAll) {
						tMaxAll = tMax;
					}
					tAvgAll += tAvg;
					index++;
					result << index << "," << tMin << "," << tMax << "," << tAvg << "," << (1000.0 / tMax) << "," << (1000.0 / tMin) << "," << (1000.0 / tAvg) << std::endl;
				}

				tAvgAll /= static_cast<uint32_t>(iterations.size());
				result << "summary,min(ms),max(ms),avg(ms),min(fps),max(fps),avg(fps)" << std::endl;
				result << index << "," << tMinAll << "," << tMaxAll << "," << tAvgAll << "," << (1000.0 / tMaxAll) << "," << (1000.0 / tMinAll) << "," << (1000.0 / tAvgAll) << std::endl;

				// Output averages to stdout
				std::cout << std::fixed << std::setprecision(3);
				std::cout << "best : " << (1000.0 / tMinAll) << " fps (" << tMinAll << " ms)" << std::endl;
				std::cout << "worst: " << (1000.0 / tMaxAll) << " fps (" << tMaxAll << " ms)" << std::endl;
				std::cout << "avg  : " << (1000.0 / tAvgAll) << " fps (" << tAvgAll << " ms)" << std::endl;
				std::cout << std::endl;
#if defined(_WIN32)
				fclose(stream);
				FreeConsole();
#endif
			}
		}
	};
}