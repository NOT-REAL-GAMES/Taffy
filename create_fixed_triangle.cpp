#include <iostream>
#include <vector>
#include <filesystem>
#include <fstream>
#include "include/tools.h"
#include "include/mesh_shader_generator.h"

using namespace Taffy;
using namespace tremor::taffy::tools;

bool createFixedTriangle(const std::string& output_path) {
    std::cout << "🔧 Creating triangle with FIXED color interpolation..." << std::endl;
    
    // Create a simple triangle asset
    Asset asset;
    // Version is set in header constructor
    asset.set_creator("Fixed Mesh Shader Generator");
    asset.set_description("Triangle with properly interpolated colors using mesh shaders");
    // Features will be set when creating the asset
    
    // Create vertex data with colors
    struct Vertex {
        Vec3Q position;
        float normal[3];
        float color[4];
        float uv[2];
        float tangent[4];
    };
    
    std::vector<Vertex> vertices;
    
    // Vertex 0 - Bottom left (Red)
    Vertex v0{};
    v0.position = Vec3Q::fromFloat(glm::vec3(-0.5f, -0.5f, 0.0f));
    v0.normal[0] = 0.0f; v0.normal[1] = 0.0f; v0.normal[2] = 1.0f;
    v0.color[0] = 1.0f; v0.color[1] = 0.0f; v0.color[2] = 0.0f; v0.color[3] = 1.0f;
    v0.uv[0] = 0.0f; v0.uv[1] = 0.0f;
    vertices.push_back(v0);
    
    // Vertex 1 - Bottom right (Green)
    Vertex v1{};
    v1.position = Vec3Q::fromFloat(glm::vec3(0.5f, -0.5f, 0.0f));
    v1.normal[0] = 0.0f; v1.normal[1] = 0.0f; v1.normal[2] = 1.0f;
    v1.color[0] = 0.0f; v1.color[1] = 1.0f; v1.color[2] = 0.0f; v1.color[3] = 1.0f;
    v1.uv[0] = 1.0f; v1.uv[1] = 0.0f;
    vertices.push_back(v1);
    
    // Vertex 2 - Top (Blue)
    Vertex v2{};
    v2.position = Vec3Q::fromFloat(glm::vec3(0.0f, 0.5f, 0.0f));
    v2.normal[0] = 0.0f; v2.normal[1] = 0.0f; v2.normal[2] = 1.0f;
    v2.color[0] = 0.0f; v2.color[1] = 0.0f; v2.color[2] = 1.0f; v2.color[3] = 1.0f;
    v2.uv[0] = 0.5f; v2.uv[1] = 1.0f;
    vertices.push_back(v2);
    
    std::cout << "📊 Vertex data:" << std::endl;
    std::cout << "  Vertex 0: Red   at (-0.5, -0.5)" << std::endl;
    std::cout << "  Vertex 1: Green at ( 0.5, -0.5)" << std::endl;
    std::cout << "  Vertex 2: Blue  at ( 0.0,  0.5)" << std::endl;
    
    // Create indices for indexed rendering
    std::vector<uint32_t> indices = {0, 1, 2};
    
    // Create geometry chunk
    GeometryChunk geom_header{};
    geom_header.vertex_count = vertices.size();
    geom_header.index_count = static_cast<uint32_t>(indices.size());
    std::cout << "DEBUG: Setting index_count to " << geom_header.index_count << std::endl;
    geom_header.vertex_stride = sizeof(Vertex);
    geom_header.vertex_format = VertexFormat::Position3D | VertexFormat::Normal | 
                               VertexFormat::TexCoord0 | VertexFormat::Color | 
                               VertexFormat::Tangent;
    
    // IMPORTANT: Set render mode to MeshShader
    geom_header.render_mode = GeometryChunk::MeshShader;
    geom_header.lod_level = 0;
    
    // Calculate bounds
    geom_header.bounds_min = vertices[0].position;
    geom_header.bounds_max = vertices[0].position;
    for (const auto& v : vertices) {
        // Update bounds manually
        if (v.position.x < geom_header.bounds_min.x) geom_header.bounds_min.x = v.position.x;
        if (v.position.y < geom_header.bounds_min.y) geom_header.bounds_min.y = v.position.y;
        if (v.position.z < geom_header.bounds_min.z) geom_header.bounds_min.z = v.position.z;
        if (v.position.x > geom_header.bounds_max.x) geom_header.bounds_max.x = v.position.x;
        if (v.position.y > geom_header.bounds_max.y) geom_header.bounds_max.y = v.position.y;
        if (v.position.z > geom_header.bounds_max.z) geom_header.bounds_max.z = v.position.z;
    }
    
    // Mesh shader info
    geom_header.ms_max_vertices = 3;
    geom_header.ms_max_primitives = 1;
    geom_header.ms_workgroup_size[0] = 1;
    geom_header.ms_workgroup_size[1] = 1;
    geom_header.ms_workgroup_size[2] = 1;
    geom_header.ms_primitive_type = GeometryChunk::Triangles;
    
    // Pack geometry data
    std::vector<uint8_t> geom_data;
    geom_data.resize(sizeof(GeometryChunk) + vertices.size() * sizeof(Vertex) + indices.size() * sizeof(uint32_t));
    std::memcpy(geom_data.data(), &geom_header, sizeof(GeometryChunk));
    std::memcpy(geom_data.data() + sizeof(GeometryChunk), vertices.data(), 
                vertices.size() * sizeof(Vertex));
    std::memcpy(geom_data.data() + sizeof(GeometryChunk) + vertices.size() * sizeof(Vertex),
                indices.data(), indices.size() * sizeof(uint32_t));
    
    asset.add_chunk(ChunkType::GEOM, geom_data, "fixed_triangle_geometry");
    
    // Generate shaders using the new generator
    FixedMeshShaderGenerator::ShaderConfig config;
    config.max_vertices = 3;
    config.max_primitives = 1;
    config.vertex_count = 3;
    config.vertex_stride_bytes = sizeof(Vertex);
    config.primitive_type = GeometryChunk::Triangles;
    config.has_indices = true;
    config.index_count = static_cast<uint32_t>(indices.size());
    
    // Define vertex attributes
    config.attributes = {
        {VertexAttribute::Vec3Q, 0, 0, "position"},
        {VertexAttribute::Float3, 24, 1, "normal"},
        {VertexAttribute::Float4, 36, 2, "color"},
        {VertexAttribute::Float2, 52, 3, "uv"},
        {VertexAttribute::Float4, 60, 4, "tangent"}
    };
    
    // Generate GLSL
    std::string mesh_glsl = FixedMeshShaderGenerator::generateMeshShader(config);
    std::string frag_glsl = FixedMeshShaderGenerator::generateFragmentShader(config);
    
    std::cout << "\n📝 Generated shaders:" << std::endl;
    std::cout << "  Mesh shader: " << mesh_glsl.size() << " bytes" << std::endl;
    std::cout << "  Fragment shader: " << frag_glsl.size() << " bytes" << std::endl;
    
    // Write shaders to debug files
    std::ofstream mesh_file("/tmp/debug_mesh.glsl");
    mesh_file << mesh_glsl;
    mesh_file.close();
    
    std::ofstream frag_file("/tmp/debug_frag.glsl");
    frag_file << frag_glsl;
    frag_file.close();
    
    std::cout << "  Debug files written to /tmp/debug_mesh.glsl and /tmp/debug_frag.glsl" << std::endl;
    
    // Compile to SPIR-V
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
    options.SetOptimizationLevel(shaderc_optimization_level_zero);
    
    // Compile mesh shader
    auto mesh_result = compiler.CompileGlslToSpv(
        mesh_glsl, shaderc_mesh_shader, "mesh.glsl", options);
    
    if (mesh_result.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::cerr << "❌ Mesh shader compilation failed: " 
                  << mesh_result.GetErrorMessage() << std::endl;
        return false;
    }
    
    std::vector<uint32_t> mesh_spirv(mesh_result.begin(), mesh_result.end());
    
    // Compile fragment shader
    auto frag_result = compiler.CompileGlslToSpv(
        frag_glsl, shaderc_fragment_shader, "frag.glsl", options);
    
    if (frag_result.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::cerr << "❌ Fragment shader compilation failed: " 
                  << frag_result.GetErrorMessage() << std::endl;
        return false;
    }
    
    std::vector<uint32_t> frag_spirv(frag_result.begin(), frag_result.end());
    
    std::cout << "✅ Shaders compiled successfully!" << std::endl;
    std::cout << "  Mesh SPIR-V: " << mesh_spirv.size() * 4 << " bytes" << std::endl;
    std::cout << "  Fragment SPIR-V: " << frag_spirv.size() * 4 << " bytes" << std::endl;
    
    // Create shader chunk
    ShaderChunk shader_header{};
    shader_header.shader_count = 2;
    
    // Register names
    HashRegistry::register_string("fixed_mesh_shader");
    HashRegistry::register_string("fixed_fragment_shader");
    HashRegistry::register_string("main");
    
    // Mesh shader info
    ShaderChunk::Shader mesh_info{};
    mesh_info.name_hash = fnv1a_hash("fixed_mesh_shader");
    mesh_info.entry_point_hash = fnv1a_hash("main");
    mesh_info.stage = ShaderChunk::Shader::ShaderStage::MeshShader;
    mesh_info.spirv_size = mesh_spirv.size() * sizeof(uint32_t);
    mesh_info.max_vertices = 3;
    mesh_info.max_primitives = 1;
    mesh_info.workgroup_size[0] = 1;
    mesh_info.workgroup_size[1] = 1;
    mesh_info.workgroup_size[2] = 1;
    
    // Fragment shader info
    ShaderChunk::Shader frag_info{};
    frag_info.name_hash = fnv1a_hash("fixed_fragment_shader");
    frag_info.entry_point_hash = fnv1a_hash("main");
    frag_info.stage = ShaderChunk::Shader::ShaderStage::Fragment;
    frag_info.spirv_size = frag_spirv.size() * sizeof(uint32_t);
    
    // Pack shader data
    size_t shader_data_size = sizeof(ShaderChunk) + 2 * sizeof(ShaderChunk::Shader) +
                             mesh_info.spirv_size + frag_info.spirv_size;
    std::vector<uint8_t> shader_data(shader_data_size);
    
    size_t offset = 0;
    std::memcpy(shader_data.data() + offset, &shader_header, sizeof(shader_header));
    offset += sizeof(shader_header);
    
    std::memcpy(shader_data.data() + offset, &mesh_info, sizeof(mesh_info));
    offset += sizeof(mesh_info);
    
    std::memcpy(shader_data.data() + offset, &frag_info, sizeof(frag_info));
    offset += sizeof(frag_info);
    
    std::memcpy(shader_data.data() + offset, mesh_spirv.data(), mesh_info.spirv_size);
    offset += mesh_info.spirv_size;
    
    std::memcpy(shader_data.data() + offset, frag_spirv.data(), frag_info.spirv_size);
    
    asset.add_chunk(ChunkType::SHDR, shader_data, "fixed_shaders");
    
    // Create basic material
    MaterialChunk mat_header{};
    mat_header.material_count = 1;
    
    MaterialChunk::Material material{};
    std::strncpy(material.name, "fixed_material", sizeof(material.name) - 1);
    material.albedo[0] = 1.0f;
    material.albedo[1] = 1.0f;
    material.albedo[2] = 1.0f;
    material.albedo[3] = 1.0f;
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
    
    asset.add_chunk(ChunkType::MTRL, mat_data, "fixed_material");
    
    // Save the asset
    std::filesystem::create_directories(std::filesystem::path(output_path).parent_path());
    
    if (!asset.save_to_file(output_path)) {
        std::cerr << "❌ Failed to save asset!" << std::endl;
        return false;
    }
    
    std::cout << "\n✅ Fixed triangle asset created successfully!" << std::endl;
    std::cout << "📁 Saved to: " << output_path << std::endl;
    std::cout << "🎨 Features:" << std::endl;
    std::cout << "  - Proper color interpolation" << std::endl;
    std::cout << "  - Hardware barycentric coordinates" << std::endl;
    std::cout << "  - Mesh shader render mode" << std::endl;
    std::cout << "  - Red, Green, Blue gradient" << std::endl;
    
    return true;
}

// main() removed - this is now called from Taffy.cpp