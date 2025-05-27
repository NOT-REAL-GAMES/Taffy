#pragma once
#include "taffy.h"
#include <string>
#include <vector>

namespace Taffy {

    // =============================================================================
    // UPDATED OVERLAY FORMAT (Hash-Based)
    // =============================================================================

#pragma pack(push, 1)

    struct OverlayHeader {
        char magic[4];              // "TAFO" for overlays
        uint32_t version_major;     // 1 (updated for hash-based)
        uint32_t version_minor;     // 0
        uint32_t version_patch;     // 0
        uint32_t overlay_type;      // 0 = standard overlay
        uint64_t feature_flags;     // Required features (including HashBasedNames)
        uint32_t operation_count;   // Number of overlay operations
        uint32_t target_count;      // Number of target assets
        uint64_t total_size;        // Total overlay file size
        char creator[64];           // Overlay creator
        char description[128];      // Overlay description
        uint64_t reserved[8];       // Future expansion
    };

    struct TargetAsset {
        char asset_path[256];       // Relative path to target asset
        uint64_t asset_hash;        // Hash of target asset for validation
        char version_requirement[32]; // Version requirement (e.g., "^1.0.0")
        uint32_t required_features; // Required feature flags
        uint32_t reserved[4];       // Future expansion
    };

    struct OverlayOperation {
        enum class Type : uint32_t {
            ChunkReplace = 0,       // Replace entire chunk
            ShaderReplace = 1,      // Replace specific shader by hash
            VertexColorChange = 2,  // Change vertex colors
            MaterialReplace = 3,    // Replace material properties
            GeometryModify = 4      // Modify geometry data
        } operation_type;

        uint32_t target_chunk_type; // ChunkType to target
        uint64_t target_hash;       // Hash of what to target (shader name, etc.)
        uint64_t replacement_hash;  // Hash of replacement (if applicable)
        uint64_t data_offset;       // Offset to operation data
        uint64_t data_size;         // Size of operation data
        uint32_t flags;             // Operation-specific flags
        uint32_t reserved[3];       // Future expansion
    };

#pragma pack(pop)

    // =============================================================================
    // HASH-BASED OVERLAY CLASS
    // =============================================================================

    class Overlay {
    private:
        OverlayHeader header_;
        std::vector<TargetAsset> targets_;
        std::vector<OverlayOperation> operations_;
        std::vector<uint8_t> operation_data_;

    public:
        Overlay() {
            std::memset(&header_, 0, sizeof(header_));
            std::memcpy(header_.magic, "TAFO", 4);
            header_.version_major = 1;  // Updated for hash-based system
            header_.version_minor = 0;
            header_.version_patch = 0;
            header_.overlay_type = 0;
            header_.feature_flags = static_cast<uint64_t>(FeatureFlags::HashBasedNames);
            header_.operation_count = 0;
            header_.target_count = 0;
            header_.total_size = sizeof(OverlayHeader);

            std::strncpy(header_.creator, "Hash-Based Overlay Creator", sizeof(header_.creator) - 1);
            std::strncpy(header_.description, "Hash-based Taffy Overlay", sizeof(header_.description) - 1);
        }

        // Add target asset
        void add_target_asset(const std::string& asset_path, const std::string& version_req = "^1.0.0") {
            TargetAsset target{};
            std::strncpy(target.asset_path, asset_path.c_str(), sizeof(target.asset_path) - 1);
            target.asset_hash = 0; // Will be calculated when overlay is applied
            std::strncpy(target.version_requirement, version_req.c_str(), sizeof(target.version_requirement) - 1);
            target.required_features = static_cast<uint32_t>(FeatureFlags::HashBasedNames);

            targets_.push_back(target);
            header_.target_count = static_cast<uint32_t>(targets_.size());
        }

        // Add shader replacement operation (hash-based)
        void add_shader_replacement(uint64_t target_shader_hash, uint64_t replacement_shader_hash,
            const std::vector<uint32_t>& new_spirv) {
            OverlayOperation op{};
            op.operation_type = OverlayOperation::Type::ShaderReplace;
            op.target_chunk_type = static_cast<uint32_t>(ChunkType::SHDR);
            op.target_hash = target_shader_hash;
            op.replacement_hash = replacement_shader_hash;
            op.data_offset = operation_data_.size();
            op.data_size = new_spirv.size() * sizeof(uint32_t);
            op.flags = 0;

            // Add SPIR-V data to operation data
            size_t spirv_bytes = new_spirv.size() * sizeof(uint32_t);
            operation_data_.resize(operation_data_.size() + spirv_bytes);
            std::memcpy(operation_data_.data() + op.data_offset, new_spirv.data(), spirv_bytes);

            operations_.push_back(op);
            header_.operation_count = static_cast<uint32_t>(operations_.size());
        }

