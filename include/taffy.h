﻿/**
 * Taffy: The Web 3.0 Interactive Asset Format
 * Version 0.1 - Foundation Implementation
 *
 * "Real-Time First, Universal Second, Intelligent Third"
 *
 * This header defines the core Taffy format that will evolve from
 * basic geometry loading to AI-native interactive experiences.
 */

#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <optional>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>        // ← ADD THIS
#include <limits>          // ← ADD THIS

 // Assume Tremor's quantized coordinate system exists
#include "quan.h"  // Vec3Q, etc.

namespace Taffy {


        // =============================================================================
        // FNV-1a Hash - CONSTEXPR ONLY (no linker issues)
        // =============================================================================

        constexpr uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
        constexpr uint64_t FNV_PRIME = 1099511628211ULL;

        constexpr uint64_t fnv1a_hash(const char* str) {
            uint64_t hash = FNV_OFFSET_BASIS;
            while (*str) {
                hash ^= static_cast<uint64_t>(*str++);
                hash *= FNV_PRIME;
            }
            return hash;
        }

        // Compile-time hash macro
#define TAFFY_HASH(str) (Taffy::fnv1a_hash(str))

        // =============================================================================
        // Hash Registry - DECLARATION ONLY
        // =============================================================================

        class HashRegistry {
        public:
            static void register_string(const std::string& str);
            static uint64_t register_and_hash(const std::string& str);
            static std::string lookup_string(uint64_t hash);
            static bool has_collision(const std::string& str);
            static void debug_print_all();

        private:
            static std::unordered_map<uint64_t, std::string> hash_to_string_;
        };

        // =============================================================================
        // QUANTIZED COORDINATES
        // =============================================================================


        // =============================================================================
        // ENUMS AND FLAGS
        // =============================================================================

        enum class VertexFormat : uint32_t {
            Position3D = 1 << 0,
            Position2D = 1 << 1,
            Normal = 1 << 2,
            Tangent = 1 << 3,
            TexCoord0 = 1 << 4,
            TexCoord1 = 1 << 5,
            Color = 1 << 6,
            BoneIndices = 1 << 7,
            BoneWeights = 1 << 8,
            Custom0 = 1 << 16,
            Custom1 = 1 << 17,
            Custom2 = 1 << 18,
            Custom3 = 1 << 19
        };

        inline VertexFormat operator|(VertexFormat a, VertexFormat b) {
            return static_cast<VertexFormat>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
        }

        inline VertexFormat operator&(VertexFormat a, VertexFormat b) {
            return static_cast<VertexFormat>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
        }

        enum class MaterialFlags : uint32_t {
            None = 0,
            DoubleSided = 1 << 0,
            Transparent = 1 << 1,
            Emissive = 1 << 2,
            Unlit = 1 << 3,
            CastShadows = 1 << 4,
            ReceiveShadows = 1 << 5,
            Wireframe = 1 << 6,
            Custom0 = 1 << 16,
            Custom1 = 1 << 17,
            Custom2 = 1 << 18,
            Custom3 = 1 << 19
        };

        inline MaterialFlags operator|(MaterialFlags a, MaterialFlags b) {
            return static_cast<MaterialFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
        }

        enum class ChunkType : uint32_t {
            GEOM = 0x4D4F4547,  // 'GEOM'
            MTRL = 0x4C52544D,  // 'MTRL'
            SHDR = 0x52444853,  // 'SHDR'
            TXTR = 0x52545854,  // 'TXTR'
            ANIM = 0x4D494E41,  // 'ANIM'
            SCPT = 0x54504353,  // 'SCPT'
            PHYS = 0x53594850,  // 'PHYS'
            AUDI = 0x49445541,  // 'AUDI'
            FONT = 0x544e4f46,  // 'FONT' - SDF font data
            OVRL = 0x4C52564F,  // 'OVRL'
            CHKO = 0x4F4B4843,  // 'CHKO'
            FRAC = 0x43415246,  // 'FRAC'
            PART = 0x54524150,  // 'PART'
            SVGU = 0x55475653,  // 'SVGU'
            DEPS = 0x53504544,  // 'DEPS'
        };

