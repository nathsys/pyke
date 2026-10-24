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

template <bool white, int depth, bool print_move, bool ep = false>
NO_INLINE uint64_t count_moves(Position& pos);

template <bool white, Piece p, int depth>
static FORCE_INLINE void mark_check(Position& pos, Square to) {
	if constexpr (depth > 1 && (p == KNIGHT || p == PAWN)) {
		BitBoard to_mask = square_to_mask(to);
		BitBoard attacks = p == KNIGHT
			? get_knight_move(to)
			: get_pawn_left<white>(can_capture_left(to_mask)) | get_pawn_right<white>(can_capture_right(to_mask));
		if (attacks & pos.board.get_piece_board<!white, KING>()) pos.checker = to_mask;
	}
}

template <bool white, int depth>
static FORCE_INLINE Hash tt_index_key(Position& pos) {
	return pos.hash ^ zobrist::keys.depth[depth] ^ (white ? 0 : zobrist::keys.side_to_move);
}

template <bool white, int depth>
static FORCE_INLINE void apply_hash(Position& pos, Hash delta) {
	if constexpr (depth >= 3) {
		pos.hash ^= delta;
		if (pos.tt) __builtin_prefetch(&pos.tt[tt_index_key<!white, depth - 1>(pos) & pos.tt_mask]);
	}
}

template <int depth>
static FORCE_INLINE void undo_hash(Position& pos, Hash delta) {
	if constexpr (depth >= 3) pos.hash ^= delta;
}

template <bool white, int depth>
static FORCE_INLINE uint64_t count_with_rights(Position& pos, CastlingRights mask) {
	CastlingRights cr = pos.cr;
	pos.cr &= mask;
	uint64_t ret = count_moves<!white, depth - 1, false>(pos);
	pos.cr = cr;
	return ret;
}

/*
 *	ROOK
 */

