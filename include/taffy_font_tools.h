#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace tremor::taffy::tools {

// Load a TTF/OTF font file
std::vector<uint8_t> loadFontFile(const std::string& font_path);

// Create an SDF font asset from a TTF/OTF font file
// Parameters:
//   output_path: Path to save the generated .taf file
//   font_path: Path to the input TTF/OTF font file
//   font_size: Font size in pixels for SDF generation
//   texture_size: Size of the texture atlas (square, e.g., 512 for 512x512)
//   sdf_range: SDF range in pixels (typically 4-8)
// Returns: true if successful, false otherwise
bool createSDFFontAsset(const std::string& output_path, 
                       const std::string& font_path, 
                       int font_size, 
                       int texture_size, 
                       float sdf_range);

} // namespace tremor::taffy::tools