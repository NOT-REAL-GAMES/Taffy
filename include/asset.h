#pragma once
#include "taffy.h"

namespace Taffy {

    Asset::Asset() {
        std::memset(&header_, 0, sizeof(header_));
        std::strncpy(header_.magic, "TAF!", 4);
        header_.version_major = 1;
        header_.version_minor = 0;
        header_.version_patch = 0;
        header_.asset_type = 0; // Master asset
        header_.feature_flags = FeatureFlags::None;
        header_.chunk_count = 0;
        header_.dependency_count = 0;
        header_.ai_model_count = 0;
        header_.total_size = sizeof(AssetHeader);
        header_.created_timestamp = 0; // TODO: Set actual timestamp
        std::strncpy(header_.creator, "Unknown", sizeof(header_.creator) - 1);
        std::strncpy(header_.description, "Taffy Asset", sizeof(header_.description) - 1);
    }

    // =============================================================================
    // BASIC PROPERTIES
    // =============================================================================

    void Asset::set_creator(const std::string& creator) {
        std::strncpy(header_.creator, creator.c_str(), sizeof(header_.creator) - 1);
        header_.creator[sizeof(header_.creator) - 1] = '\0';
    }

    void Asset::set_description(const std::string& description) {
        std::strncpy(header_.description, description.c_str(), sizeof(header_.description) - 1);
        header_.description[sizeof(header_.description) - 1] = '\0';
    }

    void Asset::set_feature_flags(FeatureFlags flags) {
        header_.feature_flags = flags;
    }

    std::string Asset::get_creator() const {
        return std::string(header_.creator);
    }

    std::string Asset::get_description() const {
        return std::string(header_.description);
    }

    FeatureFlags Asset::get_feature_flags() const {
        return header_.feature_flags;
    }

    // =============================================================================
    // FEATURE CHECKING
    // =============================================================================

    bool Asset::has_feature(FeatureFlags flag) const {
        return (header_.feature_flags & flag) == flag;
    }

    // =============================================================================
    // CHUNK MANAGEMENT
    // =============================================================================

    void Asset::add_chunk(ChunkType type, const std::vector<uint8_t>& data, const std::string& name) {
        // Store chunk data
        chunk_data_[type] = data;

        // Create directory entry
        ChunkDirectoryEntry entry{};
        entry.type = type;
        entry.flags = 0;
        entry.offset = 0; // Will be calculated during save
        entry.size = data.size();
        entry.checksum = calculate_crc32(data.data(), data.size());
        std::strncpy(entry.name, name.c_str(), sizeof(entry.name) - 1);
        entry.name[sizeof(entry.name) - 1] = '\0';

        // Add to directory
        chunk_directory_.push_back(entry);
        header_.chunk_count = static_cast<uint32_t>(chunk_directory_.size());

        std::cout << "  📦 Added chunk: " << name << " (" << data.size() << " bytes)" << std::endl;
    }

    bool Asset::has_chunk(ChunkType type) const {
        return chunk_data_.find(type) != chunk_data_.end();
    }

