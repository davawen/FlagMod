#pragma once

#include <string>
#include <charconv>
#include <vector>
#include <optional>
#include <stdexcept>

#include <fmt/core.h>

#include "errors.hpp"

namespace FlagMod {

	// Used in overload resolving (function specialization doesn't work with things such as vector)
	template <typename T>
	struct lexical_type{};

	template <typename T>
	concept lexically_convertible = requires {
		lexical_conversion(std::string_view{}, lexical_type<T>{});
	};

	inline constexpr std::string lexical_conversion(std::string_view input, lexical_type<std::string>) { return std::string(input);}
	inline constexpr char lexical_conversion(std::string_view input, lexical_type<char>) { 
		if(input.length() == 0) throw InvalidArgument("");
		return input[0]; 
	}

	template <typename T>
	inline std::vector<T> lexical_conversion(std::string_view input, lexical_type<std::vector<T>>) {
		std::vector<T> v;

		size_t last_pos = 0;
		for(size_t i = 0; i <= input.length(); i++) {
			if(i == input.length() || (input[i] == ',' && last_pos < input.length())) { // OR short circuit to avoid out of bounds access
				auto substr = input.substr(last_pos, i-last_pos);
				try {
					auto value = lexical_conversion<T>(substr, static_cast<T *>(nullptr));
					v.push_back(std::move(value));
				}
				catch(InvalidArgument &e) {
					throw InvalidArgument(fmt::format("in list parsing of \"{}\": \n{}", substr, e.msg));
				}
				last_pos = i+1;
			}
		}

		return v;
	}

#ifndef DEFINE_NUMERIC_STR_TO_VALUE
	#define DEFINE_NUMERIC_STR_TO_VALUE(type) \
		inline /*constexpr*/ type lexical_conversion(std::string_view input, lexical_type<type>) { \
			type out; \
			auto [__, ec] = std::from_chars(input.data(), input.data() + input.size(), out); \
			if(ec == std::errc::invalid_argument) throw InvalidArgument(fmt::format("invalid input for numeric type " #type ": not a number ({})", input)); \
			else if(ec == std::errc::result_out_of_range) throw InvalidArgument("input too big for numeric type " #type); \
			else return out; \
		}
#endif

	DEFINE_NUMERIC_STR_TO_VALUE(int)
	DEFINE_NUMERIC_STR_TO_VALUE(unsigned int)
	DEFINE_NUMERIC_STR_TO_VALUE(long)
	DEFINE_NUMERIC_STR_TO_VALUE(unsigned long)
	DEFINE_NUMERIC_STR_TO_VALUE(long long)
	DEFINE_NUMERIC_STR_TO_VALUE(unsigned long long)
	DEFINE_NUMERIC_STR_TO_VALUE(float)
	DEFINE_NUMERIC_STR_TO_VALUE(double)
	DEFINE_NUMERIC_STR_TO_VALUE(long double)

	#undef DEFINE_NUMERIC_STR_TO_VALUE
}
