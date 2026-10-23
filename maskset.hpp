#pragma once

#include <cstdint>

#include "board.hpp"
#include "defaults.hpp"
#include "piece_moves.hpp"

struct MaskSet {
	// Squares that are empty or occupied by the opponent.
	BitBoard targets;

	// Pinmask.
	BitBoard pinmask_dg = 0;
	BitBoard pinmask_orth = 0;
	BitBoard check_mask = 0;
	BitBoard nopin = 0;

	uint8_t checkers = 0;

	inline void reset() {
		pinmask_dg = 0;
		pinmask_orth = 0;
		check_mask = 0;
		checkers = 0;
	}
};

// Create all the needed masks for the current position.
template <bool white>
FORCE_INLINE BitBoard pawn_knight_checkers(Board& b, Square king_square) {
	BitBoard k_checkers = get_knight_move(king_square) & b.get_piece_board<!white, KNIGHT>();
	BitBoard king_mask = square_to_mask(king_square);
	BitBoard p_checkers =
		(get_pawn_left<white>(can_capture_left(king_mask)) | get_pawn_right<white>(can_capture_right(king_mask)))
		& b.get_piece_board<!white, PAWN>();
	return k_checkers | p_checkers;
}

template <bool white>
FORCE_INLINE MaskSet& create_masks(Board& b, Square king_square, BitBoard checker, MaskSet& ret) {
	ret.reset();
	ret.targets = ~b.get_player_occ<white>();

	BitBoard opp_board = b.get_player_occ<!white>();
	BitBoard own_board = b.get_player_occ<white>();
	BitBoard eq = b.get_piece_board<!white, QUEEN>();
	BitBoard eb = b.get_piece_board<!white, BISHOP>();
	BitBoard er = b.get_piece_board<!white, ROOK>();
	BitBoard diag_pinners =
		(bishop_rays[king_square] & (eb | eq)) ? get_bishop_move(king_square, opp_board) & (eb | eq) : 0;
	BitBoard orth_pinners =
		(rook_rays[king_square] & (er | eq)) ? get_rook_move(king_square, opp_board) & (er | eq) : 0;

	if (checker) {
		ret.check_mask |= checker;
		ret.checkers += popcnt(checker);
	}

	auto process_pinners = [&](BitBoard& pinboard, BitBoard& goal_mask) {
		while (pinboard) {
			Square src = pop(pinboard);
			BitBoard between = between_squares[king_square][src];

			switch (popcnt(between & own_board)) {
			case 0:
				ret.check_mask |= between;
				ret.checkers++;
				break;
			case 1:
				goal_mask |= between;
				break;
			}
		}
	};

	process_pinners(diag_pinners, ret.pinmask_dg);
	process_pinners(orth_pinners, ret.pinmask_orth);
	ret.nopin = ~(ret.pinmask_dg | ret.pinmask_orth);

	return ret;
}

