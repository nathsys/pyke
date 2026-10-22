#include <cstdlib>
#include <iostream>
#include <string>

#include "perft.hpp"
#include "pyke.hpp"

using namespace pyke;

constexpr int DEFAULT_DEPTH = 7;

int main(int argc, char* argv[]) {
	int depth = argc > 1 ? std::atoi(argv[1]) : DEFAULT_DEPTH;
	if (depth < 1 || depth > 10) {
		std::cerr << "Depth must be between 1 and 10.\n";
		return 1;
	}

	Position pos;
	for (int i = 2; i < argc; ++i) move_from_string(argv[i], pos);

	std::cout << "Pyke chess move generator by Nathanael Mohanu\n";
	pos.board.print_board();
	perft(pos, depth);

	return 0;
}