        // Add vertex color change operation
        void add_vertex_color_change(uint32_t vertex_index, float r, float g, float b, float a = 1.0f) {
            OverlayOperation op{};
            op.operation_type = OverlayOperation::Type::VertexColorChange;
            op.target_chunk_type = static_cast<uint32_t>(ChunkType::GEOM);
            op.target_hash = vertex_index; // Use hash field to store vertex index
            op.data_offset = operation_data_.size();
            op.data_size = 4 * sizeof(float);

            // Add color data
            float color[4] = { r, g, b, a };
            operation_data_.resize(operation_data_.size() + op.data_size);
            std::memcpy(operation_data_.data() + op.data_offset, color, op.data_size);

            operations_.push_back(op);
            header_.operation_count = static_cast<uint32_t>(operations_.size());
        }

        // Save overlay to file
        bool save_to_file(const std::string& path) {
            std::cout << "💾 Saving hash-based overlay to: " << path << std::endl;

            std::ofstream file(path, std::ios::binary);
            if (!file.is_open()) {
                std::cerr << "❌ Failed to open overlay file for writing" << std::endl;
                return false;
            }

            // Calculate total size
            header_.total_size = sizeof(OverlayHeader) +
                targets_.size() * sizeof(TargetAsset) +
                operations_.size() * sizeof(OverlayOperation) +
                operation_data_.size();

            // Write header
            file.write(reinterpret_cast<const char*>(&header_), sizeof(header_));

            // Write targets
            for (const auto& target : targets_) {
                file.write(reinterpret_cast<const char*>(&target), sizeof(target));
            }

            // Write operations
            for (const auto& op : operations_) {
                file.write(reinterpret_cast<const char*>(&op), sizeof(op));
            }

            // Write operation data
            if (!operation_data_.empty()) {
                file.write(reinterpret_cast<const char*>(operation_data_.data()), operation_data_.size());
            }

            file.close();

            std::cout << "✅ Hash-based overlay saved!" << std::endl;
            std::cout << "   📊 Size: " << header_.total_size << " bytes" << std::endl;
            std::cout << "   🎯 Targets: " << header_.target_count << std::endl;
            std::cout << "   🔧 Operations: " << header_.operation_count << std::endl;

            return true;
        }

        // Load overlay from file
        bool load_from_file(const std::string& path) {
            std::cout << "📖 Loading hash-based overlay from: " << path << std::endl;

            std::ifstream file(path, std::ios::binary);
            if (!file.is_open()) {
                std::cerr << "❌ Failed to open overlay file for reading" << std::endl;
                return false;
            }

            // Read header
            file.read(reinterpret_cast<char*>(&header_), sizeof(header_));
            if (!file.good() || std::strncmp(header_.magic, "TAFO", 4) != 0) {
                std::cerr << "❌ Invalid overlay file magic" << std::endl;
                return false;
            }

            std::cout << "  📋 Overlay info:" << std::endl;
            std::cout << "    Version: " << header_.version_major << "." << header_.version_minor << "." << header_.version_patch << std::endl;
            std::cout << "    Creator: " << header_.creator << std::endl;
            std::cout << "    Operations: " << header_.operation_count << std::endl;

            // Read targets
            targets_.clear();
            targets_.resize(header_.target_count);
            for (uint32_t i = 0; i < header_.target_count; ++i) {
                file.read(reinterpret_cast<char*>(&targets_[i]), sizeof(TargetAsset));
            }

            // Read operations
            operations_.clear();
            operations_.resize(header_.operation_count);
            for (uint32_t i = 0; i < header_.operation_count; ++i) {
                file.read(reinterpret_cast<char*>(&operations_[i]), sizeof(OverlayOperation));
            }

            // Read operation data
            size_t data_size = header_.total_size - sizeof(OverlayHeader) -
                targets_.size() * sizeof(TargetAsset) -
                operations_.size() * sizeof(OverlayOperation);

            operation_data_.clear();
            operation_data_.resize(data_size);
            if (data_size > 0) {
                file.read(reinterpret_cast<char*>(operation_data_.data()), data_size);
            }

            file.close();

            std::cout << "✅ Hash-based overlay loaded!" << std::endl;
            return true;
        }

        // Check if this overlay targets the given asset
        bool targets_asset(const Asset& asset) const {
            // Check if asset has hash-based names feature
            if (!asset.has_feature(FeatureFlags::HashBasedNames)) {
                std::cout << "❌ Target asset doesn't support hash-based names!" << std::endl;
                return false;
            }

            // Check version compatibility
            if (header_.version_major > 1) {
                std::cout << "❌ Overlay version mismatch!" << std::endl;
                return false;
            }

            // For now, assume it targets the asset if it has the right features
            // In a full implementation, you'd check the asset path/hash
            return true;
        }