    std::optional<std::vector<uint8_t>> Asset::get_chunk_data(ChunkType type) const {
        auto it = chunk_data_.find(type);
        if (it != chunk_data_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    size_t Asset::get_chunk_count() const {
        return chunk_directory_.size();
    }

    std::vector<ChunkType> Asset::get_chunk_types() const {
        std::vector<ChunkType> types;
        for (const auto& [type, data] : chunk_data_) {
            types.push_back(type);
        }
        return types;
    }

    // =============================================================================
    // FILE I/O
    // =============================================================================

    bool Asset::save_to_file(const std::string& path) {
        std::cout << "💾 Saving asset to: " << path << std::endl;

        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "❌ Failed to open file for writing: " << path << std::endl;
            return false;
        }

        // Calculate total size and chunk offsets
        header_.total_size = sizeof(AssetHeader) +
            chunk_directory_.size() * sizeof(ChunkDirectoryEntry);

        uint64_t current_offset = header_.total_size;
        for (auto& entry : chunk_directory_) {
            entry.offset = current_offset;
            current_offset += entry.size;
        }
        header_.total_size = current_offset;

        // Write header
        file.write(reinterpret_cast<const char*>(&header_), sizeof(header_));

        // Write chunk directory
        for (const auto& entry : chunk_directory_) {
            file.write(reinterpret_cast<const char*>(&entry), sizeof(entry));
        }

        // Write chunk data
        for (const auto& entry : chunk_directory_) {
            auto it = chunk_data_.find(entry.type);
            if (it != chunk_data_.end()) {
                file.write(reinterpret_cast<const char*>(it->second.data()), it->second.size());
            }
        }

        file.close();

        std::cout << "✅ Asset saved successfully!" << std::endl;
        std::cout << "   📊 Size: " << header_.total_size << " bytes" << std::endl;
        std::cout << "   📦 Chunks: " << header_.chunk_count << std::endl;

        return true;
    }

    bool Asset::load_from_file_safe(const std::string& path) {
        std::cout << "📖 Loading asset from: " << path << std::endl;

        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "❌ Failed to open file for reading: " << path << std::endl;
            return false;
        }

        // Read header
        file.read(reinterpret_cast<char*>(&header_), sizeof(header_));
        if (!file.good()) {
            std::cerr << "❌ Failed to read asset header" << std::endl;
            return false;
        }

        // Validate magic
        if (std::strncmp(header_.magic, "TAF!", 4) != 0) {
            std::cerr << "❌ Invalid asset magic: " << std::string(header_.magic, 4) << std::endl;
            return false;
        }

        std::cout << "  📋 Asset info:" << std::endl;
        std::cout << "    Version: " << header_.version_major << "." << header_.version_minor << "." << header_.version_patch << std::endl;
        std::cout << "    Creator: " << header_.creator << std::endl;
        std::cout << "    Description: " << header_.description << std::endl;
        std::cout << "    Chunks: " << header_.chunk_count << std::endl;

        // Debug: Show current file position
        std::cout << "  📍 File position before chunk directory: " << file.tellg() << std::endl;
        std::cout << "  📐 sizeof(AssetHeader): " << sizeof(AssetHeader) << std::endl;
        std::cout << "  📐 sizeof(ChunkDirectoryEntry): " << sizeof(ChunkDirectoryEntry) << std::endl;
        
        // Read chunk directory
        chunk_directory_.clear();
        chunk_directory_.resize(header_.chunk_count);

        for (uint32_t i = 0; i < header_.chunk_count; ++i) {
            file.read(reinterpret_cast<char*>(&chunk_directory_[i]), sizeof(ChunkDirectoryEntry));
            if (!file.good()) {
                std::cerr << "❌ Failed to read chunk directory entry " << i << std::endl;
                return false;
            }
            
            // Debug: Print what we just read
            const auto& entry = chunk_directory_[i];
            std::cout << "  📄 Chunk " << i << ": type=0x" << std::hex << static_cast<uint32_t>(entry.type) 
                      << std::dec << ", offset=" << entry.offset 
                      << ", size=" << entry.size 
                      << ", name='" << entry.name << "'" << std::endl;
        }

        // Get file size for validation
        file.seekg(0, std::ios::end);
        size_t file_size = file.tellg();
        file.seekg(sizeof(AssetHeader) + chunk_directory_.size() * sizeof(ChunkDirectoryEntry));
        
        std::cout << "  📊 File size: " << file_size << " bytes" << std::endl;
        std::cout << "  📊 Expected total size from header: " << header_.total_size << " bytes" << std::endl;
        
        if (file_size != header_.total_size) {
            std::cerr << "  ⚠️  WARNING: File size mismatch! File might be truncated." << std::endl;
        }
        
        // Read chunk data
        chunk_data_.clear();
        for (const auto& entry : chunk_directory_) {
            // Debug output
            std::cout << "  📦 Reading chunk: " << entry.name 
                      << " (type: 0x" << std::hex << static_cast<uint32_t>(entry.type) << std::dec
                      << ", offset: " << entry.offset 
                      << ", size: " << entry.size << ")" << std::endl;
            
            // Validate chunk bounds
            if (entry.offset + entry.size > file_size) {
                std::cerr << "❌ Chunk extends beyond file! Offset: " << entry.offset 
                          << ", Size: " << entry.size 
                          << ", File size: " << file_size << std::endl;
                return false;
            }
            
            file.clear(); // Clear any error flags before seeking
            file.seekg(entry.offset);
            if (!file.good()) {
                std::cerr << "❌ Failed to seek to offset " << entry.offset << " for chunk: " << entry.name << std::endl;
                std::cerr << "   File state: " << (file.eof() ? "EOF" : "not EOF") 
                          << ", " << (file.fail() ? "FAIL" : "not FAIL")
                          << ", " << (file.bad() ? "BAD" : "not BAD") << std::endl;
                return false;
            }

            std::vector<uint8_t> data(entry.size);
            file.read(reinterpret_cast<char*>(data.data()), entry.size);
            
            // Check if we read the expected amount of data
            size_t bytes_read = file.gcount();
            if (bytes_read != entry.size) {
                std::cerr << "❌ Failed to read chunk data for: " << entry.name << std::endl;
                std::cerr << "   Attempted to read " << entry.size << " bytes at offset " << entry.offset << std::endl;
                std::cerr << "   Actually read: " << bytes_read << " bytes" << std::endl;
                std::cerr << "   File state: " << (file.eof() ? "EOF" : "not EOF") 
                          << ", " << (file.fail() ? "FAIL" : "not FAIL")
                          << ", " << (file.bad() ? "BAD" : "not BAD") << std::endl;
                
                // If we read partial data, show what checksum we got
                if (bytes_read > 0) {
                    uint32_t partial_crc = calculate_crc32(data.data(), bytes_read);
                    std::cerr << "   Partial data CRC: 0x" << std::hex << partial_crc << std::dec << std::endl;
                    std::cerr << "   Expected CRC: 0x" << std::hex << entry.checksum << std::dec << std::endl;
                }
                return false;
            }

            // Verify checksum
            uint32_t calculated_crc = calculate_crc32(data.data(), data.size());
            if (calculated_crc != entry.checksum) {
                std::cerr << "❌ Checksum mismatch for chunk: " << entry.name << std::endl;
                std::cerr << "   Expected: 0x" << std::hex << entry.checksum << std::endl;
                std::cerr << "   Calculated: 0x" << calculated_crc << std::dec << std::endl;
                return false;
            }

            chunk_data_[entry.type] = std::move(data);
            std::cout << "  📦 Loaded chunk: " << entry.name << " (" << entry.size << " bytes)" << std::endl;
        }

        file.close();

        std::cout << "✅ Asset loaded successfully!" << std::endl;
        return true;
    }

