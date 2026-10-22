#pragma once

#include <cstdint>

#include "board.hpp"
#include "gamestate.hpp"
#include "maskset.hpp"
#include "stack.hpp"

struct Position {
	Board board;
	uint8_t ep_flag = 0;
	CastlingRights cr = 0b1111;
	BitBoard checker = 0;
	bool white_turn = true;
	Stack<MaskSet> masks;
	Square bksq = 4;
	Square wksq = 60;

	template <bool white>
	constexpr inline void set_ksq(const Square ksq) {
		if constexpr (white) {
			wksq = ksq;
		} else {
			bksq = ksq;
		}
	}

	template <bool white>
	constexpr inline Square get_ksq() {
		if constexpr (white) {
			return wksq;
		} else {
			return bksq;
		}
	}

	template <bool white, Piece p>
	inline BitBoard piece_brd() {
		return board.get_piece_board<white, p>();
	}

	// Returns whether a square is under attack.
	template <bool white>
	FORCE_INLINE bool is_attacked(Square square) {
		return (get_pawn_move<white, PawnMoveType::ATTACKS>(square, board.occ_board)
				& board.get_piece_board<!white, PAWN>())
			|| (get_knight_move(square) & board.get_piece_board<!white, KNIGHT>())
			|| (get_rook_move(square, board.occ_board)
				& (board.get_piece_board<!white, ROOK>() | board.get_piece_board<!white, QUEEN>()))
			|| (get_bishop_move(square, board.occ_board)
				& (board.get_piece_board<!white, BISHOP>() | board.get_piece_board<!white, QUEEN>()))
			|| (get_king_move(square) & board.get_piece_board<!white, KING>());
	}
};

