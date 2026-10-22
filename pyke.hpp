#pragma once

#include <cstdint>

#include "gamestate.hpp"
#include "make_move.hpp"
#include "maskset.hpp"
#include "piece_moves.hpp"
#include "position.hpp"
#include "stack.hpp"
#include "util.hpp"

namespace pyke {

template <bool white, int dtg, bool print_move, bool ep = false>
NO_INLINE uint64_t count_moves(Position& pos);

template <bool white, Piece p, int dtg>
static FORCE_INLINE void mark_check(Position& pos, Square to) {
	if constexpr (dtg > 1 && (p == KNIGHT || p == PAWN)) {
		BitBoard to_mask = square_to_mask(to);
		BitBoard attacks = p == KNIGHT
			? get_knight_move(to)
			: get_pawn_left<white>(can_capture_left(to_mask)) | get_pawn_right<white>(can_capture_right(to_mask));
		if (attacks & pos.board.get_piece_board<!white, KING>()) pos.checker = to_mask;
	}
}

template <bool white, int dtg>
static FORCE_INLINE uint64_t count_with_rights(Position& pos, CastlingRights mask) {
	CastlingRights cr = pos.cr;
	pos.cr &= mask;
	uint64_t ret = count_moves<!white, dtg - 1, false>(pos);
	pos.cr = cr;
	return ret;
}

/*
 *	ROOK
 */

// Count rook moves.
template <bool white, Piece p, bool capture, int dtg, bool print_move>
static FORCE_INLINE uint64_t generate_rook_moves(BitBoard cmt, Square from, Position& pos) {
	if (!cmt) return 0;
	uint64_t ret = 0;

	while (cmt) {
		uint64_t loc_ret = 0;
		Square to = pop(cmt);
		BitBoard move = square_to_mask(from) | square_to_mask(to);
		if constexpr (capture) {
			// Capture moves.
			const Piece captured = pos.board.get_piece<!white>(to);
			BitBoard to_mask = square_to_mask(to);

			capture_move_wrapper<white, p>(pos.board, captured, move, to_mask);
			loc_ret += count_with_rights<white, dtg>(pos, castle_mask[from] & castle_mask[to]);
			unmake_capture_wrapper<white, p>(pos.board, captured, move, to_mask);
		} else {
			plain_move<white, p>(pos.board, move);
			loc_ret += count_with_rights<white, dtg>(pos, castle_mask[from]);
			unmake_plain_move<white, p>(pos.board, move);
		}
		if constexpr (print_move) print_movecnt(from, to, loc_ret);
		ret += loc_ret;
	}
	return ret;
}

/*
 *	GENERAL
 */

template <bool white, Piece p, int dtg, bool print_move>
static FORCE_INLINE uint64_t count_captures(BitBoard cmt, Square from, Position& pos) {
	if (!cmt) return 0;
	if constexpr (p == ROOK) {
		return generate_rook_moves<white, ROOK, true, dtg, print_move>(cmt, from, pos);
	} else {
		uint64_t ret = 0;
		while (cmt) {
			uint64_t loc_ret = 0;
			Square to = pop(cmt);
			// Capture moves.
			const Piece captured = pos.board.get_piece<!white>(to);
			BitBoard move = square_to_mask(from) | square_to_mask(to);
			BitBoard to_mask = square_to_mask(to);

			capture_move_wrapper<white, p>(pos.board, captured, move, to_mask);
			mark_check<white, p, dtg>(pos, to);
			loc_ret += count_with_rights<white, dtg>(pos, castle_mask[to]);
			unmake_capture_wrapper<white, p>(pos.board, captured, move, to_mask);

			if constexpr (print_move) print_movecnt(from, to, loc_ret);
			ret += loc_ret;
		}
		return ret;
	}
}

template <bool white, Piece p, int dtg, bool print_move>
static FORCE_INLINE uint64_t count_plain(BitBoard cmt, Square from, Position& pos) {
	if (!cmt) return 0;
	if constexpr (p == ROOK) {
		return generate_rook_moves<white, ROOK, false, dtg, print_move>(cmt, from, pos);
	} else {
		uint64_t ret = 0;
		while (cmt) {
			uint64_t loc_ret = 0;
			Square to = pop(cmt);
			BitBoard move = square_to_mask(from) | square_to_mask(to);

			plain_move<white, p>(pos.board, move);
			mark_check<white, p, dtg>(pos, to);
			loc_ret += count_moves<!white, dtg - 1, false>(pos);
			unmake_plain_move<white, p>(pos.board, move);

			if constexpr (print_move) print_movecnt(from, to, loc_ret);
			ret += loc_ret;
		}
		return ret;
	}
}

// Splits the reachable squares into non-captures and captures and calls the appropriate function.
template <bool white, Piece p, int dtg, bool print_move, MoveType mt = p>
static FORCE_INLINE uint64_t generate_moves(BitBoard cmt, BitBoard pieces, Position& pos) {
	if (!(cmt && pieces)) return 0;
	uint64_t ret = 0;
	// For all instances of given piece.
	while (pieces) {
		Square from = pop(pieces);
		if constexpr (dtg <= 1 && !print_move) {
			ret += popcnt(cmt & make_reach_board<white, p>(from, pos.board));
		} else {
			BitBoard piece_moves_to = cmt & make_reach_board<white, mt>(from, pos.board);
			BitBoard captures = piece_moves_to & pos.board.occ_board;
			BitBoard non_captures = piece_moves_to & ~captures;

			// Non-captures + captured.
			ret += count_plain<white, p, dtg, print_move>(non_captures, from, pos);
			ret += count_captures<white, p, dtg, print_move>(captures, from, pos);
		}
	}
	return ret;
}

/*
 *	KING
 */

// Create the castling move for given player and direction.
template <bool white, bool kingside, int dtg, bool print_move>
static FORCE_INLINE uint64_t generate_castle_move(Position& pos) {
	Board& b = pos.board;
	constexpr Square to = white ? (kingside ? 62 : 58) : (kingside ? 6 : 2);
	constexpr Square middle_square = white ? (kingside ? 61 : 59) : (kingside ? 5 : 3);
	constexpr Square ksq = white ? 60 : 4;

	if (!(pos.cr & (white ? (kingside ? wk_mask : wq_mask) : (kingside ? bk_mask : bq_mask)))) {
		return 0;
	} else if (b.square_occ(to) | b.square_occ(middle_square)) {
		return 0;
	} else if (!kingside && b.square_occ(queenside_middle_squares[white])) {
		return 0;
	} else if (pos.is_attacked<white>(middle_square) || pos.is_attacked<white>(to)) {
		return 0;
	} else if constexpr (dtg <= 1 && !print_move) {
		return 1;
	} else {
		constexpr uint8_t code = white ? (kingside ? 0 : 1) : (kingside ? 2 : 3);
		castle_move<white, code>(pos.board);
		pos.set_ksq<white>(to);
		uint64_t ret = count_with_rights<white, dtg>(pos, castle_mask[ksq]);
		unmake_castle_move<white, code>(pos.board);
		pos.set_ksq<white>(ksq);
		if constexpr (print_move) print_movecnt(ksq, to, ret);
		return ret;
	}
}

// Create king moves.
template <bool white, int dtg, bool print_move>
static FORCE_INLINE uint64_t generate_king_moves(BitBoard cmt, Position& pos) {
	Square ksq = pos.get_ksq<white>();
	cmt &= get_king_move(ksq);
	BitBoard captures = cmt & pos.board.get_player_occ<!white>();
	BitBoard non_captures = cmt & ~captures;
	uint64_t ret = 0;
	while (non_captures) {
		Square to = pop(non_captures);
		uint64_t loc_ret = 0;
		BitBoard move = square_to_mask(ksq) | square_to_mask(to);

		plain_move<white, KING>(pos.board, move);
		pos.set_ksq<white>(to);
		if (!pos.is_attacked<white>(to)) loc_ret += count_with_rights<white, dtg>(pos, castle_mask[ksq]);
		unmake_plain_move<white, KING>(pos.board, move);

		if constexpr (print_move) print_movecnt(ksq, to, loc_ret);
		ret += loc_ret;
	}

	while (captures) {
		Square to = pop(captures);
		uint64_t loc_ret = 0;
		Piece captured = pos.board.get_piece<!white>(to);
		BitBoard move = square_to_mask(ksq) | square_to_mask(to);
		BitBoard to_mask = square_to_mask(to);

		capture_move_wrapper<white, KING>(pos.board, captured, move, to_mask);
		pos.set_ksq<white>(to);
		if (!pos.is_attacked<white>(to)) loc_ret += count_with_rights<white, dtg>(pos, castle_mask[ksq] & castle_mask[to]);
		unmake_capture_wrapper<white, KING>(pos.board, captured, move, to_mask);

		if constexpr (print_move) print_movecnt(ksq, to, loc_ret);
		ret += loc_ret;
	}

	pos.set_ksq<white>(ksq);

	return ret;
}

/*
 *	PAWNS
 */

// Make ep move and count. Offset is whether ep comes from left or right.
template <bool white, int offset, int dtg, bool print_move>
static FORCE_INLINE uint64_t make_en_passant(Position& pos, uint8_t ep) {
	uint64_t loc_ret;
	sq_pair epsq = get_ep_squares<white, offset>(ep);
	BitBoard move = square_to_mask(epsq.first) | square_to_mask(epsq.second);
	BitBoard capture_sq = square_to_mask(white ? epsq.second + 8 : epsq.second - 8);

	ep_move<white>(pos.board, move, capture_sq);
	loc_ret = 0;
	if (!pos.is_attacked<white>(pos.get_ksq<white>())) {
		mark_check<white, PAWN, dtg>(pos, epsq.second);
		loc_ret = count_moves<!white, dtg - 1, false>(pos);
	}

	unmake_ep_move<white>(pos.board, move, capture_sq);

	if constexpr (print_move) print_movecnt(epsq.first, epsq.second, loc_ret);
	return loc_ret;
}

// Count nodes following from ep moves.
template <bool white, int dtg, bool print_move>
static FORCE_INLINE uint64_t generate_ep_moves(Position& pos, uint8_t ep) {
	return (ep & 0x80 ? make_en_passant<white, -1, dtg, print_move>(pos, ep) : 0)
		+ (ep & 0x40 ? make_en_passant<white, 1, dtg, print_move>(pos, ep) : 0);
}

// Pawn double pushes.
template <bool white, int dtg, bool print_move>
static FORCE_INLINE NodeCount generate_pawn_double(BitBoard cmt, Position& pos, BitBoard source) {
	if constexpr (dtg <= 1 && !print_move) {
		return popcnt(get_pawn_double<white>(source, pos.board.occ_board) & cmt);
	} else {
		NodeCount ret = 0;
		while (source) {
			BitBoard from = popextr(source);
			BitBoard to_board = cmt & get_pawn_double<white>(from, pos.board.occ_board);
			while (to_board) {
				BitBoard to = popextr(to_board);
				BitBoard move = from | to;
				bool ep = pawn_double<white>(pos.board, move, to, pos.ep_flag);
				mark_check<white, PAWN, dtg>(pos, lbit(to));
				NodeCount loc_ret = ep ? count_moves<!white, dtg - 1, false, true>(pos)
									   : count_moves<!white, dtg - 1, false, false>(pos);
				unmake_pawn_double<white>(pos.board, move);
				if constexpr (print_move) print_movecnt(lbit(from), lbit(to), loc_ret);
				ret += loc_ret;
			}
		}
		return ret;
	}
}

// Concrete generator for pawn moves. Creates double pushes, pushes and captures, in bulk if possible.
template <bool white, int dtg, bool print_move>
static FORCE_INLINE uint64_t generate_pawn_moves(BitBoard cmt, BitBoard pieces, Position& pos) {
	if (!(cmt && pieces)) return 0;
	BitBoard occ = pos.board.occ_board;
	BitBoard cmt_free = cmt & ~occ;
	BitBoard cmt_captures = cmt & occ;
	BitBoard pawns_on_start = pieces & (white ? pawn_start_w : pawn_start_b);
	uint64_t ret = generate_pawn_double<white, dtg, print_move>(cmt, pos, pawns_on_start);

	// Generate moves in bulk.
	if constexpr (dtg <= 1 && !print_move) {
		ret += popcnt(get_pawn_forward<white>(pieces) & cmt_free);
		ret += popcnt(get_pawn_left<white>(can_capture_left(pieces)) & cmt_captures);
		ret += popcnt(get_pawn_right<white>(can_capture_right(pieces)) & cmt_captures);
	} else {
		while (pieces) {
			Square from = pop(pieces);
			BitBoard captures = get_pawn_move<white, PawnMoveType::ATTACKS>(from, occ) & cmt_captures;
			BitBoard non_captures = get_pawn_move<white, PawnMoveType::FORWARD>(from, occ) & cmt_free;
			ret += count_plain<white, PAWN, dtg, print_move>(non_captures, from, pos);
			ret += count_captures<white, PAWN, dtg, print_move>(captures, from, pos);
		}
	}
	return ret;
}

template <bool white, Piece promo, int dtg, bool print_move>
static FORCE_INLINE uint64_t make_promotion(Position& pos, Square from, Square to) {
	BitBoard to_mask = square_to_mask(to);
	BitBoard move = square_to_mask(from) | to_mask;
	const Piece captured = pos.board.get_piece<!white>(to);

	if (captured != EMPTY)
		capture_move_wrapper<white, PAWN>(pos.board, captured, move, to_mask);
	else
		plain_move<white, PAWN>(pos.board, move);
	toggle_promotion<white, promo>(pos.board, to_mask);
	mark_check<white, promo, dtg>(pos, to);

	uint64_t ret = count_with_rights<white, dtg>(pos, castle_mask[to]);

	toggle_promotion<white, promo>(pos.board, to_mask);
	if (captured != EMPTY)
		unmake_capture_wrapper<white, PAWN>(pos.board, captured, move, to_mask);
	else
		unmake_plain_move<white, PAWN>(pos.board, move);

	if constexpr (print_move) print_movecnt(from, to, ret, promo);
	return ret;
}

template <bool white, int dtg, bool print_move>
static FORCE_INLINE uint64_t generate_promotions(BitBoard cmt, BitBoard pieces, Position& pos) {
	if (!(cmt && pieces)) return 0;
	BitBoard occ = pos.board.occ_board;
	BitBoard cmt_free = cmt & ~occ;
	BitBoard cmt_captures = cmt & occ;

	if constexpr (dtg <= 1 && !print_move) {
		return 4
			* (popcnt(get_pawn_forward<white>(pieces) & cmt_free)
			   + popcnt(get_pawn_left<white>(can_capture_left(pieces)) & cmt_captures)
			   + popcnt(get_pawn_right<white>(can_capture_right(pieces)) & cmt_captures));
	} else {
		uint64_t ret = 0;
		while (pieces) {
			Square from = pop(pieces);
			BitBoard targets = (get_pawn_move<white, PawnMoveType::ATTACKS>(from, occ) & cmt_captures)
				| (get_pawn_move<white, PawnMoveType::FORWARD>(from, occ) & cmt_free);
			while (targets) {
				Square to = pop(targets);
				ret += make_promotion<white, QUEEN, dtg, print_move>(pos, from, to);
				ret += make_promotion<white, ROOK, dtg, print_move>(pos, from, to);
				ret += make_promotion<white, BISHOP, dtg, print_move>(pos, from, to);
				ret += make_promotion<white, KNIGHT, dtg, print_move>(pos, from, to);
			}
		}
		return ret;
	}
}

// Wrapper.
template <bool white, int dtg, bool print_move>
static FORCE_INLINE uint64_t generate_pawn(Position& pos, MaskSet& msk) {
	BitBoard can_move_from = pos.board.get_piece_board<white, PAWN>();
	BitBoard pawns_on_promo = can_move_from & (white ? promotion_from_w : promotion_from_b);
	can_move_from &= ~pawns_on_promo;
	BitBoard occ = pos.board.occ_board;
	BitBoard unpinned = can_move_from & msk.nopin;
	BitBoard pinned = can_move_from & ~msk.nopin;

	// Unpinned + pinned.
	uint64_t ret = generate_pawn_moves<white, dtg, print_move>(msk.cmt, unpinned, pos);
	if (pinned) {
		ret += generate_pawn_moves<white, dtg, print_move>(msk.cmt & msk.pinmask_dg & occ, pinned & msk.pinmask_dg, pos)
			+ generate_pawn_moves<white, dtg, print_move>(msk.cmt & msk.pinmask_orth & ~occ, pinned & msk.pinmask_orth, pos);
	}
	if (pawns_on_promo) {
		ret += generate_promotions<white, dtg, print_move>(msk.cmt, pawns_on_promo & msk.nopin, pos)
			+ generate_promotions<white, dtg, print_move>(msk.cmt & msk.pinmask_dg & occ, pawns_on_promo & msk.pinmask_dg, pos)
			+ generate_promotions<white, dtg, print_move>(
				msk.cmt & msk.pinmask_orth & ~occ, pawns_on_promo & msk.pinmask_orth, pos
			);
	}
	return ret;
}

/*
 *	SLIDERS
 */

template <bool white, int dtg, bool print_move>
static FORCE_INLINE uint64_t generate_sliders(Position& pos, MaskSet& msk) {
	BitBoard bishops = pos.board.get_piece_board<white, BISHOP>();
	BitBoard rooks = pos.board.get_piece_board<white, ROOK>();
	BitBoard queens = pos.board.get_piece_board<white, QUEEN>();
	BitBoard unp_b = bishops & msk.nopin;
	BitBoard unp_r = rooks & msk.nopin;
	BitBoard unp_q = queens & msk.nopin;
	BitBoard pin_cmt_diag = msk.cmt & msk.pinmask_dg;
	BitBoard pin_cmt_orth = msk.cmt & msk.pinmask_orth;
	BitBoard dg_not_orth = msk.pinmask_dg & ~msk.pinmask_orth;
	BitBoard orth_not_dg = msk.pinmask_orth & ~msk.pinmask_dg;
	BitBoard pin_b = bishops & dg_not_orth;
	BitBoard pin_q_diag = queens & dg_not_orth;
	BitBoard pin_r = rooks & orth_not_dg;
	BitBoard pin_q_orth = queens & orth_not_dg;

	// Pinned + unpinned.
	return generate_moves<white, BISHOP, dtg, print_move>(pin_cmt_diag, pin_b, pos)
		+ generate_moves<white, BISHOP, dtg, print_move>(msk.cmt, unp_b, pos)
		+ generate_moves<white, QUEEN_DIAG, dtg, print_move>(pin_cmt_diag, pin_q_diag, pos)
		+ generate_moves<white, QUEEN, dtg, print_move>(msk.cmt, unp_q, pos)
		+ generate_moves<white, QUEEN_ORTH, dtg, print_move>(pin_cmt_orth, pin_q_orth, pos)
		+ generate_moves<white, ROOK, dtg, print_move>(pin_cmt_orth, pin_r, pos)
		+ generate_moves<white, ROOK, dtg, print_move>(msk.cmt, unp_r, pos);
}

/*
 *	KNIGHTS
 */

// Count knight moves.
template <bool white, int dtg, bool print_move>
static FORCE_INLINE NodeCount generate_knight(Position& pos, MaskSet& msk) {
	return generate_moves<white, KNIGHT, dtg, print_move>(msk.cmt, pos.piece_brd<white, KNIGHT>() & msk.nopin, pos);
}

/*
 *	MAIN
 */

// Main counting function.
template <bool white, int dtg, bool print_move, bool ep>
NO_INLINE uint64_t count_moves(Position& pos) {
	if constexpr (dtg < 1)
		return 1;
	else {
		uint8_t ep_flag = ep ? pos.ep_flag : 0;
		BitBoard checker = print_move ? pawn_knight_checkers<white>(pos.board, pos.get_ksq<white>()) : pos.checker;
		pos.checker = 0;

		// Make masks.
		MaskSet& msk = create_masks<white>(pos.board, pos.get_ksq<white>(), checker, pos.masks.go_next());

		// King moves can always be generated.
		uint64_t ret = generate_king_moves<white, dtg, print_move>(msk.cmt, pos);

		// If double check, only king can move. Else, limit the target squares to the checkmask and skip castling.
		switch (msk.checkers) {
		case 0:
			ret += generate_castle_move<white, true, dtg, print_move>(pos);
			ret += generate_castle_move<white, false, dtg, print_move>(pos);
			break;
		case 1:
			msk.cmt &= msk.check_mask;
			break;
		default:
			pos.masks.point_prev();
			return ret;
		}

		// Generate moves.
		ret += generate_sliders<white, dtg, print_move>(pos, msk);
		ret += generate_pawn<white, dtg, print_move>(pos, msk);
		ret += generate_knight<white, dtg, print_move>(pos, msk);
		if constexpr (ep) ret += generate_ep_moves<white, dtg, print_move>(pos, ep_flag);
		pos.masks.point_prev();
		return ret;
	}
}

}  // namespace pyke

