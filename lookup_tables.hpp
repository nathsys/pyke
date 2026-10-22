#pragma once

#include <immintrin.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>

#include "defaults.hpp"
#include "util.hpp"

// Lookup tables for piece moves.
// Bishop and rook tables are created below, these are made with a bit of help from "Chess Programming"'s YouTube
// videos.
constexpr uint64_t KING_MOVE_SQUARES[] = {
	4665729213955833856ULL,
	11592265440851656704ULL,
	5796132720425828352ULL,
	2898066360212914176ULL,
	1449033180106457088ULL,
	724516590053228544ULL,
	362258295026614272ULL,
	144959613005987840ULL,
	13853283560024178688ULL,
	16186183351374184448ULL,
	8093091675687092224ULL,
	4046545837843546112ULL,
	2023272918921773056ULL,
	1011636459460886528ULL,
	505818229730443264ULL,
	216739030602088448ULL,
	54114388906344448ULL,
	63227278716305408ULL,
	31613639358152704ULL,
	15806819679076352ULL,
	7903409839538176ULL,
	3951704919769088ULL,
	1975852459884544ULL,
	846636838289408ULL,
	211384331665408ULL,
	246981557485568ULL,
	123490778742784ULL,
	61745389371392ULL,
	30872694685696ULL,
	15436347342848ULL,
	7718173671424ULL,
	3307175149568ULL,
	825720045568ULL,
	964771708928ULL,
	482385854464ULL,
	241192927232ULL,
	120596463616ULL,
	60298231808ULL,
	30149115904ULL,
	12918652928ULL,
	3225468928ULL,
	3768639488ULL,
	1884319744ULL,
	942159872ULL,
	471079936ULL,
	235539968ULL,
	117769984ULL,
	50463488ULL,
	12599488ULL,
	14721248ULL,
	7360624ULL,
	3680312ULL,
	1840156ULL,
	920078ULL,
	460039ULL,
	197123ULL,
	49216ULL,
	57504ULL,
	28752ULL,
	14376ULL,
	7188ULL,
	3594ULL,
	1797ULL,
	770ULL
};

constexpr uint64_t KNIGHT_MOVE_SQUARES[] = {
	9077567998918656ULL,
	4679521487814656ULL,
	38368557762871296ULL,
	19184278881435648ULL,
	9592139440717824ULL,
	4796069720358912ULL,
	2257297371824128ULL,
	1128098930098176ULL,
	2305878468463689728ULL,
	1152939783987658752ULL,
	9799982666336960512ULL,
	4899991333168480256ULL,
	2449995666584240128ULL,
	1224997833292120064ULL,
	576469569871282176ULL,
	288234782788157440ULL,
	4620693356194824192ULL,
	11533718717099671552ULL,
	5802888705324613632ULL,
	2901444352662306816ULL,
	1450722176331153408ULL,
	725361088165576704ULL,
	362539804446949376ULL,
	145241105196122112ULL,
	18049583422636032ULL,
	45053588738670592ULL,
	22667534005174272ULL,
	11333767002587136ULL,
	5666883501293568ULL,
	2833441750646784ULL,
	1416171111120896ULL,
	567348067172352ULL,
	70506185244672ULL,
	175990581010432ULL,
	88545054707712ULL,
	44272527353856ULL,
	22136263676928ULL,
	11068131838464ULL,
	5531918402816ULL,
	2216203387392ULL,
	275414786112ULL,
	687463207072ULL,
	345879119952ULL,
	172939559976ULL,
	86469779988ULL,
	43234889994ULL,
	21609056261ULL,
	8657044482ULL,
	1075839008ULL,
	2685403152ULL,
	1351090312ULL,
	675545156ULL,
	337772578ULL,
	168886289ULL,
	84410376ULL,
	33816580ULL,
	4202496ULL,
	10489856ULL,
	5277696ULL,
	2638848ULL,
	1319424ULL,
	659712ULL,
	329728ULL,
	132096ULL
};

// Create lookup tables for bishops and rooks.

constexpr uint64_t make_bishop_mask(uint8_t square) {
	square = 63 - square;
	// result attacks bitboard.
	uint64_t attacks = 0ULL;

	// init ranks & files.
	int r = 0;
	int f = 0;

	// init target rank & files.
	int tr = square / 8;
	int tf = square % 8;

	// mask relevant bishop occupancy bits.
	for (r = tr + 1, f = tf + 1; r <= 6 && f <= 6; r++, f++) attacks |= (1ULL << (r * 8 + f));
	for (r = tr - 1, f = tf + 1; r >= 1 && f <= 6; r--, f++) attacks |= (1ULL << (r * 8 + f));
	for (r = tr + 1, f = tf - 1; r <= 6 && f >= 1; r++, f--) attacks |= (1ULL << (r * 8 + f));
	for (r = tr - 1, f = tf - 1; r >= 1 && f >= 1; r--, f--) attacks |= (1ULL << (r * 8 + f));

	// return attack map.
	return attacks;
}

