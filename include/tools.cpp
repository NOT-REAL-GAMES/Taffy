#include "tools.h"
#include "quan.h"
#include "asset.h"

#include <iomanip>

#include <iostream>
#include <unordered_set>

#include <string>
#include <sstream>
#include <unordered_map>

using namespace tremor::taffy::tools;

namespace Taffy {

    // =============================================================================
    // STATIC MEMBER DEFINITION (This was causing the linker error!)
    // =============================================================================

    std::unordered_map<uint64_t, std::string> HashRegistry::hash_to_string_;

    // =============================================================================
    // HASH REGISTRY IMPLEMENTATIONS
    // =============================================================================

    void HashRegistry::register_string(const std::string& str) {
        uint64_t hash = fnv1a_hash(str.c_str());
        hash_to_string_[hash] = str;
    }

    uint64_t HashRegistry::register_and_hash(const std::string& str) {
        uint64_t hash = fnv1a_hash(str.c_str());
        hash_to_string_[hash] = str;
        return hash;
    }

    std::string HashRegistry::lookup_string(uint64_t hash) {
        auto it = hash_to_string_.find(hash);
        if (it != hash_to_string_.end()) {
            return it->second;
        }
        return "UNKNOWN_HASH_0x" + std::to_string(hash);
    }

    bool HashRegistry::has_collision(const std::string& str) {
        uint64_t hash = fnv1a_hash(str.c_str());
        auto it = hash_to_string_.find(hash);
        return (it != hash_to_string_.end() && it->second != str);
    }

    void HashRegistry::debug_print_all() {
        std::cout << "🔍 Hash Registry Contents:" << std::endl;
        for (const auto& [hash, str] : hash_to_string_) {
            std::cout << "  0x" << std::hex << hash << std::dec << " -> \"" << str << "\"" << std::endl;
        }
    }

}

namespace tremor::taffy::tools {

    bool createHotPinkShaderOverlay(const std::string& output_path) {
    std::cout << "🌈 Creating HOT PINK shader overlay..." << std::endl;
    
    using namespace Taffy;
    
    Overlay overlay;
    overlay.add_target_asset("assets/cube.taf", "^1.0.0");  // Target the cube asset
    
    // Create a simple hot pink fragment shader
    std::string hotPinkFragShader = R"(
#version 460

layout(location = 0) out vec4 fragColor;

void main() {
    fragColor = vec4(0.0, 1.0, 1.0, 1.0);
}
)";
    
    // Compile the shader
    TaffyAssetCompiler compiler;
    std::vector<uint32_t> compiledFrag = compiler.compileGLSLToSpirv(
        hotPinkFragShader, 
        shaderc_fragment_shader,
        "hot_pink_fragment_shader"
    );
    
    if (compiledFrag.empty()) {
        std::cerr << "Failed to compile hot pink fragment shader" << std::endl;
        return false;
    }
    
    std::cout << "    📊 Compiled hot pink shader size: " << compiledFrag.size() * sizeof(uint32_t) << " bytes" << std::endl;
    
    // Add shader replacement operation
    // Hash the shader names to match what's in the TAF file
    uint64_t original_frag_hash = fnv1a_hash("data_driven_fragment_shader");
    uint64_t replacement_frag_hash = fnv1a_hash("hot_pink_fragment_shader");
    
    std::cout << "    📊 Original fragment shader hash: 0x" << std::hex << original_frag_hash << std::dec << std::endl;
    std::cout << "    📊 Replacement fragment shader hash: 0x" << std::hex << replacement_frag_hash << std::dec << std::endl;
    
    overlay.add_shader_replacement(original_frag_hash, replacement_frag_hash, compiledFrag);
    
    std::filesystem::create_directories(std::filesystem::path(output_path).parent_path());
    
    if (overlay.save_to_file(output_path)) {
        std::cout << "✅ Hot pink shader overlay created!" << std::endl;
        std::cout << "   🎯 Replaces fragment shader with solid hot pink output" << std::endl;
        return true;
    }
    
    return false;
}

} // namespace tremor::taffy::tools

// =============================================================================
// WIREFRAME SHADER OVERLAY CREATOR
// =============================================================================

// =============================================================================
// OVERLAY VALIDATION TOOL
// =============================================================================

