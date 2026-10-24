#pragma once

#include <cmath>
#include <cstdint>
#include <ctime>
#include <iostream>

#include "pyke.hpp"

template <bool white, int depth, bool print_move>
static uint64_t count_side(Position& pos) {
	return pos.ep_flag ? pyke::count_moves<white, depth, print_move, true>(pos)
					   : pyke::count_moves<white, depth, print_move, false>(pos);
}

template <int depth, bool print_move>
static uint64_t count_root(Position& pos) {
	BitBoard checker = pos.checker;
	uint8_t ep_flag = pos.ep_flag;
	pos.hash = zobrist::pieces(pos.board);
	uint64_t nodes =
		pos.white_turn ? count_side<true, depth, print_move>(pos) : count_side<false, depth, print_move>(pos);
	pos.checker = checker;
	pos.ep_flag = ep_flag;
	return nodes;
}

template <bool print_move>
static uint64_t count_nodes(Position& pos, int depth) {
	switch (depth) {
	case 1:
		return count_root<1, print_move>(pos);
	case 2:
		return count_root<2, print_move>(pos);
	case 3:
		return count_root<3, print_move>(pos);
	case 4:
		return count_root<4, print_move>(pos);
	case 5:
		return count_root<5, print_move>(pos);
	case 6:
		return count_root<6, print_move>(pos);
	case 7:
		return count_root<7, print_move>(pos);
	case 8:
		return count_root<8, print_move>(pos);
	case 9:
		return count_root<9, print_move>(pos);
	case 10:
		return count_root<10, print_move>(pos);
	default:
		return 0;
	}
}

inline uint64_t perft(Position& pos, int depth) {
	clock_t start = clock();
	uint64_t nodes = count_nodes<true>(pos, depth);
	clock_t end = clock();
	double seconds = double(end - start) / CLOCKS_PER_SEC;
	std::cout << "\nNodes: " << nodes << "\nTime:  " << seconds << " s\n";
	std::cout << "Speed: " << std::round(nodes / seconds / 1e4) / 100 << " MNPS\n";
	return nodes;
}