constexpr uint64_t make_rook_mask(uint8_t square) {
	square = 63 - square;
	// result attacks bitboard.
	uint64_t attacks = 0ULL;

	// init ranks & files.
	int r = 0;
	int f = 0;

	// init target rank & files.
	int tr = square / 8;
	int tf = square % 8;

	// mask relevant rook occupancy bits.
	for (r = tr + 1; r <= 6; r++) attacks |= (1ULL << (r * 8 + tf));
	for (r = tr - 1; r >= 1; r--) attacks |= (1ULL << (r * 8 + tf));
	for (f = tf + 1; f <= 6; f++) attacks |= (1ULL << (tr * 8 + f));
	for (f = tf - 1; f >= 1; f--) attacks |= (1ULL << (tr * 8 + f));

	// return attack map.
	return attacks;
}

// Bishop moving logic.
constexpr uint64_t bishop_attack_on_fly(uint8_t square, uint64_t occupation) {
	uint64_t move_board = 0b0;

	int square_copy = square;
	uint64_t bit_mask = 1ULL << (63 - square);
	// Continue until another piece is found. Repeat for each direction.
	// Up left.
	while (square_copy >= 0 && square_copy % 8 != 0) {
		bit_mask <<= 9;
		square_copy -= 9;
		move_board |= bit_mask;
		if (occupation & bit_mask) break;  // Stop if a piece is found.
	}

	square_copy = square;
	bit_mask = 1ULL << (63 - square);
	// Move up-right direction.
	while (square_copy >= 0 && square_copy % 8 != 7) {
		bit_mask <<= 7;
		square_copy -= 7;
		move_board |= bit_mask;
		if (occupation & bit_mask) break;  // Stop if a piece is found.
	}

	square_copy = square;
	bit_mask = 1ULL << (63 - square);

	// Move down-right direction.
	while (square_copy < 64 && square_copy % 8 != 7) {
		bit_mask >>= 9;
		square_copy += 9;
		move_board |= bit_mask;
		if (occupation & bit_mask) break;  // Stop if a piece is found.
	}

	square_copy = square;
	bit_mask = 1ULL << (63 - square);

	// Move down-left direction.
	while (square_copy < 64 && square_copy % 8 != 0) {
		bit_mask >>= 7;
		square_copy += 7;
		move_board |= bit_mask;
		if (occupation & bit_mask) break;  // Stop if a piece is found.
	}

	return move_board;
}

// Rook move logic.
constexpr uint64_t rook_attack_on_fly(uint8_t square, uint64_t occupation) {
	uint64_t move_board = 0b0;
	int square_copy = square;
	uint64_t bit_mask = 1ULL << (63 - square);
	// Continue until another piece is found. Repeat for each direction.
	// left.
	while (square_copy >= 0 && square_copy % 8 != 0) {
		bit_mask <<= 1;
		square_copy--;
		move_board |= bit_mask;
		if (occupation & bit_mask) break;  // Stop if a piece is found.
	}

	square_copy = square;
	bit_mask = 1ULL << (63 - square);

	// Move up.
	while (square_copy >= 0) {
		bit_mask <<= 8;
		square_copy -= 8;
		move_board |= bit_mask;
		if (occupation & bit_mask) break;  // Stop if a piece is found.
	}

	square_copy = square;
	bit_mask = 1ULL << (63 - square);

	// Move right.
	while (square_copy < 64 && square_copy % 8 != 7) {
		bit_mask >>= 1;
		square_copy++;
		move_board |= bit_mask;
		if (occupation & bit_mask) break;  // Stop if a piece is found.
	}

	square_copy = square;
	bit_mask = 1ULL << (63 - square);

	// Move down.
	while (square_copy < 64) {
		bit_mask >>= 8;
		square_copy += 8;
		move_board |= bit_mask;
		if (occupation & bit_mask) break;  // Stop if a piece is found.
	}

	return move_board;
}

// Make lookup tables for the masks.
constexpr std::array<uint64_t, 64> make_bishop_masks() {
	std::array<uint64_t, 64> values{};
	for (int square = 0; square < 64; square++) {
		// Make masks for all squares.
		values[square] = make_bishop_mask(square);
	}
	return values;
}

