#pragma once

#include <cstdint>
#include <utility>

typedef uint8_t Square;
typedef uint8_t Piece;
typedef uint64_t BitBoard;
typedef uint8_t File;
typedef std::pair<Square, Square> sq_pair;
typedef uint8_t CastlingRights;
typedef uint8_t MoveType;
typedef uint64_t NodeCount;
typedef uint64_t Hash;

// Default bitboards.
constexpr uint64_t INIT_ROOK_SQUARES = 0b1000000100000000000000000000000000000000000000000000000010000001ULL;
constexpr uint64_t INIT_KNIGHT_SQUARES = 0b0100001000000000000000000000000000000000000000000000000001000010ULL;
constexpr uint64_t INIT_BISHOP_SQUARES = 0b0010010000000000000000000000000000000000000000000000000000100100ULL;
constexpr uint64_t INIT_QUEEN_SQUARES = 0b0001000000000000000000000000000000000000000000000000000000010000ULL;
constexpr uint64_t INIT_KING_SQUARES = 0b0000100000000000000000000000000000000000000000000000000000001000ULL;
constexpr uint64_t INIT_PAWN_SQUARES = 0b0000000011111111000000000000000000000000000000001111111100000000ULL;
constexpr uint64_t INIT_BLACK_PIECES = 0b1111111111111111000000000000000000000000000000000000000000000000ULL;
constexpr uint64_t INIT_TOTAL_SQUARES = 0b1111111111111111000000000000000000000000000000001111111111111111ULL;
constexpr uint64_t INIT_WHITE_PIECES = 0b0000000000000000000000000000000000000000000000001111111111111111ULL;

// Pieces.
inline constexpr Piece EMPTY = 0;
inline constexpr Piece PAWN = 1;
inline constexpr Piece KING = 2;
inline constexpr Piece ROOK = 3;
inline constexpr Piece BISHOP = 4;
inline constexpr Piece KNIGHT = 5;
inline constexpr Piece QUEEN = 6;
inline constexpr Piece QUEEN_DIAG = 7;
inline constexpr Piece QUEEN_ORTH = 8;

inline constexpr int popcnt(uint64_t b) { return __builtin_popcountll(b); }

#define FORCE_INLINE inline __attribute__((always_inline))
#define NO_INLINE	 __attribute__((noinline))

enum class PawnMoveType { ATTACKS, FORWARD, DOUBLE_FORWARD, NON_DOUBLE, ALL };

inline constexpr BitBoard pawn_start_w = INIT_PAWN_SQUARES & INIT_WHITE_PIECES;
inline constexpr BitBoard pawn_start_b = INIT_PAWN_SQUARES & INIT_BLACK_PIECES;
inline constexpr BitBoard promotion_from_w = pawn_start_b;
inline constexpr BitBoard promotion_from_b = pawn_start_w;

inline constexpr uint8_t king_start_squares[4] = {60, 60, 4, 4};
inline constexpr uint8_t rook_start_squares[4] = {63, 56, 7, 0};
inline constexpr uint8_t king_end_squares[4] = {62, 58, 6, 2};
inline constexpr uint8_t rook_end_squares[4] = {61, 59, 5, 3};
inline constexpr uint8_t queenside_middle_squares[2] = {1, 57};

inline constexpr CastlingRights bq_mask = 0b0001;
inline constexpr CastlingRights bk_mask = 0b0010;
inline constexpr CastlingRights wq_mask = 0b0100;
inline constexpr CastlingRights wk_mask = 0b1000;

inline constexpr BitBoard can_capture_left(BitBoard b) { return b & 0x7F7F7F7F7F7F7F7FULL; }
inline constexpr BitBoard can_capture_right(BitBoard b) { return b & 0xFEFEFEFEFEFEFEFEULL; }

