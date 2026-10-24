#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

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

static std::vector<TTBucket> make_table(Position& pos, uint64_t megabytes) {
	uint64_t entries = 1;
	while (entries * 2 * sizeof(TTBucket) <= megabytes << 20) entries *= 2;
	std::vector<TTBucket> table(megabytes ? entries : 0);
	pos.tt = table.empty() ? nullptr : table.data();
	pos.tt_mask = entries - 1;
	return table;
}

static int uci_loop(Position& pos) {
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
	std::vector<std::string> args;
	uint64_t hash_mb = 0;
	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == "--hash" && i + 1 < argc)
			hash_mb = std::strtoull(argv[++i], nullptr, 10);
		else
			args.push_back(arg);
	}

	Position pos;
	std::vector<TTBucket> table = make_table(pos, hash_mb);
	if (args.empty()) return uci_loop(pos);

	int depth = std::atoi(args[0].c_str());
	if (depth < 1 || depth > 10) {
		std::cerr << "Depth must be between 1 and 10.\n";
		return 1;
	}

	for (size_t i = 1; i < args.size(); ++i) move_from_string(args[i], pos);

	std::cout << "Pyke chess move generator by Nathanael Mohanu\n";
	if (pos.tt) std::cout << "Hash table: " << ((pos.tt_mask + 1) * sizeof(TTBucket) >> 20) << " MB\n";
	pos.board.print_board();
	perft(pos, depth);

	return 0;
}
