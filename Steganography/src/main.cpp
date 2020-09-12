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

constexpr void encode_bit(unsigned char& dest, unsigned char& src,uint8_t n = 0) noexcept {
	constexpr unsigned char mask = 1;
	dest = (~mask & dest) | (mask & (src>>n));
}

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
	auto get_width() const noexcept { return width; }
	auto get_height() const noexcept { return height; }
	auto& get_pixels() const noexcept { return pixels; }
	auto get_num_pixels() const noexcept { return width * height; }
	uint8_t get_bit_depth() const noexcept {
		assert(pixels.size() / (width * height) < std::numeric_limits<uint8_t>::max()); 
		return static_cast<uint8_t>(pixels.size() / (width * height)); 
	}
	void encode_bytes(std::vector<unsigned char>& payload, size_t offset=0) {
		if (payload.size() * 8 + offset > pixels.size()) {
			throw std::runtime_error(fmt::format("Payload with {} bytes and offset {} is {} bytes to big to be encoded.", payload.size(), offset, (payload.size() * 8 - (pixels.size() - offset)) / 8));
		}
		for (size_t byte = 0; byte < payload.size(); ++byte) {
			auto currentPixelChannel = offset + 8 * byte;
			for (uint8_t bit = 0; bit < sizeof(unsigned char) * 8; ++bit) {
				encode_bit(pixels[currentPixelChannel + bit], payload[byte],bit);
			}
		}
		spdlog::info("Used {} bytes of picture data to carry {} byte of payload.", payload.size() * 8, payload.size());
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
	std::puts(pngpicname.c_str());
	if (text) std::puts(text->c_str());
	if (file) std::puts(file->c_str());

	RawPixels rawPixels{pngpicname.c_str()};
	spdlog::info("picture is {} pixels wide and {} pixels tall", rawPixels.get_width(), rawPixels.get_height());
	spdlog::info("picture was loaded with {} byte per pixel", rawPixels.get_bit_depth());
	std::vector<unsigned char> payload;
	payload.resize(text->size()+1);
	std::memcpy(payload.data(), text->data(), text->size() + 1);
	rawPixels.encode_bytes(payload);


	return 0;
}

