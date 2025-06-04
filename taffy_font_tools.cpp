#include "include/taffy_font_tools.h"
#include "include/tools.h"
#include "include/quan.h"
#include "include/asset.h"
#include <iostream>
#include <cstring>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <cmath>
#include <fstream>

// STB_TRUETYPE implementation
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

namespace tremor::taffy::tools {

std::vector<uint8_t> loadFontFile(const std::string& font_path) {
    std::ifstream file(font_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open font file: " << font_path << std::endl;
        return {};
    }
    
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        std::cerr << "Failed to read font file: " << font_path << std::endl;
        return {};
    }
    
    return buffer;
}

bool createSDFFontAsset(const std::string& output_path, const std::string& font_path, 
                       int font_size, int texture_size, float sdf_range) {
    std::cout << "📝 Creating SDF font asset..." << std::endl;
    std::cout << "   Font: " << font_path << std::endl;
    std::cout << "   Size: " << font_size << " px" << std::endl;
    std::cout << "   Texture: " << texture_size << "x" << texture_size << std::endl;
    std::cout << "   SDF Range: " << sdf_range << " pixels" << std::endl;
    
    using namespace Taffy;
    
    // Load font file
    std::vector<uint8_t> fontData;
    
    // Resolve font path relative to project root if needed
    std::filesystem::path resolvedFontPath = font_path;
    if (!std::filesystem::exists(resolvedFontPath) && !std::filesystem::path(font_path).is_absolute()) {
        // Try relative to project root (assuming we're in build/bin)
        resolvedFontPath = std::filesystem::path("../..") / font_path;
    }
    
    // If font_path is "dummy.ttf", use built-in font data
    if (font_path == "dummy.ttf" || !std::filesystem::exists(resolvedFontPath)) {
        std::cout << "⚠️  Using built-in fallback font (font not found: " << font_path << ")" << std::endl;
        // We'll still use the test SDF for now, but could embed a simple bitmap font
        // For now, continue with the test implementation
    } else {
        fontData = loadFontFile(resolvedFontPath.string());
        if (fontData.empty()) {
            std::cerr << "Failed to load font file: " << font_path << std::endl;
            return false;
        }
    }
    
    // Initialize stb_truetype if we have font data
    stbtt_fontinfo font;
    bool hasRealFont = false;
    float scale = 0.0f;
    
    if (!fontData.empty()) {
        std::cout << "   Font data loaded: " << fontData.size() << " bytes" << std::endl;
        if (!stbtt_InitFont(&font, fontData.data(), 0)) {
            std::cerr << "Failed to initialize font with stb_truetype" << std::endl;
            return false;
        }
        hasRealFont = true;
        scale = stbtt_ScaleForPixelHeight(&font, font_size);
        std::cout << "   ✅ Font initialized successfully, scale: " << scale << std::endl;
        
        // Test a character
        int advance, lsb;
        stbtt_GetCodepointHMetrics(&font, 'H', &advance, &lsb);
        std::cout << "   Test: 'H' advance=" << advance << ", lsb=" << lsb << std::endl;
    }
    
    Asset asset;
    asset.set_creator("Taffy SDF Font Generator");
    asset.set_description("Test SDF font with basic glyphs");
    asset.set_feature_flags(FeatureFlags::SDFFont);
    
    // Create FontChunk
    FontChunk fontChunk{};
    
    // For this test, we'll create a simple font with ASCII characters 32-126
    const uint32_t firstChar = 32;  // Space
    const uint32_t lastChar = 126;  // Tilde
    const uint32_t glyphCount = lastChar - firstChar + 1;
    
    fontChunk.glyph_count = glyphCount;
    fontChunk.texture_width = texture_size;
    fontChunk.texture_height = texture_size;
    fontChunk.texture_format = 1; // R8 format
    fontChunk.sdf_range = sdf_range;
    fontChunk.font_size = static_cast<float>(font_size);
    // Get font metrics
    if (hasRealFont) {
        int ascent, descent, lineGap;
        stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
        fontChunk.ascent = ascent / 2.0f * scale;
        fontChunk.descent = -descent / 2.0f * scale;
        fontChunk.line_height = (ascent - descent + lineGap) * scale;
    } else {
        fontChunk.ascent = font_size * 0.8f;
        fontChunk.descent = font_size * 0.2f;
        fontChunk.line_height = font_size * 1.2f;
    }
    fontChunk.first_codepoint = firstChar;
    fontChunk.last_codepoint = lastChar;
    fontChunk.kerning_pair_count = 0; // No kerning for test font
    
    // Calculate offsets
    size_t header_size = sizeof(FontChunk);
    size_t glyph_array_size = glyphCount * sizeof(FontChunk::Glyph);
    size_t texture_data_size = texture_size * texture_size; // R8 format = 1 byte per pixel
    
    fontChunk.glyph_data_offset = header_size;
    fontChunk.texture_data_offset = header_size + glyph_array_size;
    fontChunk.texture_data_size = texture_data_size;
    fontChunk.kerning_data_offset = 0; // No kerning data
    
    // Create glyph array
    std::vector<FontChunk::Glyph> glyphs(glyphCount);
    
    // Calculate glyph size based on SDF requirements
    // Add padding around each glyph for SDF
    int padding = static_cast<int>(sdf_range + 2);
    int maxGlyphSize = font_size + padding * 2;
    
    // Try to fit all glyphs efficiently
    // Start with a reasonable cell size based on the max glyph size
    int glyphCellSize = maxGlyphSize + 4; // Add a bit of padding between glyphs
    int glyphsPerRow = texture_size / glyphCellSize;
    
    // Make sure we have enough rows
    int rowsNeeded = (glyphCount + glyphsPerRow - 1) / glyphsPerRow;
    int totalHeight = rowsNeeded * glyphCellSize;
    
    // If it doesn't fit, we need a bigger texture or smaller glyphs
    if (totalHeight > texture_size) {
        std::cerr << "⚠️  Warning: Need " << totalHeight << "px height but texture is only " 
                  << texture_size << "px. Some glyphs may not fit!" << std::endl;
    }
    
    std::cout << "   Grid layout: " << glyphsPerRow << " glyphs per row, cell size: " << glyphCellSize << "px" << std::endl;
    std::cout << "   Max glyph size: " << maxGlyphSize << "px (font size " << font_size << " + padding " << padding << ")" << std::endl;
    
    // Generate texture data
    std::cout << "🎨 Generating SDF texture..." << std::endl;
    std::vector<uint8_t> textureData(texture_data_size, 0);
    
    // Process each glyph
    for (uint32_t i = 0; i < glyphCount; i++) {
        FontChunk::Glyph& glyph = glyphs[i];
        glyph.codepoint = firstChar + i;
        
        // Calculate position in grid
        int row = i / glyphsPerRow;
        int col = i % glyphsPerRow;
        int cellX = col * glyphCellSize;
        int cellY = row * glyphCellSize;
        
        // Debug specific glyphs
        if (glyph.codepoint == 'H' || glyph.codepoint == 'i') {
            std::cout << "   Glyph '" << (char)glyph.codepoint << "' (index " << i << "): row=" << row 
                      << ", col=" << col << ", pos=(" << cellX << ", " << cellY << ")" << std::endl;
        }
        
        if (hasRealFont) {
            // Get glyph metrics
            int advance, lsb;
            stbtt_GetCodepointHMetrics(&font, glyph.codepoint, &advance, &lsb);
            
            int x0, y0, x1, y1;
            stbtt_GetCodepointBitmapBox(&font, glyph.codepoint, scale, scale, &x0, &y0, &x1, &y1);
            
            int glyphWidth = x1 - x0;
            int glyphHeight = y1 - y0;
            
            // Generate SDF for this glyph
            if (glyphWidth > 0 && glyphHeight > 0) {
                int sdfWidth = glyphWidth + padding * 2;
                int sdfHeight = glyphHeight + padding * 2;
                
                // Generate SDF using stb_truetype
                unsigned char onedge_value = 128;
                float pixel_dist_scale = (float)onedge_value / sdf_range;
                
                int xoff, yoff;
                unsigned char* sdfBitmap = stbtt_GetCodepointSDF(&font, 
                    scale, 
                    glyph.codepoint,
                    padding,
                    onedge_value,
                    pixel_dist_scale,
                    &sdfWidth, &sdfHeight,
                    &xoff, &yoff);
                
                if (sdfBitmap) {
                    if (glyph.codepoint == 'H' || glyph.codepoint == 'i') {
                        std::cout << "     Generated SDF for '" << (char)glyph.codepoint 
                                  << "': " << sdfWidth << "x" << sdfHeight << " at offset (" << xoff << ", " << yoff << ")" << std::endl;
                    }
                    // Make sure it fits in the cell
                    if (sdfWidth <= glyphCellSize && sdfHeight <= glyphCellSize) {
                        // Copy SDF to texture atlas
                        for (int y = 0; y < sdfHeight && (cellY + y) < texture_size; y++) {
                            for (int x = 0; x < sdfWidth && (cellX + x) < texture_size; x++) {
                                textureData[(cellY + y) * texture_size + (cellX + x)] = sdfBitmap[y * sdfWidth + x];
                            }
                        }
                        
                        // Set glyph UV coordinates
                        glyph.uv_x = static_cast<float>(cellX) / texture_size;
                        glyph.uv_y = static_cast<float>(cellY) / texture_size;
                        glyph.uv_width = static_cast<float>(sdfWidth) / texture_size;
                        glyph.uv_height = static_cast<float>(sdfHeight) / texture_size;
                        
                        // Set glyph metrics
                        glyph.width = glyphWidth;
                        glyph.height = glyphHeight;
                        glyph.bearing_x = x0 + xoff * scale;
                        glyph.bearing_y = -(y0 + yoff);  // Negate for proper baseline alignment
                        glyph.advance = advance * scale * 0.9f;
                    } else {
                        // Glyph too big, mark as empty
                        glyph.width = 0;
                        glyph.height = 0;
                        glyph.uv_width = 0;
                        glyph.uv_height = 0;
                        glyph.advance = advance * scale;
                    }
                    
                    // Free the SDF bitmap
                    stbtt_FreeSDF(sdfBitmap, nullptr);
                } else {
                    // Failed to generate SDF
                    glyph.width = 0;
                    glyph.height = 0;
                    glyph.uv_width = 0;
                    glyph.uv_height = 0;
                    glyph.advance = advance * scale;
                }
            } else {
                // Empty glyph (like space)
                glyph.width = 0;
                glyph.height = 0;
                glyph.bearing_x = 0;
                glyph.bearing_y = 0;
                glyph.advance = advance * scale;
                glyph.uv_x = 0;
                glyph.uv_y = 0;
                glyph.uv_width = 0;
                glyph.uv_height = 0;
            }
        } else {
            // Fallback to test pattern
            glyph.uv_x = static_cast<float>(cellX) / texture_size;
            glyph.uv_y = static_cast<float>(cellY) / texture_size;
            glyph.uv_width = static_cast<float>(glyphCellSize) / texture_size;
            glyph.uv_height = static_cast<float>(glyphCellSize) / texture_size;
            
            glyph.width = glyphCellSize * 0.8f;
            glyph.height = glyphCellSize * 0.8f;
            glyph.bearing_x = 0;
            glyph.bearing_y = glyph.height;
            glyph.advance = glyph.width * 1.1f;
            
            // Generate test SDF
            for (int y = 0; y < glyphCellSize && (cellY + y) < texture_size; y++) {
                for (int x = 0; x < glyphCellSize && (cellX + x) < texture_size; x++) {
                    float dx = x - glyphCellSize / 2.0f;
                    float dy = y - glyphCellSize / 2.0f;
                    float dist = std::sqrt(dx * dx + dy * dy);
                    float signedDist = dist - glyphCellSize / 4.0f;
                    float normalized = (signedDist / sdf_range + 1.0f) * 0.5f;
                    normalized = std::max(0.0f, std::min(1.0f, normalized));
                    textureData[(cellY + y) * texture_size + (cellX + x)] = static_cast<uint8_t>(normalized * 255);
                }
            }
        }
    }
    
    // Assemble chunk data
    size_t total_size = header_size + glyph_array_size + texture_data_size;
    std::vector<uint8_t> chunk_data(total_size);
    uint8_t* ptr = chunk_data.data();
    
    // Write header
    std::memcpy(ptr, &fontChunk, header_size);
    ptr += header_size;
    
    // Write glyph array
    std::memcpy(ptr, glyphs.data(), glyph_array_size);
    ptr += glyph_array_size;
    
    // Write texture data
    std::memcpy(ptr, textureData.data(), texture_data_size);
    
    // Add the font chunk to the asset
    asset.add_chunk(ChunkType::FONT, chunk_data, "test_sdf_font");
    
    // Save to file
    std::filesystem::create_directories(std::filesystem::path(output_path).parent_path());
    
    if (asset.save_to_file(output_path)) {
        std::cout << "✅ SDF font asset created: " << output_path << std::endl;
        std::cout << "   📊 Total size: " << total_size << " bytes" << std::endl;
        std::cout << "   🔤 Glyphs: " << glyphCount << " (ASCII " << firstChar << "-" << lastChar << ")" << std::endl;
        std::cout << "   🖼️  Texture: " << texture_size << "x" << texture_size << " R8" << std::endl;
        return true;
    }
    
    return false;
}

} // namespace tremor::taffy::tools