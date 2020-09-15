#pragma once
#include<span>
#include<cstdint>
#include<optional>
#include<string>
#include<fmt\core.h>
#include<exception>

using uchar_t = unsigned char;

namespace Header {
	/*
	* Textheader:
	* 1 byte for content_type
	* 8 byte for length of payload
	*
	* Fileheader:
	* 1 byte for content_type
	* 8 byte for length of payload
	* 8 byte for file_ending
	*/
	enum class Type : unsigned char {
		Text = 0,
		File = 1
	};
	enum Size : unsigned char {
		Text = 9,
		File = 17,
		Min = Text,
		Max = File
	};
	enum Offset : unsigned char {
		ContentType = 0,
		PayloadLength = 1,
		FileEnding = 9
	};
	constexpr Size header_size(const Type type) {
		switch (type) {
		case Type::Text: return Size::Text;
		case Type::File: return Size::File;
		}
		/*throw std::runtime_error(fmt::format("header_size of given type not found. Error in File {} on line {}", __FILE__, __LINE__ ));*/
	}

	struct Info {
		// Every member has to have exact same size on all platforms since values get memcopyed in from raw bytes
		Type type;
		uint64_t size;
		std::optional<std::string> fileEnding;
	};
};

std::span<uchar_t> encode_bytes(std::span<uchar_t> dest, std::span<const uchar_t> src);
void decode_bytes(std::span<uchar_t> dest, std::span<const uchar_t> src);
std::span<uchar_t> encode_header(std::span<uchar_t> bytes, Header::Info info);
Header::Info decode_header(std::span<const uchar_t> bytes);