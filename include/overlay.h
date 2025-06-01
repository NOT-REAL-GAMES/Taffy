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
            GeometryModify = 4,      // Modify geometry data
            VertexPositionChange = 5,    // Modify individual vertex positions
            VertexAttributeChange = 6,   // Modify any vertex attribute
            GeometryTransform = 7,       // Apply transformation matrix to all vertices
            GeometryScale = 8,           // Scale geometry
            GeometryRotate = 9,          // Rotate geometry
            GeometryTranslate = 10,      // Translate geometry
            UVModification = 11,         // Modify UV coordinates
            NormalRecalculation = 12,    // Recalculate normals after position changes
            VertexSubset = 13            // Apply operation to subset of vertices
        
        } operation_type;

        uint32_t target_chunk_type; // ChunkType to target
        uint64_t target_hash;       // Hash of what to target (shader name, etc.)
        uint64_t replacement_hash;  // Hash of replacement (if applicable)
        uint64_t data_offset;       // Offset to operation data
        uint64_t data_size;         // Size of operation data
        uint32_t flags;             // Operation-specific flags
        uint32_t reserved[3];       // Future expansion
    };

    struct TransformationData {
        float matrix[16];           // 4x4 transformation matrix
        uint32_t flags;             // Which components to transform (position, normal, etc.)
        uint32_t vertex_start;      // Start vertex index (for subset operations)
        uint32_t vertex_count;      // Number of vertices to affect
        uint32_t reserved[4];
    };

    struct AttributeModification {
        uint32_t attribute_offset;  // Offset within vertex structure
        uint32_t attribute_size;    // Size of attribute in bytes (4, 8, 12, 16)
        uint32_t vertex_index;      // Which vertex to modify (or UINT32_MAX for all)
        uint32_t operation_type;    // 0=replace, 1=add, 2=multiply, 3=normalize
        float values[4];            // New/modification values
        uint32_t reserved[4];
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

            std::cout << "    📊 Calculated operation data size: " << data_size << " bytes" << std::endl;
            std::cout << "    📊 Header size: " << sizeof(OverlayHeader) << " bytes" << std::endl;
            std::cout << "    📊 Target size: " << sizeof(TargetAsset) << " bytes" << std::endl;
            std::cout << "    📊 Operation size: " << sizeof(OverlayOperation) << " bytes" << std::endl;

            operation_data_.clear();
            operation_data_.resize(data_size);
            if (data_size > 0) {
                file.read(reinterpret_cast<char*>(operation_data_.data()), data_size);
                std::cout << "    ✅ Read " << data_size << " bytes of operation data" << std::endl;
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
            std::cout << "      Vertex format: 0x" << std::hex << static_cast<uint32_t>(geom_header.vertex_format) << std::dec << std::endl;

            // Validate vertex index
            if (op.target_hash >= geom_header.vertex_count) {
                std::cerr << "    ❌ Vertex index out of range: " << op.target_hash << std::endl;
                return false;
            }

            // Get new color from operation data
            std::cout << "    📊 Operation data size: " << op.data_size << " bytes" << std::endl;
            std::cout << "    📊 Operation data offset: " << op.data_offset << std::endl;
            std::cout << "    📊 Total operation data available: " << operation_data_.size() << " bytes" << std::endl;
            
            if (op.data_size < 4 * sizeof(float)) {
                std::cerr << "    ❌ Insufficient color data! Expected " << (4 * sizeof(float)) << " bytes, got " << op.data_size << std::endl;
                return false;
            }

            float new_color[4];
            std::memcpy(new_color, operation_data_.data() + op.data_offset, 4 * sizeof(float));

            std::cout << "    🌈 New color: (" << new_color[0] << ", " << new_color[1]
                << ", " << new_color[2] << ", " << new_color[3] << ")" << std::endl;

            // Calculate vertex offset
            size_t vertex_data_offset = sizeof(GeometryChunk);
            size_t target_vertex_offset = vertex_data_offset + (op.target_hash * geom_header.vertex_stride);

            // Calculate color offset based on vertex format
            // Check if asset uses quantized coordinates
            bool uses_quantized = asset.has_feature(FeatureFlags::QuantizedCoords);
            
            size_t color_offset_in_vertex;
            if (uses_quantized) {
                // MeshVertex layout with Vec3Q:
                // - position: 24 bytes (Vec3Q - 3x int64_t) 
                // - normal: 12 bytes (3 floats)
                // - color: 16 bytes (4 floats) <- This is what we want
                // - texCoord: 8 bytes (2 floats)
                // - padding: 8 bytes (2 floats)
                color_offset_in_vertex = 36; // Vec3Q(24) + normal(12)
                std::cout << "    ✅ Using Vec3Q position format (24 bytes)" << std::endl;
            } else {
                // Standard float positions:
                // - position: 12 bytes (3 floats)
                // - normal: 12 bytes (3 floats)
                // - color: 16 bytes (4 floats)
                color_offset_in_vertex = 24; // position(12) + normal(12)
                std::cout << "    ✅ Using float position format (12 bytes)" << std::endl;
            }

            size_t absolute_color_offset = target_vertex_offset + color_offset_in_vertex;

            std::cout << "    📍 Color offset in vertex: " << color_offset_in_vertex << " bytes" << std::endl;
            std::cout << "    📍 Absolute color offset: " << absolute_color_offset << " bytes" << std::endl;

            // Validate offset
            if (absolute_color_offset + 4 * sizeof(float) > geom_data->size()) {
                std::cerr << "    ❌ Color offset extends beyond chunk!" << std::endl;

                // Debug: print vertex structure
                std::cout << "    🐛 Debug vertex structure at index " << op.target_hash << ":" << std::endl;
                const uint8_t* vertex_ptr = chunk_ptr + target_vertex_offset;
                for (size_t i = 0; i < std::min((uint32_t)size_t(64), geom_header.vertex_stride); i += 4) {
                    if (target_vertex_offset + i + 4 <= geom_data->size()) {
                        float value;
                        std::memcpy(&value, vertex_ptr + i, sizeof(float));
                        std::cout << "      Offset " << i << ": " << value << std::endl;
                    }
                }

                return false;
            }

            // Create mutable copy of geometry data
            std::vector<uint8_t> modified_geom_data = *geom_data;

            // Modify the vertex color
            std::memcpy(modified_geom_data.data() + absolute_color_offset, new_color, 4 * sizeof(float));

            // Update the asset with modified geometry
            asset.remove_chunk(ChunkType::GEOM);
            asset.add_chunk(ChunkType::GEOM, modified_geom_data, "modified_triangle_geometry");

            std::cout << "    ✅ Vertex color changed successfully!" << std::endl;
            return true;
        }
    };

    class EnhancedOverlay {
    private:
        OverlayHeader header_;
        std::vector<TargetAsset> targets_;
        std::vector<OverlayOperation> operations_;
        std::vector<uint8_t> operation_data_;

    public:
        EnhancedOverlay() {
            std::memset(&header_, 0, sizeof(header_));
            std::memcpy(header_.magic, "TAFO", 4);
            header_.version_major = 2;  // Enhanced version
            header_.version_minor = 0;
            header_.version_patch = 0;
            header_.overlay_type = 0;
            header_.feature_flags = static_cast<uint64_t>(FeatureFlags::HashBasedNames);

            std::strncpy(header_.creator, "Enhanced Data-Driven Overlay", sizeof(header_.creator) - 1);
            std::strncpy(header_.description, "Data-driven geometry overlay", sizeof(header_.description) - 1);
        }

        // Add target asset
        void add_target_asset(const std::string& asset_path, const std::string& version_req = "^1.0.0") {
            TargetAsset target{};
            std::strncpy(target.asset_path, asset_path.c_str(), sizeof(target.asset_path) - 1);
            target.asset_hash = 0;
            std::strncpy(target.version_requirement, version_req.c_str(), sizeof(target.version_requirement) - 1);
            target.required_features = static_cast<uint32_t>(FeatureFlags::HashBasedNames);

            targets_.push_back(target);
            header_.target_count = static_cast<uint32_t>(targets_.size());
        }

        // Scale entire geometry
        void add_scale_operation(float scale_x, float scale_y, float scale_z) {
            TransformationData transform{};

            // Create scale matrix
            std::fill(std::begin(transform.matrix), std::end(transform.matrix), 0.0f);
            transform.matrix[0] = scale_x;   // X scale
            transform.matrix[5] = scale_y;   // Y scale
            transform.matrix[10] = scale_z;  // Z scale
            transform.matrix[15] = 1.0f;     // W component

            transform.flags = 0x01; // Transform positions only
            transform.vertex_start = 0;
            transform.vertex_count = UINT32_MAX; // All vertices

            add_operation(OverlayOperation::Type::GeometryScale,
                static_cast<uint32_t>(ChunkType::GEOM),
                0, 0, transform);
        }

        // Rotate geometry around an axis
        void add_rotation_operation(float angle_radians, float axis_x, float axis_y, float axis_z) {
            TransformationData transform{};

            // Create rotation matrix
            float c = std::cos(angle_radians);
            float s = std::sin(angle_radians);
            float t = 1.0f - c;

            // Normalize axis
            float length = std::sqrt(axis_x * axis_x + axis_y * axis_y + axis_z * axis_z);
            axis_x /= length;
            axis_y /= length;
            axis_z /= length;

            // Rodrigues' rotation formula
            transform.matrix[0] = t * axis_x * axis_x + c;
            transform.matrix[1] = t * axis_x * axis_y - s * axis_z;
            transform.matrix[2] = t * axis_x * axis_z + s * axis_y;
            transform.matrix[3] = 0.0f;

            transform.matrix[4] = t * axis_x * axis_y + s * axis_z;
            transform.matrix[5] = t * axis_y * axis_y + c;
            transform.matrix[6] = t * axis_y * axis_z - s * axis_x;
            transform.matrix[7] = 0.0f;

            transform.matrix[8] = t * axis_x * axis_z - s * axis_y;
            transform.matrix[9] = t * axis_y * axis_z + s * axis_x;
            transform.matrix[10] = t * axis_z * axis_z + c;
            transform.matrix[11] = 0.0f;

            transform.matrix[12] = 0.0f;
            transform.matrix[13] = 0.0f;
            transform.matrix[14] = 0.0f;
            transform.matrix[15] = 1.0f;

            transform.flags = 0x03; // Transform positions and normals
            transform.vertex_start = 0;
            transform.vertex_count = UINT32_MAX;

            add_operation(OverlayOperation::Type::GeometryRotate,
                static_cast<uint32_t>(ChunkType::GEOM),
                0, 0, transform);
        }

        // Translate geometry
        void add_translation_operation(float translate_x, float translate_y, float translate_z) {
            TransformationData transform{};

            // Create translation matrix
            std::fill(std::begin(transform.matrix), std::end(transform.matrix), 0.0f);
            transform.matrix[0] = 1.0f;
            transform.matrix[5] = 1.0f;
            transform.matrix[10] = 1.0f;
            transform.matrix[15] = 1.0f;
            transform.matrix[12] = translate_x;
            transform.matrix[13] = translate_y;
            transform.matrix[14] = translate_z;

            transform.flags = 0x01; // Transform positions only
            transform.vertex_start = 0;
            transform.vertex_count = UINT32_MAX;

            add_operation(OverlayOperation::Type::GeometryTranslate,
                static_cast<uint32_t>(ChunkType::GEOM),
                0, 0, transform);
        }

        // Modify specific vertex position
        void add_vertex_position_change(uint32_t vertex_index, float x, float y, float z) {
            AttributeModification attr_mod{};
            attr_mod.attribute_offset = 0; // Position is typically at offset 0
            attr_mod.attribute_size = 12; // 3 floats = 12 bytes
            attr_mod.vertex_index = vertex_index;
            attr_mod.operation_type = 0; // Replace
            attr_mod.values[0] = x;
            attr_mod.values[1] = y;
            attr_mod.values[2] = z;
            attr_mod.values[3] = 0.0f; // Unused for position

            add_operation(OverlayOperation::Type::VertexPositionChange,
                static_cast<uint32_t>(ChunkType::GEOM),
                vertex_index, 0, attr_mod);
        }

        // Modify UV coordinates
        void add_uv_modification(uint32_t vertex_index, float u, float v, bool flip_u = false, bool flip_v = false) {
            AttributeModification attr_mod{};
            attr_mod.attribute_offset = calculateUVOffset(); // Will be calculated based on vertex format
            attr_mod.attribute_size = 8; // 2 floats = 8 bytes
            attr_mod.vertex_index = vertex_index;
            attr_mod.operation_type = 0; // Replace

            // Apply flipping if requested
            attr_mod.values[0] = flip_u ? (1.0f - u) : u;
            attr_mod.values[1] = flip_v ? (1.0f - v) : v;
            attr_mod.values[2] = 0.0f; // Unused
            attr_mod.values[3] = 0.0f; // Unused

            add_operation(OverlayOperation::Type::UVModification,
                static_cast<uint32_t>(ChunkType::GEOM),
                vertex_index, 0, attr_mod);
        }

        // Modify vertex normal
        void add_normal_change(uint32_t vertex_index, float nx, float ny, float nz, bool normalize = true) {
            AttributeModification attr_mod{};
            attr_mod.attribute_offset = 12; // Normal typically after position (12 bytes)
            attr_mod.attribute_size = 12; // 3 floats = 12 bytes
            attr_mod.vertex_index = vertex_index;
            attr_mod.operation_type = normalize ? 3 : 0; // Normalize or replace
            attr_mod.values[0] = nx;
            attr_mod.values[1] = ny;
            attr_mod.values[2] = nz;
            attr_mod.values[3] = 0.0f; // Unused

            add_operation(OverlayOperation::Type::VertexAttributeChange,
                static_cast<uint32_t>(ChunkType::GEOM),
                vertex_index, 0, attr_mod);
        }

        // Apply operation to subset of vertices
        void add_subset_color_change(uint32_t start_vertex, uint32_t vertex_count,
            float r, float g, float b, float a = 1.0f) {
            AttributeModification attr_mod{};
            attr_mod.attribute_offset = 24; // Color typically after position(12) + normal(12)
            attr_mod.attribute_size = 16; // 4 floats = 16 bytes
            attr_mod.vertex_index = UINT32_MAX; // Special value for subset operation
            attr_mod.operation_type = 0; // Replace
            attr_mod.values[0] = r;
            attr_mod.values[1] = g;
            attr_mod.values[2] = b;
            attr_mod.values[3] = a;

            // Store subset info in transformation data
            TransformationData subset_info{};
            subset_info.vertex_start = start_vertex;
            subset_info.vertex_count = vertex_count;

            add_operation(OverlayOperation::Type::VertexSubset,
                static_cast<uint32_t>(ChunkType::GEOM),
                start_vertex, vertex_count, attr_mod, subset_info);
        }

        // Save and load functions (similar to base Overlay class)
        bool save_to_file(const std::string& path) {
            std::cout << "💾 Saving enhanced overlay to: " << path << std::endl;

            std::ofstream file(path, std::ios::binary);
            if (!file.is_open()) {
                std::cerr << "❌ Failed to open overlay file for writing" << std::endl;
                return false;
            }

            header_.total_size = sizeof(OverlayHeader) +
                targets_.size() * sizeof(TargetAsset) +
                operations_.size() * sizeof(OverlayOperation) +
                operation_data_.size();

            file.write(reinterpret_cast<const char*>(&header_), sizeof(header_));

            for (const auto& target : targets_) {
                file.write(reinterpret_cast<const char*>(&target), sizeof(target));
            }

            for (const auto& op : operations_) {
                file.write(reinterpret_cast<const char*>(&op), sizeof(op));
            }

            if (!operation_data_.empty()) {
                file.write(reinterpret_cast<const char*>(operation_data_.data()), operation_data_.size());
            }

            file.close();

            std::cout << "✅ Enhanced overlay saved!" << std::endl;
            std::cout << "   📊 Size: " << header_.total_size << " bytes" << std::endl;
            std::cout << "   🎯 Operations: " << header_.operation_count << std::endl;

            return true;
        }

        // Apply enhanced overlay to asset
        bool apply_to_asset(Asset& asset) const {
            std::cout << "🔧 Applying enhanced overlay..." << std::endl;

            for (const auto& op : operations_) {
                switch (static_cast<OverlayOperation::Type>(op.operation_type)) {
                case OverlayOperation::Type::VertexColorChange:
                    if (!apply_vertex_color_change(asset, op)) {
                        return false;
                    }
                    break;

                case OverlayOperation::Type::VertexPositionChange:
                    if (!apply_vertex_position_change(asset, op)) {
                        return false;
                    }
                    break;

                case OverlayOperation::Type::GeometryScale:
                    if (!apply_geometry_transform(asset, op, "scale")) {
                        return false;
                    }
                    break;

                case OverlayOperation::Type::GeometryRotate:
                    if (!apply_geometry_transform(asset, op, "rotate")) {
                        return false;
                    }
                    break;

                case OverlayOperation::Type::GeometryTranslate:
                    if (!apply_geometry_transform(asset, op, "translate")) {
                        return false;
                    }
                    break;

                case OverlayOperation::Type::UVModification:
                    if (!apply_uv_modification(asset, op)) {
                        return false;
                    }
                    break;

                case OverlayOperation::Type::VertexSubset:
                    if (!apply_vertex_subset_operation(asset, op)) {
                        return false;
                    }
                    break;

                default:
                    std::cout << "⚠️ Unsupported enhanced operation: " << (uint32_t)op.operation_type << std::endl;
                    break;
                }
            }

            std::cout << "✅ Enhanced overlay applied successfully!" << std::endl;
            return true;
        }

    private:
        template<typename T>
        void add_operation(OverlayOperation::Type op_type, uint32_t chunk_type,
            uint64_t target_hash, uint64_t replacement_hash,
            const T& data) {
            OverlayOperation op{};
            op.operation_type = static_cast<OverlayOperation::Type>(op_type);
            op.target_chunk_type = chunk_type;
            op.target_hash = target_hash;
            op.replacement_hash = replacement_hash;
            op.data_offset = operation_data_.size();
            op.data_size = sizeof(T);
            op.flags = 0;

            // Add data to operation data buffer
            size_t old_size = operation_data_.size();
            operation_data_.resize(old_size + sizeof(T));
            std::memcpy(operation_data_.data() + old_size, &data, sizeof(T));

            operations_.push_back(op);
            header_.operation_count = static_cast<uint32_t>(operations_.size());
        }

        template<typename T1, typename T2>
        void add_operation(OverlayOperation::Type op_type, uint32_t chunk_type,
            uint64_t target_hash, uint64_t replacement_hash,
            const T1& data1, const T2& data2) {
            OverlayOperation op{};
            op.operation_type = static_cast<OverlayOperation::Type>(op_type);
            op.target_chunk_type = chunk_type;
            op.target_hash = target_hash;
            op.replacement_hash = replacement_hash;
            op.data_offset = operation_data_.size();
            op.data_size = sizeof(T1) + sizeof(T2);
            op.flags = 0;

            // Add both data structures to operation data buffer
            size_t old_size = operation_data_.size();
            operation_data_.resize(old_size + sizeof(T1) + sizeof(T2));
            std::memcpy(operation_data_.data() + old_size, &data1, sizeof(T1));
            std::memcpy(operation_data_.data() + old_size + sizeof(T1), &data2, sizeof(T2));

            operations_.push_back(op);
            header_.operation_count = static_cast<uint32_t>(operations_.size());
        }

        uint32_t calculateUVOffset() const {
            // This would analyze the target asset's vertex format to determine UV offset
            // For now, assume standard layout: position(12) + normal(12) + color(16) + uv(8)
            return 40;
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
            std::cout << "      Vertex format: 0x" << std::hex << static_cast<uint32_t>(geom_header.vertex_format) << std::dec << std::endl;

            // Validate vertex index
            if (op.target_hash >= geom_header.vertex_count) {
                std::cerr << "    ❌ Vertex index out of range: " << op.target_hash << std::endl;
                return false;
            }

            // Get new color from operation data
            std::cout << "    📊 Operation data size: " << op.data_size << " bytes" << std::endl;
            std::cout << "    📊 Operation data offset: " << op.data_offset << std::endl;
            std::cout << "    📊 Total operation data available: " << operation_data_.size() << " bytes" << std::endl;
            
            if (op.data_size < 4 * sizeof(float)) {
                std::cerr << "    ❌ Insufficient color data! Expected " << (4 * sizeof(float)) << " bytes, got " << op.data_size << std::endl;
                return false;
            }

            float new_color[4];
            std::memcpy(new_color, operation_data_.data() + op.data_offset, 4 * sizeof(float));

            std::cout << "    🌈 New color: (" << new_color[0] << ", " << new_color[1]
                << ", " << new_color[2] << ", " << new_color[3] << ")" << std::endl;

            // Calculate vertex offset
            size_t vertex_data_offset = sizeof(GeometryChunk);
            size_t target_vertex_offset = vertex_data_offset + (op.target_hash * geom_header.vertex_stride);

            // Calculate color offset based on vertex format
            // Check if asset uses quantized coordinates
            bool uses_quantized = asset.has_feature(FeatureFlags::QuantizedCoords);
            
            size_t color_offset_in_vertex;
            if (uses_quantized) {
                // MeshVertex layout with Vec3Q:
                // - position: 24 bytes (Vec3Q - 3x int64_t) 
                // - normal: 12 bytes (3 floats)
                // - color: 16 bytes (4 floats) <- This is what we want
                // - texCoord: 8 bytes (2 floats)
                // - padding: 8 bytes (2 floats)
                color_offset_in_vertex = 36; // Vec3Q(24) + normal(12)
                std::cout << "    ✅ Using Vec3Q position format (24 bytes)" << std::endl;
            } else {
                // Standard float positions:
                // - position: 12 bytes (3 floats)
                // - normal: 12 bytes (3 floats)
                // - color: 16 bytes (4 floats)
                color_offset_in_vertex = 24; // position(12) + normal(12)
                std::cout << "    ✅ Using float position format (12 bytes)" << std::endl;
            }

            size_t absolute_color_offset = target_vertex_offset + color_offset_in_vertex;

            std::cout << "    📍 Color offset in vertex: " << color_offset_in_vertex << " bytes" << std::endl;
            std::cout << "    📍 Absolute color offset: " << absolute_color_offset << " bytes" << std::endl;

            // Validate offset
            if (absolute_color_offset + 4 * sizeof(float) > geom_data->size()) {
                std::cerr << "    ❌ Color offset extends beyond chunk!" << std::endl;

                // Debug: print vertex structure
                std::cout << "    🐛 Debug vertex structure at index " << op.target_hash << ":" << std::endl;
                const uint8_t* vertex_ptr = chunk_ptr + target_vertex_offset;
                for (size_t i = 0; i < std::min((uint32_t)size_t(64), geom_header.vertex_stride); i += 4) {
                    if (target_vertex_offset + i + 4 <= geom_data->size()) {
                        float value;
                        std::memcpy(&value, vertex_ptr + i, sizeof(float));
                        std::cout << "      Offset " << i << ": " << value << std::endl;
                    }
                }

                return false;
            }

            // Create mutable copy of geometry data
            std::vector<uint8_t> modified_geom_data = *geom_data;

            // Modify the vertex color
            std::memcpy(modified_geom_data.data() + absolute_color_offset, new_color, 4 * sizeof(float));

            // Update the asset with modified geometry
            asset.remove_chunk(ChunkType::GEOM);
            asset.add_chunk(ChunkType::GEOM, modified_geom_data, "modified_triangle_geometry");

            std::cout << "    ✅ Vertex color changed successfully!" << std::endl;
            return true;
        }
        bool apply_vertex_position_change(Asset& asset, const OverlayOperation& op) const {
            std::cout << "  📍 Applying vertex position change..." << std::endl;

            if (op.data_size < sizeof(AttributeModification)) {
                std::cerr << "    ❌ Insufficient data for position change!" << std::endl;
                return false;
            }

            AttributeModification attr_mod;
            std::memcpy(&attr_mod, operation_data_.data() + op.data_offset, sizeof(attr_mod));

            auto geom_data = asset.get_chunk_data(ChunkType::GEOM);
            if (!geom_data) {
                return false;
            }

            GeometryChunk geom_header;
            std::memcpy(&geom_header, geom_data->data(), sizeof(geom_header));

            std::vector<uint8_t> modified_geom_data = *geom_data;

            // Calculate position offset
            size_t vertex_data_offset = sizeof(GeometryChunk);
            size_t target_vertex_offset = vertex_data_offset + (attr_mod.vertex_index * geom_header.vertex_stride);
            size_t absolute_position_offset = target_vertex_offset + attr_mod.attribute_offset;

            if (absolute_position_offset + attr_mod.attribute_size <= modified_geom_data.size()) {
                std::memcpy(modified_geom_data.data() + absolute_position_offset,
                    attr_mod.values, attr_mod.attribute_size);

                asset.remove_chunk(ChunkType::GEOM);
                asset.add_chunk(ChunkType::GEOM, modified_geom_data, "position_modified_geometry");

                std::cout << "    ✅ Vertex position changed successfully!" << std::endl;
                return true;
            }

            return false;
        }

        bool apply_geometry_transform(Asset& asset, const OverlayOperation& op, const std::string& transform_type) const {
            std::cout << "  🔄 Applying geometry " << transform_type << "..." << std::endl;

            if (op.data_size < sizeof(TransformationData)) {
                std::cerr << "    ❌ Insufficient data for transformation!" << std::endl;
                return false;
            }

            TransformationData transform;
            std::memcpy(&transform, operation_data_.data() + op.data_offset, sizeof(transform));

            // Apply transformation to all vertices in the geometry
            auto geom_data = asset.get_chunk_data(ChunkType::GEOM);
            if (!geom_data) {
                return false;
            }

            GeometryChunk geom_header;
            std::memcpy(&geom_header, geom_data->data(), sizeof(geom_header));

            std::vector<uint8_t> modified_geom_data = *geom_data;

            // Apply transformation matrix to vertices
            uint8_t* vertex_data_ptr = modified_geom_data.data() + sizeof(GeometryChunk);

            uint32_t start_vertex = (transform.vertex_count == UINT32_MAX) ? 0 : transform.vertex_start;
            uint32_t end_vertex = (transform.vertex_count == UINT32_MAX) ? geom_header.vertex_count :
                std::min(transform.vertex_start + transform.vertex_count, geom_header.vertex_count);

            for (uint32_t i = start_vertex; i < end_vertex; ++i) {
                uint8_t* vertex_ptr = vertex_data_ptr + (i * geom_header.vertex_stride);

                // Transform position if flag is set
                if (transform.flags & 0x01) {
                    float* position = reinterpret_cast<float*>(vertex_ptr);
                    transformVector3(position, transform.matrix);
                }

                // Transform normal if flag is set
                if (transform.flags & 0x02) {
                    float* normal = reinterpret_cast<float*>(vertex_ptr + 12); // Assuming normal at offset 12
                    transformVector3(normal, transform.matrix);
                    normalizeVector3(normal);
                }
            }

            asset.remove_chunk(ChunkType::GEOM);
            asset.add_chunk(ChunkType::GEOM, modified_geom_data, transform_type + "_transformed_geometry");

            std::cout << "    ✅ Geometry " << transform_type << " applied successfully!" << std::endl;
            return true;
        }

        bool apply_uv_modification(Asset& asset, const OverlayOperation& op) const {
            std::cout << "  🗺️ Applying UV modification..." << std::endl;

            // Implementation similar to other attribute modifications
            // but specifically for UV coordinates

            return true; // Placeholder
        }

        bool apply_vertex_subset_operation(Asset& asset, const OverlayOperation& op) const {
            std::cout << "  📊 Applying subset operation..." << std::endl;

            // Implementation for operations on vertex subsets

            return true; // Placeholder
        }

        size_t calculateColorOffsetForFormat(VertexFormat format) const {
            size_t offset = 0;

            if (static_cast<uint32_t>(format) & static_cast<uint32_t>(VertexFormat::Position3D)) {
                offset += 12; // 3 floats
            }
            if (static_cast<uint32_t>(format) & static_cast<uint32_t>(VertexFormat::Normal)) {
                offset += 12; // 3 floats
            }
            // Color comes next in our standard layout

            return offset;
        }

        void transformVector3(float* vec, const float* matrix) const {
            float x = vec[0], y = vec[1], z = vec[2];
            vec[0] = matrix[0] * x + matrix[4] * y + matrix[8] * z + matrix[12];
            vec[1] = matrix[1] * x + matrix[5] * y + matrix[9] * z + matrix[13];
            vec[2] = matrix[2] * x + matrix[6] * y + matrix[10] * z + matrix[14];
        }

        void normalizeVector3(float* vec) const {
            float length = std::sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
            if (length > 0.0f) {
                vec[0] /= length;
                vec[1] /= length;
                vec[2] /= length;
            }
        }
    };


} // namespace Taffy