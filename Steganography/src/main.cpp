#define __cpp_lib_bitops

#include<lodepng.h>
#include<argparse\argparse.hpp>
#include<string_view>
#include<exception>
#include<cstdio>
#include<vector>
#include<string>
#include<spdlog\spdlog.h>
#include<fmt\core.h>
#include<cstdint>
#include<cstddef>
#include<cassert>
#include<span>
#include<numbers>
#include<fstream>
#include<filesystem>
#include<iterator>
#include<variant>
#include<bit>
#include<iostream>
#include"EncodeDecode.h"

struct Header::Info;

template<class T>
constexpr T swap_endian(T src) {
	for (size_t i = 0; i < (sizeof(src) / 2); ++i) {
		unsigned char front;
		unsigned char back;
		std::memcpy(&front, reinterpret_cast<unsigned char*>(&src) + i, 1);
		std::memcpy(&back, reinterpret_cast<unsigned char*>(&src) + sizeof(src) - 1 - i, 1);
		std::swap(front, back);
		std::memcpy(reinterpret_cast<unsigned char*>(&src) + i, &front, 1);
		std::memcpy(reinterpret_cast<unsigned char*>(&src) + sizeof(src) - 1 - i, &back, 1);
	}
	return src;
}

using DecodeVariant = std::variant<std::string, std::vector<uchar_t>>;

struct DecodeData {
	Header::Info info;
	DecodeVariant var;
};

class RawPixels {
private:
	std::vector<unsigned char> pixels;
	size_t width;
	size_t height;
public:
	RawPixels(std::string_view filename){
		unsigned int w, h;
		auto errorcode = lodepng::decode(pixels, w, h, filename.data());
		if (errorcode) {
			throw std::runtime_error(lodepng_error_text(errorcode));
		}
		else {
			width = static_cast<size_t>(w);
			height = static_cast<size_t>(h);
		}
	}
	void save_to_disk(std::string_view filename) const {
		auto errorcode = lodepng::encode(filename.data(), pixels, width, height);
		if (errorcode) {
			throw std::runtime_error(lodepng_error_text(errorcode));
		}
	}

	auto get_width() const noexcept { return width; }
	auto get_height() const noexcept { return height; }
	auto& get_pixels() const noexcept { return pixels; }
	auto get_num_pixels() const noexcept { return width * height; }
	uint8_t get_bit_depth() const noexcept {
		assert(pixels.size() / (width * height) < std::numeric_limits<uint8_t>::max()); 
		return static_cast<uint8_t>(pixels.size() / (width * height)); 
	}

	void encode_text(std::string_view text) {
		// Encodes passed string_view without null-termination character
		using namespace Header;
		Info info{ .type = Type::Text, .size = text.size() };
		auto view = encode_header({ pixels.begin(),pixels.end() }, info);
		std::vector<uchar_t> tmp(text.size());
		std::memcpy(tmp.data(), text.data(), text.size());
		view = encode_bytes(view, { tmp.begin(),text.size() });
	}

	void encode_file(std::string_view filename) {
		namespace fs = std::filesystem;
		using namespace Header;
		auto fileSize = fs::file_size(filename);
		std::ifstream input(filename.data(), std::ios::binary);
		std::vector<uchar_t> buffer(std::istreambuf_iterator<char>(input), {});
		//buffer.reserve(fileSize);
		auto pointIndex = filename.find_last_of(".");
		Info info{ .type = Type::File, .size = buffer.size(), .fileEnding = std::string(filename.substr(pointIndex)) };
		auto view = encode_header({ pixels.begin(),pixels.end() }, info);
		view = encode_bytes(view, { buffer.begin(),buffer.end() });
	}

	auto decode() -> DecodeData {
		using namespace Header;
		auto info = decode_header({ pixels.begin(),pixels.end() });
		std::vector<uchar_t> buffer(info.size);
		decode_bytes({ buffer.begin(),buffer.end() }, { pixels.begin() + 8 * header_size(info.type), 8 * info.size });
		switch (info.type) {
		case Type::Text:
		{
			std::string s(buffer.size(),'\0');
			std::memcpy(s.data(), buffer.data(), buffer.size());
			return { .info = info, .var = s };
		}
		case Type::File:
			return { .info = info, .var = buffer };
		}
	}
};

int main(int argc, char* argv[]) {

	argparse::ArgumentParser program("Stegano");
	program.add_argument("pngpic")
		.help("Name of picture to hide something in")
		.action([](const std::string& value) { return value; });
	program.add_argument("--text")
		.required()
		.help("String to hide in picture")
		.default_value(std::string{})
		.action([](const std::string& value) { return value; });
	program.add_argument("--file")
		.required()
		.help("Name of file to hide in picture")
		.default_value(std::string{})
		.action([](const std::string& value) { return value; });

	try {
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err) {
		std::puts(err.what());
		std::cout << program;
		std::terminate();
	}

	std::string pngpicname = program.get("pngpic");
	std::optional<std::string> text = [&]() -> std::optional<std::string> {
		if (program.get("--text").empty())
			return {};
		else
			return program.get("--text");
	}();
	std::optional<std::string> file = [&]() -> std::optional<std::string> {
		if (program.get("--file").empty())
			return {};
		else
			return program.get("--file");
	}();

	spdlog::info("Carrier name: {}",pngpicname);
	if (text) spdlog::info("Payload name: {}", text->c_str());
	if (file) spdlog::info("Payload name: {}", file->c_str());

	try {
		RawPixels rawPixels{pngpicname.c_str()};

		if (text || file) {	// Encode
			if (text) {
				rawPixels.encode_text(text.value());
			}
			else if(file) {
				rawPixels.encode_file(file.value());
			}
			auto pointIndex = pngpicname.find_last_of('.');
			auto newPngPicName = fmt::format("{}_encoded{}", pngpicname.substr(0, pointIndex), pngpicname.substr(pointIndex, pngpicname.size() - pointIndex));
			rawPixels.save_to_disk(newPngPicName);
			spdlog::info("Saved picture with encoded data to {}.", newPngPicName);
		}
		else {	// Decode
			auto res = rawPixels.decode();
			auto info = res.info;
			if (std::holds_alternative<std::string>(res.var)) {
				spdlog::info("Decoded Message: \t{}", std::get<std::string>(res.var));
			}
			else if (std::holds_alternative<std::vector<uchar_t>>(res.var)) {
				auto& data = std::get<std::vector<uchar_t>>(res.var);
				spdlog::info("Decoded {} bytes of binary filedata.", data.size());
				fmt::print("Please insert filename without type suffix: ");
				std::string newFileName;
				std::getline(std::cin, newFileName);
				newFileName.append(info.fileEnding.value());
				std::ofstream out(newFileName, std::ios::out | std::ios::binary | std::ios::trunc);
				out.write(reinterpret_cast<char*>(data.data()), data.size()).flush();
				if (!out) {
					spdlog::warn("Writing file with name {} failed. File could be in a corrupted state.", newFileName);
				}
				else {
					spdlog::info("Wrote decoded file with name {}.", newFileName);
				}
			}
		}

	}catch (std::exception& e) {
		spdlog::error("An Exception occured while loading or processing {}. \nException Message: {}", pngpicname, e.what());
	}
	return 0;
}

