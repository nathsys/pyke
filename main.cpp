#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

#include "perft.hpp"
#include "pyke.hpp"

using namespace pyke;

const std::string START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

static void set_position(Position& pos, std::istringstream& in) {
	std::string token, fen;
	in >> token;
	if (token == "startpos") {
		fen = START_FEN;
		in >> token;
	} else if (token == "fen") {
		while (in >> token && token != "moves") fen += token + " ";
	}

	set_fen(pos, fen);
	if (token == "moves")
		while (in >> token) move_from_string(token, pos);
}

static int uci_loop() {
	Position pos;
	std::string line;
	while (std::getline(std::cin, line)) {
		std::istringstream in(line);
		std::string command;
		in >> command;

		if (command == "uci") {
			std::cout << "id name Pyke\nid author Nathanael Mohanu\nuciok" << std::endl;
		} else if (command == "isready") {
			std::cout << "readyok" << std::endl;
		} else if (command == "ucinewgame") {
			set_fen(pos, START_FEN);
		} else if (command == "position") {
			set_position(pos, in);
		} else if (command == "go") {
			std::string type;
			int depth = 0;
			in >> type >> depth;
			if (type != "perft" || depth < 1 || depth > 10) continue;
			uint64_t nodes = count_nodes<true>(pos, depth);
			std::cout << "\nNodes searched: " << nodes << "\n" << std::endl;
		} else if (command == "quit") {
			break;
		}
	}
	return 0;
}

int main(int argc, char* argv[]) {
	if (argc < 2) return uci_loop();

	int depth = std::atoi(argv[1]);
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
