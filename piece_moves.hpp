#pragma once

#include <immintrin.h>

#include <cstdint>

#include "board.hpp"
#include "lookup_tables.hpp"

// King move.
static FORCE_INLINE BitBoard get_king_move(const Square square) { return KING_MOVE_SQUARES[square]; }
// Knight move logic.
static FORCE_INLINE BitBoard get_knight_move(const Square square) { return KNIGHT_MOVE_SQUARES[square]; }

// Bishop moving logic.
static FORCE_INLINE BitBoard get_bishop_move(const Square square, const BitBoard occ) {
	return bishop_pext_atk[bishop_pext_offset[square] + static_cast<uint32_t>(pext(occ, bishop_mask_table[square]))];
}

// Rook move logic.
static FORCE_INLINE BitBoard get_rook_move(const Square square, const BitBoard occ) {
	return rook_pext_atk[rook_pext_offset[square] + static_cast<uint32_t>(pext(occ, rook_mask_table[square]))];
}

// Queen move logic.
static FORCE_INLINE BitBoard get_queen_move(const Square square, const BitBoard occ) {
	return get_bishop_move(square, occ) | get_rook_move(square, occ);
}

template <bool white>
static FORCE_INLINE BitBoard get_pawn_left(const BitBoard piece_board) {
	if constexpr (white)
		return (piece_board << 9);
	else
		return (piece_board >> 7);
}

template <bool white>
static FORCE_INLINE BitBoard get_pawn_right(const BitBoard piece_board) {
	if constexpr (white)
		return (piece_board << 7);
	else
		return (piece_board >> 9);
}

// Get pawn diagonals, without trimming any form of illegal moves or edge cases.
template <bool white>
static FORCE_INLINE BitBoard get_pawn_diags(const BitBoard piece_board) {
	if constexpr (white)
		return (piece_board << 9) | (piece_board << 7);
	else
		return (piece_board >> 9) | (piece_board >> 7);
}

// Only the attack squares. Used to check if king is checked by pawn.
template <bool white>
static FORCE_INLINE BitBoard get_pawn_attacks(const BitBoard piece_board) {
	Square s = lbit(piece_board);
	if constexpr (white)
		return (get_pawn_diags<white>(piece_board)) & pawn_edge_masks[0][s];
	else
		return (get_pawn_diags<white>(piece_board)) & pawn_edge_masks[1][s];
}

// Get pawn forward.
template <bool white>
static FORCE_INLINE BitBoard get_pawn_forward(const BitBoard piece_board) {
	if constexpr (white)
		return piece_board << 8;
	else
		return piece_board >> 8;
}

// Pawn move including double push.
template <bool white>
static FORCE_INLINE BitBoard get_pawn_double(const BitBoard piece_board, const BitBoard occ) {
	BitBoard single = get_pawn_forward<white>(piece_board) & ~occ;
	return get_pawn_forward<white>(single) & ~occ;
}

// Calls the requested pawn move type function.
template <bool white, PawnMoveType type>
static FORCE_INLINE BitBoard get_pawn_move(const Square s, BitBoard occ) {
	BitBoard p_board = square_to_mask(s);
	switch (type) {
	case PawnMoveType::ATTACKS:
		return get_pawn_attacks<white>(p_board);
	case PawnMoveType::FORWARD:
		return get_pawn_forward<white>(p_board);
	case PawnMoveType::NON_DOUBLE:
		return get_pawn_attacks<white>(p_board) | get_pawn_forward<white>(p_board);
	case PawnMoveType::DOUBLE_FORWARD:
		return get_pawn_double<white>(p_board, occ);
	case PawnMoveType::ALL:
		return get_pawn_move<white, PawnMoveType::NON_DOUBLE>(s, occ)
			| get_pawn_move<white, PawnMoveType::DOUBLE_FORWARD>(s, occ);
	}
}

constexpr static std::array<std::array<uint64_t, 64>, 64> create_betweens() {
	std::array<std::array<uint64_t, 64>, 64> ret{};
	for (int s1 = 0; s1 < 64; s1++) {
		for (int s2 = 0; s2 < 64; s2++) {
			if (s1 == s2) continue;
			BitBoard rs1 = get_rook_move(s1, square_to_mask(s2));
			BitBoard rs2 = get_rook_move(s2, square_to_mask(s1));
			BitBoard ds1 = get_bishop_move(s1, square_to_mask(s2));
			BitBoard ds2 = get_bishop_move(s2, square_to_mask(s1));

			if (rs1 & square_to_mask(s2)) {
				BitBoard orth = rs1 & rs2;
				ret[s1][s2] |= orth | square_to_mask(s2);
			} else if (ds1 & square_to_mask(s2)) {
				BitBoard diag = ds1 & ds2;
				ret[s1][s2] |= diag | square_to_mask(s2);
			} else {
				continue;
			}
		}
	}
	return ret;
}

