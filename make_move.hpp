#pragma once

#include <cstdint>
#include <string>

#include "defaults.hpp"
#include "position.hpp"

// Add piece to board.
template <bool white, Piece p>
static FORCE_INLINE void add_to_board(Board& b, BitBoard mask) {
	*b.get_board_pointer<white, p>() |= mask;
	b.occ_board |= mask;
	if constexpr (white)
		b.w_board |= mask;
	else
		b.b_board |= mask;
}

// Remove piece from board.
template <bool white, Piece p>
static FORCE_INLINE void remove_from_board(Board& b, BitBoard mask) {
	*b.get_board_pointer<white, p>() ^= mask;
	b.occ_board ^= mask;
	if constexpr (white)
		b.w_board ^= mask;
	else
		b.b_board ^= mask;
}

// Move a piece.
template <bool white, Piece p>
static FORCE_INLINE void move_piece(Board& b, BitBoard move) {
	*b.get_board_pointer<white, p>() ^= move;
	b.occ_board ^= move;
	if constexpr (white)
		b.w_board ^= move;
	else
		b.b_board ^= move;
}

// Undo a piece move.
template <bool white, Piece p>
static FORCE_INLINE void unmake_move_piece(Board& b, BitBoard move) {
	move_piece<white, p>(b, move);
}

// Do a plain, non capturing move.
template <bool white, Piece p>
static FORCE_INLINE void plain_move(Board& b, BitBoard move) {
	move_piece<white, p>(b, move);
}

// Undo a non capturing move.
template <bool white, Piece p>
static FORCE_INLINE void unmake_plain_move(Board& b, BitBoard move) {
	move_piece<white, p>(b, move);
}

// Castle and update castling rights.
template <bool white, uint8_t code>
static FORCE_INLINE void castle_move(Board& b) {
	move_piece<white, KING>(b, square_to_mask(king_start_squares[code]) | square_to_mask(king_end_squares[code]));
	move_piece<white, ROOK>(b, square_to_mask(rook_start_squares[code]) | square_to_mask(rook_end_squares[code]));
}

// Undo castling move.
template <bool white, uint8_t code>
static FORCE_INLINE void unmake_castle_move(Board& b) {
	unmake_move_piece<white, KING>(
		b, square_to_mask(king_start_squares[code]) | square_to_mask(king_end_squares[code])
	);
	unmake_move_piece<white, ROOK>(
		b, square_to_mask(rook_start_squares[code]) | square_to_mask(rook_end_squares[code])
	);
}

// Do capture move.
template <bool white, Piece p, Piece captured>
static FORCE_INLINE void capture_move(Board& b, BitBoard move, BitBoard to) {
	remove_from_board<!white, captured>(b, to);
	move_piece<white, p>(b, move);
}

// Undo capture move.
template <bool white, Piece p, Piece captured>
static FORCE_INLINE void unmake_capture_move(Board& b, BitBoard move, BitBoard to) {
	unmake_move_piece<white, p>(b, move);
	add_to_board<!white, captured>(b, to);
}

// Do ep move.
template <bool white>
static FORCE_INLINE void ep_move(Board& b, BitBoard move, BitBoard capture_sq) {
	move_piece<white, PAWN>(b, move);
	remove_from_board<!white, PAWN>(b, capture_sq);
}

// Undo ep move.
template <bool white>
static FORCE_INLINE void unmake_ep_move(Board& b, BitBoard move, BitBoard capture_sq) {
	unmake_move_piece<white, PAWN>(b, move);
	add_to_board<!white, PAWN>(b, capture_sq);
}

// Do pawn double forward move.
template <bool white>
static FORCE_INLINE bool pawn_double(Board& b, BitBoard move, BitBoard to, uint8_t& ep_flag) {
	move_piece<white, PAWN>(b, move);

	// pawn moved two forward, update en passant status.
	bool left_is_opp_pawn = b.get_piece_board<!white, PAWN>() & (to << 1);
	bool right_is_opp_pawn = b.get_piece_board<!white, PAWN>() & (to >> 1);
	File file = lbit(to) % 8;

	// Edge of board cases.
	bool edge_left = file == 0;
	bool edge_right = file == 7;

	// Check if en passant is possible.
	bool possible_left = left_is_opp_pawn && !edge_left;
	bool possible_right = right_is_opp_pawn && !edge_right;

	// Update flag.
	ep_flag = 0;
	if (possible_left) set_en_passant(true, file, ep_flag);
	if (possible_right) set_en_passant(false, file, ep_flag);

	return possible_left || possible_right;
}

