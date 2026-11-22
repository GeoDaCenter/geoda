/**
 * GeoDa TM, Copyright (C) 2011-2015 by Luc Anselin - all rights reserved
 *
 * This file is part of GeoDa.
 *
 * GeoDa is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GeoDa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GEODA_CENTER_SPREG_SHA256_H__
#define __GEODA_CENTER_SPREG_SHA256_H__

#include <cstdint>
#include <cstring>
#include <string>

/**
 * SHA-256 (FIPS 180-4), self contained, no dependencies.
 *
 * GeoDa already links libcurl, but curl offers no digest API that is portable
 * across backends (Secure Transport on macOS, OpenSSL elsewhere), and pulling
 * in OpenSSL for one hash would be a new build dependency on every platform.
 * The engine download is verified against a digest that GeoDa ships, so this
 * has to be local and correct; it is checked against the standard test vectors
 * in spreg_engine/tools/test_sha256.cpp.
 *
 * Header only, and free of wx, so that it can be tested on its own.
 */
namespace SpregEngine {

class Sha256 {
public:
	Sha256() { Reset(); }

	void Reset()
	{
		h_[0] = 0x6a09e667u; h_[1] = 0xbb67ae85u; h_[2] = 0x3c6ef372u; h_[3] = 0xa54ff53au;
		h_[4] = 0x510e527fu; h_[5] = 0x9b05688cu; h_[6] = 0x1f83d9abu; h_[7] = 0x5be0cd19u;
		length_ = 0;
		pending_ = 0;
	}

	void Update(const void* data, size_t len)
	{
		const unsigned char* p = static_cast<const unsigned char*>(data);
		length_ += static_cast<uint64_t>(len) * 8;
		while (len > 0) {
			size_t take = 64 - pending_;
			if (take > len) take = len;
			std::memcpy(block_ + pending_, p, take);
			pending_ += take;
			p += take;
			len -= take;
			if (pending_ == 64) {
				Transform(block_);
				pending_ = 0;
			}
		}
	}

	/** Finishes the hash.  Call once: the state is left in the finalised form. */
	void Final(unsigned char out[32])
	{
		const uint64_t bits = length_;
		block_[pending_++] = 0x80;
		if (pending_ > 56) {
			while (pending_ < 64) block_[pending_++] = 0;
			Transform(block_);
			pending_ = 0;
		}
		while (pending_ < 56) block_[pending_++] = 0;
		for (int i = 7; i >= 0; --i) {
			block_[pending_++] = static_cast<unsigned char>((bits >> (8 * i)) & 0xff);
		}
		Transform(block_);
		pending_ = 0;
		for (int i = 0; i < 8; ++i) {
			out[4 * i + 0] = static_cast<unsigned char>((h_[i] >> 24) & 0xff);
			out[4 * i + 1] = static_cast<unsigned char>((h_[i] >> 16) & 0xff);
			out[4 * i + 2] = static_cast<unsigned char>((h_[i] >> 8) & 0xff);
			out[4 * i + 3] = static_cast<unsigned char>(h_[i] & 0xff);
		}
	}

	/** Lower-case hex digest; finishes the hash, so call it last. */
	std::string HexDigest()
	{
		unsigned char out[32];
		Final(out);
		static const char* digits = "0123456789abcdef";
		std::string hex;
		hex.reserve(64);
		for (int i = 0; i < 32; ++i) {
			hex.push_back(digits[(out[i] >> 4) & 0x0f]);
			hex.push_back(digits[out[i] & 0x0f]);
		}
		return hex;
	}

private:
	static uint32_t Rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

	void Transform(const unsigned char* chunk)
	{
		static const uint32_t K[64] = {
			0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
			0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
			0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
			0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
			0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
			0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
			0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
			0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
			0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
			0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
			0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
			0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
			0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
			0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
			0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
			0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
		};

		uint32_t w[64];
		for (int i = 0; i < 16; ++i) {
			w[i] = (static_cast<uint32_t>(chunk[4 * i + 0]) << 24)
				 | (static_cast<uint32_t>(chunk[4 * i + 1]) << 16)
				 | (static_cast<uint32_t>(chunk[4 * i + 2]) << 8)
				 | static_cast<uint32_t>(chunk[4 * i + 3]);
		}
		for (int i = 16; i < 64; ++i) {
			const uint32_t s0 = Rotr(w[i - 15], 7) ^ Rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
			const uint32_t s1 = Rotr(w[i - 2], 17) ^ Rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
			w[i] = w[i - 16] + s0 + w[i - 7] + s1;
		}

		uint32_t a = h_[0], b = h_[1], c = h_[2], d = h_[3];
		uint32_t e = h_[4], f = h_[5], g = h_[6], hh = h_[7];
		for (int i = 0; i < 64; ++i) {
			const uint32_t S1 = Rotr(e, 6) ^ Rotr(e, 11) ^ Rotr(e, 25);
			const uint32_t ch = (e & f) ^ ((~e) & g);
			const uint32_t t1 = hh + S1 + ch + K[i] + w[i];
			const uint32_t S0 = Rotr(a, 2) ^ Rotr(a, 13) ^ Rotr(a, 22);
			const uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
			const uint32_t t2 = S0 + maj;
			hh = g; g = f; f = e; e = d + t1;
			d = c; c = b; b = a; a = t1 + t2;
		}
		h_[0] += a; h_[1] += b; h_[2] += c; h_[3] += d;
		h_[4] += e; h_[5] += f; h_[6] += g; h_[7] += hh;
	}

	uint32_t h_[8];
	uint64_t length_;
	unsigned char block_[64];
	size_t pending_;
};

} // namespace SpregEngine

#endif
