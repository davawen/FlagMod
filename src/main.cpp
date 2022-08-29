#include <iostream>
#include <concepts>

#include "flags.hpp"

#ifndef VERSION
#define VERSION "1.0.0"
#endif

int main(int argc, char** argv)
{
	auto flags = FlagMod::Flags(argc, argv).name("test").version(VERSION);
	
	auto flag_help = flags.flag("h,help", "Show this help and exit");
	auto flag_test = flags.flag("t,test", "Test option");
	auto flag_testb = flags.option_required<int>("b", "Test option B", "123");

	auto [help] = flags.parse(flag_help);
	if(help) {
		flags.print_help();
		return 0;
	}

	auto [test, testb] = flags.parse(flag_test, flag_testb);
	if(test) std::cout << "Test was given\n";
	else std::cout << "Test wasn't given\n";

	std::cout << testb << '\n';

    return 0;
}