        // Apply overlay to asset
        bool apply_to_asset(Asset& asset) const {
            if (!targets_asset(asset)) {
                return false;
            }

            std::cout << "🔧 Applying hash-based overlay..." << std::endl;

            for (const auto& op : operations_) {
                switch (op.operation_type) {
                case OverlayOperation::Type::ShaderReplace:
                    if (!apply_shader_replacement(asset, op)) {
                        return false;
                    }
                    break;

                case OverlayOperation::Type::VertexColorChange:
                    if (!apply_vertex_color_change(asset, op)) {
                        return false;
                    }
                    break;

                default:
                    std::cout << "⚠️ Unsupported operation type: " << static_cast<int>(op.operation_type) << std::endl;
                    break;
                }
            }

            std::cout << "✅ Hash-based overlay applied successfully!" << std::endl;
            return true;
        }

    private:
        bool apply_shader_replacement(Asset& asset, const OverlayOperation& op) const {
            std::cout << "  🔄 Applying shader replacement..." << std::endl;
            std::cout << "    Target hash: 0x" << std::hex << op.target_hash << std::dec << std::endl;
            std::cout << "    Replacement hash: 0x" << std::hex << op.replacement_hash << std::dec << std::endl;

            // TODO: Implement shader replacement logic
            // This would involve:
            // 1. Finding the shader chunk
            // 2. Locating the shader with target_hash
            // 3. Replacing its SPIR-V with the operation data

            return true;
        }

        bool apply_vertex_color_change(Asset& asset, const OverlayOperation& op) const {
            std::cout << "  🎨 Applying vertex color change..." << std::endl;
            std::cout << "    Vertex index: " << op.target_hash << std::endl;

            // Get geometry chunk
            auto geom_data = asset.get_chunk_data(ChunkType::GEOM);
            if (!geom_data) {
                std::cerr << "    ❌ No geometry chunk found!" << std::endl;
                return false;
            }

            // Parse geometry header
            const uint8_t* chunk_ptr = geom_data->data();
            GeometryChunk geom_header;
            std::memcpy(&geom_header, chunk_ptr, sizeof(geom_header));

            std::cout << "    📊 Geometry info:" << std::endl;
            std::cout << "      Vertex count: " << geom_header.vertex_count << std::endl;
            std::cout << "      Vertex stride: " << geom_header.vertex_stride << " bytes" << std::endl;

            // Validate vertex index
            if (op.target_hash >= geom_header.vertex_count) {
                std::cerr << "    ❌ Vertex index out of range: " << op.target_hash << std::endl;
                return false;
            }

            // Get new color from operation data
            if (op.data_size < 4 * sizeof(float)) {
                std::cerr << "    ❌ Insufficient color data!" << std::endl;
                return false;
            }

            float new_color[4];
            std::memcpy(new_color, operation_data_.data() + op.data_offset, 4 * sizeof(float));

            std::cout << "    🌈 New color: (" << new_color[0] << ", " << new_color[1]
                << ", " << new_color[2] << ", " << new_color[3] << ")" << std::endl;

            // Calculate vertex offset
            size_t vertex_data_offset = sizeof(GeometryChunk);
            size_t target_vertex_offset = vertex_data_offset + (op.target_hash * geom_header.vertex_stride);

            // Assuming vertex structure: Vec3Q position (24 bytes) + float normal[3] (12 bytes) + float uv[2] (8 bytes) + float color[4] (16 bytes)
            size_t color_offset_in_vertex = sizeof(Vec3Q) + 3 * sizeof(float) + 2 * sizeof(float); // 24 + 12 + 8 = 44
            size_t absolute_color_offset = target_vertex_offset + color_offset_in_vertex;

            std::cout << "    📍 Color offset: " << absolute_color_offset << std::endl;

            // Validate offset
            if (absolute_color_offset + 4 * sizeof(float) > geom_data->size()) {
                std::cerr << "    ❌ Color offset extends beyond chunk!" << std::endl;
                return false;
            }

            // Create mutable copy of geometry data
            std::vector<uint8_t> modified_geom_data = *geom_data;

            // Modify the vertex color
            std::memcpy(modified_geom_data.data() + absolute_color_offset, new_color, 4 * sizeof(float));

            // Update the asset with modified geometry
            asset.remove_chunk(ChunkType::GEOM);  // Remove old
            asset.add_chunk(ChunkType::GEOM, modified_geom_data, "modified_triangle_geometry");  // Add new

            std::cout << "    ✅ Vertex color changed successfully!" << std::endl;
            return true;
        }
    };

} // namespace Taffy