        enum class FeatureFlags : uint64_t {
            None = 0,
            QuantizedCoords = 1ULL << 0,
            MeshShaders = 1ULL << 1,
            EmbeddedShaders = 1ULL << 2,
            SPIRVCross = 1ULL << 3,
            HashBasedNames = 1ULL << 4,
            Fracturing = 1ULL << 5,
            ParticleSystems = 1ULL << 6,
            PBRMaterials = 1ULL << 7,
            Animation = 1ULL << 8,
            Physics = 1ULL << 9,
            Audio = 1ULL << 10,
            Scripting = 1ULL << 11,
            MultiLOD = 1ULL << 12,
            VirtualTextures = 1ULL << 13,
            SVGUI = 1ULL << 14,
            OverlaySupport = 1ULL << 15,
            SDFFont = 1ULL << 16,         // Signed Distance Field fonts
            AIBehavior = 1ULL << 32,
            NPUProcessing = 1ULL << 33,
            LocalLLM = 1ULL << 34,
            PsychologicalAI = 1ULL << 35,
        };

        inline FeatureFlags operator|(FeatureFlags a, FeatureFlags b) {
            return static_cast<FeatureFlags>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
        }

        inline FeatureFlags operator&(FeatureFlags a, FeatureFlags b) {
            return static_cast<FeatureFlags>(static_cast<uint64_t>(a) & static_cast<uint64_t>(b));
        }

        // =============================================================================
        // CHUNK STRUCTURES
        // =============================================================================
#pragma pack(push, 1)

        struct GeometryChunk {
            uint32_t vertex_count;
            uint32_t index_count;
            uint32_t vertex_stride;
            VertexFormat vertex_format;
            Vec3Q bounds_min;
            Vec3Q bounds_max;
            float lod_distance;
            uint32_t lod_level;

            // Mesh shader configuration
            enum RenderMode : uint32_t {
                Traditional = 0,    // Use vertex/index buffers
                MeshShader = 1      // Use mesh shader
            } render_mode;

            uint32_t ms_max_vertices;      // Mesh shader bounds_max output vertices
            uint32_t ms_max_primitives;    // Mesh shader bounds_max output primitives
            uint32_t ms_workgroup_size[3]; // Mesh shader local work size

            enum PrimitiveType : uint32_t {
                Triangles = 0,
                Lines = 1,
                Points = 2
            } ms_primitive_type;

            uint32_t ms_flags;             // Additional mesh shader flags
            uint32_t reserved[2];          // Reduced for new fields
        };

        // Vertex attribute descriptor for data-driven shaders
        struct VertexAttribute {
            enum Type : uint32_t {
                Float = 0,
                Float2 = 1,
                Float3 = 2,
                Float4 = 3,
                Int = 4,
                Int2 = 5,
                Int3 = 6,
                Int4 = 7,
                UInt = 8,
                UInt2 = 9,
                UInt3 = 10,
                UInt4 = 11,
                Vec3Q = 12
            };

            Type type;
            uint32_t offset;    // Offset in bytes from start of vertex
            uint32_t location;  // Shader output location
            char name[32];      // Semantic name (position, normal, color, etc.)
        };

        struct MaterialChunk {
            uint32_t material_count;
            uint32_t reserved[7];

            struct Material {
                char name[32];
                float albedo[4];
                float emission[3];
                float metallic;
                float roughness;
                float normal_intensity;
                float alpha_cutoff;
                uint32_t albedo_texture;
                uint32_t normal_texture;
                uint32_t metallic_roughness_texture;
                uint32_t emission_texture;
                uint32_t occlusion_texture;
                MaterialFlags flags;
                uint32_t reserved[8];
            };
        };



        struct ShaderChunk {
            uint32_t shader_count;
            uint32_t reserved[3];

            struct Shader {
                uint64_t name_hash;
                uint64_t entry_point_hash;

                enum class ShaderStage : uint32_t {
                    Vertex = 0,
                    Fragment = 1,
                    Geometry = 2,
                    Compute = 3,
                    MeshShader = 4,
                    TaskShader = 5
                } stage;

                uint32_t spirv_size;
                uint32_t max_vertices;
                uint32_t max_primitives;
                uint32_t workgroup_size[3];
                uint32_t reserved[4];
            };
        };