constexpr std::array<uint64_t, 64> make_rook_masks() {
	std::array<uint64_t, 64> values{};
	for (int square = 0; square < 64; square++) {
		// Make masks for all squares.
		values[square] = make_rook_mask(square);
	}
	return values;
}

// Mask lookup table instances.
constexpr std::array<uint64_t, 64> bishop_mask_table = make_bishop_masks();
constexpr std::array<uint64_t, 64> rook_mask_table = make_rook_masks();

constexpr std::array<std::array<uint64_t, 64>, 2> make_pawn_edge_masks() {
	std::array<std::array<uint64_t, 64>, 2> ret = {};

	for (int s = 0; s < 64; s++) {
		ret[0][s] = (0xFF00ULL << (56 - (s & ~7)));
		ret[1][s] = (0xFFULL << (48 - (s & ~7)));
	}

	return ret;
}

static const std::array<std::array<uint64_t, 64>, 2> pawn_edge_masks = make_pawn_edge_masks();

// PEXT

static constexpr uint32_t rook_pext_size = 102400;
static constexpr uint32_t bishop_pext_size = 5248;

// Rook offsets.
static std::array<BitBoard, 64> init_rook_pext_offset() {
	std::array<BitBoard, 64> ret;
	uint32_t offset = 0;
	for (Square s = 0; s < 64; s++) {
		uint32_t perm_amount = 1 << popcnt(rook_mask_table[s]);
		ret[s] = offset;
		offset += perm_amount;
	}

	return ret;
}

static const std::array<BitBoard, 64> rook_pext_offset = init_rook_pext_offset();

// Bishop offsets.
static std::array<BitBoard, 64> init_bishop_pext_offset() {
	std::array<BitBoard, 64> ret;
	uint32_t offset = 0;
	for (Square s = 0; s < 64; s++) {
		uint32_t perm_amount = 1 << popcnt(bishop_mask_table[s]);
		ret[s] = offset;
		offset += perm_amount;
	}

	return ret;
}

static const std::array<BitBoard, 64> bishop_pext_offset = init_bishop_pext_offset();

// Bishop attacks.
static std::array<BitBoard, bishop_pext_size> init_bishop_pext_atk() {
	std::array<BitBoard, bishop_pext_size> ret{};
	uint32_t offset = 0;
	for (Square s = 0; s < 64; ++s) {
		uint32_t mask_bits = popcnt(bishop_mask_table[s]);
		uint32_t perm_amount = 1 << mask_bits;

		for (uint32_t perm = 0; perm < perm_amount; ++perm) {
			BitBoard blocker = _pdep_u64(static_cast<uint64_t>(perm), bishop_mask_table[s]);
			ret[offset + perm] = bishop_attack_on_fly(s, blocker);
		}
		offset += perm_amount;
	}

	return ret;
}
static const std::array<BitBoard, bishop_pext_size> bishop_pext_atk = init_bishop_pext_atk();

// Rook attacks.
static std::array<BitBoard, rook_pext_size> init_rook_pext_atk() {
	std::array<BitBoard, rook_pext_size> ret{};
	uint32_t offset = 0;
	for (Square s = 0; s < 64; ++s) {
		uint32_t mask_bits = popcnt(rook_mask_table[s]);
		uint32_t perm_amount = 1 << mask_bits;

		for (uint32_t perm = 0; perm < perm_amount; ++perm) {
			BitBoard blocker = _pdep_u64(static_cast<uint64_t>(perm), rook_mask_table[s]);
			ret[offset + perm] = rook_attack_on_fly(s, blocker);
		}
		offset += perm_amount;
	}

	return ret;
}
static const std::array<BitBoard, rook_pext_size> rook_pext_atk = init_rook_pext_atk();

// En passant squares.
static constexpr inline std::pair<Square, Square> ep_sqs_wl[8] =
	{{0, 0}, {24, 17}, {25, 18}, {26, 19}, {27, 20}, {28, 21}, {29, 22}, {30, 23}};

static constexpr inline std::pair<Square, Square> ep_sqs_wr[8] =
	{{25, 16}, {26, 17}, {27, 18}, {28, 19}, {29, 20}, {30, 21}, {31, 22}, {0, 0}};

static constexpr inline std::pair<Square, Square> ep_sqs_bl[8] =
	{{0, 0}, {32, 41}, {33, 42}, {34, 43}, {35, 44}, {36, 45}, {37, 46}, {38, 47}};

static constexpr inline std::pair<Square, Square> ep_sqs_br[8] =
	{{33, 40}, {34, 41}, {35, 42}, {36, 43}, {37, 44}, {38, 45}, {39, 46}, {0, 0}};

