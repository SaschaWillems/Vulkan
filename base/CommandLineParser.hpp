/*
 * Simple command line parse
 *
 * Copyright (C) 2016-2022 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#pragma once

#include <vector>
#include <unordered_map>
#include <string>

class CommandLineParser
{
public:
	struct CommandLineOption {
		std::vector<std::string> commands;
		std::string value;
		bool hasValue = false;
		std::string help;
		bool set = false;
	};
	std::unordered_map<std::string, CommandLineOption> options;

	void add(std::string name, std::vector<std::string> commands, bool hasValue, std::string help)
	{
		options[name].commands = commands;
		options[name].help = help;
		options[name].set = false;
		options[name].hasValue = hasValue;
		options[name].value = "";
	}

	void printHelp()
	{
		std::cout << "Available command line options:\n";
		for (auto option : options) {
			std::cout << " ";
			for (size_t i = 0; i < option.second.commands.size(); i++) {
				std::cout << option.second.commands[i];
				if (i < option.second.commands.size() - 1) {
					std::cout << ", ";
				}
			}
			std::cout << ": " << option.second.help << "\n";
		}
		std::cout << "Press any key to close...";
	}

	void parse(std::vector<const char*> arguments)
	{
		bool printHelp = false;
		// Known arguments
		for (auto& option : options) {
			for (auto& command : option.second.commands) {
				for (size_t i = 0; i < arguments.size(); i++) {
					if (strcmp(arguments[i], command.c_str()) == 0) {
						option.second.set = true;
						// Get value
						if (option.second.hasValue) {
							if (arguments.size() > i + 1) {
								option.second.value = arguments[i + 1];
							}
							if (option.second.value == "") {
								printHelp = true;
								break;
							}
						}
					}
				}
			}
		}
		// Print help for unknown arguments or missing argument values
		if (printHelp) {
			options["help"].set = true;
		}
	}

	void parse(int argc, char* argv[])
	{
		std::vector<const char*> args;
		for (int i = 0; i < argc; i++) { 
			args.push_back(argv[i]); 
		};
		parse(args);
	}

	bool isSet(std::string name)
	{
		return ((options.find(name) != options.end()) && options[name].set);
	}

	std::string getValueAsString(std::string name, std::string defaultValue)
	{
		assert(options.find(name) != options.end());
		std::string value = options[name].value;
		return (value != "") ? value : defaultValue;
	}

	int32_t getValueAsInt(std::string name, int32_t defaultValue)
	{
		assert(options.find(name) != options.end());
		std::string value = options[name].value;
		if (value != "") {
			char* numConvPtr;
			int32_t intVal = strtol(value.c_str(), &numConvPtr, 10);
			return (intVal > 0) ? intVal : defaultValue;
		}
		else {
			return defaultValue;
		}
		return int32_t();
	}

};