// Returns the reach of a given piece. For pawns, it returns the reach without double push.
template <bool white, Piece piece>
static FORCE_INLINE BitBoard make_reach_board(Square square, Board& b) {
	switch (piece) {
	case PAWN:
		return get_pawn_move<white, PawnMoveType::NON_DOUBLE>(square, b.occ_board);
	case KING:
		return get_king_move(square);
	case ROOK:
	case QUEEN_ORTH:
		return get_rook_move(square, b.occ_board);
	case BISHOP:
	case QUEEN_DIAG:
		return get_bishop_move(square, b.occ_board);
	case KNIGHT:
		return get_knight_move(square);
	case QUEEN:
		return get_queen_move(square, b.occ_board);
	}
}

const static std::array<std::array<uint64_t, 64>, 64> between_squares = create_betweens();

#if defined(__AVX512F__) && defined(__AVX512VPOPCNTDQ__)
#define SETWISE_COUNT

inline constexpr BitBoard file_a = 0x8080808080808080ULL;
inline constexpr BitBoard file_b = 0x4040404040404040ULL;
inline constexpr BitBoard file_g = 0x0202020202020202ULL;
inline constexpr BitBoard file_h = 0x0101010101010101ULL;
inline constexpr BitBoard rank_1 = 0x00000000000000FFULL;
inline constexpr BitBoard rank_2 = 0x000000000000FF00ULL;
inline constexpr BitBoard rank_7 = 0x00FF000000000000ULL;
inline constexpr BitBoard rank_8 = 0xFF00000000000000ULL;

static FORCE_INLINE uint64_t count_setwise(
	BitBoard orth, BitBoard diag, BitBoard knights, BitBoard empty, BitBoard targets
) {
	const __m512i rot1 = _mm512_setr_epi64(8, 56, 63, 1, 7, 9, 55, 57);
	const __m512i rot2 = _mm512_setr_epi64(16, 48, 62, 2, 14, 18, 46, 50);
	const __m512i rot4 = _mm512_setr_epi64(32, 32, 60, 4, 28, 36, 28, 36);
	const __m512i wrap = _mm512_setr_epi64(
		~rank_1, ~rank_8, ~file_a, ~file_h,
		~(file_a | rank_1), ~(file_h | rank_1), ~(file_a | rank_8), ~(file_h | rank_8)
	);
	const __m512i jumps = _mm512_setr_epi64(15, 17, 6, 10, 47, 49, 54, 58);
	const __m512i jump_wrap = _mm512_setr_epi64(
		~(file_a | rank_1 | rank_2), ~(file_h | rank_1 | rank_2),
		~(file_a | file_b | rank_1), ~(file_g | file_h | rank_1),
		~(file_a | rank_8 | rank_7), ~(file_h | rank_8 | rank_7),
		~(file_a | file_b | rank_8), ~(file_g | file_h | rank_8)
	);
	const __m512i target_vec = _mm512_set1_epi64(targets);

	__m512i gen = _mm512_mask_blend_epi64(0xF0, _mm512_set1_epi64(orth), _mm512_set1_epi64(diag));
	__m512i pro = _mm512_and_si512(_mm512_set1_epi64(empty), wrap);
	gen = _mm512_or_si512(gen, _mm512_and_si512(pro, _mm512_rolv_epi64(gen, rot1)));
	pro = _mm512_and_si512(pro, _mm512_rolv_epi64(pro, rot1));
	gen = _mm512_or_si512(gen, _mm512_and_si512(pro, _mm512_rolv_epi64(gen, rot2)));
	pro = _mm512_and_si512(pro, _mm512_rolv_epi64(pro, rot2));
	gen = _mm512_or_si512(gen, _mm512_and_si512(pro, _mm512_rolv_epi64(gen, rot4)));
	__m512i slides = _mm512_and_si512(_mm512_rolv_epi64(gen, rot1), _mm512_and_si512(wrap, target_vec));

	__m512i hops = _mm512_rolv_epi64(_mm512_set1_epi64(knights), jumps);
	hops = _mm512_and_si512(hops, _mm512_and_si512(jump_wrap, target_vec));

	return _mm512_reduce_add_epi64(_mm512_add_epi64(_mm512_popcnt_epi64(slides), _mm512_popcnt_epi64(hops)));
}
#endif

static std::array<BitBoard, 64> make_rays(bool diag) {
	std::array<BitBoard, 64> ret{};
	for (int s = 0; s < 64; s++) ret[s] = diag ? get_bishop_move(s, 0) : get_rook_move(s, 0);
	return ret;
}

const static std::array<BitBoard, 64> bishop_rays = make_rays(true);
const static std::array<BitBoard, 64> rook_rays = make_rays(false);