// Count rook moves.
template <bool white, Piece p, bool capture, int depth, bool print_move>
static FORCE_INLINE uint64_t generate_rook_moves(BitBoard targets, Square from, Position& pos) {
	if (!targets) return 0;
	uint64_t ret = 0;

	while (targets) {
		uint64_t loc_ret = 0;
		Square to = pop(targets);
		BitBoard move = square_to_mask(from) | square_to_mask(to);
		if constexpr (capture) {
			// Capture moves.
			const Piece captured = pos.board.get_piece<!white>(to);
			BitBoard to_mask = square_to_mask(to);

			Hash delta = zobrist::piece_key<white, p>(from) ^ zobrist::piece_key<white, p>(to)
				^ zobrist::piece_key<!white>(captured, to);

			capture_move_wrapper<white, p>(pos.board, captured, move, to_mask);
			apply_hash<white, depth>(pos, delta);
			loc_ret += count_with_rights<white, depth>(pos, castle_mask[from] & castle_mask[to]);
			undo_hash<depth>(pos, delta);
			unmake_capture_wrapper<white, p>(pos.board, captured, move, to_mask);
		} else {
			Hash delta = zobrist::piece_key<white, p>(from) ^ zobrist::piece_key<white, p>(to);

			plain_move<white, p>(pos.board, move);
			apply_hash<white, depth>(pos, delta);
			loc_ret += count_with_rights<white, depth>(pos, castle_mask[from]);
			undo_hash<depth>(pos, delta);
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

template <bool white, Piece p, int depth, bool print_move>
static FORCE_INLINE uint64_t count_captures(BitBoard targets, Square from, Position& pos) {
	if (!targets) return 0;
	if constexpr (p == ROOK) {
		return generate_rook_moves<white, ROOK, true, depth, print_move>(targets, from, pos);
	} else {
		uint64_t ret = 0;
		while (targets) {
			uint64_t loc_ret = 0;
			Square to = pop(targets);
			// Capture moves.
			const Piece captured = pos.board.get_piece<!white>(to);
			BitBoard move = square_to_mask(from) | square_to_mask(to);
			BitBoard to_mask = square_to_mask(to);
			Hash delta = zobrist::piece_key<white, p>(from) ^ zobrist::piece_key<white, p>(to)
				^ zobrist::piece_key<!white>(captured, to);

			capture_move_wrapper<white, p>(pos.board, captured, move, to_mask);
			mark_check<white, p, depth>(pos, to);
			apply_hash<white, depth>(pos, delta);
			loc_ret += count_with_rights<white, depth>(pos, castle_mask[to]);
			undo_hash<depth>(pos, delta);
			unmake_capture_wrapper<white, p>(pos.board, captured, move, to_mask);

			if constexpr (print_move) print_movecnt(from, to, loc_ret);
			ret += loc_ret;
		}
		return ret;
	}
}

template <bool white, Piece p, int depth, bool print_move>
static FORCE_INLINE uint64_t count_plain(BitBoard targets, Square from, Position& pos) {
	if (!targets) return 0;
	if constexpr (p == ROOK) {
		return generate_rook_moves<white, ROOK, false, depth, print_move>(targets, from, pos);
	} else {
		uint64_t ret = 0;
		while (targets) {
			uint64_t loc_ret = 0;
			Square to = pop(targets);
			BitBoard move = square_to_mask(from) | square_to_mask(to);
			Hash delta = zobrist::piece_key<white, p>(from) ^ zobrist::piece_key<white, p>(to);

			plain_move<white, p>(pos.board, move);
			mark_check<white, p, depth>(pos, to);
			apply_hash<white, depth>(pos, delta);
			loc_ret += count_moves<!white, depth - 1, false>(pos);
			undo_hash<depth>(pos, delta);
			unmake_plain_move<white, p>(pos.board, move);

			if constexpr (print_move) print_movecnt(from, to, loc_ret);
			ret += loc_ret;
		}
		return ret;
	}
}

// Splits the reachable squares into non-captures and captures and calls the appropriate function.
template <bool white, Piece p, int depth, bool print_move, MoveType mt = p>
static FORCE_INLINE uint64_t generate_moves(BitBoard targets, BitBoard pieces, Position& pos) {
	if (!(targets && pieces)) return 0;
	uint64_t ret = 0;
	// For all instances of given piece.
	while (pieces) {
		Square from = pop(pieces);
		if constexpr (depth <= 1 && !print_move) {
			ret += popcnt(targets & make_reach_board<white, p>(from, pos.board));
		} else {
			BitBoard piece_moves_to = targets & make_reach_board<white, mt>(from, pos.board);
			BitBoard captures = piece_moves_to & pos.board.occ_board;
			BitBoard non_captures = piece_moves_to & ~captures;

			// Non-captures + captured.
			ret += count_plain<white, p, depth, print_move>(non_captures, from, pos);
			ret += count_captures<white, p, depth, print_move>(captures, from, pos);
		}
	}
	return ret;
}

/*
 *	KING
 */

// Create the castling move for given player and direction.
template <bool white, bool kingside, int depth, bool print_move>
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
	} else if constexpr (depth <= 1 && !print_move) {
		return 1;
	} else {
		constexpr uint8_t code = white ? (kingside ? 0 : 1) : (kingside ? 2 : 3);
		constexpr Hash delta = zobrist::piece_key<white, KING>(ksq) ^ zobrist::piece_key<white, KING>(to)
			^ zobrist::piece_key<white, ROOK>(rook_start_squares[code])
			^ zobrist::piece_key<white, ROOK>(rook_end_squares[code]);
		castle_move<white, code>(pos.board);
		pos.set_ksq<white>(to);
		apply_hash<white, depth>(pos, delta);
		uint64_t ret = count_with_rights<white, depth>(pos, castle_mask[ksq]);
		undo_hash<depth>(pos, delta);
		unmake_castle_move<white, code>(pos.board);
		pos.set_ksq<white>(ksq);
		if constexpr (print_move) print_movecnt(ksq, to, ret);
		return ret;
	}
}

// Create king moves.
template <bool white, int depth, bool print_move>
static FORCE_INLINE uint64_t generate_king_moves(BitBoard targets, Position& pos) {
	Square ksq = pos.get_ksq<white>();
	targets &= get_king_move(ksq);
	if constexpr (depth <= 1 && !print_move) {
		BitBoard occ = pos.board.occ_board ^ square_to_mask(ksq);
		uint64_t ret = 0;
		while (targets) ret += !pos.is_attacked<white>(pop(targets), occ);
		return ret;
	}
	BitBoard captures = targets & pos.board.get_player_occ<!white>();
	BitBoard non_captures = targets & ~captures;
	uint64_t ret = 0;
	while (non_captures) {
		Square to = pop(non_captures);
		uint64_t loc_ret = 0;
		BitBoard move = square_to_mask(ksq) | square_to_mask(to);
		Hash delta = zobrist::piece_key<white, KING>(ksq) ^ zobrist::piece_key<white, KING>(to);

		plain_move<white, KING>(pos.board, move);
		pos.set_ksq<white>(to);
		if (!pos.is_attacked<white>(to)) {
			apply_hash<white, depth>(pos, delta);
			loc_ret += count_with_rights<white, depth>(pos, castle_mask[ksq]);
			undo_hash<depth>(pos, delta);
		}
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
		Hash delta = zobrist::piece_key<white, KING>(ksq) ^ zobrist::piece_key<white, KING>(to)
			^ zobrist::piece_key<!white>(captured, to);

		capture_move_wrapper<white, KING>(pos.board, captured, move, to_mask);
		pos.set_ksq<white>(to);
		if (!pos.is_attacked<white>(to)) {
			apply_hash<white, depth>(pos, delta);
			loc_ret += count_with_rights<white, depth>(pos, castle_mask[ksq] & castle_mask[to]);
			undo_hash<depth>(pos, delta);
		}
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
template <bool white, int offset, int depth, bool print_move>
static FORCE_INLINE uint64_t make_en_passant(Position& pos, uint8_t ep) {
	uint64_t loc_ret;
	sq_pair epsq = get_ep_squares<white, offset>(ep);
	BitBoard move = square_to_mask(epsq.first) | square_to_mask(epsq.second);
	Square captured = white ? epsq.second + 8 : epsq.second - 8;
	BitBoard capture_sq = square_to_mask(captured);
	Hash delta = zobrist::piece_key<white, PAWN>(epsq.first) ^ zobrist::piece_key<white, PAWN>(epsq.second)
		^ zobrist::piece_key<!white, PAWN>(captured);

	ep_move<white>(pos.board, move, capture_sq);
	loc_ret = 0;
	if (!pos.is_attacked<white>(pos.get_ksq<white>())) {
		mark_check<white, PAWN, depth>(pos, epsq.second);
		apply_hash<white, depth>(pos, delta);
		loc_ret = count_moves<!white, depth - 1, false>(pos);
		undo_hash<depth>(pos, delta);
	}

	unmake_ep_move<white>(pos.board, move, capture_sq);

	if constexpr (print_move) print_movecnt(epsq.first, epsq.second, loc_ret);
	return loc_ret;
}

// Count nodes following from ep moves.
template <bool white, int depth, bool print_move>
static FORCE_INLINE uint64_t generate_ep_moves(Position& pos, uint8_t ep) {
	return (ep & 0x80 ? make_en_passant<white, -1, depth, print_move>(pos, ep) : 0)
		+ (ep & 0x40 ? make_en_passant<white, 1, depth, print_move>(pos, ep) : 0);
}

// Pawn double pushes.
template <bool white, int depth, bool print_move>
static FORCE_INLINE NodeCount generate_pawn_double(BitBoard targets, Position& pos, BitBoard source) {
	if constexpr (depth <= 1 && !print_move) {
		return popcnt(get_pawn_double<white>(source, pos.board.occ_board) & targets);
	} else {
		NodeCount ret = 0;
		while (source) {
			BitBoard from = popextr(source);
			BitBoard to_board = targets & get_pawn_double<white>(from, pos.board.occ_board);
			while (to_board) {
				BitBoard to = popextr(to_board);
				BitBoard move = from | to;
				Hash delta = zobrist::piece_key<white, PAWN>(lbit(from)) ^ zobrist::piece_key<white, PAWN>(lbit(to));
				bool ep = pawn_double<white>(pos.board, move, to, pos.ep_flag);
				mark_check<white, PAWN, depth>(pos, lbit(to));
				apply_hash<white, depth>(pos, delta);
				NodeCount loc_ret = ep ? count_moves<!white, depth - 1, false, true>(pos)
									   : count_moves<!white, depth - 1, false, false>(pos);
				undo_hash<depth>(pos, delta);
				unmake_pawn_double<white>(pos.board, move);
				if constexpr (print_move) print_movecnt(lbit(from), lbit(to), loc_ret);
				ret += loc_ret;
			}
		}
		return ret;
	}
}

// Concrete generator for pawn moves. Creates double pushes, pushes and captures, in bulk if possible.
template <bool white, int depth, bool print_move>
static FORCE_INLINE uint64_t generate_pawn_moves(BitBoard targets, BitBoard pieces, Position& pos) {
	if (!(targets && pieces)) return 0;
	BitBoard occ = pos.board.occ_board;
	BitBoard targets_free = targets & ~occ;
	BitBoard targets_captures = targets & occ;
	BitBoard pawns_on_start = pieces & (white ? pawn_start_w : pawn_start_b);
	uint64_t ret = generate_pawn_double<white, depth, print_move>(targets, pos, pawns_on_start);

	// Generate moves in bulk.
	if constexpr (depth <= 1 && !print_move) {
		ret += popcnt(get_pawn_forward<white>(pieces) & targets_free);
		ret += popcnt(get_pawn_left<white>(can_capture_left(pieces)) & targets_captures);
		ret += popcnt(get_pawn_right<white>(can_capture_right(pieces)) & targets_captures);
	} else {
		while (pieces) {
			Square from = pop(pieces);
			BitBoard captures = get_pawn_move<white, PawnMoveType::ATTACKS>(from, occ) & targets_captures;
			BitBoard non_captures = get_pawn_move<white, PawnMoveType::FORWARD>(from, occ) & targets_free;
			ret += count_plain<white, PAWN, depth, print_move>(non_captures, from, pos);
			ret += count_captures<white, PAWN, depth, print_move>(captures, from, pos);
		}
	}
	return ret;
}

template <bool white, Piece promo, int depth, bool print_move>
static FORCE_INLINE uint64_t make_promotion(Position& pos, Square from, Square to) {
	BitBoard to_mask = square_to_mask(to);
	BitBoard move = square_to_mask(from) | to_mask;
	const Piece captured = pos.board.get_piece<!white>(to);

	if (captured != EMPTY)
		capture_move_wrapper<white, PAWN>(pos.board, captured, move, to_mask);
	else
		plain_move<white, PAWN>(pos.board, move);
	toggle_promotion<white, promo>(pos.board, to_mask);
	mark_check<white, promo, depth>(pos, to);
	Hash delta = zobrist::piece_key<white, PAWN>(from) ^ zobrist::piece_key<white, promo>(to)
		^ zobrist::piece_key<!white>(captured, to);

	apply_hash<white, depth>(pos, delta);
	uint64_t ret = count_with_rights<white, depth>(pos, castle_mask[to]);
	undo_hash<depth>(pos, delta);

	toggle_promotion<white, promo>(pos.board, to_mask);
	if (captured != EMPTY)
		unmake_capture_wrapper<white, PAWN>(pos.board, captured, move, to_mask);
	else
		unmake_plain_move<white, PAWN>(pos.board, move);

	if constexpr (print_move) print_movecnt(from, to, ret, promo);
	return ret;
}

template <bool white, int depth, bool print_move>
static FORCE_INLINE uint64_t generate_promotions(BitBoard targets, BitBoard pieces, Position& pos) {
	if (!(targets && pieces)) return 0;
	BitBoard occ = pos.board.occ_board;
	BitBoard targets_free = targets & ~occ;
	BitBoard targets_captures = targets & occ;

	if constexpr (depth <= 1 && !print_move) {
		return 4
			* (popcnt(get_pawn_forward<white>(pieces) & targets_free)
			   + popcnt(get_pawn_left<white>(can_capture_left(pieces)) & targets_captures)
			   + popcnt(get_pawn_right<white>(can_capture_right(pieces)) & targets_captures));
	} else {
		uint64_t ret = 0;
		while (pieces) {
			Square from = pop(pieces);
			BitBoard to_squares = (get_pawn_move<white, PawnMoveType::ATTACKS>(from, occ) & targets_captures)
				| (get_pawn_move<white, PawnMoveType::FORWARD>(from, occ) & targets_free);
			while (to_squares) {
				Square to = pop(to_squares);
				ret += make_promotion<white, QUEEN, depth, print_move>(pos, from, to);
				ret += make_promotion<white, ROOK, depth, print_move>(pos, from, to);
				ret += make_promotion<white, BISHOP, depth, print_move>(pos, from, to);
				ret += make_promotion<white, KNIGHT, depth, print_move>(pos, from, to);
			}
		}
		return ret;
	}
}

// Wrapper.
template <bool white, int depth, bool print_move>
static FORCE_INLINE uint64_t generate_pawn(Position& pos, MaskSet& masks) {
	BitBoard can_move_from = pos.board.get_piece_board<white, PAWN>();
	BitBoard pawns_on_promo = can_move_from & (white ? promotion_from_w : promotion_from_b);
	can_move_from &= ~pawns_on_promo;
	BitBoard occ = pos.board.occ_board;
	BitBoard unpinned = can_move_from & masks.nopin;
	BitBoard pinned = can_move_from & ~masks.nopin;

	// Unpinned + pinned.
	uint64_t ret = generate_pawn_moves<white, depth, print_move>(masks.targets, unpinned, pos);
	if (pinned) {
		BitBoard dg_targets = masks.targets & masks.pinmask_dg & occ;
		BitBoard orth_targets = masks.targets & masks.pinmask_orth & ~occ;
		ret += generate_pawn_moves<white, depth, print_move>(dg_targets, pinned & masks.pinmask_dg, pos)
			+ generate_pawn_moves<white, depth, print_move>(orth_targets, pinned & masks.pinmask_orth, pos);
	}
	if (pawns_on_promo) {
		BitBoard dg_targets = masks.targets & masks.pinmask_dg & occ;
		BitBoard orth_targets = masks.targets & masks.pinmask_orth & ~occ;
		ret += generate_promotions<white, depth, print_move>(masks.targets, pawns_on_promo & masks.nopin, pos)
			+ generate_promotions<white, depth, print_move>(dg_targets, pawns_on_promo & masks.pinmask_dg, pos)
			+ generate_promotions<white, depth, print_move>(orth_targets, pawns_on_promo & masks.pinmask_orth, pos);
	}
	return ret;
}

/*
 *	SLIDERS
 */

template <bool white, int depth, bool print_move>
static FORCE_INLINE uint64_t generate_sliders(Position& pos, MaskSet& masks) {
	BitBoard bishops = pos.board.get_piece_board<white, BISHOP>();
	BitBoard rooks = pos.board.get_piece_board<white, ROOK>();
	BitBoard queens = pos.board.get_piece_board<white, QUEEN>();
	BitBoard unp_b = bishops & masks.nopin;
	BitBoard unp_r = rooks & masks.nopin;
	BitBoard unp_q = queens & masks.nopin;
	BitBoard pin_targets_diag = masks.targets & masks.pinmask_dg;
	BitBoard pin_targets_orth = masks.targets & masks.pinmask_orth;
	BitBoard dg_not_orth = masks.pinmask_dg & ~masks.pinmask_orth;
	BitBoard orth_not_dg = masks.pinmask_orth & ~masks.pinmask_dg;
	BitBoard pin_b = bishops & dg_not_orth;
	BitBoard pin_q_diag = queens & dg_not_orth;
	BitBoard pin_r = rooks & orth_not_dg;
	BitBoard pin_q_orth = queens & orth_not_dg;

#ifdef SETWISE_COUNT
	if constexpr (depth <= 1 && !print_move) {
		BitBoard knights = pos.piece_brd<white, KNIGHT>() & masks.nopin;
		return generate_moves<white, BISHOP, depth, print_move>(pin_targets_diag, pin_b, pos)
			+ generate_moves<white, QUEEN_DIAG, depth, print_move>(pin_targets_diag, pin_q_diag, pos)
			+ generate_moves<white, QUEEN_ORTH, depth, print_move>(pin_targets_orth, pin_q_orth, pos)
			+ generate_moves<white, ROOK, depth, print_move>(pin_targets_orth, pin_r, pos)
			+ count_setwise(unp_r | unp_q, unp_b | unp_q, knights, ~pos.board.occ_board, masks.targets);
	}
#endif

	// Pinned + unpinned.
	return generate_moves<white, BISHOP, depth, print_move>(pin_targets_diag, pin_b, pos)
		+ generate_moves<white, BISHOP, depth, print_move>(masks.targets, unp_b, pos)
		+ generate_moves<white, QUEEN_DIAG, depth, print_move>(pin_targets_diag, pin_q_diag, pos)
		+ generate_moves<white, QUEEN, depth, print_move>(masks.targets, unp_q, pos)
		+ generate_moves<white, QUEEN_ORTH, depth, print_move>(pin_targets_orth, pin_q_orth, pos)
		+ generate_moves<white, ROOK, depth, print_move>(pin_targets_orth, pin_r, pos)
		+ generate_moves<white, ROOK, depth, print_move>(masks.targets, unp_r, pos);
}

/*
 *	KNIGHTS
 */

// Count knight moves.
template <bool white, int depth, bool print_move>
static FORCE_INLINE NodeCount generate_knight(Position& pos, MaskSet& masks) {
#ifdef SETWISE_COUNT
	if constexpr (depth <= 1 && !print_move) return 0;
#endif
	BitBoard knights = pos.piece_brd<white, KNIGHT>() & masks.nopin;
	return generate_moves<white, KNIGHT, depth, print_move>(masks.targets, knights, pos);
}

/*
 *	MAIN
 */

template <bool white, int depth, bool print_move, bool ep>
static FORCE_INLINE uint64_t count_node(Position& pos) {
	{
		uint8_t ep_flag = ep ? pos.ep_flag : 0;
		BitBoard checker = pos.checker;
		pos.checker = 0;

		// Make masks.
		MaskSet local_masks;
		MaskSet& slot = depth <= 1 ? local_masks : pos.masks.go_next();
		MaskSet& masks = create_masks<white>(pos.board, pos.get_ksq<white>(), checker, slot);

		// King moves can always be generated.
		uint64_t ret = generate_king_moves<white, depth, print_move>(masks.targets, pos);

		// If double check, only king can move. Else, limit the target squares to the checkmask and skip castling.
		switch (masks.checkers) {
		case 0:
			ret += generate_castle_move<white, true, depth, print_move>(pos);
			ret += generate_castle_move<white, false, depth, print_move>(pos);
			break;
		case 1:
			masks.targets &= masks.check_mask;
			break;
		default:
			if constexpr (depth > 1) pos.masks.point_prev();
			return ret;
		}

		// Generate moves.
		ret += generate_sliders<white, depth, print_move>(pos, masks);
		ret += generate_pawn<white, depth, print_move>(pos, masks);
		ret += generate_knight<white, depth, print_move>(pos, masks);
		if constexpr (ep) ret += generate_ep_moves<white, depth, print_move>(pos, ep_flag);
		if constexpr (depth > 1) pos.masks.point_prev();
		return ret;
	}
}

// Main counting function.
template <bool white, int depth, bool print_move, bool ep>
NO_INLINE uint64_t count_moves(Position& pos) {
	if constexpr (depth < 1)
		return 1;
	else {
		TTBucket* bucket = nullptr;
		Hash key = 0;
		if constexpr (depth >= 2 && !print_move) {
			if (pos.tt) {
				Hash index_key = tt_index_key<white, depth>(pos);
				key = index_key ^ zobrist::keys.castling[pos.cr] ^ (ep ? zobrist::keys.en_passant[pos.ep_flag & 7] : 0);
				bucket = &pos.tt[index_key & pos.tt_mask];
				if (bucket->deep.key == key || bucket->recent.key == key) {
					pos.checker = 0;
					return bucket->deep.key == key ? bucket->deep.nodes() : bucket->recent.nodes();
				}
			}
		}

		uint64_t nodes = count_node<white, depth, print_move, ep>(pos);
		if (bucket) {
			TTEntry entry = {key, nodes | uint64_t(depth) << 56};
			if (depth >= bucket->deep.depth())
				bucket->deep = entry;
			else
				bucket->recent = entry;
		}
		return nodes;
	}
}

}  // namespace pyke
