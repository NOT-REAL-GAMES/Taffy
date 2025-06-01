/**
 * Taffy Asset Compiler
 * 
 * A tool to generate .taf asset files from various inputs
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cstdint>
#include <string>
#include <sstream>
#include "include/taffy.h"

using namespace Taffy;

// Simple CRC32 implementation
uint32_t calculate_crc32(const uint8_t* data, size_t size) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < size; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    return ~crc;
}

class TaffyCompiler {
public:
    AssetHeader header;
    std::vector<ChunkDirectoryEntry> chunks;
    std::vector<std::vector<uint8_t>> chunk_data;

    TaffyCompiler() {
        memset(&header, 0, sizeof(header));
        strncpy(header.magic, "TAF!", 4);
        header.version_major = 1;
        header.version_minor = 0;
        header.version_patch = 0;
        header.asset_type = 0; // Master asset
        header.feature_flags = FeatureFlags::None;
    }

    void set_creator(const std::string& creator) {
        strncpy(header.creator, creator.c_str(), sizeof(header.creator) - 1);
    }

    void set_description(const std::string& desc) {
        strncpy(header.description, desc.c_str(), sizeof(header.description) - 1);
    }

    void add_chunk(ChunkType type, const std::string& name, const std::vector<uint8_t>& data) {
        ChunkDirectoryEntry entry;
        memset(&entry, 0, sizeof(entry));
        
        entry.type = type;
        entry.flags = 0;
        entry.size = data.size();
        entry.checksum = calculate_crc32(data.data(), data.size());
        strncpy(entry.name, name.c_str(), sizeof(entry.name) - 1);
        
        chunks.push_back(entry);
        chunk_data.push_back(data);
    }

    bool write(const std::string& filename) {
        std::ofstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Failed to create file: " << filename << std::endl;
            return false;
        }

        // Update header
        header.chunk_count = chunks.size();
        
        // Calculate offsets
        uint64_t current_offset = sizeof(AssetHeader) + chunks.size() * sizeof(ChunkDirectoryEntry);
        for (size_t i = 0; i < chunks.size(); i++) {
            chunks[i].offset = current_offset;
            current_offset += chunks[i].size;
        }
        header.total_size = current_offset;

        // Write header
        file.write(reinterpret_cast<const char*>(&header), sizeof(header));

        // Write chunk directory
        for (const auto& chunk : chunks) {
            file.write(reinterpret_cast<const char*>(&chunk), sizeof(chunk));
        }

        // Write chunk data
        for (const auto& data : chunk_data) {
            file.write(reinterpret_cast<const char*>(data.data()), data.size());
        }

        std::cout << "✅ Created " << filename << " (" << header.total_size << " bytes)" << std::endl;
        std::cout << "   Chunks: " << chunks.size() << std::endl;
        for (const auto& chunk : chunks) {
            std::cout << "   - " << chunk.name << " (" << chunk.size << " bytes)" << std::endl;
        }

        return true;
    }
};

// Create a simple triangle asset
void create_triangle_asset() {
    TaffyCompiler compiler;
    compiler.set_creator("Taffy Compiler v1.0");
    compiler.set_description("Simple triangle with quantized coordinates");

    // Create geometry chunk for mesh shader pipeline
    struct GeometryHeader {
        uint32_t vertex_count;
        uint32_t vertex_stride;
        uint32_t index_count;
        uint32_t index_stride;
        uint32_t primitive_count;
        uint32_t render_mode;  // 0 = traditional, 1 = mesh shader
        uint32_t vertex_format; // Bitmask of vertex attributes
        uint32_t reserved[9];
    };

    struct TriangleVertex {
        Vec3Q_Packed position;
        uint32_t color;
    };

    std::vector<uint8_t> geom_data;
    
    // Write geometry header
    GeometryHeader geom_header = {};
    geom_header.vertex_count = 3;
    geom_header.vertex_stride = sizeof(TriangleVertex);
    geom_header.index_count = 0;  // No indices for simple triangle
    geom_header.index_stride = 0;
    geom_header.primitive_count = 1;
    geom_header.render_mode = 1;  // Mesh shader mode
    geom_header.vertex_format = static_cast<uint32_t>(VertexFormat::Position3D) | 
                                static_cast<uint32_t>(VertexFormat::Color);
    
    geom_data.insert(geom_data.end(),
                     reinterpret_cast<uint8_t*>(&geom_header),
                     reinterpret_cast<uint8_t*>(&geom_header) + sizeof(geom_header));
    
    // Triangle vertices (in quantized coordinates)
    // Using simple coordinates that won't overflow
    TriangleVertex vertices[3] = {
        {{0, 0, 0}, 0xFF0000FF},           // Red
        {{128000, 0, 0}, 0xFF00FF00},      // Green (1 meter in X)
        {{64000, 111000, 0}, 0xFFFF0000}   // Blue (0.5m X, ~0.866m Y)
    };

    // Write vertices
    geom_data.insert(geom_data.end(),
                     reinterpret_cast<uint8_t*>(&vertices[0]),
                     reinterpret_cast<uint8_t*>(&vertices[3]));

    compiler.add_chunk(ChunkType::GEOM, "triangle_geometry", geom_data);

    // Create material chunk
    std::vector<uint8_t> mtrl_data;
    std::string material_name = "simple_material";
    uint32_t name_len = material_name.length();
    mtrl_data.insert(mtrl_data.end(),
                     reinterpret_cast<uint8_t*>(&name_len),
                     reinterpret_cast<uint8_t*>(&name_len) + sizeof(name_len));
    mtrl_data.insert(mtrl_data.end(), material_name.begin(), material_name.end());

    compiler.add_chunk(ChunkType::MTRL, "triangle_material", mtrl_data);

    // Create shader chunk with mesh shader info
    std::vector<uint8_t> shdr_data;
    
    struct ShaderHeader {
        uint32_t shader_count;
        uint32_t total_size;
        uint32_t reserved[6];
    };
    
    struct ShaderEntry {
        uint32_t stage;  // 0x20 = mesh shader, 0x40 = task shader, 0x10 = fragment
        uint32_t size;
        uint32_t offset;
        char name[32];
        uint32_t reserved[4];
    };
    
    ShaderHeader shdr_header = {};
    shdr_header.shader_count = 2;  // Mesh + Fragment
    
    // For now, we'll just put placeholder data
    // In a real implementation, you'd compile GLSL to SPIR-V here
    std::vector<uint8_t> mesh_shader_data = {0x03, 0x02, 0x23, 0x07}; // SPIR-V magic
    std::vector<uint8_t> frag_shader_data = {0x03, 0x02, 0x23, 0x07}; // SPIR-V magic
    
    shdr_header.total_size = sizeof(ShaderHeader) + 
                            2 * sizeof(ShaderEntry) + 
                            mesh_shader_data.size() + 
                            frag_shader_data.size();
    
    // Write header
    shdr_data.insert(shdr_data.end(),
                     reinterpret_cast<uint8_t*>(&shdr_header),
                     reinterpret_cast<uint8_t*>(&shdr_header) + sizeof(shdr_header));
    
    // Write shader entries
    ShaderEntry mesh_entry = {};
    mesh_entry.stage = 0x20;  // Mesh shader
    mesh_entry.size = mesh_shader_data.size();
    mesh_entry.offset = sizeof(ShaderHeader) + 2 * sizeof(ShaderEntry);
    strncpy(mesh_entry.name, "mesh_shader", sizeof(mesh_entry.name) - 1);
    
    ShaderEntry frag_entry = {};
    frag_entry.stage = 0x10;  // Fragment shader
    frag_entry.size = frag_shader_data.size();
    frag_entry.offset = mesh_entry.offset + mesh_entry.size;
    strncpy(frag_entry.name, "fragment_shader", sizeof(frag_entry.name) - 1);
    
    shdr_data.insert(shdr_data.end(),
                     reinterpret_cast<uint8_t*>(&mesh_entry),
                     reinterpret_cast<uint8_t*>(&mesh_entry) + sizeof(mesh_entry));
    shdr_data.insert(shdr_data.end(),
                     reinterpret_cast<uint8_t*>(&frag_entry),
                     reinterpret_cast<uint8_t*>(&frag_entry) + sizeof(frag_entry));
    
    // Write shader data
    shdr_data.insert(shdr_data.end(), mesh_shader_data.begin(), mesh_shader_data.end());
    shdr_data.insert(shdr_data.end(), frag_shader_data.begin(), frag_shader_data.end());
    
    compiler.add_chunk(ChunkType::SHDR, "mesh_shaders", shdr_data);

    // Write the asset
    compiler.write("triangle.taf");
}

// Create asset from command line
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Taffy Asset Compiler" << std::endl;
        std::cout << "Usage: " << argv[0] << " <command> [options]" << std::endl;
        std::cout << "\nCommands:" << std::endl;
        std::cout << "  triangle     Create a simple triangle asset" << std::endl;
        std::cout << "  box          Create a box asset" << std::endl;
        std::cout << "  convert      Convert from other formats" << std::endl;
        return 1;
    }

    std::string command = argv[1];

    if (command == "triangle") {
        create_triangle_asset();
    }
    else {
        std::cerr << "Unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}