        struct AudioChunk {
            uint32_t node_count;          // Number of audio processing nodes
            uint32_t connection_count;    // Number of node connections
            uint32_t pattern_count;       // Number of tracker patterns
            uint32_t sample_count;        // Number of wavetable samples
            uint32_t parameter_count;     // Number of exposed parameters
            uint32_t sample_rate;         // Default sample rate (e.g. 48000)
            uint32_t tick_rate;           // Tracker ticks per second
            uint32_t streaming_count;      // Number of streaming audio entries
            uint32_t reserved[4];

            // Audio processing node types
            enum class NodeType : uint32_t {
                // Generators
                Oscillator = 0,
                WaveTablePlayer = 1,
                NoiseGenerator = 2,
                Sampler = 3,
                StreamingSampler = 4,    // Streaming audio from disk
                
                // Processors
                Filter = 10,
                Amplifier = 11,
                Envelope = 12,
                LFO = 13,
                Delay = 14,
                Reverb = 15,
                Distortion = 16,
                Compressor = 17,
                
                // Utility
                Mixer = 20,
                Splitter = 21,
                Math = 22,
                
                // Game-aware nodes
                GameState = 30,          // Reads game state (player count, etc)
                Proximity = 31,          // Spatial audio based on distance
                CombatIntensity = 32,    // Reacts to combat events
                
                // Control
                PatternPlayer = 40,      // Plays tracker patterns
                Parameter = 41,          // Exposed parameter input
                Random = 42,             // Random value generator
                
                // Custom VM nodes
                VMNode = 100             // Custom bytecode processing
            };

            struct Node {
                uint32_t id;             // Unique node identifier
                NodeType type;           // Node type
                uint64_t name_hash;      // Hash of node name
                float position[2];       // Visual position in editor
                uint32_t input_count;    // Number of inputs
                uint32_t output_count;   // Number of outputs
                uint32_t param_offset;   // Offset into parameter data
                uint32_t param_count;    // Number of parameters for this node
                uint32_t reserved[4];
            };

            struct Connection {
                uint32_t source_node;    // Source node ID
                uint32_t source_output;  // Output index on source
                uint32_t dest_node;      // Destination node ID  
                uint32_t dest_input;     // Input index on destination
                float strength;          // Connection strength (0-1)
                uint32_t reserved[3];
            };

            struct Pattern {
                uint64_t name_hash;      // Pattern name hash
                uint32_t channel_count;  // Number of channels
                uint32_t row_count;      // Number of rows
                uint32_t ticks_per_row;  // Timing resolution
                uint32_t data_offset;    // Offset to pattern data
                uint32_t data_size;      // Size of pattern data
                uint32_t reserved[3];
            };

            struct Parameter {
                uint64_t name_hash;      // Parameter name hash
                float default_value;     // Default value
                float min_value;         // Minimum value
                float max_value;         // Maximum value
                float curve;             // Response curve (1.0 = linear)
                uint32_t flags;          // Parameter flags
                uint32_t reserved[2];
            };

            struct WaveTable {
                uint64_t name_hash;      // Sample name hash
                uint32_t sample_count;   // Number of samples
                uint32_t channel_count;  // 1 = mono, 2 = stereo
                uint32_t bit_depth;      // 8, 16, 24, or 32
                uint32_t data_offset;    // Offset to sample data
                uint32_t data_size;      // Size of sample data
                float base_frequency;    // Base frequency for pitch
                uint32_t loop_start;     // Loop start point
                uint32_t loop_end;       // Loop end point
                uint32_t reserved[3];
            };
            
            // Streaming audio chunk - references external audio data
            struct StreamingAudio {
                uint64_t name_hash;         // Stream name hash
                uint32_t sample_rate;       // Sample rate (e.g., 44100, 48000)
                uint32_t channel_count;     // 1 = mono, 2 = stereo
                uint32_t bit_depth;         // 8, 16, 24, or 32
                uint32_t total_samples;     // Total number of samples
                uint32_t chunk_size;        // Samples per streaming chunk
                uint32_t chunk_count;       // Number of chunks
                uint64_t data_offset;       // Offset to first audio chunk
                uint32_t format;            // Audio format (0=PCM, 1=float)
                uint32_t reserved[7];
            };
        };

#pragma pack(pop)

        // =============================================================================
        // ASSET HEADER STRUCTURE
        // =============================================================================
#pragma pack(push, 1)  // Force byte alignment to prevent padding issues
        
