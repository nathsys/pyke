#pragma once

#include <cstdint>

#include "board.hpp"
#include "defaults.hpp"

namespace zobrist {

constexpr uint64_t splitmix64(uint64_t& state) {
	uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
	return z ^ (z >> 31);
}

struct KeyTable {
	Hash piece_square[2][7][64] = {};
	Hash castling[16] = {};
	Hash en_passant[8] = {};
	Hash side_to_move = 0;
	Hash depth[16] = {};
};

constexpr KeyTable make_keys() {
	KeyTable table;
	uint64_t seed = 0x5EED5EED5EED5EEDULL;
	for (int color = 0; color < 2; color++)
		for (int piece = 1; piece < 7; piece++)
			for (int square = 0; square < 64; square++) table.piece_square[color][piece][square] = splitmix64(seed);
	for (Hash& key : table.castling) key = splitmix64(seed);
	for (Hash& key : table.en_passant) key = splitmix64(seed);
	table.side_to_move = splitmix64(seed);
	for (Hash& key : table.depth) key = splitmix64(seed);
	return table;
}

inline constexpr KeyTable keys = make_keys();

template <bool white, Piece p>
constexpr Hash piece_key(Square square) {
	constexpr Piece type = p == QUEEN_DIAG || p == QUEEN_ORTH ? QUEEN : p;
	return keys.piece_square[white][type][square];
}

template <bool white>
inline Hash piece_key(Piece p, Square square) {
	return keys.piece_square[white][p][square];
}

inline Hash pieces(Board& b) {
	Hash hash = 0;
	for (Square s = 0; s < 64; s++) {
		hash ^= piece_key<true>(b.get_piece<true>(s), s);
		hash ^= piece_key<false>(b.get_piece<false>(s), s);
	}
	return hash;
}

}  // namespace zobrist

struct TTEntry {
	Hash key;
	uint64_t data;

	uint64_t nodes() const { return data & ((1ULL << 56) - 1); }
	int depth() const { return data >> 56; }
};

struct TTBucket {
	TTEntry deep;
	TTEntry recent;
};