// Undo double pawn move.
template <bool white>
static FORCE_INLINE void unmake_pawn_double(Board& b, BitBoard move) {
	unmake_move_piece<white, PAWN>(b, move);
}

template <bool white, Piece p>
static FORCE_INLINE void toggle_promotion(Board& b, BitBoard to) {
	*b.get_board_pointer<white, PAWN>() ^= to;
	*b.get_board_pointer<white, p>() ^= to;
}

template <bool white, Piece p>
static FORCE_INLINE void capture_move_wrapper(Board& b, Piece captured, BitBoard move, BitBoard capture_sq) {
	switch (captured) {
	case PAWN:
		capture_move<white, p, PAWN>(b, move, capture_sq);
		break;
	case KNIGHT:
		capture_move<white, p, KNIGHT>(b, move, capture_sq);
		break;
	case BISHOP:
		capture_move<white, p, BISHOP>(b, move, capture_sq);
		break;
	case ROOK:
		capture_move<white, p, ROOK>(b, move, capture_sq);
		break;
	case QUEEN:
		capture_move<white, p, QUEEN>(b, move, capture_sq);
		break;
	}
}

template <bool white, Piece p>
static FORCE_INLINE void unmake_capture_wrapper(Board& b, Piece captured, BitBoard move, BitBoard capture_sq) {
	switch (captured) {
	case PAWN:
		unmake_capture_move<white, p, PAWN>(b, move, capture_sq);
		break;
	case KNIGHT:
		unmake_capture_move<white, p, KNIGHT>(b, move, capture_sq);
		break;
	case BISHOP:
		unmake_capture_move<white, p, BISHOP>(b, move, capture_sq);
		break;
	case ROOK:
		unmake_capture_move<white, p, ROOK>(b, move, capture_sq);
		break;
	case QUEEN:
		unmake_capture_move<white, p, QUEEN>(b, move, capture_sq);
		break;
	}
}

// Add piece to board.
static void add_to_board(Board& b, Square s, Piece p, bool white) {
	BitBoard mask = square_to_mask(s);
	*b.get_board_pointer(white, p) |= mask;
	b.occ_board |= mask;
	if (white)
		b.w_board |= mask;
	else
		b.b_board |= mask;
}

// Remove piece from board.
static void remove_from_board(Board& b, Square s, Piece p, bool white) {
	BitBoard mask = ~square_to_mask(s);
	*b.get_board_pointer(white, p) &= mask;
	b.occ_board &= mask;
	if (white)
		b.w_board &= mask;
	else
		b.b_board &= mask;
}

// Move a piece.
static void move_piece(Square from, Square to, Board& b, bool white, Piece p) {
	remove_from_board(b, from, p, white);
	add_to_board(b, to, p, white);
}

static void move_from_string(const std::string& m, Position& p) {
	Square from = notation_to_square(m.substr(0, 2));
	Square to = notation_to_square(m.substr(2, 2));
	bool white = p.white_turn;
	Piece moved = white ? p.board.get_piece<true>(from) : p.board.get_piece<false>(from);
	Piece captured = white ? p.board.get_piece<false>(to) : p.board.get_piece<true>(to);

	if (captured != EMPTY) remove_from_board(p.board, to, captured, !white);
	if (moved == PAWN && captured == EMPTY && from % 8 != to % 8)
		remove_from_board(p.board, white ? to + 8 : to - 8, PAWN, !white);
	move_piece(from, to, p.board, white, moved);

	if (m.size() > 4) {
		Piece promo = m[4] == 'q' ? QUEEN : m[4] == 'r' ? ROOK : m[4] == 'b' ? BISHOP : KNIGHT;
		remove_from_board(p.board, to, PAWN, white);
		add_to_board(p.board, to, promo, white);
	}

	if (moved == KING && (to == from + 2 || to + 2 == from)) {
		bool kingside = to > from;
		move_piece(kingside ? to + 1 : to - 2, kingside ? to - 1 : to + 1, p.board, white, ROOK);
	}

	if (moved == KING) {
		if (white)
			p.set_ksq<true>(to);
		else
			p.set_ksq<false>(to);
	}

	p.white_turn = !white;
}