        // Non-aligned version of Vec3Q for file I/O
        struct Vec3Q_Packed {
            int64_t x, y, z;
        };
        
        struct AssetHeader {
            char magic[4];              // "TAF!" for master assets, "TAFO" for overlays
            uint32_t version_major;
            uint32_t version_minor;
            uint32_t version_patch;
            uint32_t asset_type;        // 0 = master, 1 = overlay
            FeatureFlags feature_flags; // 64-bit capability mask
            uint32_t chunk_count;
            uint32_t dependency_count;
            uint32_t ai_model_count;
            uint64_t total_size;
            Vec3Q_Packed world_bounds_min;     // Quantized world bounds (packed version)
            Vec3Q_Packed world_bounds_max;
            uint64_t created_timestamp;
            char creator[64];           // Creator/tool name
            char description[128];      // Asset description
            uint32_t reserved[16];      // Future expansion
        };

        struct ChunkDirectoryEntry {
            ChunkType type;             // Chunk type identifier
            uint32_t flags;             // Chunk-specific flags
            uint64_t offset;            // Offset from start of file
            uint64_t size;              // Size of chunk data
            uint32_t checksum;          // CRC32 checksum
            char name[32];              // Chunk name (for debugging)
            uint32_t reserved[4];       // Future expansion
        };
        // =============================================================================
        // SDF FONT CHUNK - Signed Distance Field font rendering
        // =============================================================================
        struct FontChunk {
            uint32_t glyph_count;          // Number of glyphs in the font
            uint32_t texture_width;        // Width of the SDF texture atlas
            uint32_t texture_height;       // Height of the SDF texture atlas
            uint32_t texture_format;       // Texture format (usually R8 for SDF)
            float sdf_range;               // Distance range in pixels for SDF
            float font_size;               // Base font size used for generation
            float ascent;                  // Font ascent in pixels
            float descent;                 // Font descent in pixels
            float line_height;             // Recommended line height
            uint32_t first_codepoint;      // First Unicode codepoint in the font
            uint32_t last_codepoint;       // Last Unicode codepoint in the font
            uint32_t kerning_pair_count;   // Number of kerning pairs
            uint64_t texture_data_offset;  // Offset to texture data from chunk start
            uint64_t texture_data_size;    // Size of texture data in bytes
            uint64_t glyph_data_offset;    // Offset to glyph data array
            uint64_t kerning_data_offset;  // Offset to kerning pair data
            uint32_t reserved[8];          // Reserved for future use

            // Glyph information
            struct Glyph {
                uint32_t codepoint;        // Unicode codepoint
                float uv_x;                // Texture coordinate X (0-1)
                float uv_y;                // Texture coordinate Y (0-1)
                float uv_width;            // Width in texture space (0-1)
                float uv_height;           // Height in texture space (0-1)
                float width;               // Glyph width in pixels
                float height;              // Glyph height in pixels
                float bearing_x;           // Horizontal bearing
                float bearing_y;           // Vertical bearing
                float advance;             // Horizontal advance
                uint32_t reserved[2];      // Reserved for future use
            };

            // Kerning pair information
            struct KerningPair {
                uint32_t first;            // First character codepoint
                uint32_t second;           // Second character codepoint
                float amount;              // Kerning amount in pixels
                uint32_t reserved;         // Reserved for future use
            };

            // Font metrics for different styles (optional)
            struct FontStyle {
                uint32_t style_flags;      // Bold, italic, etc.
                float weight;              // Font weight (100-900)
                float slant;               // Italic slant angle
                float outline_width;       // For outlined fonts
                uint32_t reserved[4];      // Reserved
            };
        };

#pragma pack(pop)  // Restore default alignment

        // =============================================================================
        // MAIN ASSET CLASS
        // =============================================================================

        class Asset {
        private:
            AssetHeader header_;
            std::vector<ChunkDirectoryEntry> chunk_directory_;
            std::unordered_map<ChunkType, std::vector<uint8_t>> chunk_data_;

        public:
            inline Asset();

