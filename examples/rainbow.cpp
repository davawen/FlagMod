#include <iostream>

#include "flags.hpp"

int main(int argc, char **argv) {
	auto flags = FlagMod::Flags(argc, argv).name("rainbow").version("1.0.0");

	auto flag_help = flags.flag("h,help", "Print this help and exits.");
	auto flag_text = flags.positional<std::string>("Text");

	auto [help] = flags.parse(flag_help);
	if(help) {
		flags.print_help();
		return 0;
	}

	auto [text] = flags.parse(flag_text);
	int code = 91;
	for(auto c : text) {
		std::cout << "\x1b[" << code << "m" << c;

		code++;
		if(code == 97) code = 91;
	}
	std::cout << '\n';

	return 0;
}