    // =============================================================================
    // UTILITY FUNCTIONS
    // =============================================================================

    void Asset::print_info() const {
        std::cout << "\n📋 ASSET INFORMATION" << std::endl;
        std::cout << "===================" << std::endl;
        std::cout << "Creator: " << header_.creator << std::endl;
        std::cout << "Description: " << header_.description << std::endl;
        std::cout << "Version: " << header_.version_major << "." << header_.version_minor << "." << header_.version_patch << std::endl;
        std::cout << "Type: " << (header_.asset_type == 0 ? "Master Asset" : "Overlay") << std::endl;
        std::cout << "Total Size: " << header_.total_size << " bytes" << std::endl;
        std::cout << "Chunk Count: " << header_.chunk_count << std::endl;

        std::cout << "\nFeature Flags:" << std::endl;
        if (has_feature(FeatureFlags::QuantizedCoords)) std::cout << "  ✅ Quantized Coordinates" << std::endl;
        if (has_feature(FeatureFlags::MeshShaders)) std::cout << "  ✅ Mesh Shaders" << std::endl;
        if (has_feature(FeatureFlags::EmbeddedShaders)) std::cout << "  ✅ Embedded Shaders" << std::endl;
        if (has_feature(FeatureFlags::HashBasedNames)) std::cout << "  ✅ Hash-Based Names" << std::endl;
        if (has_feature(FeatureFlags::PBRMaterials)) std::cout << "  ✅ PBR Materials" << std::endl;

        std::cout << "\nChunks:" << std::endl;
        for (const auto& entry : chunk_directory_) {
            std::cout << "  📦 " << entry.name << " (" << entry.size << " bytes)" << std::endl;
        }
    }

    uint32_t Asset::calculate_crc32(const uint8_t* data, size_t length) const {
        uint32_t crc = 0xFFFFFFFF;
        for (size_t i = 0; i < length; ++i) {
            crc ^= data[i];
            for (int j = 0; j < 8; ++j) {
                crc = (crc >> 1) ^ (0xEDB88320 * (crc & 1));
            }
        }
        return ~crc;
    }

    bool Asset::remove_chunk(ChunkType type) {
        auto data_it = chunk_data_.find(type);
        if (data_it == chunk_data_.end()) {
            return false; // Chunk doesn't exist
        }

        // Remove from chunk data
        chunk_data_.erase(data_it);

        // Remove from chunk directory
        auto dir_it = std::find_if(chunk_directory_.begin(), chunk_directory_.end(),
            [type](const ChunkDirectoryEntry& entry) {
                return entry.type == type;
            });

        if (dir_it != chunk_directory_.end()) {
            std::cout << "  🗑️ Removed chunk: " << dir_it->name << std::endl;
            chunk_directory_.erase(dir_it);
            header_.chunk_count = static_cast<uint32_t>(chunk_directory_.size());
            return true;
        }

        return false;
    }

    uint64_t Asset::get_file_size() const {
        // Calculate total file size:
        // Header + Chunk Directory + All Chunk Data
        uint64_t size = sizeof(AssetHeader);
        size += chunk_directory_.size() * sizeof(ChunkDirectoryEntry);

        for (const auto& [type, data] : chunk_data_) {
            size += data.size();
        }

        return size;
    }
}