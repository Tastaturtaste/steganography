#define __cpp_lib_bitops

#include"EncodeDecode.h"
#include<cassert>
#include<array>
#include<string>
#include<algorithm>
#include<spdlog\spdlog.h>
#include<bit>

#ifndef __cpp_lib_bitops
template<std::unsigned_integral I>
constexpr I rotl(I val, int_least8_t n) {
	return (val << n) | (static_cast<unsigned int>(val) >> (-n & (sizeof(I) * CHAR_BIT - 1)));
}
#else
using std::rotl;
#endif

constexpr inline void encode_bit_n_to_last(unsigned char& dest, const unsigned char& src, uint8_t n = 0) noexcept {
	constexpr unsigned char mask = 1;
	dest = (~mask & dest) | (mask & (src >> n));
}

constexpr inline void decode_bit_last_to_n(unsigned char& dest, const unsigned char& src, uint8_t n = 0) noexcept {
	constexpr unsigned char masksrc = 1u;
	constexpr unsigned char maskdest = static_cast<unsigned char>(~masksrc);
	dest = (rotl(maskdest, n) & dest) | rotl(static_cast<unsigned char>(masksrc & src), n);
}

std::span<uchar_t> encode_bytes(std::span<uchar_t> dest, std::span<const uchar_t> src) {
	// Encodes bytes in little-endian order
	assert(CHAR_BIT == 8);
	assert(src.size() * 8 <= dest.size());
	#pragma omp parallel for
	for (size_t byte = 0; byte < src.size(); ++byte) {
		for (int_least8_t bit = 0; bit < 8; ++bit) {
			encode_bit_n_to_last(dest[8 * byte + bit], src[byte], bit);
		}
	}
	//spdlog::info("Used {} bytes of picture data to carry {} byte of payload.", 8*src.size(), src.size());
	return dest.subspan(src.size() * 8); // returns subspan after last encoded byte to make chaining possible
}

void decode_bytes(std::span<uchar_t> dest, std::span<const uchar_t> src) {
	// Decodes bytes in little-endian order
	assert(CHAR_BIT == 8);
	assert(src.size() % 8 == 0);
	assert(src.size() <= dest.size() * 8);
	#pragma omp parallel for
	for (size_t byte = 0; byte * 8 < src.size(); ++byte) {
		for (int_least8_t bit = 0; bit < 8; ++bit) {
			decode_bit_last_to_n(dest[byte], src[8 * byte + bit], bit);
		}
	}
	//spdlog::info("Read {} bytes of picture data to decode {} byte of payload.", src.size(), dest.size() / 8);
}

std::span<uchar_t> encode_header(std::span<uchar_t> bytes, Header::Info info) {
	using namespace Header;
	std::array<uchar_t, Size::Max> headerBytes;
	std::memcpy(headerBytes.data() + Offset::ContentType, &info.type, sizeof(info.type));
	std::memcpy(headerBytes.data() + Offset::PayloadLength, &info.size, sizeof(info.size));
	switch (info.type) {
	case Type::Text: break;
	case Type::File:
		const auto numChars = info.fileEnding.value().size();
		std::memcpy(headerBytes.data() + Offset::FileEnding, info.fileEnding.value().c_str(), numChars);
		std::fill_n(headerBytes.data() + Offset::FileEnding + numChars, 8 - numChars, '\0');
		break;
	}
	return encode_bytes(bytes, { headerBytes.data(), header_size(info.type) });
}

Header::Info decode_header(std::span<const uchar_t> bytes) {
	using namespace Header;
	assert(bytes.size() * 8 >= Size::Min);
	std::array<uchar_t, Size::Max> headerBytes;
	decode_bytes({ headerBytes.begin(),headerBytes.end() }, bytes.subspan(0, headerBytes.size() * 8));
	Info info{};
	std::memcpy(&info.type, headerBytes.data() + Offset::ContentType, sizeof(info.type));
	std::memcpy(&info.size, headerBytes.data() + Offset::PayloadLength, sizeof(info.size));
	switch (info.type) {
	case Type::File:
		char fileEnding[8];
		std::memcpy(&fileEnding[0], headerBytes.data() + Offset::FileEnding, 8);
		info.fileEnding = std::string(fileEnding);
	}
	return info;
}