#include <iostream>
#include <concepts>

#include "flags.hpp"

enum OpType { PLUS, SUB, MUL, DIV };

namespace FlagMod {
	inline constexpr OpType lexical_conversion(std::string_view input, lexical_type<OpType>) {
		if(input.empty()) throw InvalidArgument("No operation given");
		switch(input[0]) {
			case '+': return OpType::PLUS;
			case '-': return OpType::SUB;
			case '*': return OpType::MUL;
			case '/': return OpType::DIV;
			default: throw InvalidArgument("Uknown operation given");
		};
	}
}

int main(int argc, char** argv)
{
	auto flags = FlagMod::Flags(argc, argv).name("math").version("1.0.0");
	
	auto flag_help = flags.flag("h,help", "Show this help and exit");
	auto flag_operator = flags.option_required<OpType>("o,operator", "Which operation to use on the numbers (+,-,/,*)", "+");
	auto flag_a = flags.positional<int>("Number A");
	auto flag_b = flags.positional<int>("Number B");

	auto [help] = flags.parse(flag_help);
	if(help) {
		flags.print_help();
		return 0;
	}

	auto [op, a, b] = flags.parse(flag_operator, flag_a, flag_b);
	switch(op) {
		case PLUS:
			std::cout << a + b << '\n';
			break;
		case SUB:
			std::cout << a - b << '\n';
			break;
		case DIV:
			std::cout << (float)a / (float)b << '\n';
			break;
		case MUL:
			std::cout << a * b << '\n';
			break;
	}

    return 0;
}
