#pragma once

#include <cmath>
#include <cstdint>
#include <ctime>
#include <iostream>

#include "pyke.hpp"

template <int depth>
static uint64_t perft_internal(Position& pos) {
	clock_t start = clock();
	uint64_t nodes;

	if (pos.white_turn)
		nodes = pyke::count_moves<true, depth, true>(pos);
	else
		nodes = pyke::count_moves<false, depth, true>(pos);

	clock_t end = clock();
	double seconds = double(end - start) / CLOCKS_PER_SEC;
	std::cout << "\nNodes: " << nodes << "\nTime:  " << seconds << " s\n";
	std::cout << "Speed: " << std::round(nodes / seconds / 1e4) / 100 << " MNPS\n";
	return nodes;
}

static uint64_t perft(Position& pos, int depth) {
	switch (depth) {
	case 1:
		return perft_internal<1>(pos);
	case 2:
		return perft_internal<2>(pos);
	case 3:
		return perft_internal<3>(pos);
	case 4:
		return perft_internal<4>(pos);
	case 5:
		return perft_internal<5>(pos);
	case 6:
		return perft_internal<6>(pos);
	case 7:
		return perft_internal<7>(pos);
	case 8:
		return perft_internal<8>(pos);
	case 9:
		return perft_internal<9>(pos);
	case 10:
		return perft_internal<10>(pos);
	default:
		std::cout << "Depth must be between 1 and 10.\n";
		return 0;
	}
}
