#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "perft.hpp"
#include "pyke.hpp"

using namespace pyke;

const std::string START = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
const std::string KIWIPETE = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
const std::string POSITION_3 = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1";
const std::string POSITION_4 = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
const std::string POSITION_5 = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
const std::string POSITION_6 = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";
const std::string KNIGHT_PAWN_CHECK = "2k3q1/8/2N1P3/8/3n4/3p4/2K5/8 w - - 0 2";

struct PerftCase {
	std::string fen;
	std::vector<std::string> moves;
	int depth;
	uint64_t expected;
};

const std::vector<PerftCase> cases = {
	{START, {}, 1, 20},
	{START, {}, 2, 400},
	{START, {}, 3, 8902},
	{START, {}, 4, 197281},
	{START, {}, 5, 4865609},
	{START, {}, 6, 119060324},
	{START, {"e2e3", "g8f6", "f1e2", "e7e6", "g1f3", "f8e7"}, 6, 695065640},
	{START, {"d2d4", "d7d5", "b1c3", "b8c6", "c1f4", "c8f5", "d1d2", "d8d7"}, 6, 1994297937},
	{START, {"g2g4", "b7b6", "f1h3", "c8b7", "e2e3", "e7e6", "g1e2"}, 5, 31637674},
	{START, {"e2e4", "e7e5", "g1f3", "b8c6", "f1c4", "g8f6"}, 6, 948211582},
	{START, {"e2e3", "g8f6", "f1e2", "e7e6", "g1f3", "f8e7", "a2a4", "e8g8"}, 4, 699761},
	{START, {"e2e4", "b8c6", "e4e5", "c6b8", "e1e2", "b8c6", "e2f3", "c6b8", "f3g4"}, 2, 490},
	{START, {"b2b4", "c7c5", "b4c5", "b7b6", "c5b6", "c8b7", "b6a7"}, 5, 13832566},
	{START, {"a2a4", "b7b5", "a4b5", "a7a6", "b5a6", "c8b7", "a6b7"}, 6, 379973606},
	{START, {"b2b4", "a7a5", "g1f3", "a5b4", "a2a3", "b4a3", "c1b2", "a3b2"}, 6, 461603984},
	{START, {"a2a4", "b7b5", "a4b5", "a7a6", "b5a6", "c8b7", "a6b7", "g8f6", "b7a8q"}, 3, 13905},
	{START, {"b2b4", "a7a5", "g1f3", "a5b4", "a2a3", "b4a3", "c1b2", "a3b2", "e2e3", "b2a1n"}, 4, 471595},
	{START, {"e2e4", "d7d5", "e4d5", "g8f6", "b1c3", "f6d5", "c3d5", "d8d5", "g1f3", "c8g4"}, 4, 1173149},
	{START, {"e2e4", "e7e5", "g1f3", "b8c6", "f3e5", "c6e5", "d2d4", "e5f3"}, 3, 3014},
	{KIWIPETE, {}, 1, 48},
	{KIWIPETE, {}, 2, 2039},
	{KIWIPETE, {}, 3, 97862},
	{KIWIPETE, {}, 4, 4085603},
	{KIWIPETE, {}, 5, 193690690},
	{POSITION_3, {}, 1, 14},
	{POSITION_3, {}, 2, 191},
	{POSITION_3, {}, 3, 2812},
	{POSITION_3, {}, 4, 43238},
	{POSITION_3, {}, 5, 674624},
	{POSITION_3, {}, 6, 11030083},
	{POSITION_4, {}, 1, 6},
	{POSITION_4, {}, 2, 264},
	{POSITION_4, {}, 3, 9467},
	{POSITION_4, {}, 4, 422333},
	{POSITION_4, {}, 5, 15833292},
	{POSITION_5, {}, 1, 44},
	{POSITION_5, {}, 2, 1486},
	{POSITION_5, {}, 3, 62379},
	{POSITION_5, {}, 4, 2103487},
	{POSITION_5, {}, 5, 89941194},
	{POSITION_6, {}, 1, 46},
	{POSITION_6, {}, 2, 2079},
	{POSITION_6, {}, 3, 89890},
	{POSITION_6, {}, 4, 3894594},
	{POSITION_6, {}, 5, 164075551},
	{KNIGHT_PAWN_CHECK, {}, 1, 7},
	{KNIGHT_PAWN_CHECK, {}, 2, 173},
	{KNIGHT_PAWN_CHECK, {}, 3, 2098},
	{KNIGHT_PAWN_CHECK, {}, 4, 51084},
};

static int run_cases(TTBucket* tt, Hash tt_mask) {
	int failed = 0;
	for (const PerftCase& c : cases) {
		Position pos;
		pos.tt = tt;
		pos.tt_mask = tt_mask;
		set_fen(pos, c.fen);
		for (const std::string& m : c.moves) move_from_string(m, pos);
		uint64_t nodes = count_nodes<false>(pos, c.depth);

		bool ok = nodes == c.expected;
		if (!ok) failed++;
		std::cout << (ok ? "ok   " : "FAIL ") << (tt ? "hashed " : "") << "depth " << c.depth << " ";
		std::cout << (c.fen == START ? "start" : c.fen) << " [";
		for (size_t i = 0; i < c.moves.size(); i++) std::cout << (i ? " " : "") << c.moves[i];
		std::cout << "]: " << nodes;
		if (!ok) std::cout << " (expected " << c.expected << ")";
		std::cout << '\n';
	}

	std::cout << cases.size() - failed << "/" << cases.size() << (tt ? " passed with hashing\n" : " passed\n");
	return failed;
}

int main() {
	std::vector<TTBucket> table(1 << 21);
	int failed = run_cases(nullptr, 0) + run_cases(table.data(), table.size() - 1);
	return failed ? 1 : 0;
}