            Asset(const Asset& other)
                : header_(other.header_)
                , chunk_directory_(other.chunk_directory_)
                , chunk_data_(other.chunk_data_) {
                std::cout << "📋 Asset copied" << std::endl;
            }
            Asset& operator=(const Asset& other) {
                if (this != &other) {
                    header_ = other.header_;
                    chunk_directory_ = other.chunk_directory_;
                    chunk_data_ = other.chunk_data_;
                    std::cout << "📋 Asset copy-assigned" << std::endl;
                }
                return *this;
            }
            Asset(Asset&& other) noexcept
                : header_(std::move(other.header_))
                , chunk_directory_(std::move(other.chunk_directory_))
                , chunk_data_(std::move(other.chunk_data_)) {
                std::cout << "🚀 Asset moved" << std::endl;
            }
            Asset& operator=(Asset&& other) noexcept {
                if (this != &other) {
                    header_ = std::move(other.header_);
                    chunk_directory_ = std::move(other.chunk_directory_);
                    chunk_data_ = std::move(other.chunk_data_);
                    std::cout << "🚀 Asset move-assigned" << std::endl;
                }
                return *this;
            }

            std::unique_ptr<Asset> clone() const {
                auto cloned = std::make_unique<Asset>(*this);
                std::cout << "🔄 Asset cloned" << std::endl;
                return cloned;
            }

            // Basic properties
            inline void set_creator(const std::string& creator);
            inline void set_description(const std::string& description);
            inline void set_feature_flags(FeatureFlags flags);
            inline std::string get_creator() const;
            inline std::string get_description() const;
            inline FeatureFlags get_feature_flags() const;

            // Feature checking
            inline bool has_feature(FeatureFlags flag) const;

            // Chunk management
            inline void add_chunk(ChunkType type, const std::vector<uint8_t>& data, const std::string& name = "");
            inline bool has_chunk(ChunkType type) const;
            inline bool remove_chunk(ChunkType type);
            inline std::optional<std::vector<uint8_t>> get_chunk_data(ChunkType type) const;
            inline size_t get_chunk_count() const;
            inline std::vector<ChunkType> get_chunk_types() const;

            inline uint64_t get_file_size() const;

            // File I/O
            inline bool save_to_file(const std::string& path);
            inline bool load_from_file_safe(const std::string& path);

            // Utility
            inline void print_info() const;

            inline void copy_from(const Asset& other);

            inline AssetHeader get_header() const{ return header_; }

        private:
            inline uint32_t calculate_crc32(const uint8_t* data, size_t length) const;
        };

        // =============================================================================
        // COMPILE-TIME HASH CONSTANTS
        // =============================================================================

        namespace ShaderHashes {
            constexpr uint64_t TRIANGLE_MESH = TAFFY_HASH("triangle_mesh_shader");
            constexpr uint64_t TRIANGLE_FRAG = TAFFY_HASH("triangle_fragment_shader");
            constexpr uint64_t WIREFRAME_MESH = TAFFY_HASH("wireframe_mesh_shader");
            constexpr uint64_t ANIMATED_MESH = TAFFY_HASH("animated_mesh_shader");
            constexpr uint64_t MAIN_ENTRY = TAFFY_HASH("main");
        }

}

/*namespace std {
    template<>
    struct hash<Taffy::ChunkType> {
        size_t operator()(const Taffy::ChunkType& type) const {
            return hash<uint32_t>()(static_cast<uint32_t>(type));
        }

        hash() = default;
    };
}*/
/**
 * IMPLEMENTATION NOTES:
 *
 * This implementation provides:
 * - Complete file I/O with validation
 * - CRC32 checksums for data integrity
 * - Flexible chunk system that's easily extensible
 * - Type-safe vertex format handling
 * - Quantized coordinate support (requires Tremor's quan.h)
 * - Clean API for asset creation and manipulation
 *
 * v0.1 Features Implemented:
 * ? Basic geometry loading/saving
 * ? PBR material support
 * ? Chunk-based extensible format
 * ? Data validation and checksums
 * ? Cross-platform binary format
 * ? Quantized coordinate system integration
 *
 * Next Steps for v0.2:
 * - Texture chunk implementation
 * - Basic animation support
 * - Compression (LZ4/ZSTD)
 * - Streaming optimizations
 *
 * The format is designed to grow incrementally while maintaining
 * backward compatibility. New chunk types can be added without
 * breaking existing assets.
 */