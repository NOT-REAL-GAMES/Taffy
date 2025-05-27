#pragma once
#include "taffy.h"

namespace Taffy {

    inline ChunkDirectoryEntry create_chunk_entry(ChunkType type, uint64_t size, const std::string& name) {
        ChunkDirectoryEntry entry{};

        entry.type = type;
        entry.flags = 0;
        entry.offset = 0; // Will be set during save
        entry.size = size;
        entry.checksum = 0; // Will be calculated

        std::memset(entry.name, 0, sizeof(entry.name));
        std::strncpy(entry.name, name.c_str(), sizeof(entry.name) - 1);

        std::memset(entry.reserved, 0, sizeof(entry.reserved));

        return entry;
    }

    inline Asset::Asset() {
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

    inline void Asset::set_creator(const std::string& creator) {
        std::strncpy(header_.creator, creator.c_str(), sizeof(header_.creator) - 1);
        header_.creator[sizeof(header_.creator) - 1] = '\0';
    }

    inline void Asset::set_description(const std::string& description) {
        std::strncpy(header_.description, description.c_str(), sizeof(header_.description) - 1);
        header_.description[sizeof(header_.description) - 1] = '\0';
    }

    inline void Asset::set_feature_flags(FeatureFlags flags) {
        header_.feature_flags = flags;
    }

    inline std::unique_ptr<Asset> Asset::clone() const {
        auto cloned = std::make_unique<Asset>();

        // Copy header and directory
        cloned->header_ = header_;
        cloned->chunk_directory_ = chunk_directory_;

        // Deep copy all chunk data
        for (const auto& [chunk_type, chunk_data] : chunk_data_) {
            cloned->chunk_data_[chunk_type] = chunk_data; // Vector copy
        }

        return cloned;
    }

    inline void Asset::copy_from(const Asset& other) {
        header_ = other.header_;
        chunk_directory_ = other.chunk_directory_;
        chunk_data_.clear();

        // Deep copy all chunk data
        for (const auto& [chunk_type, chunk_data] : other.chunk_data_) {
            chunk_data_[chunk_type] = chunk_data;
        }
    }

    inline std::string Asset::get_creator() const {
        return std::string(header_.creator);
    }

    inline std::string Asset::get_description() const {
        return std::string(header_.description);
    }

    inline FeatureFlags Asset::get_feature_flags() const {
        return header_.feature_flags;
    }

    // =============================================================================
    // FEATURE CHECKING
    // =============================================================================

    inline bool Asset::has_feature(FeatureFlags flag) const {
        return (header_.feature_flags & flag) == flag;
    }

    // =============================================================================
    // CHUNK MANAGEMENT
    // =============================================================================

    inline void Asset::add_chunk(ChunkType type, const std::vector<uint8_t>& data, const std::string& name) {
        std::cout << "  📦 Adding chunk: " << name << " (" << data.size() << " bytes)" << std::endl;

        // Store chunk data
        chunk_data_[type] = data;

        // Create directory entry using safe helper
        ChunkDirectoryEntry entry = create_chunk_entry(type, data.size(), name);
        entry.checksum = calculate_crc32(data.data(), data.size());

        // Add to directory
        chunk_directory_.push_back(entry);
        header_.chunk_count = static_cast<uint32_t>(chunk_directory_.size());

        std::cout << "    ✅ Chunk added. Total chunks: " << header_.chunk_count << std::endl;
        std::cout << "    🔍 Entry debug:" << std::endl;
        std::cout << "      Type: 0x" << std::hex << static_cast<uint32_t>(entry.type) << std::dec << std::endl;
        std::cout << "      Size: " << entry.size << " bytes" << std::endl;
        std::cout << "      Name: " << entry.name << std::endl;
        std::cout << "      Checksum: 0x" << std::hex << entry.checksum << std::dec << std::endl;
    }

    inline bool Asset::has_chunk(ChunkType type) const {
        return chunk_data_.find(type) != chunk_data_.end();
    }

    inline std::optional<std::vector<uint8_t>> Asset::get_chunk_data(ChunkType type) const {
        auto it = chunk_data_.find(type);
        if (it != chunk_data_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    inline size_t Asset::get_chunk_count() const {
        return chunk_directory_.size();
    }

    inline std::vector<ChunkType> Asset::get_chunk_types() const {
        std::vector<ChunkType> types;
        for (const auto& [type, data] : chunk_data_) {
            types.push_back(type);
        }
        return types;
    }

    // =============================================================================
    // FILE I/O
    // =============================================================================

    inline bool Asset::save_to_file(const std::string& path) {
        std::cout << "💾 Saving asset to: " << path << std::endl;

        // Pre-save validation
        std::cout << "  🔍 Pre-save validation:" << std::endl;
        std::cout << "    Header magic: " << std::string(header_.magic, 4) << std::endl;
        std::cout << "    Header version: " << header_.version_major << "." << header_.version_minor << "." << header_.version_patch << std::endl;
        std::cout << "    Header chunk count: " << header_.chunk_count << std::endl;
        std::cout << "    Actual chunks in directory: " << chunk_directory_.size() << std::endl;
        std::cout << "    Actual chunks in data map: " << chunk_data_.size() << std::endl;

        if (header_.chunk_count != chunk_directory_.size() ||
            header_.chunk_count != chunk_data_.size()) {
            std::cerr << "❌ Chunk count mismatch detected!" << std::endl;
            return false;
        }

        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "❌ Failed to open file for writing: " << path << std::endl;
            return false;
        }

        // Calculate total size and chunk offsets
        uint64_t header_size = sizeof(AssetHeader);
        uint64_t directory_size = chunk_directory_.size() * sizeof(ChunkDirectoryEntry);
        uint64_t data_start_offset = header_size + directory_size;

        std::cout << "  📊 File layout calculation:" << std::endl;
        std::cout << "    Header size: " << header_size << " bytes" << std::endl;
        std::cout << "    Directory size: " << directory_size << " bytes" << std::endl;
        std::cout << "    Data starts at: " << data_start_offset << " bytes" << std::endl;

        // Update chunk offsets and total size
        uint64_t current_offset = data_start_offset;
        for (auto& entry : chunk_directory_) {
            entry.offset = current_offset;
            current_offset += entry.size;

            std::cout << "    Chunk '" << entry.name << "': offset=" << entry.offset
                << ", size=" << entry.size << std::endl;
        }

        header_.total_size = current_offset;
        std::cout << "  📦 Total file size: " << header_.total_size << " bytes" << std::endl;

        // Write header with validation
        std::cout << "  ✍️ Writing header..." << std::endl;
        size_t header_written = file.tellp();
        file.write(reinterpret_cast<const char*>(&header_), sizeof(header_));
        if (!file.good()) {
            std::cerr << "❌ Failed to write header!" << std::endl;
            return false;
        }
        std::cout << "    ✅ Header written at offset " << header_written << std::endl;

        // Write chunk directory with validation
        std::cout << "  ✍️ Writing chunk directory..." << std::endl;
        for (size_t i = 0; i < chunk_directory_.size(); ++i) {
            size_t entry_offset = file.tellp();
            file.write(reinterpret_cast<const char*>(&chunk_directory_[i]), sizeof(ChunkDirectoryEntry));
            if (!file.good()) {
                std::cerr << "❌ Failed to write chunk directory entry " << i << std::endl;
                return false;
            }
            std::cout << "    ✅ Entry " << i << " (" << chunk_directory_[i].name
                << ") written at offset " << entry_offset << std::endl;
        }

        // Write chunk data with validation
        std::cout << "  ✍️ Writing chunk data..." << std::endl;
        for (const auto& entry : chunk_directory_) {
            ChunkType type = static_cast<ChunkType>(entry.type);
            auto it = chunk_data_.find(type);
            if (it == chunk_data_.end()) {
                std::cerr << "❌ Chunk data missing for: " << entry.name << std::endl;
                return false;
            }

            size_t data_offset = file.tellp();
            if (data_offset != entry.offset) {
                std::cerr << "❌ Offset mismatch for " << entry.name
                    << ": expected " << entry.offset << ", actual " << data_offset << std::endl;
                return false;
            }

            file.write(reinterpret_cast<const char*>(it->second.data()), it->second.size());
            if (!file.good()) {
                std::cerr << "❌ Failed to write chunk data for: " << entry.name << std::endl;
                return false;
            }

            std::cout << "    ✅ Chunk '" << entry.name << "' data written at offset "
                << data_offset << " (" << it->second.size() << " bytes)" << std::endl;

            // Validate SPIR-V magic if this is a shader chunk
            if (type == ChunkType::SHDR && it->second.size() >= sizeof(ShaderChunk) + 4) {
                // Check if first shader has valid SPIR-V magic
                size_t spirv_offset = sizeof(ShaderChunk) + 2 * sizeof(ShaderChunk::Shader);
                if (spirv_offset + 4 <= it->second.size()) {
                    uint32_t magic;
                    std::memcpy(&magic, it->second.data() + spirv_offset, sizeof(magic));
                    std::cout << "    🔍 SPIR-V magic check: 0x" << std::hex << magic << std::dec;
                    if (magic == 0x07230203) {
                        std::cout << " ✅ VALID" << std::endl;
                    }
                    else {
                        std::cout << " ❌ INVALID (should be 0x07230203)" << std::endl;
                    }
                }
            }
        }

        file.close();

        std::cout << "✅ Asset saved successfully!" << std::endl;
        std::cout << "   📊 Final size: " << header_.total_size << " bytes" << std::endl;
        std::cout << "   📦 Chunks: " << header_.chunk_count << std::endl;

        return true;
    }

    inline bool Asset::load_from_file_safe(const std::string& path) {
        std::cout << "📖 Loading asset from: " << path << std::endl;

        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "❌ Failed to open file for reading: " << path << std::endl;
            return false;
        }

        // Get file size for validation
        file.seekg(0, std::ios::end);
        size_t file_size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::cout << "  📊 File size: " << file_size << " bytes" << std::endl;

        if (file_size < sizeof(AssetHeader)) {
            std::cerr << "❌ File too small for header: " << file_size
                << " bytes (need " << sizeof(AssetHeader) << ")" << std::endl;
            return false;
        }

        // Read header with validation
        std::cout << "  📖 Reading header..." << std::endl;
        AssetHeader temp_header{};
        file.read(reinterpret_cast<char*>(&temp_header), sizeof(AssetHeader));
        if (!file.good()) {
            std::cerr << "❌ Failed to read asset header" << std::endl;
            return false;
        }

        // Debug header contents immediately
        std::cout << "  🔍 Raw header debug:" << std::endl;
        std::cout << "    Magic bytes: [" << std::hex;
        for (int i = 0; i < 4; ++i) {
            std::cout << "0x" << (unsigned char)temp_header.magic[i];
            if (i < 3) std::cout << ", ";
        }
        std::cout << "]" << std::dec << std::endl;

        std::cout << "    Magic string: \"" << std::string(temp_header.magic, 4) << "\"" << std::endl;
        std::cout << "    Version raw: " << temp_header.version_major << "."
            << temp_header.version_minor << "." << temp_header.version_patch << std::endl;
        std::cout << "    Chunk count raw: " << temp_header.chunk_count << std::endl;
        std::cout << "    Total size raw: " << temp_header.total_size << std::endl;
        std::cout << "    Feature flags raw: 0x" << std::hex << static_cast<uint64_t>(temp_header.feature_flags) << std::dec << std::endl;

        // Validate magic with better error reporting
        bool magic_valid = true;
        if (temp_header.magic[0] != 'T' || temp_header.magic[1] != 'A' ||
            temp_header.magic[2] != 'F' || temp_header.magic[3] != '!') {
            std::cerr << "❌ Invalid asset magic!" << std::endl;
            std::cerr << "   Expected: ['T', 'A', 'F', '!']" << std::endl;
            std::cerr << "   Got: ['" << temp_header.magic[0] << "', '"
                << temp_header.magic[1] << "', '" << temp_header.magic[2]
                << "', '" << temp_header.magic[3] << "']" << std::endl;
            magic_valid = false;
        }

        // Validate version (should be reasonable)
        if (temp_header.version_major > 100 || temp_header.version_minor > 100 ||
            temp_header.version_patch > 1000) {
            std::cerr << "❌ Suspicious version numbers: " << temp_header.version_major
                << "." << temp_header.version_minor << "." << temp_header.version_patch << std::endl;
            magic_valid = false;
        }

        // Validate chunk count (should be reasonable)
        if (temp_header.chunk_count > 1000) {
            std::cerr << "❌ Suspicious chunk count: " << temp_header.chunk_count << std::endl;
            magic_valid = false;
        }

        // Validate total size
        if (temp_header.total_size != file_size) {
            std::cerr << "❌ Size mismatch - Header says " << temp_header.total_size
                << " bytes, file is " << file_size << " bytes" << std::endl;
            magic_valid = false;
        }

        if (!magic_valid) {
            std::cerr << "❌ Header validation failed - file may be corrupted" << std::endl;

            // Try to diagnose the issue
            std::cout << "🔍 Attempting to diagnose corruption..." << std::endl;
            file.seekg(0);
            char first_16_bytes[16];
            file.read(first_16_bytes, 16);

            std::cout << "  First 16 bytes of file:" << std::endl;
            for (int i = 0; i < 16; ++i) {
                std::cout << "    [" << i << "] = 0x" << std::hex
                    << (unsigned char)first_16_bytes[i] << std::dec
                    << " ('" << (isprint(first_16_bytes[i]) ? first_16_bytes[i] : '.') << "')" << std::endl;
            }

            return false;
        }

        // Header validation passed, copy it
        header_ = temp_header;

        std::cout << "  📋 Asset info:" << std::endl;
        std::cout << "    Version: " << header_.version_major << "." << header_.version_minor << "." << header_.version_patch << std::endl;
        std::cout << "    Creator: " << header_.creator << std::endl;
        std::cout << "    Description: " << header_.description << std::endl;
        std::cout << "    Chunks: " << header_.chunk_count << std::endl;

        // Validate chunk directory size
        size_t expected_directory_size = header_.chunk_count * sizeof(ChunkDirectoryEntry);
        size_t available_after_header = file_size - sizeof(AssetHeader);

        std::cout << "  📂 Chunk directory validation:" << std::endl;
        std::cout << "    Expected directory size: " << expected_directory_size << " bytes" << std::endl;
        std::cout << "    Available after header: " << available_after_header << " bytes" << std::endl;

        if (expected_directory_size > available_after_header) {
            std::cerr << "❌ Not enough space for chunk directory!" << std::endl;
            return false;
        }

        // Read chunk directory
        chunk_directory_.clear();
        chunk_directory_.resize(header_.chunk_count);

        for (uint32_t i = 0; i < header_.chunk_count; ++i) {
            size_t entry_pos = file.tellg();
            file.read(reinterpret_cast<char*>(&chunk_directory_[i]), sizeof(ChunkDirectoryEntry));
            if (!file.good()) {
                std::cerr << "❌ Failed to read chunk directory entry " << i
                    << " at position " << entry_pos << std::endl;
                return false;
            }

            // Validate chunk directory entry
            ChunkDirectoryEntry& entry = chunk_directory_[i];
            std::cout << "  📦 Chunk " << i << " (" << entry.name << "):" << std::endl;
            std::cout << "    Type: 0x" << std::hex << static_cast<uint32_t>(entry.type) << std::dec << std::endl;
            std::cout << "    Offset: " << entry.offset << std::endl;
            std::cout << "    Size: " << entry.size << " bytes" << std::endl;
            std::cout << "    Checksum: 0x" << std::hex << entry.checksum << std::dec << std::endl;

            // Validate chunk offset and size
            if (entry.offset >= file_size || entry.offset + entry.size > file_size) {
                std::cerr << "❌ Chunk " << i << " (" << entry.name
                    << ") extends beyond file!" << std::endl;
                std::cerr << "   Offset: " << entry.offset << ", Size: " << entry.size
                    << ", File size: " << file_size << std::endl;
                return false;
            }
        }

        // Read chunk data
        chunk_data_.clear();
        for (const auto& entry : chunk_directory_) {
            std::cout << "  📥 Loading chunk: " << entry.name << "..." << std::endl;

            file.seekg(entry.offset);
            if (!file.good()) {
                std::cerr << "❌ Failed to seek to chunk: " << entry.name << std::endl;
                return false;
            }

            std::vector<uint8_t> data(entry.size);
            file.read(reinterpret_cast<char*>(data.data()), entry.size);
            if (!file.good()) {
                std::cerr << "❌ Failed to read chunk data for: " << entry.name << std::endl;
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

            ChunkType type = static_cast<ChunkType>(entry.type);
            chunk_data_[type] = std::move(data);
            std::cout << "    ✅ Loaded chunk: " << entry.name << " (" << entry.size << " bytes)" << std::endl;
        }

        file.close();

        std::cout << "✅ Asset loaded successfully!" << std::endl;
        return true;
    }

    // =============================================================================
    // UTILITY FUNCTIONS
    // =============================================================================

    inline void Asset::print_info() const {
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

    inline uint32_t Asset::calculate_crc32(const uint8_t* data, size_t length) const {
        uint32_t crc = 0xFFFFFFFF;
        for (size_t i = 0; i < length; ++i) {
            crc ^= data[i];
            for (int j = 0; j < 8; ++j) {
                crc = (crc >> 1) ^ (0xEDB88320 * (crc & 1));
            }
        }
        return ~crc;
    }




}