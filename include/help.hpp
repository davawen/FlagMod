#pragma once

#include <string>
#include <vector>
#include <optional>
#include <algorithm>
#include <numeric>
#include <ranges>
#include <cstring>

#include "fmt/core.h"
#include "fmt/format.h"

namespace fmt {

template <typename T>
struct formatter<std::optional<T>> {
	std::string_view underlying_fmt;
	std::string_view or_else;

	constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
		// {:<if present{}><if not present>}
		auto it = ctx.begin(), end = ctx.end();
		auto get_marker = [&it, end]() constexpr -> std::string_view {
			if(it == end || *it != '<') return std::string_view(nullptr, 0); // no match
			auto start = ++it;

			while(it != end && (*it++ != '>'));
			if(it == end) throw fmt::format_error("invalid format, unfinished range");

			return std::string_view(start, it - 1);
		};

		underlying_fmt = "{}";
		or_else = "";

		auto first = get_marker();
		if(first.data() != nullptr) underlying_fmt = first;

		auto second = get_marker();
		if(second.data() != nullptr) or_else = second;

		// Check if reached the end of the range:
		if (it != end && *it != '}') throw fmt::format_error("invalid format, no end bracket");
		return it;
	}

	template <typename FormatContext>
	auto format(const std::optional<T>& p, FormatContext& ctx) const -> decltype(ctx.out()) {
		if(p.has_value()) {
			return vformat_to(ctx.out(), underlying_fmt, format_arg_store<FormatContext, T>{*p});
		}
		else {
			return format_to(ctx.out(), "{}", or_else);
		}
	}
};
}

namespace FlagMod {
struct Help {
	struct OptionHelp {
		std::optional<std::string> name;
		std::optional<char> short_name;
		std::string help;
		std::optional<std::string> default_value;

		std::string format_prefix() const {
			return fmt::format("{:<-{},><    >} {:<--{}>}", short_name, name);
		}
		std::string format_help() const {
			return fmt::format("{} {:<(default: {})>}", help, default_value);
		}
	};

	struct FlagHelp {
		std::optional<std::string> name;
		std::optional<char> short_name;
		std::string help;

		std::string format_prefix() const {
			return fmt::format("{:<-{},><    >} {:<--{}>}", short_name, name);
		}
		std::string format_help() const {
			return help;
		}
	};

	struct PositionalHelp {
		std::string label;
	};

	std::string name, version, executable;
	std::vector<OptionHelp> option_help;
	std::vector<FlagHelp> flag_help;
	std::vector<PositionalHelp> positional_help;

	std::string format_flag_help(const std::string &name) {
		namespace ranges = std::ranges;

		auto matching = [&name](const auto &x){ return x.name == name; };
		auto format = [](const auto &x) {
			return fmt::format("    {} {}\n", x.format_prefix(), x.format_help());
		};

		if(auto it = ranges::find_if(option_help, matching); it != option_help.end()) {
			return format(*it);
		}
		else if(auto it = ranges::find_if(flag_help, matching); it != flag_help.end()) {
			return format(*it);
		}
		return "";
	}

	template <typename T>
	std::string format_flags(const std::vector<T> &v) {
		namespace ranges = std::ranges;

		size_t max_length = 0;
		std::vector<std::pair<std::string, const T *>> flags;
		ranges::transform(v, std::back_inserter(flags),
			[&max_length](const T &x) {
				auto str = fmt::format("    {}", x.format_prefix());
				max_length = std::max(max_length, str.length());
				return std::pair{ str, &x };
			}
		);

		return std::accumulate(flags.cbegin(), flags.cend(), std::string("\n"), 
			[max_length](std::string a, const std::pair<const std::string, const T *> &b) {
				return std::move(a) + std::move(b.first) + fmt::format("{} {}\n", std::string(max_length - b.first.length(), ' '), b.second->format_help());
			}
		);
	}

	void print_usage(FILE *output) {
		fmt::print(output, "Usage: {} [options...]{}\n\n", executable, std::accumulate(positional_help.begin(), positional_help.end(), std::string(""), 
			[](std::string a, PositionalHelp &b) {
				return std::move(a) + fmt::format(" {{{}}}", b.label);
			}
		));
	}

	void print_help(FILE *output = stdout) {
		fmt::print(output, "{} version {}\n\n", name, version);

		print_usage(output);

		if(flag_help.size() > 0) {
			fmt::print(output, "FLAGS:{}\n", format_flags<FlagHelp>(flag_help));
		}

		if(option_help.size() > 0){
			fmt::print(output, "OPTIONS:{}\n", format_flags<OptionHelp>(option_help));
		}
	}

};
}
