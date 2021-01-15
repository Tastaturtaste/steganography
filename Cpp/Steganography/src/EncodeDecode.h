#pragma once
#include<span>
#include<cstdint>
#include<optional>
#include<string>

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
		File = 1,
		Invalid
	};
	enum class Size : unsigned char {
		Text = 9,
		File = 17,
		Min = Text,
		Max = File
	};

	enum class Offset : unsigned char {
		ContentType = 0,
		PayloadLength = 1,
		FileEnding = 9
	};

	constexpr inline std::underlying_type_t<Size> get(Size size) noexcept { return static_cast<std::underlying_type_t<Size>>(size); }
	constexpr inline std::underlying_type_t<Offset> get(Offset offset) noexcept { return static_cast<std::underlying_type_t<Offset>>(offset); }

	template<class T>
	requires requires (T t) { static_cast<Size>(t); }
	constexpr T header_size(const Type type) {
		switch (type) {
		case Type::Text: return static_cast<T>(Size::Text);
		case Type::File: return static_cast<T>(Size::File);
		}
		return std::numeric_limits<T>::max();
	}
	constexpr Size header_size(const Type type) {
		switch (type) {
		case Type::Text: return Size::Text;
		case Type::File: return Size::File;
		}
		return std::numeric_limits<Size>::max();
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