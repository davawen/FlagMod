#pragma once

#include <string>
#include <vector>
#include <optional>
#include <tuple>
#include <algorithm>
#include <numeric>
#include <type_traits>
#include <ranges>

#include "fmt/core.h"

#include "lexical_conversion.hpp"
#include "errors.hpp"
#include "help.hpp"

namespace FlagMod {
	constexpr auto nullopt = std::nullopt;

template <typename T, bool Required>
struct Option {
	using type = T;
	using out = std::conditional_t<Required, T, std::optional<T>>;

	std::optional<std::string> name;
	std::optional<char> short_name;

	std::optional<std::string> default_value;
	std::optional<std::string_view> value;

	std::string id() const {
		return fmt::format("{}{}", short_name, name);
	}
};

struct Flag {
	std::optional<std::string> name;
	std::optional<char> short_name;

	bool value;

	std::string id() const {
		return fmt::format("{}{}", short_name, name);
	}
};

template <typename T>
struct Positional {
	std::string label;
	
	std::optional<std::string_view> value;
};

template <typename T>
concept stringifiable = requires(T x) {
	std::to_string(x);
};

class Flags {
	std::vector<std::string> args;
	Help help;

	auto get_flag_names(const std::string &schema) {
		std::pair<std::optional<std::string>, std::optional<char>> out{nullopt, nullopt};
		size_t pos = schema.find(',');
		if(pos == std::string::npos) {
			if(schema.length() == 0) throw InvalidFlagSpec("flag schema is empty");
			else if(schema.length() == 1) out.second = schema[0];
			else out.first = schema;
		}
		else {
			std::pair<std::string, std::string> split{ schema.substr(0, pos), schema.substr(pos + 1) };

			if(split.first.length() == 1 && split.second.length() == 1) throw InvalidFlagSpec("flag schema can't have two short names");
			else if(split.first.length() != 1 && split.second.length() != 1) throw InvalidFlagSpec("flag schema can't have two long names");
			else if(split.first.length() == 1) {
				out.first = split.second;
				out.second = split.first[0];
			}
			else if(split.second.length() == 1) {
				out.first = split.first;
				out.second = split.second[0];
			}
			else throw InvalidFlagSpec("flag schema contains a trailing comma");
		}

		return out;
	}

	/// Base case for `option_present`
	std::pair<std::nullptr_t, size_t> option_present(std::string_view) {
		return { nullptr, 0 };
	}

	/// Ignore non-flag arguments
	template <typename T, typename ... Ts>
	auto option_present(std::string_view str, T &, Ts & ... options) {
		return option_present(str, options...);
	}

	/// @returns { nullptr, _ } if flag isn't detected, else returns a pointer to the found flag's string value and the position of the character right after the flag in the string
	template <typename T, bool R, typename ... Ts>
	std::pair<std::optional<std::string_view> *, size_t> option_present(const std::string_view str_, Option<T, R> &option, Ts & ... options) {
		std::string_view str = str_;
		if(str.starts_with("--") && (str.remove_prefix(str.find_first_not_of('-')), (option.name.has_value() && str == *option.name))) {
			return { &option.value, option.name->size() + 2 };
		}
		else if(option.short_name.has_value() && str.starts_with('-') && (str.remove_prefix(1), (str.back() == *option.short_name))) { // allows for flags with options at the end
			return { &option.value, 2 };
		}
		else {
			return option_present(str_, options...); // Calls base case if no flag remain
		}
	}

	/// Base case for `flag_present`
	bool flag_present(std::string_view) {
		return false;
	}

	/// Ignore non-switch arguments
	template <typename T, typename ... Ts>
	bool flag_present(std::string_view str, T &, Ts & ... flags) {
		return flag_present(str, flags...);
	}

	/// Turns on switches present in the given string
	/// @return wether at least one switch was present
	template <typename ... Ts>
	bool flag_present(const std::string_view str_, Flag &s, Ts & ... flags) {
		std::string_view str = str_;
		// If string starts with multiple dashes, removing the prefix disallows a match with `short_name`
		if(str.starts_with("--") && (str.remove_prefix(str.find_first_not_of('-')), (s.name.has_value() && str == *s.name))) {
			s.value = true;
			return true;
		}
		else if( s.short_name.has_value() && str.starts_with('-') && str.find(*s.short_name) != std::string::npos ) {
			s.value = true;
			return flag_present(str_, flags...) || true;
		}
		else {
			return flag_present(str_, flags...);
		}
	}

	/// Base case for `set_positional`
	void set_positional(std::string_view) {}

	template <typename T, typename ... Ts>
	void set_positional(std::string_view str, T &, Ts & ... flags) {
		return set_positional(str, flags...);
	}

	template <typename T, typename ... Ts>
	void set_positional(std::string_view str, Positional<T> &positional, Ts & ... flags) {
		if(!positional.value.has_value()) {
			positional.value = str;
		}
		else {
			set_positional(str, flags...);
		}
	}

	bool parse_flag(const Flag &s) {
		return s.value;
	}