bool validateOverlayCompatibility(const std::string& overlay_path, const std::string& asset_path) {
    std::cout << "🔍 Validating overlay compatibility..." << std::endl;
    
    // Load overlay
    Taffy::Overlay overlay;
    if (!overlay.load_from_file(overlay_path)) {
        std::cout << "❌ Failed to load overlay" << std::endl;
        return false;
    }
    
    // Load asset
    Taffy::Asset asset;
    if (!asset.load_from_file_safe(asset_path)) {
        std::cout << "❌ Failed to load asset" << std::endl;
        return false;
    }
    
    // Check compatibility
    if (overlay.targets_asset(asset)) {
        std::cout << "✅ Overlay is compatible with asset!" << std::endl;
        return true;
    } else {
        std::cout << "❌ Overlay is NOT compatible with asset!" << std::endl;
        return false;
    }
}

    bool validateSPIRV(const std::vector<uint32_t>& spirv, const std::string& name) {
        std::cout << "🔍 SPIR-V Validation: " << name << std::endl;

        if (spirv.empty()) {
            std::cout << "  ❌ Empty SPIR-V!" << std::endl;
            return false;
        }

        if (spirv.size() < 5) {
            std::cout << "  ❌ SPIR-V too small: " << spirv.size() << " words" << std::endl;
            return false;
        }

        // Check magic number
        if (spirv[0] != 0x07230203) {
            std::cout << "  ❌ Invalid SPIR-V magic: 0x" << std::hex << spirv[0] << std::dec << std::endl;
            std::cout << "     Expected: 0x07230203" << std::endl;
            return false;
        }

        std::cout << "  ✅ Magic: 0x" << std::hex << spirv[0] << std::dec << std::endl;
        std::cout << "  📊 Version: " << spirv[1] << std::endl;
        std::cout << "  📊 Generator: 0x" << std::hex << spirv[2] << std::dec << std::endl;
        std::cout << "  📊 Bound: " << spirv[3] << std::endl;
        std::cout << "  📊 Schema: " << spirv[4] << std::endl;
        std::cout << "  📊 Size: " << spirv.size() << " words (" << spirv.size() * 4 << " bytes)" << std::endl;

        return true;
    }

    void dumpSPIRVBytes(const std::vector<uint32_t>& spirv, const std::string& name, size_t max_words = 8) {
        std::cout << "🔍 SPIR-V Hex Dump: " << name << std::endl;

        size_t words_to_dump = std::min(spirv.size(), max_words);

        for (size_t i = 0; i < words_to_dump; ++i) {
            uint32_t word = spirv[i];
            std::cout << "  [" << i << "] = 0x" << std::hex << std::setfill('0') << std::setw(8) << word << std::dec;

            // Show as bytes
            std::cout << " (bytes: ";
            for (int j = 0; j < 4; ++j) {
                uint8_t byte = (word >> (j * 8)) & 0xFF;
                std::cout << "0x" << std::hex << std::setfill('0') << std::setw(2) << (int)byte;
                if (j < 3) std::cout << " ";
            }
            std::cout << ")" << std::dec << std::endl;
        }

        if (spirv.size() > max_words) {
            std::cout << "  ... (" << (spirv.size() - max_words) << " more words)" << std::endl;
        }
    }

    void dumpRawBytes(const uint8_t* data, size_t size, const std::string& name, size_t max_bytes = 32) {
        std::cout << "🔍 Raw Byte Dump: " << name << std::endl;

        size_t bytes_to_dump = std::min(size, max_bytes);

        for (size_t i = 0; i < bytes_to_dump; ++i) {
            if (i % 16 == 0) {
                std::cout << "  " << std::hex << std::setfill('0') << std::setw(4) << i << ": ";
            }

            std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)data[i] << " ";

            if ((i + 1) % 16 == 0 || i == bytes_to_dump - 1) {
                // Pad to align the ASCII part
                for (size_t j = (i % 16) + 1; j < 16; ++j) {
                    std::cout << "   ";
                }

                std::cout << " |";
                for (size_t j = i - (i % 16); j <= i; ++j) {
                    char c = (data[j] >= 32 && data[j] <= 126) ? data[j] : '.';
                    std::cout << c;
                }
                std::cout << "|" << std::dec << std::endl;
            }
        }

        if (size > max_bytes) {
            std::cout << "  ... (" << (size - max_bytes) << " more bytes)" << std::endl;
        }
    }

    std::vector<uint32_t> TaffyAssetCompiler::compileGLSLToSpirv(const std::string& source,
        shaderc_shader_kind kind,
        const std::string& name) {

        std::cout << "🔨 Compiling " << name << " to SPIR-V..." << std::endl;
        std::cout << "  📝 GLSL source length: " << source.length() << " characters" << std::endl;
        std::cout << "  🎯 Shader kind: " << kind << std::endl;

        auto result = compiler_->CompileGlslToSpv(source, kind, name.c_str(), "main", *options_);

        if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
            std::cerr << "❌ Shader compilation failed: " << result.GetErrorMessage() << std::endl;
            return {};
        }

        std::vector<uint32_t> spirv(result.cbegin(), result.cend());
        std::cout << "✅ Compiled " << name << " (" << spirv.size() * 4 << " bytes)" << std::endl;

        // VALIDATE IMMEDIATELY after compilation
        if (!validateSPIRV(spirv, name + "_fresh_compilation")) {
            std::cerr << "❌ Freshly compiled SPIR-V is invalid!" << std::endl;
            dumpSPIRVBytes(spirv, name + "_invalid_fresh");
            return {};
        }

        std::cout << "  ✅ Fresh compilation validation passed" << std::endl;
        return spirv;
    }

    // =============================================================================
    // GLSL SHADER SOURCES (Human-readable!)
    // =============================================================================


    inline bool HashBasedShaderCreator::createShaderChunkHash(Taffy::Asset& asset,
        const std::vector<uint32_t>& mesh_spirv,
        const std::vector<uint32_t>& frag_spirv) {

        std::cout << "🔧 Creating HASH-BASED shader chunk..." << std::endl;

        using namespace Taffy;

        // Validate input data
        if (mesh_spirv.empty() || frag_spirv.empty()) {
            std::cerr << "❌ Empty SPIR-V data!" << std::endl;
            return false;
        }

        // Register names and get hashes
        uint64_t mesh_name_hash = HashRegistry::register_and_hash("triangle_mesh_shader");
        uint64_t frag_name_hash = HashRegistry::register_and_hash("triangle_fragment_shader");
        uint64_t main_hash = HashRegistry::register_and_hash("main");

        std::cout << "  📋 Registered hashes:" << std::endl;
        std::cout << "    'triangle_mesh_shader' -> 0x" << std::hex << mesh_name_hash << std::dec << std::endl;
        std::cout << "    'triangle_fragment_shader' -> 0x" << std::hex << frag_name_hash << std::dec << std::endl;
        std::cout << "    'main' -> 0x" << std::hex << main_hash << std::dec << std::endl;

        // Calculate sizes
        size_t mesh_spirv_bytes = mesh_spirv.size() * sizeof(uint32_t);
        size_t frag_spirv_bytes = frag_spirv.size() * sizeof(uint32_t);

        size_t total_size = sizeof(ShaderChunk) +
            2 * sizeof(ShaderChunk::Shader) +
            mesh_spirv_bytes +
            frag_spirv_bytes;

        std::vector<uint8_t> shader_data(total_size, 0);
        size_t offset = 0;

        // Write header
        ShaderChunk header{};
        header.shader_count = 2;
        std::memcpy(shader_data.data() + offset, &header, sizeof(header));
        offset += sizeof(header);

        // Write mesh shader info
        ShaderChunk::Shader mesh_info{};
        mesh_info.name_hash = mesh_name_hash;
        mesh_info.entry_point_hash = main_hash;
        mesh_info.stage = ShaderChunk::Shader::ShaderStage::MeshShader;
        mesh_info.spirv_size = static_cast<uint32_t>(mesh_spirv_bytes);
        mesh_info.max_vertices = 3;
        mesh_info.max_primitives = 1;
        mesh_info.workgroup_size[0] = 1;
        mesh_info.workgroup_size[1] = 1;
        mesh_info.workgroup_size[2] = 1;

        std::memcpy(shader_data.data() + offset, &mesh_info, sizeof(mesh_info));
        offset += sizeof(mesh_info);

        // Write fragment shader info
        ShaderChunk::Shader frag_info{};
        frag_info.name_hash = frag_name_hash;
        frag_info.entry_point_hash = main_hash;
        frag_info.stage = ShaderChunk::Shader::ShaderStage::Fragment;
        frag_info.spirv_size = static_cast<uint32_t>(frag_spirv_bytes);

        std::memcpy(shader_data.data() + offset, &frag_info, sizeof(frag_info));
        offset += sizeof(frag_info);

        // Write mesh SPIR-V data
        size_t mesh_spirv_offset = offset;
        std::memcpy(shader_data.data() + offset, mesh_spirv.data(), mesh_spirv_bytes);
        offset += mesh_spirv_bytes;

        // VALIDATE SPIR-V MAGIC - should work perfectly now!
        uint32_t written_magic;
        std::memcpy(&written_magic, shader_data.data() + mesh_spirv_offset, sizeof(uint32_t));
        std::cout << "  🔍 SPIR-V magic: 0x" << std::hex << written_magic << std::dec;

        if (written_magic == 0x07230203) {
            std::cout << " ✅ PERFECT!" << std::endl;
        }
        else {
            std::cout << " ❌ CORRUPTED!" << std::endl;
            return false;
        }

        // Write fragment SPIR-V data
        std::memcpy(shader_data.data() + offset, frag_spirv.data(), frag_spirv_bytes);

        // Add chunk to asset
        asset.add_chunk(ChunkType::SHDR, shader_data, "hash_based_shaders");

        std::cout << "🎉 Hash-based shader chunk created successfully!" << std::endl;
        return true;
    }

    bool HashBasedShaderCreator::validateHashShaderChunk(const Taffy::Asset& asset) {
        using namespace Taffy;

        std::cout << "🔍 Validating hash-based shader chunk..." << std::endl;

        auto shader_data = asset.get_chunk_data(ChunkType::SHDR);
        if (!shader_data) {
            std::cout << "❌ No shader chunk found in asset" << std::endl;
            return false;
        }

        if (shader_data->size() < sizeof(ShaderChunk)) {
            std::cout << "❌ Shader chunk too small: " << shader_data->size()
                << " bytes (need at least " << sizeof(ShaderChunk) << ")" << std::endl;
            return false;
        }

        // Read header
        ShaderChunk header;
        std::memcpy(&header, shader_data->data(), sizeof(header));

        std::cout << "  📊 Shader chunk header:" << std::endl;
        std::cout << "    Shader count: " << header.shader_count << std::endl;
        std::cout << "    Total chunk size: " << shader_data->size() << " bytes" << std::endl;

        if (header.shader_count == 0 || header.shader_count > 100) {
            std::cout << "❌ Invalid shader count: " << header.shader_count << std::endl;
            return false;
        }

        size_t expected_min_size = sizeof(ShaderChunk) +
            header.shader_count * sizeof(ShaderChunk::Shader);
        if (shader_data->size() < expected_min_size) {
            std::cout << "❌ Chunk too small for " << header.shader_count
                << " shaders. Need at least " << expected_min_size << " bytes" << std::endl;
            return false;
        }

        size_t offset = sizeof(header);
        size_t total_spirv_size = 0;

        // Validate each shader
        for (uint32_t i = 0; i < header.shader_count; ++i) {
            if (offset + sizeof(ShaderChunk::Shader) > shader_data->size()) {
                std::cout << "❌ Shader " << i << " info exceeds chunk boundary" << std::endl;
                return false;
            }

            ShaderChunk::Shader shader_info;
            std::memcpy(&shader_info, shader_data->data() + offset, sizeof(shader_info));
            offset += sizeof(shader_info);

            std::cout << "  🔧 Shader " << i << " validation:" << std::endl;

            // Hash-based name validation
            std::cout << "    Name hash: 0x" << std::hex << shader_info.name_hash << std::dec;
            std::string resolved_name = HashRegistry::lookup_string(shader_info.name_hash);
            std::cout << " (\"" << resolved_name << "\")" << std::endl;

            std::cout << "    Entry hash: 0x" << std::hex << shader_info.entry_point_hash << std::dec;
            std::string resolved_entry = HashRegistry::lookup_string(shader_info.entry_point_hash);
            std::cout << " (\"" << resolved_entry << "\")" << std::endl;

            // Validate stage
            std::cout << "    Stage: ";
            switch (shader_info.stage) {
            case ShaderChunk::Shader::ShaderStage::Vertex:
                std::cout << "Vertex"; break;
            case ShaderChunk::Shader::ShaderStage::Fragment:
                std::cout << "Fragment"; break;
            case ShaderChunk::Shader::ShaderStage::Geometry:
                std::cout << "Geometry"; break;
            case ShaderChunk::Shader::ShaderStage::Compute:
                std::cout << "Compute"; break;
            case ShaderChunk::Shader::ShaderStage::MeshShader:
                std::cout << "MeshShader"; break;
            case ShaderChunk::Shader::ShaderStage::TaskShader:
                std::cout << "TaskShader"; break;
            default:
                std::cout << "UNKNOWN(" << static_cast<int>(shader_info.stage) << ")";
                std::cout << " ❌ Invalid stage!" << std::endl;
                return false;
            }
            std::cout << std::endl;

            // Validate SPIR-V size
            std::cout << "    SPIR-V size: " << shader_info.spirv_size << " bytes" << std::endl;

            if (shader_info.spirv_size == 0) {
                std::cout << "    ❌ Zero SPIR-V size!" << std::endl;
                return false;
            }

            if (shader_info.spirv_size > 10 * 1024 * 1024) { // 10MB sanity check
                std::cout << "    ❌ SPIR-V size too large: " << shader_info.spirv_size << std::endl;
                return false;
            }

            if (shader_info.spirv_size % 4 != 0) {
                std::cout << "    ❌ SPIR-V size not 4-byte aligned!" << std::endl;
                return false;
            }

            // Check if SPIR-V data would exceed chunk
            if (offset + shader_info.spirv_size > shader_data->size()) {
                std::cout << "    ❌ SPIR-V data exceeds chunk boundary!" << std::endl;
                std::cout << "       Offset: " << offset << ", Size: " << shader_info.spirv_size
                    << ", Chunk size: " << shader_data->size() << std::endl;
                return false;
            }

            // Validate SPIR-V magic number
            if (shader_info.spirv_size >= 4) {
                uint32_t magic;
                std::memcpy(&magic, shader_data->data() + offset, sizeof(magic));
                std::cout << "    SPIR-V magic: 0x" << std::hex << magic << std::dec;

                if (magic == 0x07230203) {
                    std::cout << " ✅ VALID" << std::endl;
                }
                else {
                    std::cout << " ❌ INVALID! Expected 0x07230203" << std::endl;

                    // Debug: show what we actually got
                    std::cout << "    🐛 First 16 bytes of SPIR-V data:" << std::endl;
                    for (int j = 0; j < 16 && j < shader_info.spirv_size && (offset + j) < shader_data->size(); ++j) {
                        uint8_t byte_val = shader_data->data()[offset + j];
                        std::cout << "      [" << j << "] = 0x" << std::hex << (int)byte_val
                            << " ('" << (char)(byte_val >= 32 && byte_val <= 126 ? byte_val : '.')
                            << "')" << std::dec << std::endl;
                    }
                    return false;
                }
            }

            // Mesh shader specific validation
            if (shader_info.stage == ShaderChunk::Shader::ShaderStage::MeshShader) {
                std::cout << "    Max vertices: " << shader_info.max_vertices << std::endl;
                std::cout << "    Max primitives: " << shader_info.max_primitives << std::endl;
                std::cout << "    Workgroup size: (" << shader_info.workgroup_size[0]
                    << ", " << shader_info.workgroup_size[1]
                    << ", " << shader_info.workgroup_size[2] << ")" << std::endl;

                if (shader_info.max_vertices == 0 || shader_info.max_primitives == 0) {
                    std::cout << "    ⚠️  Warning: Mesh shader with 0 vertices/primitives" << std::endl;
                }
            }

            // Check for known hash values
            if (shader_info.name_hash == ShaderHashes::TRIANGLE_MESH) {
                std::cout << "    ✅ Recognized as triangle mesh shader" << std::endl;
            }
            else if (shader_info.name_hash == ShaderHashes::TRIANGLE_FRAG) {
                std::cout << "    ✅ Recognized as triangle fragment shader" << std::endl;
            }

            offset += shader_info.spirv_size;
            total_spirv_size += shader_info.spirv_size;
            std::cout << "    ✅ Shader " << i << " validation passed" << std::endl;
        }

        // Final validation
        std::cout << "  📊 Summary:" << std::endl;
        std::cout << "    Total shaders: " << header.shader_count << std::endl;
        std::cout << "    Total SPIR-V data: " << total_spirv_size << " bytes" << std::endl;
        std::cout << "    Chunk utilization: " << offset << "/" << shader_data->size()
            << " bytes (" << (offset * 100 / shader_data->size()) << "%)" << std::endl;

        if (offset != shader_data->size()) {
            std::cout << "    ⚠️  Warning: " << (shader_data->size() - offset)
                << " bytes unused at end of chunk" << std::endl;
        }

        std::cout << "✅ Hash-based shader chunk validation PASSED!" << std::endl;
        return true;
    }

    bool TaffyAssetCompiler::createTriangleAssetHashBased(const std::string& output_path) {
        std::cout << "🚀 Creating triangle asset with HASH-BASED names..." << std::endl;

        using namespace Taffy;
        using namespace tremor::taffy::tools;


        // Compile shaders first
        TaffyAssetCompiler compiler;

        const std::string mesh_shader_glsl = R"(
#version 460
#extension GL_EXT_mesh_shader : require

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(triangles, max_vertices = 3, max_primitives = 1) out;

layout(location = 0) out vec4 fragColor[];

const vec3 positions[3] = vec3[](
    vec3( 0.0,  0.5, 0.0),
    vec3(-0.5, -0.5, 0.0),
    vec3( 0.5, -0.5, 0.0)
);

const vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),    // This vertex color can be changed by overlays!
    vec3(0.0, 0.0, 1.0)
);

