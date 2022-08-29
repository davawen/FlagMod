#pragma once

#include <exception>
#include <string>

namespace FlagMod {
struct InvalidFlagSpec : std::exception {
	std::string msg;

	InvalidFlagSpec(std::string msg) : msg(msg) {}

	const char * what() const noexcept override {
		return msg.c_str();
	}
};

struct RequiredFlagNotGiven : std::exception {
	std::string msg;

	RequiredFlagNotGiven(std::string msg) : msg(msg) {}
};

struct InvalidArgument : std::exception {
	std::string msg;

	InvalidArgument(std::string msg) : msg(msg) {}
};
}
