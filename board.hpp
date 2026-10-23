#pragma once

#include <stdexcept>

#include "defaults.hpp"
#include "util.hpp"

struct Board {
	BitBoard w_pawn = INIT_PAWN_SQUARES & INIT_WHITE_PIECES;
	BitBoard w_king = INIT_KING_SQUARES & INIT_WHITE_PIECES;
	BitBoard w_rook = INIT_ROOK_SQUARES & INIT_WHITE_PIECES;
	BitBoard w_bishop = INIT_BISHOP_SQUARES & INIT_WHITE_PIECES;
	BitBoard w_knight = INIT_KNIGHT_SQUARES & INIT_WHITE_PIECES;
	BitBoard w_queen = INIT_QUEEN_SQUARES & INIT_WHITE_PIECES;

	BitBoard b_pawn = INIT_PAWN_SQUARES & INIT_BLACK_PIECES;
	BitBoard b_king = INIT_KING_SQUARES & INIT_BLACK_PIECES;
	BitBoard b_rook = INIT_ROOK_SQUARES & INIT_BLACK_PIECES;
	BitBoard b_bishop = INIT_BISHOP_SQUARES & INIT_BLACK_PIECES;
	BitBoard b_knight = INIT_KNIGHT_SQUARES & INIT_BLACK_PIECES;
	BitBoard b_queen = INIT_QUEEN_SQUARES & INIT_BLACK_PIECES;

	BitBoard w_board = INIT_WHITE_PIECES;
	BitBoard b_board = INIT_BLACK_PIECES;

	BitBoard occ_board = INIT_TOTAL_SQUARES;

	void clear() {
		w_pawn = w_king = w_rook = w_bishop = w_knight = w_queen = 0;
		b_pawn = b_king = b_rook = b_bishop = b_knight = b_queen = 0;
		w_board = b_board = occ_board = 0;
	}

	// Returns the player's occupancy board.
	template <bool white>
	inline BitBoard get_player_occ() {
		return white ? w_board : b_board;
	}

	template <bool white, Piece p>
	FORCE_INLINE BitBoard* get_board_pointer() {
		switch (p) {
		case PAWN:
			return white ? &w_pawn : &b_pawn;
		case KING:
			return white ? &w_king : &b_king;
		case ROOK:
			return white ? &w_rook : &b_rook;
		case BISHOP:
			return white ? &w_bishop : &b_bishop;
		case KNIGHT:
			return white ? &w_knight : &b_knight;
		case QUEEN:
		case QUEEN_DIAG:
		case QUEEN_ORTH:
			return white ? &w_queen : &b_queen;
		}
	}

	inline BitBoard* get_board_pointer(bool white, Piece p) {
		switch (p) {
		case PAWN:
			return white ? &w_pawn : &b_pawn;
		case KING:
			return white ? &w_king : &b_king;
		case ROOK:
			return white ? &w_rook : &b_rook;
		case BISHOP:
			return white ? &w_bishop : &b_bishop;
		case KNIGHT:
			return white ? &w_knight : &b_knight;
		case QUEEN:
			return white ? &w_queen : &b_queen;
		}
		throw std::invalid_argument("Board does not exist.");
	}

	// Gets the board for a given color and piece.
	template <bool white, Piece p>
	FORCE_INLINE BitBoard get_piece_board() {
		return *get_board_pointer<white, p>();
	}

	// Returns the piece on the square or EMPTY by default.
	template <bool white>
	FORCE_INLINE Piece get_piece(Square square) {
		BitBoard mask = square_to_mask(square);
		if (get_piece_board<white, PAWN>() & mask) return PAWN;
		if (get_piece_board<white, KNIGHT>() & mask) return KNIGHT;
		if (get_piece_board<white, BISHOP>() & mask) return BISHOP;
		if (get_piece_board<white, ROOK>() & mask) return ROOK;
		if (get_piece_board<white, QUEEN>() & mask) return QUEEN;
		if (get_piece_board<white, KING>() & mask) return KING;
		return EMPTY;
	}

	// Returns whether a square is occupied or not.
	inline bool square_occ(Square square) { return get_bit_64(occ_board, square); }

	void print_board();
};

