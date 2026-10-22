#pragma once

#include <array>
#include <cstdint>

#include "defaults.hpp"
#include "lookup_tables.hpp"

constexpr inline CastlingRights rm_cr_w(CastlingRights flag) { return flag & 0b0011; }
constexpr inline CastlingRights rm_cr_b(CastlingRights flag) { return flag & 0b1100; }
constexpr inline CastlingRights rm_cr_wk(CastlingRights flag) { return flag & ~wk_mask; }
constexpr inline CastlingRights rm_cr_wq(CastlingRights flag) { return flag & ~wq_mask; }
constexpr inline CastlingRights rm_cr_bk(CastlingRights flag) { return flag & ~bk_mask; }
constexpr inline CastlingRights rm_cr_bq(CastlingRights flag) { return flag & ~bq_mask; }

inline constexpr std::array<CastlingRights, 64> castle_mask = [] {
	std::array<CastlingRights, 64> mask{};
	mask.fill(0b1111);
	mask[60] = rm_cr_w(0b1111);
	mask[63] = rm_cr_wk(0b1111);
	mask[56] = rm_cr_wq(0b1111);
	mask[4] = rm_cr_b(0b1111);
	mask[7] = rm_cr_bk(0b1111);
	mask[0] = rm_cr_bq(0b1111);
	return mask;
}();

inline void set_en_passant(bool left, uint8_t file, uint8_t& data) {
	if (left) {
		data |= 0b1000'0000;
	} else {
		data |= 0b0100'0000;
	}
	data |= file;
}

template <bool white, int offset>
inline std::pair<Square, Square> get_ep_squares(uint8_t ep) {
	if constexpr (white && (offset == -1)) {
		return ep_sqs_wl[ep & 0b00001111];
	} else if constexpr (white) {
		return ep_sqs_wr[ep & 0b00001111];
	} else if constexpr (!white && (offset == -1)) {
		return ep_sqs_bl[ep & 0b00001111];
	} else {
		return ep_sqs_br[ep & 0b00001111];
	}
}
