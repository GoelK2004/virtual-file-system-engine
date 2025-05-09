#include <iostream>
#include "hash.h"
typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

int hashFileName(const std::string& fileName) {
	std::vector<uint8_t> digest = sha256(fileName);
	int hashValue = 0;
	for (int i = 0; i < 4; i++) {
		hashValue = (hashValue << 8) | digest[i];
	}
	
	return hashValue;
}

const u32 K[] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

// Rotate right
inline u32 rotr(u32 x, u32 n) {
	return (x >> n) | (x << (32 - n));
}

// Selector
inline u32 ch(u32 x, u32 y, u32 z) {
	return (x & y) ^ (~x & z);
}

// Consensus
inline u32 maj(u32 x, u32 y, u32 z) {
	return (x & y) ^ (y & z) ^ (z & x);
}

// Diffusion
inline u32 bsig0(u32 x) {
	return rotr(x, 2) ^  rotr(x, 13) ^  rotr(x, 22);
}

inline u32 bsig1(u32 x) {
	return rotr(x, 6) ^  rotr(x, 11) ^  rotr(x, 25);
}

inline u32 ssig0(u32 x) {
	return rotr(x, 7) ^  rotr(x, 18) ^  (x >> 3);
}

inline u32 ssig1(u32 x) {
	return rotr(x, 17) ^  rotr(x, 19) ^  (x >> 10);
}

std::vector<u8> sha256(const std::string& input) {
	std::vector<u8> data(input.begin(), input.end());

	u64 bitLen = data.size() * 8;

	data.push_back(0x80);
	while ((data.size() * 8) % 512 != 448)	data.push_back(0x00);
	// Append original message length as 64-bit big-endian
    for (int i = 7; i >= 0; --i){
		data.push_back(static_cast<u8>((bitLen >> (i*8)) & 0xFF));
	}

	// From fractional part square root of first 8 prime numbers
	u32 h[8] = {
		0x6a09e667, 0xbb67ae85,
		0x3c6ef372, 0xa54ff53a,
		0x510e527f, 0x9b05688c,
		0x1f83d9ab, 0x5be0cd19
	};

	// Traversing per 512 bits
	for (size_t i = 0; i < data.size(); i += 64) {
		u32 w[64] = {};

		// Store 16 32-bit integers in the 'w' array
		for (int j = 0; j < 16; ++j) {
			w[j] = (data[i + j * 4] << 24) | 
			(data[i + j * 4 + 1] << 16) | 
			(data[i + j * 4 + 2] << 8) | 
			(data[i + j * 4 + 3]);
		}

		// Message schedule expansion
		for (int j = 16; j < 63; ++j) {
			w[j] = ssig1(w[j - 2]) + w[j - 7] + ssig0(w[j-15]) + w[j-16];
		}

		u32 a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], h_ = h[7];

		for (int j = 0; j < 64; ++j) {
			u32 t1 = h_ + ch(e, f, g) + K[j] + w[j];
			u32 t2 = bsig0(a) + maj(a, b, c);
			h_ = g; g = f; f = e; e = d + t1;
			d = c; c = b; b = a; a = t1 + t2;
		}

		h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += h_;

	}

	std::vector<u8> hashBytes;
    for (int i = 0; i < 8; ++i) {
        hashBytes.push_back((h[i] >> 24) & 0xFF);
        hashBytes.push_back((h[i] >> 16) & 0xFF);
        hashBytes.push_back((h[i] >> 8) & 0xFF);
        hashBytes.push_back(h[i] & 0xFF);
    }

	return hashBytes;
}