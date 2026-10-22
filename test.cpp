#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "perft.hpp"
#include "pyke.hpp"

using namespace pyke;

struct PerftCase {
	std::vector<std::string> moves;
	int depth;
	uint64_t expected;
};

const std::vector<PerftCase> cases = {
	{{}, 1, 20},
	{{}, 2, 400},
	{{}, 3, 8902},
	{{}, 4, 197281},
	{{}, 5, 4865609},
	{{}, 6, 119060324},
	{{"e2e3", "g8f6", "f1e2", "e7e6", "g1f3", "f8e7"}, 6, 695065640},
	{{"d2d4", "d7d5", "b1c3", "b8c6", "c1f4", "c8f5", "d1d2", "d8d7"}, 6, 1994297937},
	{{"g2g4", "b7b6", "f1h3", "c8b7", "e2e3", "e7e6", "g1e2"}, 5, 31637674},
	{{"e2e4", "e7e5", "g1f3", "b8c6", "f1c4", "g8f6"}, 6, 948211582},
	{{"e2e3", "g8f6", "f1e2", "e7e6", "g1f3", "f8e7", "a2a4", "e8g8"}, 4, 699761},
	{{"e2e4", "b8c6", "e4e5", "c6b8", "e1e2", "b8c6", "e2f3", "c6b8", "f3g4"}, 2, 490},
	{{"b2b4", "c7c5", "b4c5", "b7b6", "c5b6", "c8b7", "b6a7"}, 5, 13832566},
	{{"a2a4", "b7b5", "a4b5", "a7a6", "b5a6", "c8b7", "a6b7"}, 6, 379973606},
	{{"b2b4", "a7a5", "g1f3", "a5b4", "a2a3", "b4a3", "c1b2", "a3b2"}, 6, 461603984},
	{{"a2a4", "b7b5", "a4b5", "a7a6", "b5a6", "c8b7", "a6b7", "g8f6", "b7a8q"}, 3, 13905},
	{{"b2b4", "a7a5", "g1f3", "a5b4", "a2a3", "b4a3", "c1b2", "a3b2", "e2e3", "b2a1n"}, 4, 471595},
	{{"e2e4", "d7d5", "e4d5", "g8f6", "b1c3", "f6d5", "c3d5", "d8d5", "g1f3", "c8g4"}, 4, 1173149},
	{{"e2e4", "e7e5", "g1f3", "b8c6", "f3e5", "c6e5", "d2d4", "e5f3"}, 3, 3014},
};

int main() {
	int failed = 0;
	for (const PerftCase& c : cases) {
		Position pos;
		for (const std::string& m : c.moves) move_from_string(m, pos);
		uint64_t nodes = count_nodes<false>(pos, c.depth);

		bool ok = nodes == c.expected;
		if (!ok) failed++;
		std::cout << (ok ? "ok   " : "FAIL ") << "depth " << c.depth << " after [";
		for (size_t i = 0; i < c.moves.size(); i++) std::cout << (i ? " " : "") << c.moves[i];
		std::cout << "]: " << nodes;
		if (!ok) std::cout << " (expected " << c.expected << ")";
		std::cout << '\n';
	}

	std::cout << cases.size() - failed << "/" << cases.size() << " passed\n";
	return failed ? 1 : 0;
}