	/// @returns T if the flag is required or optional<T> if it isn't
	template <typename T, bool R>
	typename Option<T, R>::out parse_flag(Option<T, R> &flag) {
		if constexpr(R) {
			if(!flag.value.has_value() && !flag.default_value.has_value()) throw RequiredFlagNotGiven(fmt::format("option requires a value but wasn't given one\n{}\n", help.format_flag_help(flag.id())));
			else if(!flag.value.has_value() && flag.default_value.has_value()) flag.value = std::string_view(*flag.default_value);
			// fallthrough to try/catch
		}
		else {
			if(!flag.value.has_value()) return nullopt;
			// fallthrough to try/catch
		}
		try {
			return lexical_conversion(*flag.value, lexical_type<T>{});
		} 
		catch(InvalidArgument &e) {
			throw InvalidArgument(fmt::format("{}\ncouldn't parse input \"{}\" given to option\n{}\n", e.msg, *flag.value, flag.name, help.format_flag_help(flag.id()))); // Show invalid input instead of failing silently
		}
	}

	template <typename T>
	T parse_flag(const Positional<T> &p) {
		std::optional<T> value;
		if(p.value.has_value() && (value = lexical_conversion(*p.value, lexical_type<T>{})).has_value()) return *value;

		throw RequiredFlagNotGiven(fmt::format("argument {{{}}} not given", p.label));
	}

public:
	Flags(int argc, char **argv) {
		for(int i = 1; i < argc; i++) {
			args.push_back(argv[i]);
		}
	}

	/// Takes a parameter pack of FlagMod::Flag, FlagMod::Switch and FlagMod::Positional
	template <typename ... Ts>
	auto parse(Ts ... flags) {
		try {
			for(size_t idx = 0; idx < args.size(); idx++) {
				auto str = std::string_view(args[idx]);

				auto [ flag_value, chr_idx ] = option_present(str, flags...);
				if constexpr(!std::is_same_v<decltype(flag_value), std::nullptr_t>) { // needed to avoid compilation error on nullptr_t assignment
					if(flag_value != nullptr) { 
						if(chr_idx >= str.length() && idx+1 < args.size()) {
							*flag_value = args[idx + 1];
							idx++; // advance index
						}
						else if(str[chr_idx] == '=') {
							*flag_value = str.substr(chr_idx+1);
						}
						continue;
					}
				}

				bool switch_found = flag_present(str, flags...);

				if(flag_value == nullptr && !switch_found) {
					set_positional(str, flags...);
				}
			}

			return std::tuple{ parse_flag(flags)... };
		}
		catch(RequiredFlagNotGiven &e) {
			help.print_usage(stderr);
			fmt::print(stderr, "\x1b[1;31m\x1b[4merror:\x1b[0m {}\n", e.msg);
			// TODO: print help
			throw e;
		}
		catch(InvalidArgument &e) {
			help.print_usage(stderr);
			fmt::print(stderr, "\x1b[1;31m\x1b[4merror:\x1b[0m {}\n", e.msg);
			throw e;
		}
		catch(std::exception &e) {
			throw e;
		}
	}

	Flags &name(const std::string &name) {
		help.name = name;
		return *this;
	}

	Flags &version(const std::string &version) {
		help.version = version;
		return *this;
	}

	void print_help() {
		help.print_help();
	}

	void check_duplicate(const std::ranges::input_range auto &r, const std::optional<std::string> &name, const std::optional<char> &short_name) {
		auto result = std::ranges::find_if(r, [&name, &short_name](const auto &p){
			auto &v = p.second;
			return (v.name.has_value() && name.has_value() && *v.name == *name) || (v.short_name.has_value() && short_name.has_value() && *v.short_name == *short_name);
		});

		if(result != r.end()) throw InvalidFlagSpec(fmt::format("duplicate flags: {:<-{}, ><>}{:<--{}>}", short_name, name));
	}

	template <lexically_convertible T>
	Option<T, false> option(const std::string &schema, const std::string &help) {
		auto [name, short_name] = get_flag_names(schema);

		check_duplicate(this->help.option_help, name, short_name);
		auto out = Option<T, false> { name, short_name, nullopt, nullopt };
		this->help.option_help[out.id()] = Help::OptionHelp { name, short_name, help, nullopt };
		return out;
	}
	template <lexically_convertible T>
	Option<T, true> option_required(const std::string &schema, const std::string &help, const std::optional<std::string> default_value = nullopt) {
		auto [name, short_name] = get_flag_names(schema);

		check_duplicate(this->help.option_help, name, short_name);
		auto in = Help::OptionHelp { name, short_name, help, default_value };
		if(default_value.has_value()) {
			try {
				lexical_conversion(*default_value, lexical_type<T>{});
			}
			catch(InvalidArgument &e) {
				throw InvalidFlagSpec(fmt::format("default argument of {} can't be parsed properly\n{}", in.format_prefix(), e.msg));
			}
		}

		auto out = Option<T, true> { name, short_name, default_value, nullopt };
		this->help.option_help[out.id()] = std::move(in);

		return out;
	}

	Flag flag(const std::string &schema, const std::string &help) {
		auto [name, short_name] = get_flag_names(schema);

		check_duplicate(this->help.flag_help, name, short_name);

		auto out = Flag { name, short_name, false };
		this->help.flag_help.insert_or_assign(out.id(), Help::FlagHelp { name, short_name, help });
		return out;
	}

	template <lexically_convertible T>
	Positional<T> positional(const std::string &label) {
		this->help.positional_help.push_back(Help::PositionalHelp{ label });
		return Positional<T>{ label, nullopt };
	}
};

}