void main() {
    SetMeshOutputsEXT(3, 1);
    
    for (int i = 0; i < 3; ++i) {
        gl_MeshVerticesEXT[i].gl_Position = vec4(positions[i], 1.0);
        fragColor[i] = vec4(colors[i], 1.0);
    }
    
    gl_PrimitiveTriangleIndicesEXT[0] = uvec3(0, 1, 2);
}
)";

        const std::string fragment_shader_glsl = R"(
#version 460

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = fragColor;
}
)";

        // Pre-register all shader names we'll use
        std::cout << "  📋 Pre-registering shader names..." << std::endl;
        HashRegistry::register_string("triangle_mesh_shader");
        HashRegistry::register_string("triangle_fragment_shader");
        HashRegistry::register_string("main");
        HashRegistry::register_string("wireframe_mesh_shader");
        HashRegistry::register_string("animated_mesh_shader");

        // Show what we registered
        HashRegistry::debug_print_all();

        // Compile shaders
        auto mesh_spirv = compileGLSLToSpirv(mesh_shader_glsl, shaderc_mesh_shader, "triangle_mesh_shader");
        auto frag_spirv = compileGLSLToSpirv(fragment_shader_glsl, shaderc_fragment_shader, "triangle_frag_shader");

        if (mesh_spirv.empty() || frag_spirv.empty()) {
            std::cerr << "❌ Shader compilation failed!" << std::endl;
            return false;
        }

        // Create asset with proper feature flags
        Asset asset;
        asset.set_creator("Hash-Based Tremor Taffy Compiler");
        asset.set_description("Triangle with hash-based shader names - NO BUFFER OVERFLOWS!");
        asset.set_feature_flags(FeatureFlags::QuantizedCoords |
            FeatureFlags::MeshShaders |
            FeatureFlags::EmbeddedShaders |
            FeatureFlags::SPIRVCross); 

        // Use hash-based shader creation
        if (!HashBasedShaderCreator::createShaderChunkHash(asset, mesh_spirv, frag_spirv)) {
            return false;
        }

        // Validate immediately
        if (!HashBasedShaderCreator::validateHashShaderChunk(asset)) {
            std::cerr << "❌ Hash-based shader chunk failed validation!" << std::endl;
            return false;
        }

        // Create other chunks (these don't use names, so they're fine)
        if (!createGeometryChunk(asset)) {
            return false;
        }

        if (!createMaterialChunk(asset)) {
            return false;
        }

        // Save and validate
        std::filesystem::create_directories(std::filesystem::path(output_path).parent_path());
        if (!asset.save_to_file(output_path)) {
            std::cerr << "❌ Failed to save asset!" << std::endl;
            return false;
        }

        // Load back and validate
        Asset test_load;
        if (!test_load.load_from_file_safe(output_path)) {
            std::cerr << "❌ Failed to load back saved asset!" << std::endl;
            return false;
        }

        if (!HashBasedShaderCreator::validateHashShaderChunk(test_load)) {
            std::cerr << "❌ Saved asset failed hash validation!" << std::endl;
            return false;
        }

        std::cout << "🎉 Hash-based asset creation completed successfully!" << std::endl;
        std::cout << "   📁 File: " << output_path << std::endl;
        std::cout << "   📦 Size: " << std::filesystem::file_size(output_path) << " bytes" << std::endl;
        std::cout << "   🔥 NO BUFFER OVERFLOWS EVER AGAIN!" << std::endl;

        return true;
    }
    // =============================================================================
    // GLSL-TO-TAFFY ASSET COMPILER
    // =============================================================================

    TaffyAssetCompiler::TaffyAssetCompiler() {
            compiler_ = std::make_unique<shaderc::Compiler>();
            options_ = std::make_unique<shaderc::CompileOptions>();

            // Configure compilation options
            options_->SetOptimizationLevel(shaderc_optimization_level_performance);
            options_->SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
            options_->SetTargetSpirv(shaderc_spirv_version_1_6);

            std::cout << "🔧 Taffy Asset Compiler initialized with shaderc" << std::endl;
        }

        /**
         * Compile GLSL to SPIR-V
         */

        /**
         * Create complete Taffy asset with mesh shader triangle
         */

        /**
         * Create shader chunk with compiled SPIR-V
         */

        /**
         * Create geometry chunk (for overlay targeting - even though mesh shader generates geometry)
         */
        bool TaffyAssetCompiler::createGeometryChunk(Taffy::Asset& asset) {
            using namespace Taffy;

            std::cout << "  📐 Creating geometry chunk..." << std::endl;

            struct OverlayVertex {
                Vec3Q position;
                float normal[3];
                float uv[2];
                float color[4];
            };

            std::vector<OverlayVertex> vertices = {
                {Vec3Q{0, 50, 0}, {0,0,1}, {0.5f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},  // Yellow
                {Vec3Q{-50, -50, 0}, {0,0,1}, {0.0f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},  // Magenta
                {Vec3Q{50, -50, 0}, {0,0,1}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}  // Red
            };

            std::vector<uint32_t> indices = { 0, 1, 2 };

            GeometryChunk geom_header{};
            std::memset(&geom_header, 0, sizeof(geom_header));
            geom_header.vertex_count = static_cast<uint32_t>(vertices.size());
            geom_header.index_count = static_cast<uint32_t>(indices.size());
            geom_header.vertex_stride = sizeof(OverlayVertex);
            geom_header.vertex_format = VertexFormat::Position3D | VertexFormat::Normal |
                VertexFormat::TexCoord0 | VertexFormat::Color;
            geom_header.bounds_min = Vec3Q{ -50, -50, 0 };
            geom_header.bounds_max = Vec3Q{ 50, 50, 0 };
            geom_header.lod_distance = 1000.0f;
            geom_header.lod_level = 0;

            size_t vertex_data_size = vertices.size() * sizeof(OverlayVertex);
            size_t index_data_size = indices.size() * sizeof(uint32_t);
            size_t total_size = sizeof(GeometryChunk) + vertex_data_size + index_data_size;

            std::vector<uint8_t> geom_data(total_size);
            size_t offset = 0;

            std::memcpy(geom_data.data() + offset, &geom_header, sizeof(geom_header));
            offset += sizeof(geom_header);

            std::memcpy(geom_data.data() + offset, vertices.data(), vertex_data_size);
            offset += vertex_data_size;

            std::memcpy(geom_data.data() + offset, indices.data(), index_data_size);

            asset.add_chunk(ChunkType::GEOM, geom_data, "triangle_geometry");

            std::cout << "    ✅ " << vertices.size() << " vertices, " << indices.size() / 3 << " triangle(s)" << std::endl;
            std::cout << "    🎯 Vertex 1 (green) ready for overlay modification" << std::endl;

            return true;
        }
        /**
         * Create basic material chunk
         */
        bool TaffyAssetCompiler::createMaterialChunk(Taffy::Asset& asset) {
            using namespace Taffy;

            std::cout << "  🎨 Creating material chunk..." << std::endl;

            MaterialChunk mat_header{};
            std::memset(&mat_header, 0, sizeof(mat_header));
            mat_header.material_count = 1;

            MaterialChunk::Material material{};
            std::memset(&material, 0, sizeof(material));

            std::strncpy(material.name, "triangle_material", sizeof(material.name) - 1);
            material.name[sizeof(material.name) - 1] = '\0';

            material.albedo[0] = 1.0f; material.albedo[1] = 1.0f;
            material.albedo[2] = 1.0f; material.albedo[3] = 1.0f;
            material.metallic = 0.0f;
            material.roughness = 0.8f;
            material.normal_intensity = 1.0f;
            material.albedo_texture = UINT32_MAX;
            material.normal_texture = UINT32_MAX;
            material.metallic_roughness_texture = UINT32_MAX;
            material.emission_texture = UINT32_MAX;
            material.flags = MaterialFlags::DoubleSided;

            std::vector<uint8_t> mat_data(sizeof(MaterialChunk) + sizeof(MaterialChunk::Material));

            std::memcpy(mat_data.data(), &mat_header, sizeof(mat_header));
            std::memcpy(mat_data.data() + sizeof(mat_header), &material, sizeof(material));

            asset.add_chunk(ChunkType::MTRL, mat_data, "triangle_material");

            std::cout << "    ✅ Basic PBR material created" << std::endl;
            std::cout << "    🎨 Name: " << material.name << std::endl;

            return true;
        }


    // =============================================================================
    // SHADER VARIANT GENERATOR (For different overlay effects)
    // =============================================================================

        /**
         * Generate animated mesh shader variant
         */

    namespace Taffy {

        // =============================================================================
        // EXTENDED GEOMETRY CHUNK FOR MESH SHADER SUPPORT
        // =============================================================================



        // =============================================================================
        // MESH SHADER GENERATOR - Creates shaders based on vertex format
        // =============================================================================

        class MeshShaderGenerator {
        public:
            struct ShaderConfig {
                uint32_t max_vertices = 3;
                uint32_t max_primitives = 1;
                uint32_t workgroup_x = 1;
                uint32_t workgroup_y = 1;
                uint32_t workgroup_z = 1;
                GeometryChunk::PrimitiveType primitive_type = GeometryChunk::Triangles;
                std::vector<VertexAttribute> attributes;
                uint32_t vertex_stride = 0;  // In BYTES
                uint32_t vertex_count = 0;
                bool has_indices = false;
                uint32_t index_count = 0;
                bool prefersCompactVertexOutput = true;  // Output vertex data directly instead of reading from buffer
                bool supportsOverlays = true;  // Enable overlay system support
            };

            static std::string generateMeshShader(const ShaderConfig& config) {
                std::stringstream shader;

                // Header
                shader << "#version 460\n";
                shader << "#extension GL_EXT_mesh_shader : require\n\n";

                // Local work size
                shader << "layout(local_size_x = " << config.workgroup_x
                    << ", local_size_y = " << config.workgroup_y
                    << ", local_size_z = " << config.workgroup_z << ") in;\n";

                // Output topology
                const char* topology = (config.primitive_type == GeometryChunk::Triangles) ? "triangles" :
                    (config.primitive_type == GeometryChunk::Lines) ? "lines" : "points";

                shader << "layout(" << topology
                    << ", max_vertices = " << config.max_vertices
                    << ", max_primitives = " << config.max_primitives << ") out;\n\n";

                // Storage buffer for vertex data - using float array for easier access
                shader << "layout(set = 0, binding = 0) readonly buffer VertexBuffer {\n";
                shader << "    uint vertices[];\n";  // Using uint array to read raw data
                shader << "} vertexBuffer;\n\n";

                // Push constants
                shader << "layout(push_constant) uniform PushConstants {\n";
                shader << "    mat4 mvp;\n";  // Model-View-Projection matrix
                shader << "    uint vertex_count;\n";
                shader << "    uint primitive_count;\n";
                shader << "    uint vertex_stride_floats;\n";  // Stride in FLOATS, not bytes
                shader << "    uint index_offset_bytes;\n";  // Offset to index data in buffer
                shader << "    uint overlay_flags;\n";  // Bit flags for active overlays
                shader << "    uint overlay_data_offset;\n";  // Offset to overlay data in buffer
                shader << "} pc;\n\n";

                // Output attributes
                generateOutputDeclarations(shader, config.attributes);

                // Vec3Q helper function
                shader << "// Helper to read Vec3Q (3 x int64) and convert to vec3\n";
                shader << "vec3 readVec3Q(uint vertexIndex, uint offsetBytes) {\n";
                shader << "    // Calculate offset in uint units (4 bytes each)\n";
                shader << "    uint baseOffsetUints = (vertexIndex * pc.vertex_stride_floats * 4 + offsetBytes) / 4;\n";
                shader << "    \n";
                shader << "    // Read Vec3Q as pairs of uint32 (since GLSL doesn't have int64)\n";
                shader << "    // Each int64 is stored as two consecutive uint32s (little-endian)\n";
                shader << "    uint x_lo = vertexBuffer.vertices[baseOffsetUints + 0];\n";
                shader << "    uint x_hi = vertexBuffer.vertices[baseOffsetUints + 1];\n";
                shader << "    uint y_lo = vertexBuffer.vertices[baseOffsetUints + 2];\n";
                shader << "    uint y_hi = vertexBuffer.vertices[baseOffsetUints + 3];\n";
                shader << "    uint z_lo = vertexBuffer.vertices[baseOffsetUints + 4];\n";
                shader << "    uint z_hi = vertexBuffer.vertices[baseOffsetUints + 5];\n";
                shader << "    \n";
                shader << "    // Reconstruct int64 values and convert to float\n";
                shader << "    // Note: This assumes the values fit in float range\n";
                shader << "    double x = -1.0 + double((uint(x_hi)-2147483647)) + double((uint(x_lo)-2147483647))/4294967296.0  ;\n";
                shader << "    double y = -1.0 + double((uint(y_hi)-2147483647)) + double((uint(y_lo)-2147483647))/4294967296.0  ;\n";
                shader << "    double z = -1.0 + double((uint(z_hi)-2147483647)) + double((uint(z_lo)-2147483647))/4294967296.0  ;\n";
                shader << "    \n";
                shader << "    // Convert from quantized units to world units\n";
                shader << "    vec3 result = vec3(x/1.28, y/1.28, z/1.28);\n";
                shader << "    \n";

                shader << "    \n";
                shader << "    // Debug: Store vertex positions for debugging\n";
                shader << "    // We'll use the color output to encode position information\n";
                shader << "    if (vertexIndex < 3u) {\n";
                shader << "        // Encode the world-space position in the color for the first 3 vertices\n";
                shader << "        // This will help us debug where the vertices actually are\n";
                shader << "    }\n";
                shader << "    \n";
                shader << "    return result;\n";
                shader << "}\n\n";
                
                // Index reader function
                shader << "// Helper to read indices from buffer\n";
                shader << "uint readIndex(uint indexNum) {\n";
                shader << "    uint byte_offset = pc.index_offset_bytes + indexNum * 4u;\n";
                shader << "    uint word_offset = byte_offset / 4u;\n";
                shader << "    return vertexBuffer.vertices[word_offset];\n";
                shader << "}\n\n";

                // Other attribute accessors
                generateAttributeAccessors(shader, config);

                // Main function
                shader << "void main() {\n";
                shader << "    // Only let the first thread in the workgroup do the work\n";
                shader << "    if (gl_LocalInvocationIndex != 0) return;\n\n";
                shader << "    uint vertex_count = min(pc.vertex_count, " << config.max_vertices << "u);\n";
                shader << "    uint primitive_count = min(pc.primitive_count, " << config.max_primitives << "u);\n\n";

                shader << "    SetMeshOutputsEXT(vertex_count, primitive_count);\n\n";
                
                shader << "    // Debug: Check center of mass of vertices\n";
                shader << "    vec3 centerOfMass = vec3(0.0);\n\n";

                if (config.prefersCompactVertexOutput) {
                    // Read vertices from buffer instead of hardcoding
                    shader << "    // Read vertices from buffer and transform them\n";
                    shader << "    for (uint i = 0; i < vertex_count; ++i) {\n";
                    generateVertexProcessing(shader, config.attributes);
                    shader << "    }\n\n";
                    
                    shader << "    \n";
                    shader << "    // Debug: Override colors to show cube structure\n";
                    shader << "    // Make vertex 7 (should be back-top-right) bright white\n";
                    shader << "    color[7] = vec4(1.0, 1.0, 1.0, 1.0);\n";
                    shader << "    // Make vertex 2 (should be front-top-right) bright yellow\n";
                    shader << "    color[2] = vec4(1.0, 1.0, 0.0, 1.0);\n";
                    shader << "    // Make vertex 16 (should be right face, bottom-front) bright magenta\n";
                    shader << "    color[16] = vec4(1.0, 0.0, 1.0, 1.0);\n\n";
                    
                    // Generate primitives based on indices
                    generatePrimitiveGeneration(shader, config);
                } else {
                    // Generate vertices from buffer
                    shader << "    for (uint i = 0; i < vertex_count; ++i) {\n";
                    generateVertexProcessing(shader, config.attributes);
                    shader << "    }\n\n";
                    
                    // Generate primitives
                    generatePrimitiveGeneration(shader, config);
                }

                shader << "}\n";

                return shader.str();
            }

            static std::string generateFragmentShader(const ShaderConfig& config) {
                std::stringstream shader;

                shader << "#version 460\n";
                shader << "#extension GL_EXT_fragment_shader_barycentric : enable\n\n";

                // Input attributes (skip position as it's handled by gl_Position)
                for (size_t i = 0; i < config.attributes.size(); ++i) {
                    const auto& attr = config.attributes[i];
                    if (strcmp(attr.name, "position") == 0) continue;

                    shader << "layout(location = " << attr.location << ") in ";
                    shader << getGLSLType(attr.type) << " " << attr.name << ";\n";
                }
                
                // Manual interpolation inputs
                shader << "layout(location = 10) in flat uint primitiveID;\n";
                shader << "layout(location = 11) in vec3 barycentricCoords;\n";
                
                // Storage buffer for vertex data (same as mesh shader)
                shader << "\nlayout(set = 0, binding = 0) readonly buffer VertexBuffer {\n";
                shader << "    uint vertices[];\n";
                shader << "} vertexBuffer;\n\n";
                
                // Push constants (to know vertex stride)
                shader << "layout(push_constant) uniform PushConstants {\n";
                shader << "    mat4 mvp;\n";
                shader << "    uint vertex_count;\n";
                shader << "    uint primitive_count;\n";
                shader << "    uint vertex_stride_floats;\n";
                shader << "    uint index_offset_bytes;\n";
                shader << "    uint overlay_flags;\n";
                shader << "    uint overlay_data_offset;\n";
                shader << "} pc;\n\n";

                shader << "layout(location = 0) out vec4 fragColor;\n\n";
                
                // Add index reader function if indices are used
                if (config.has_indices) {
                    shader << "// Helper to read indices from buffer\n";
                    shader << "uint readIndex(uint indexNum) {\n";
                    shader << "    uint byte_offset = pc.index_offset_bytes + indexNum * 4u;\n";
                    shader << "    uint word_offset = byte_offset / 4u;\n";
                    shader << "    return vertexBuffer.vertices[word_offset];\n";
                    shader << "}\n\n";
                }
                
                // Add function to read color from storage buffer
                shader << "// Function to read color from vertex in storage buffer\n";
                shader << "vec4 readVertexColor(uint vertexIndex) {\n";
                shader << "    // Color is at byte offset 36 in the vertex structure\n";
                shader << "    uint colorOffsetUints = 36u / 4u; // Convert byte offset to uint offset (9 uints)\n";
                shader << "    uint offset = vertexIndex * pc.vertex_stride_floats + colorOffsetUints;\n";
                shader << "    \n";
                shader << "    return vec4(\n";
                shader << "        uintBitsToFloat(vertexBuffer.vertices[offset + 0u]),\n";
                shader << "        uintBitsToFloat(vertexBuffer.vertices[offset + 1u]),\n";
                shader << "        uintBitsToFloat(vertexBuffer.vertices[offset + 2u]),\n";
                shader << "        uintBitsToFloat(vertexBuffer.vertices[offset + 3u])\n";
                shader << "    );\n";
                shader << "}\n\n";

                shader << "void main() {\n";

                // Use color if available
                bool hasColor = false;
                for (const auto& attr : config.attributes) {
                    if (strcmp(attr.name, "color") == 0) {
                        hasColor = true;
                        if (config.prefersCompactVertexOutput) {
                            // Simple passthrough when using compact output
                            shader << "    fragColor = color;\n";
                        } else if (attr.type == VertexAttribute::Float3) {
                            shader << "    fragColor = vec4(" << attr.name << ", 1.0);\n";
                        }
                        else {
                            // Manual color interpolation using primitive ID and barycentric coordinates
                            shader << "    // Manual interpolation for mesh shader\n";
                            shader << "    uint primitiveID = uint(gl_PrimitiveID);\n";
                            shader << "    \n";
                            shader << "    // Calculate vertex indices for this primitive\n";
                            if (config.has_indices) {
                                shader << "    uint v0 = readIndex(primitiveID * 3u);\n";
                                shader << "    uint v1 = readIndex(primitiveID * 3u + 1u);\n";
                                shader << "    uint v2 = readIndex(primitiveID * 3u + 2u);\n";
                            } else {
                                shader << "    uint v0 = primitiveID * 3u;\n";
                                shader << "    uint v1 = primitiveID * 3u + 1u;\n";
                                shader << "    uint v2 = primitiveID * 3u + 2u;\n";
                            }
                            shader << "    \n";
                            shader << "    // Read vertex colors\n";
                            shader << "    vec4 color0 = readVertexColor(v0);\n";
                            shader << "    vec4 color1 = readVertexColor(v1);\n";
                            shader << "    vec4 color2 = readVertexColor(v2);\n";
                            shader << "    \n";
                            shader << "    // Use hardware barycentric coordinates if available\n";
                            shader << "    vec3 bary;\n";
                            shader << "    if (gl_BaryCoordEXT.x >= 0.0) {\n";
                            shader << "        // Hardware barycentric coordinates are available\n";
                            shader << "        bary = vec3(gl_BaryCoordEXT.x, gl_BaryCoordEXT.y, 1.0 - gl_BaryCoordEXT.x - gl_BaryCoordEXT.y);\n";
                            shader << "    } else {\n";
                            shader << "        // Fallback - use center of triangle\n";
                            shader << "        bary = vec3(0.333, 0.333, 0.334);\n";
                            shader << "    }\n";
                            shader << "    \n";
                            shader << "    // Interpolate colors using barycentric coordinates\n";
                            shader << "    fragColor = vec4(\n";
                            shader << "        color0.rgb * bary.x + color1.rgb * bary.y + color2.rgb * bary.z,\n";
                            shader << "        1.0\n";
                            shader << "    );\n";
                        }
                        hasColor = true;
                        break;
                    }
                }

                if (!hasColor) {
                    shader << "    fragColor = vec4(1.0, 1.0, 1.0, 1.0);\n";
                }

                shader << "}\n";

                return shader.str();
            }

        private:
            static void generateOutputDeclarations(std::stringstream& shader,
                const std::vector<VertexAttribute>& attributes) {
                for (const auto& attr : attributes) {
                    if (strcmp(attr.name, "position") == 0) continue; // gl_Position is built-in

                    shader << "layout(location = " << attr.location << ") out "
                        << getGLSLType(attr.type) << " " << attr.name << "[];\n";
                }
                
                // Add outputs for manual interpolation
                shader << "// Data for manual interpolation in fragment shader\n";
                shader << "layout(location = 10) out flat uint primitiveID[];\n";
                shader << "layout(location = 11) out vec3 barycentricCoords[];\n";
                shader << "\n";
            }

            static void generateAttributeAccessors(std::stringstream& shader, const ShaderConfig& config) {
                shader << "// Attribute accessor functions\n";

                for (const auto& attr : config.attributes) {
                    // Skip Vec3Q as it has its own special function
                    if (attr.type == VertexAttribute::Vec3Q) continue;

                    shader << getGLSLType(attr.type) << " read_" << attr.name
                        << "(uint vertexIndex) {\n";

                    // Calculate offset in uint32 units (since buffer is uint array)
                    uint32_t offsetUints = attr.offset / sizeof(uint32_t);
                    shader << "    uint offset = vertexIndex * pc.vertex_stride_floats + "
                        << offsetUints << "u;\n";

                    switch (attr.type) {
                    case VertexAttribute::Float:
                        shader << "    return uintBitsToFloat(vertexBuffer.vertices[offset]);\n";
                        break;

                    case VertexAttribute::Float2:
                        shader << "    return vec2(\n";
                        shader << "        uintBitsToFloat(vertexBuffer.vertices[offset]),\n";
                        shader << "        uintBitsToFloat(vertexBuffer.vertices[offset + 1u])\n";
                        shader << "    );\n";
                        break;

                    case VertexAttribute::Float3:
                        shader << "    return vec3(\n";
                        shader << "        uintBitsToFloat(vertexBuffer.vertices[offset]),\n";
                        shader << "        uintBitsToFloat(vertexBuffer.vertices[offset + 1u]),\n";
                        shader << "        uintBitsToFloat(vertexBuffer.vertices[offset + 2u])\n";
                        shader << "    );\n";
                        break;

                    case VertexAttribute::Float4:
                        shader << "    return vec4(\n";
                        shader << "        uintBitsToFloat(vertexBuffer.vertices[offset]),\n";
                        shader << "        uintBitsToFloat(vertexBuffer.vertices[offset + 1u]),\n";
                        shader << "        uintBitsToFloat(vertexBuffer.vertices[offset + 2u]),\n";
                        shader << "        uintBitsToFloat(vertexBuffer.vertices[offset + 3u])\n";
                        shader << "    );\n";
                        break;

                    default:
                        shader << "    return " << getGLSLDefaultValue(attr.type) << ";\n";
                        break;
                    }

                    shader << "}\n\n";
                }
            }

            static void generateVertexProcessing(std::stringstream& shader,
                const std::vector<VertexAttribute>& attributes) {
                for (const auto& attr : attributes) {
                    if (strcmp(attr.name, "position") == 0) {
                        if (attr.type == VertexAttribute::Vec3Q) {
                            // Use special Vec3Q reader - pass offset in bytes
                            shader << "        vec3 position = readVec3Q(i, " << attr.offset << "u);\n";
                        }
                        else {
                            shader << "        vec3 position = read_" << attr.name << "(i);\n";
                        }
                        shader << "        // Debug: Color vertices based on their Y position\n";
                        shader << "        // Vertices with Y > 0.01 should be colored differently\n";
                        shader << "        if (position.y > 0.01) {\n";
                        shader << "            // This vertex has elevated Y position - make it cyan\n";
                        shader << "            color[i] = vec4(0.0, 1.0, 1.0, 1.0);\n";
                        shader << "        }\n";
                        shader << "        // Store the untransformed position for debugging\n";
                        shader << "        centerOfMass += position;\n";
                        shader << "        gl_MeshVerticesEXT[i].gl_Position = pc.mvp * vec4(position, 1.0);\n\n";
                    }
                    else {
                        shader << "        " << attr.name << "[i] = read_" << attr.name << "(i);\n";
                    }
                }
                
                // Set primitive ID (same for all vertices of a primitive)
                shader << "        primitiveID[i] = 0u; // All vertices belong to primitive 0\n";
                
                // Set barycentric coordinates for each vertex
                shader << "        // Set barycentric coordinates for manual interpolation\n";
                shader << "        if (i == 0u) barycentricCoords[i] = vec3(1.0, 0.0, 0.0);\n";
                shader << "        else if (i == 1u) barycentricCoords[i] = vec3(0.0, 1.0, 0.0);\n";
                shader << "        else if (i == 2u) barycentricCoords[i] = vec3(0.0, 0.0, 1.0);\n";
            }

            static void generatePrimitiveGeneration(std::stringstream& shader, const ShaderConfig& config) {
                shader << "    // Generate primitives\n";

                switch (config.primitive_type) {
                case GeometryChunk::Triangles:
                    shader << "    for (uint i = 0; i < primitive_count; ++i) {\n";
                    if (config.has_indices) {
                        shader << "        uint idx0 = readIndex(i * 3u);\n";
                        shader << "        uint idx1 = readIndex(i * 3u + 1u);\n";
                        shader << "        uint idx2 = readIndex(i * 3u + 2u);\n";
                        shader << "        gl_PrimitiveTriangleIndicesEXT[i] = uvec3(idx0, idx1, idx2);\n";
                    } else {
                        shader << "        gl_PrimitiveTriangleIndicesEXT[i] = uvec3(i*3u, i*3u+1u, i*3u+2u);\n";
                    }
                    shader << "    }\n";
                    break;

                case GeometryChunk::Lines:
                    shader << "    for (uint i = 0; i < primitive_count; ++i) {\n";
                    shader << "        gl_PrimitiveLineIndicesEXT[i] = uvec2(i*2u, i*2u+1u);\n";
                    shader << "    }\n";
                    break;

                case GeometryChunk::Points:
                    shader << "    for (uint i = 0; i < primitive_count; ++i) {\n";
                    shader << "        gl_PrimitivePointIndicesEXT[i] = i;\n";
                    shader << "    }\n";
                    break;
                }
            }

            static const char* getGLSLType(VertexAttribute::Type type) {
                switch (type) {
                case VertexAttribute::Float: return "float";
                case VertexAttribute::Float2: return "vec2";
                case VertexAttribute::Float3: return "vec3";
                case VertexAttribute::Float4: return "vec4";
                case VertexAttribute::Vec3Q: return "vec3"; // Output type after conversion
                default: return "float";
                }
            }

            static const char* getGLSLDefaultValue(VertexAttribute::Type type) {
                switch (type) {
                case VertexAttribute::Float: return "0.0";
                case VertexAttribute::Float2: return "vec2(0.0)";
                case VertexAttribute::Float3: return "vec3(0.0)";
                case VertexAttribute::Float4: return "vec4(0.0, 0.0, 0.0, 1.0)";
                case VertexAttribute::Vec3Q: return "vec3(0.0)";
                default: return "0.0";
                }
            }
        };

        // =============================================================================
        // DATA-DRIVEN ASSET COMPILER
        // =============================================================================

            // Create a data-driven mesh shader asset
        bool DataDrivenAssetCompiler::createDataDrivenTriangle(const std::string& output_path) {
            std::cout << "🚀 Creating data-driven mesh shader cube with Vec3Q support..." << std::endl;

            Asset asset;
            asset.set_creator("Vec3Q Data-Driven Taffy Compiler");
            asset.set_description("Cube with Vec3Q positions and data-driven mesh shader");
            asset.set_feature_flags(FeatureFlags::QuantizedCoords |
                FeatureFlags::MeshShaders |
                FeatureFlags::EmbeddedShaders |
                FeatureFlags::HashBasedNames);

#pragma pack(push, 1)
 
            // Create vertex data with Vec3Q positions (96-byte layout to match Tremor)
            struct Vertex {
                Vec3Q position;     // 24 bytes
                float normal[3];    // 12 bytes
                float color[4];     // 16 bytes
                float uv[2];        // 8 bytes
                float tangent[4];   // 8 bytes
            }; // Total: 76 bytes (19 floats)
            
#pragma pack(pop)

            std::vector<Vertex> vertices = {
                // Front face - Red (4 vertices) - Making cube 10cm (0.1m) instead of 50cm
                {Vec3Q{-1280000, -1280000,  1280000}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
                {Vec3Q{ 1280000, -1280000,  1280000}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
                {Vec3Q{ 1280000,  1280000,  1280000}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
                {Vec3Q{-1280000,  1280000,  1280000}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
                
                // Back face - Green (4 vertices)
                {Vec3Q{ 1280000, -1280000, -1280000}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}},
                {Vec3Q{-1280000, -1280000, -1280000}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}},
                {Vec3Q{-1280000,  1280000, -1280000}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}},
                {Vec3Q{ 1280000,  1280000, -1280000}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}},
                
                // Top face - Blue (4 vertices)
                {Vec3Q{-1280000,  1280000,  1280000}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
                {Vec3Q{ 1280000,  1280000,  1280000}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
                {Vec3Q{ 1280000,  1280000, -1280000}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
                {Vec3Q{-1280000,  1280000, -1280000}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
                
                // Bottom face - Yellow (4 vertices)
                {Vec3Q{-1280000, -1280000, -1280000}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
                {Vec3Q{ 1280000, -1280000, -1280000}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
                {Vec3Q{ 1280000, -1280000,  1280000}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
                {Vec3Q{-1280000, -1280000,  1280000}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
                
                // Right face - Magenta (4 vertices)
                {Vec3Q{ 1280000, -1280000,  1280000}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
                {Vec3Q{ 1280000, -1280000, -1280000}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
                {Vec3Q{ 1280000,  1280000, -1280000}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
                {Vec3Q{ 1280000,  1280000,  1280000}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
                
                // Left face - Cyan (4 vertices)
                {Vec3Q{-1280000, -1280000, -1280000}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f, 1.0f}},
                {Vec3Q{-1280000, -1280000,  1280000}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f, 1.0f}},
                {Vec3Q{-1280000,  1280000,  1280000}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f, 1.0f}},
                {Vec3Q{-1280000,  1280000, -1280000}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f, 1.0f}}
            };

            // Create geometry chunk with mesh shader configuration  
            GeometryChunk geom_header{};
            geom_header.vertex_count = static_cast<uint32_t>(vertices.size());
            // Add indices for cube (6 faces * 2 triangles * 3 vertices = 36 indices)
            std::vector<uint32_t> indices = {
                // Front face
                0, 1, 2,    0, 2, 3,
                // Back face
                4, 5, 6,    4, 6, 7,
                // Top face
                8, 9, 10,   8, 10, 11,
                // Bottom face
                12, 13, 14, 12, 14, 15,
                // Right face
                16, 17, 18, 16, 18, 19,
                // Left face
                20, 21, 22, 20, 22, 23
            };
            geom_header.index_count = static_cast<uint32_t>(indices.size());
            geom_header.vertex_stride = sizeof(Vertex);
            std::cout << "DEBUG: Vertex struct size is " << sizeof(Vertex) << " bytes" << std::endl;
            geom_header.vertex_format = VertexFormat::Position3D | VertexFormat::Normal |
                VertexFormat::Color | VertexFormat::TexCoord0 | VertexFormat::Tangent;
            geom_header.bounds_min = Vec3Q{ -0, -0, -0 };
            geom_header.bounds_max = Vec3Q{ 0, 0, 0 };
            geom_header.lod_distance = 1000.0f;
            geom_header.lod_level = 0;

            // Mesh shader configuration
            geom_header.render_mode = GeometryChunk::MeshShader;
            geom_header.ms_max_vertices = 24;
            geom_header.ms_max_primitives = 12;
            geom_header.ms_workgroup_size[0] = 1;
            geom_header.ms_workgroup_size[1] = 1;
            geom_header.ms_workgroup_size[2] = 1;
            geom_header.ms_primitive_type = GeometryChunk::Triangles;
            geom_header.ms_flags = 0;

            // Create geometry data buffer including indices
            size_t vertex_data_size = vertices.size() * sizeof(Vertex);
            size_t index_data_size = indices.size() * sizeof(uint32_t);
            size_t total_size = sizeof(GeometryChunk) + vertex_data_size + index_data_size;

            std::vector<uint8_t> geom_data(total_size);
            size_t offset = 0;

            // Copy header
            std::memcpy(geom_data.data() + offset, &geom_header, sizeof(geom_header));
            offset += sizeof(geom_header);

            // Copy vertex data
            std::memcpy(geom_data.data() + offset, vertices.data(), vertex_data_size);
            offset += vertex_data_size;
            
            // Copy index data
            std::memcpy(geom_data.data() + offset, indices.data(), index_data_size);

            asset.add_chunk(ChunkType::GEOM, geom_data, "vec3q_cube_geometry");

            // Generate shaders based on the geometry
            MeshShaderGenerator::ShaderConfig config;
            config.max_vertices = geom_header.ms_max_vertices;
            config.max_primitives = geom_header.ms_max_primitives;
            config.workgroup_x = geom_header.ms_workgroup_size[0];
            config.workgroup_y = geom_header.ms_workgroup_size[1];
            config.workgroup_z = geom_header.ms_workgroup_size[2];
            config.primitive_type = static_cast<GeometryChunk::PrimitiveType>(geom_header.ms_primitive_type);
            config.vertex_stride = geom_header.vertex_stride;
            config.vertex_count = geom_header.vertex_count;
            config.has_indices = (geom_header.index_count > 0);
            config.index_count = geom_header.index_count;
            config.prefersCompactVertexOutput = true; // Use compact output for simplicity
            config.supportsOverlays = true; // Enable overlay support

            // Define vertex attributes with correct offsets matching Vertex structure
            config.attributes = {
                {VertexAttribute::Vec3Q, 0, 0, "position"},   // offset 0 bytes
                {VertexAttribute::Float3, 24, 1, "normal"},   // offset 24 bytes
                {VertexAttribute::Float4, 36, 2, "color"},    // offset 36 bytes
                {VertexAttribute::Float2, 52, 3, "uv"},       // offset 52 bytes
                {VertexAttribute::Float4, 60, 4, "tangent"}   // offset 60 bytes
            };

            // Generate shaders
            std::string mesh_shader_glsl = MeshShaderGenerator::generateMeshShader(config);
            std::string frag_shader_glsl = MeshShaderGenerator::generateFragmentShader(config);

            std::cout << "📝 Generated mesh shader with Vec3Q support" << std::endl;

            // Log key info
            std::cout << "📊 Vertex layout:" << std::endl;
            std::cout << "   Position (Vec3Q): offset 0, size 24 bytes" << std::endl;
            std::cout << "   Normal (vec3): offset 24, size 12 bytes" << std::endl;
            std::cout << "   Color (vec4): offset 36, size 16 bytes" << std::endl;
            std::cout << "   UV (vec2): offset 52, size 8 bytes" << std::endl;
            std::cout << "   Tangent (vec4): offset 60, size 16 bytes (xyz=tangent, w=handedness)" << std::endl;
            std::cout << "   Total vertex size: " << sizeof(Vertex) << " bytes ("
                << sizeof(Vertex) / sizeof(float) << " floats)" << std::endl;

            // Compile shaders
            tremor::taffy::tools::TaffyAssetCompiler compiler;
            auto mesh_spirv = compiler.compileGLSLToSpirv(mesh_shader_glsl, shaderc_mesh_shader, "vec3q_mesh_shader");
            auto frag_spirv = compiler.compileGLSLToSpirv(frag_shader_glsl, shaderc_fragment_shader, "vec3q_fragment_shader");

            if (mesh_spirv.empty() || frag_spirv.empty()) {
                std::cerr << "❌ Shader compilation failed!" << std::endl;
                return false;
            }

            // Create shader chunk
            if (!createDataDrivenShaderChunk(asset, mesh_spirv, frag_spirv)) {
                return false;
            }

            // Create material chunk
            if (!createBasicMaterialChunk(asset)) {
                return false;
            }

            // Save asset
            std::filesystem::create_directories(std::filesystem::path(output_path).parent_path());
            if (!asset.save_to_file(output_path)) {
                std::cerr << "❌ Failed to save asset!" << std::endl;
                return false;
            }

            std::cout << "✅ Vec3Q data-driven mesh shader cube created!" << std::endl;
            std::cout << "   📁 File: " << output_path << std::endl;
            std::cout << "   📊 Vertices: " << vertices.size() << " (cube with 6 faces)" << std::endl;
            std::cout << "   📊 Triangles: " << indices.size() / 3 << std::endl;
            std::cout << "   🎯 Vertex stride: " << sizeof(Vertex) << " bytes" << std::endl;
            std::cout << "   🔧 Push constant stride: " << sizeof(Vertex) / sizeof(float) << " floats" << std::endl;

            return true;
        }
            bool DataDrivenAssetCompiler::createDataDrivenShaderChunk(Asset& asset,
                const std::vector<uint32_t>& mesh_spirv,
                const std::vector<uint32_t>& frag_spirv) {
                using namespace Taffy;

                // Register shader names
                uint64_t mesh_name_hash = HashRegistry::register_and_hash("data_driven_mesh_shader");
                uint64_t frag_name_hash = HashRegistry::register_and_hash("data_driven_fragment_shader");
                uint64_t main_hash = HashRegistry::register_and_hash("main");

                size_t mesh_spirv_bytes = mesh_spirv.size() * sizeof(uint32_t);
                size_t frag_spirv_bytes = frag_spirv.size() * sizeof(uint32_t);
                size_t total_size = sizeof(ShaderChunk) + 2 * sizeof(ShaderChunk::Shader) + mesh_spirv_bytes + frag_spirv_bytes;

                std::vector<uint8_t> shader_data(total_size);
                size_t offset = 0;

                // Write header
                ShaderChunk header{};
                header.shader_count = 2;
                std::memcpy(shader_data.data() + offset, &header, sizeof(header));
                offset += sizeof(header);

                // Write mesh shader info
                ShaderChunk::Shader mesh_info{};
                mesh_info.name_hash = mesh_name_hash;
                mesh_info.entry_point_hash = main_hash;
                mesh_info.stage = ShaderChunk::Shader::ShaderStage::MeshShader;
                mesh_info.spirv_size = static_cast<uint32_t>(mesh_spirv_bytes);
                mesh_info.max_vertices = 24;
                mesh_info.max_primitives = 12;
                mesh_info.workgroup_size[0] = 1;
                mesh_info.workgroup_size[1] = 1;
                mesh_info.workgroup_size[2] = 1;

                std::memcpy(shader_data.data() + offset, &mesh_info, sizeof(mesh_info));
                offset += sizeof(mesh_info);

                // Write fragment shader info
                ShaderChunk::Shader frag_info{};
                frag_info.name_hash = frag_name_hash;
                frag_info.entry_point_hash = main_hash;
                frag_info.stage = ShaderChunk::Shader::ShaderStage::Fragment;
                frag_info.spirv_size = static_cast<uint32_t>(frag_spirv_bytes);

                std::memcpy(shader_data.data() + offset, &frag_info, sizeof(frag_info));
                offset += sizeof(frag_info);

                // Write SPIR-V data
                std::memcpy(shader_data.data() + offset, mesh_spirv.data(), mesh_spirv_bytes);
                offset += mesh_spirv_bytes;
                std::memcpy(shader_data.data() + offset, frag_spirv.data(), frag_spirv_bytes);

                asset.add_chunk(ChunkType::SHDR, shader_data, "data_driven_shaders");
                return true;
            }

            bool DataDrivenAssetCompiler::createBasicMaterialChunk(Asset& asset) {
                MaterialChunk mat_header{};
                mat_header.material_count = 1;

                MaterialChunk::Material material{};
                std::strncpy(material.name, "data_driven_material", sizeof(material.name) - 1);
                material.albedo[0] = 1.0f; material.albedo[1] = 1.0f; material.albedo[2] = 1.0f; material.albedo[3] = 1.0f;
                material.metallic = 0.0f;
                material.roughness = 0.8f;
                material.normal_intensity = 1.0f;
                material.albedo_texture = UINT32_MAX;
                material.normal_texture = UINT32_MAX;
                material.metallic_roughness_texture = UINT32_MAX;
                material.emission_texture = UINT32_MAX;
                material.flags = MaterialFlags::DoubleSided;

                std::vector<uint8_t> mat_data(sizeof(MaterialChunk) + sizeof(MaterialChunk::Material));
                std::memcpy(mat_data.data(), &mat_header, sizeof(mat_header));
                std::memcpy(mat_data.data() + sizeof(mat_header), &material, sizeof(material));

                asset.add_chunk(ChunkType::MTRL, mat_data, "data_driven_material");
                return true;
            }


    } // namespace Taffy

