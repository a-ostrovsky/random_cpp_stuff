#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace BmpToAsciiArt {

#ifndef BMP_FILE
#define BMP_FILE "input.bmp"
#endif

constexpr unsigned char bmp[] = {
#embed BMP_FILE

};

// BMP header sizes and field offsets, in bytes from the start of the file.
constexpr std::size_t file_header_size = 14;
constexpr std::size_t min_dib_header_size = 40;
constexpr std::size_t pixel_offset_field = 10;
constexpr std::size_t width_field = 18;
constexpr std::size_t height_field = 22;
constexpr std::size_t color_planes_field = 26;
constexpr std::size_t bits_per_pixel_field = 28;
constexpr std::size_t compression_field = 30;

constexpr unsigned bits_per_byte = 8;
constexpr unsigned bytes_per_pixel =
    3; // One byte each for blue, green and red.
constexpr unsigned bits_per_pixel = bytes_per_pixel * bits_per_byte;
constexpr unsigned color_planes = 1; // BMP requires exactly one color plane.
constexpr unsigned uncompressed = 0; // BI_RGB: no compression.
constexpr std::size_t row_alignment = 4; // BMP rows are padded to four bytes.

constexpr std::uint32_t read_le(std::size_t offset, unsigned bytes) {
  std::uint32_t value = 0;
  for (unsigned i = 0; i < bytes; ++i)
    value |= std::uint32_t{bmp[offset + i]} << (bits_per_byte * i);
  return value;
}

// The first two bytes are the BMP signature, "BM".
static_assert(sizeof(bmp) >= file_header_size + min_dib_header_size &&
                  bmp[0] == 'B' && bmp[1] == 'M',
              "Expected a Windows BMP header");
// The DIB header starts immediately after the file header with its own size.
constexpr auto dib_header_size =
    read_le(file_header_size, sizeof(std::uint32_t));
static_assert(dib_header_size >= min_dib_header_size &&
                  dib_header_size <= sizeof(bmp) - file_header_size &&
                  read_le(color_planes_field, sizeof(std::uint16_t)) ==
                      color_planes &&
                  read_le(bits_per_pixel_field, sizeof(std::uint16_t)) ==
                      bits_per_pixel &&
                  read_le(compression_field, sizeof(std::uint32_t)) ==
                      uncompressed,
              "Only uncompressed 24-bit BMPs are supported");

constexpr auto width =
    static_cast<std::int32_t>(read_le(width_field, sizeof(std::uint32_t)));
constexpr auto signed_height =
    static_cast<std::int32_t>(read_le(height_field, sizeof(std::uint32_t)));
static_assert(width > 0 && signed_height != 0, "Invalid BMP dimensions");
constexpr auto height = signed_height < 0 ? -std::int64_t{signed_height}
                                          : std::int64_t{signed_height};
constexpr std::size_t pixel_offset =
    read_le(pixel_offset_field, sizeof(std::uint32_t));
// Round each row's byte count up to the next alignment boundary.
constexpr auto row_stride =
    (std::size_t{width} * bytes_per_pixel + row_alignment - 1) / row_alignment *
    row_alignment;
static_assert(pixel_offset >=
                      file_header_size + std::uint64_t{dib_header_size} &&
                  pixel_offset <= sizeof(bmp) &&
                  height <= (sizeof(bmp) - pixel_offset) / row_stride,
              "Invalid or truncated BMP pixel data");

// Dark pixels use dense characters; reverse this ramp to invert the output.
constexpr auto shades =
    std::to_array<std::string_view>({"█", "▓", "▒", "░", " "});
constexpr std::size_t max_shade_bytes =
    3; // UTF-8 bytes for each block character.

// BMP stores pixels in BGR order.
constexpr std::size_t blue_offset = 0;
constexpr std::size_t green_offset = 1;
constexpr std::size_t red_offset = 2;
// Perceived brightness weights, scaled by 1000 to keep arithmetic integral.
constexpr unsigned blue_weight = 114;
constexpr unsigned green_weight = 587;
constexpr unsigned red_weight = 299;
constexpr unsigned brightness_scale = blue_weight + green_weight + red_weight;
constexpr unsigned channel_levels = 1u << bits_per_byte;

// Increase cell_width for fewer columns. Characters are roughly twice as tall
// as they are wide, so average twice as many pixels vertically.
constexpr int cell_width = 1;
constexpr int character_aspect_ratio = 2;
constexpr int cell_height = character_aspect_ratio * cell_width;
static_assert(cell_width > 0);

static constexpr auto ascii_art() {
  // Round up to include partial cells at the right and bottom edges.
  constexpr auto columns = (width + std::int64_t{cell_width} - 1) / cell_width;
  constexpr auto rows = (height + cell_height - 1) / cell_height;
  // Two extra bytes reserve a leading newline and a trailing null terminator;
  // each row reserves one additional byte for its newline.
  std::array<char, 2 + rows*(columns * max_shade_bytes + 1)> result{};
  std::size_t output = 0;
  result[output++] = '\n';
  for (std::int64_t y = 0; y < height; y += cell_height) {
    for (std::int64_t x = 0; x < width; x += cell_width) {
      std::uint64_t brightness = 0;
      std::uint64_t count = 0;
      for (auto py = y; py < y + cell_height && py < height; ++py) {
        const auto row = signed_height < 0 ? py : height - 1 - py;
        for (auto px = x; px < x + cell_width && px < width; ++px) {
          const auto offset = pixel_offset +
                              static_cast<std::size_t>(row) * row_stride +
                              static_cast<std::size_t>(px) * bytes_per_pixel;
          brightness += blue_weight * bmp[offset + blue_offset] +
                        green_weight * bmp[offset + green_offset] +
                        red_weight * bmp[offset + red_offset];
          ++count;
        }
      }
      const auto shade = shades[brightness * shades.size() /
                                (count * channel_levels * brightness_scale)];
      for (char byte : shade)
        result[output++] = byte;
    }
    result[output++] = '\n';
  }
  return result;
}

constexpr auto art_bytes = ascii_art();
constexpr std::string_view art{art_bytes.data()};
static_assert(art.starts_with('\n'), "ASCII art must start with a newline");
static_assert(false, art);

} // namespace BmpToAsciiArt
