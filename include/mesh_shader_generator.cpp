#include "mesh_shader_generator.h"
#include <cstring>
#include <iomanip>

namespace Taffy {

const char* FixedMeshShaderGenerator::getGLSLType(VertexAttribute::Type type) {
    switch (type) {
        case VertexAttribute::Float: return "float";
        case VertexAttribute::Float2: return "vec2";
        case VertexAttribute::Float3: return "vec3";
        case VertexAttribute::Float4: return "vec4";
        case VertexAttribute::Int: return "int";
        case VertexAttribute::Int2: return "ivec2";
        case VertexAttribute::Int3: return "ivec3";
        case VertexAttribute::Int4: return "ivec4";
        case VertexAttribute::UInt: return "uint";
        case VertexAttribute::UInt2: return "uvec2";
        case VertexAttribute::UInt3: return "uvec3";
        case VertexAttribute::UInt4: return "uvec4";
        case VertexAttribute::Vec3Q: return "vec3"; // Converted from quantized
        default: return "vec4";
    }
}

size_t FixedMeshShaderGenerator::getAttributeSize(VertexAttribute::Type type) {
    switch (type) {
        case VertexAttribute::Float: return 4;
        case VertexAttribute::Float2: return 8;
        case VertexAttribute::Float3: return 12;
        case VertexAttribute::Float4: return 16;
        case VertexAttribute::Int: return 4;
        case VertexAttribute::Int2: return 8;
        case VertexAttribute::Int3: return 12;
        case VertexAttribute::Int4: return 16;
        case VertexAttribute::UInt: return 4;
        case VertexAttribute::UInt2: return 8;
        case VertexAttribute::UInt3: return 12;
        case VertexAttribute::UInt4: return 16;
        case VertexAttribute::Vec3Q: return 24; // 3 x int64
        default: return 16;
    }
}

void FixedMeshShaderGenerator::generateVertexReaders(std::stringstream& shader, const ShaderConfig& config) {
    // Generate reader functions for each attribute
    for (const auto& attr : config.attributes) {
        if (attr.type == VertexAttribute::Vec3Q) {
            // Special case for Vec3Q
            shader << "vec3 read_" << attr.name << "(uint vertexIndex) {\n";
            shader << "    uint baseOffsetUints = (vertexIndex * pc.vertex_stride_floats * 4 + " 
                   << attr.offset << ") / 4;\n";
            shader << "    uint x_lo = vertexBuffer.vertices[baseOffsetUints + 0];\n";
            shader << "    uint x_hi = vertexBuffer.vertices[baseOffsetUints + 1];\n";
            shader << "    uint y_lo = vertexBuffer.vertices[baseOffsetUints + 2];\n";
            shader << "    uint y_hi = vertexBuffer.vertices[baseOffsetUints + 3];\n";
            shader << "    uint z_lo = vertexBuffer.vertices[baseOffsetUints + 4];\n";
            shader << "    uint z_hi = vertexBuffer.vertices[baseOffsetUints + 5];\n";
            shader << "    \n";
            shader << "    // Reconstruct 64-bit signed integers from pairs of 32-bit uints\n";
            shader << "    // Note: GLSL doesn't have int64_t, so we work with doubles\n";
            shader << "    double x = double(int(x_lo)) + double(int(x_hi)) * 4294967296.0;\n";
            shader << "    double y = double(int(y_lo)) + double(int(y_hi)) * 4294967296.0;\n";
            shader << "    double z = double(int(z_lo)) + double(int(z_hi)) * 4294967296.0;\n";
            shader << "    \n";
            shader << "    const float SCALE = 1.0 / 128000.0;\n";
            shader << "    return vec3(float(x) * SCALE, float(y) * SCALE, float(z) * SCALE);\n";
            shader << "}\n\n";
        } else {
            // Regular attributes
            shader << getGLSLType(attr.type) << " read_" << attr.name << "(uint vertexIndex) {\n";
            shader << "    uint offsetFloats = vertexIndex * pc.vertex_stride_floats + " 
                   << (attr.offset / 4) << "u;\n";
            
            // Read the appropriate number of components
            const char* glslType = getGLSLType(attr.type);
            if (strstr(glslType, "vec") || strstr(glslType, "float")) {
                // Float types
                if (attr.type == VertexAttribute::Float) {
                    shader << "    return uintBitsToFloat(vertexBuffer.vertices[offsetFloats]);\n";
                } else if (attr.type == VertexAttribute::Float2) {
                    shader << "    return vec2(\n";
                    shader << "        uintBitsToFloat(vertexBuffer.vertices[offsetFloats]),\n";
                    shader << "        uintBitsToFloat(vertexBuffer.vertices[offsetFloats + 1u])\n";
                    shader << "    );\n";
                } else if (attr.type == VertexAttribute::Float3) {
                    shader << "    return vec3(\n";
                    shader << "        uintBitsToFloat(vertexBuffer.vertices[offsetFloats]),\n";
                    shader << "        uintBitsToFloat(vertexBuffer.vertices[offsetFloats + 1u]),\n";
                    shader << "        uintBitsToFloat(vertexBuffer.vertices[offsetFloats + 2u])\n";
                    shader << "    );\n";
                } else if (attr.type == VertexAttribute::Float4) {
                    shader << "    return vec4(\n";
                    shader << "        uintBitsToFloat(vertexBuffer.vertices[offsetFloats]),\n";
                    shader << "        uintBitsToFloat(vertexBuffer.vertices[offsetFloats + 1u]),\n";
                    shader << "        uintBitsToFloat(vertexBuffer.vertices[offsetFloats + 2u]),\n";
                    shader << "        uintBitsToFloat(vertexBuffer.vertices[offsetFloats + 3u])\n";
                    shader << "    );\n";
                }
            } else {
                // Integer types - just cast
                shader << "    // TODO: Implement integer attribute reading\n";
                shader << "    return " << glslType << "(0);\n";
            }
            
            shader << "}\n\n";
        }
    }
}

std::string FixedMeshShaderGenerator::generateMeshShader(const ShaderConfig& config) {
    std::stringstream shader;
    
    // Header
    shader << "#version 460\n";
    shader << "#extension GL_EXT_mesh_shader : require\n\n";
    
    // Workgroup size
    shader << "layout(local_size_x = " << config.workgroup_x
           << ", local_size_y = " << config.workgroup_y
           << ", local_size_z = " << config.workgroup_z << ") in;\n";
    
    // Output topology
    const char* topology = (config.primitive_type == GeometryChunk::Triangles) ? "triangles" :
                          (config.primitive_type == GeometryChunk::Lines) ? "lines" : "points";
    
    shader << "layout(" << topology
           << ", max_vertices = " << config.max_vertices
           << ", max_primitives = " << config.max_primitives << ") out;\n\n";
    
    // Storage buffer for vertex data
    shader << "layout(set = 0, binding = 0) readonly buffer VertexBuffer {\n";
    shader << "    uint vertices[];\n";
    shader << "} vertexBuffer;\n\n";
    
    // Push constants
    shader << "layout(push_constant) uniform PushConstants {\n";
    shader << "    mat4 mvp;\n";
    shader << "    uint vertex_count;\n";
    shader << "    uint primitive_count;\n";
    shader << "    uint vertex_stride_floats;\n";
    shader << "    uint index_offset_bytes;\n";
    shader << "} pc;\n\n";
    
    // Output attributes - IMPORTANT: These need to be per-vertex arrays
    for (const auto& attr : config.attributes) {
        if (strcmp(attr.name, "position") == 0) continue; // gl_Position is built-in
        
        shader << "layout(location = " << attr.location << ") out "
               << getGLSLType(attr.type) << " out_" << attr.name << "[];\n";
    }
    
    // For barycentric interpolation support, output vertex indices
    shader << "\n// For manual interpolation in fragment shader\n";
    shader << "layout(location = 10) out flat uint vertexIndices[];\n";
    shader << "// Manual barycentric coordinates per vertex\n";
    shader << "layout(location = 11) out vec3 vertexBarycentrics[];\n\n";
    
    // Generate vertex reader functions
    generateVertexReaders(shader, config);
    
    // Add index reading function if indices are present
    if (config.has_indices) {
        shader << "uint readIndex(uint indexNum) {\n";
        shader << "    uint byte_offset = 224 + indexNum * 4u;\n";
        shader << "    uint word_offset = byte_offset / 4u;\n";
        shader << "    return vertexBuffer.vertices[word_offset];\n";
        shader << "}\n\n";
    }
    
    // Main function
    shader << "void main() {\n";
    shader << "    // Only first thread does the work\n";
    shader << "    if (gl_LocalInvocationIndex != 0) return;\n\n";
    
    shader << "    uint vertex_count = min(pc.vertex_count, " << config.max_vertices << "u);\n";
    shader << "    uint primitive_count = min(pc.primitive_count, " << config.max_primitives << "u);\n\n";
    
    shader << "    SetMeshOutputsEXT(vertex_count, primitive_count);\n\n";
    
    // Process vertices
    generateVertexProcessing(shader, config);
    
    // Generate primitives
    generatePrimitiveAssembly(shader, config);
    
    shader << "}\n";
    
    return shader.str();
}

void FixedMeshShaderGenerator::generateVertexProcessing(std::stringstream& shader, const ShaderConfig& config) {
    shader << "    // Process vertices\n";
    shader << "    for (uint i = 0; i < vertex_count; ++i) {\n";
    
    // Read and transform position
    for (const auto& attr : config.attributes) {
        if (strcmp(attr.name, "position") == 0) {
            shader << "        vec3 position = read_" << attr.name << "(i);\n";
            shader << "        gl_MeshVerticesEXT[i].gl_Position = pc.mvp * vec4(position, 1.0);\n";
            break;
        }
    }
    
    // Read and output other attributes
    for (const auto& attr : config.attributes) {
        if (strcmp(attr.name, "position") == 0) continue;
        
        shader << "        out_" << attr.name << "[i] = read_" << attr.name << "(i);\n";
        
        // DEBUG: Comment out color override to use actual vertex colors
         if (strcmp(attr.name, "color") == 0) {
             shader << "        // DEBUG: Force known colors to verify output\n";
             shader << "        if (i == 0u) out_color[i] = vec4(1.0, 0.0, 0.0, 1.0); // Red\n";
             shader << "        else if (i == 1u) out_color[i] = vec4(0.0, 1.0, 0.0, 1.0); // Green\n";
             shader << "        else if (i == 2u) out_color[i] = vec4(1.0, 1.0, 0.0, 1.0); // Yellow\n";
         }
    }
    
    // Store vertex index for manual interpolation
    shader << "        vertexIndices[i] = i;\n";
    
    // Set barycentric coordinates for each vertex
    shader << "        // Set barycentric coordinates for this vertex\n";
    shader << "        if (i == 0u) vertexBarycentrics[i] = vec3(1.0, 0.0, 0.0);\n";
    shader << "        else if (i == 1u) vertexBarycentrics[i] = vec3(0.0, 1.0, 0.0);\n";
    shader << "        else if (i == 2u) vertexBarycentrics[i] = vec3(0.0, 0.0, 1.0);\n";
    
    shader << "    }\n\n";
}

void FixedMeshShaderGenerator::generatePrimitiveAssembly(std::stringstream& shader, const ShaderConfig& config) {
    shader << "    // Generate primitives\n";
    
    if (config.has_indices) {
        // Add index reading function at the beginning of main
        shader << "    // Index buffer reading\n";
        if (config.primitive_type == GeometryChunk::Triangles) {
            shader << "    for (uint i = 0; i < primitive_count; ++i) {\n";
            shader << "        uint idx0 = readIndex(i * 3u);\n";
            shader << "        uint idx1 = readIndex(i * 3u + 1u);\n";
            shader << "        uint idx2 = readIndex(i * 3u + 2u);\n";
            shader << "        gl_PrimitiveTriangleIndicesEXT[i] = uvec3(idx0, idx1, idx2);\n";
            shader << "    }\n";
        }
    } else {
        if (config.primitive_type == GeometryChunk::Triangles) {
            shader << "    for (uint i = 0; i < primitive_count; ++i) {\n";
            shader << "        gl_PrimitiveTriangleIndicesEXT[i] = uvec3(i*3u, i*3u+1u, i*3u+2u);\n";
            shader << "    }\n";
        } else if (config.primitive_type == GeometryChunk::Lines) {
            shader << "    for (uint i = 0; i < primitive_count; ++i) {\n";
            shader << "        gl_PrimitiveLineIndicesEXT[i] = uvec2(i*2u, i*2u+1u);\n";
            shader << "    }\n";
        } else {
            shader << "    for (uint i = 0; i < primitive_count; ++i) {\n";
            shader << "        gl_PrimitivePointIndicesEXT[i] = i;\n";
            shader << "    }\n";
        }
    }
}

std::string FixedMeshShaderGenerator::generateFragmentShader(const ShaderConfig& config) {
    std::stringstream shader;
    
    shader << "#version 460\n";
    shader << "#extension GL_EXT_fragment_shader_barycentric : enable\n\n";
    
    // Input attributes from mesh shader
    for (const auto& attr : config.attributes) {
        if (strcmp(attr.name, "position") == 0) continue;
        
        shader << "layout(location = " << attr.location << ") in "
               << getGLSLType(attr.type) << " in_" << attr.name << ";\n";
    }
    
    // Barycentric coordinates
    shader << "\n// Built-in barycentric coordinates\n";
    shader << "// gl_BaryCoordEXT gives us the barycentric coordinates\n\n";
    
    // For manual interpolation fallback
    shader << "layout(location = 10) in flat uint vertexIndex;\n";
    shader << "// Manual barycentric coordinates from mesh shader\n";
    shader << "layout(location = 11) in vec3 manualBarycentrics;\n\n";
    
    // Storage buffer access (for manual interpolation if needed)
    shader << "layout(set = 0, binding = 0) readonly buffer VertexBuffer {\n";
    shader << "    uint vertices[];\n";
    shader << "} vertexBuffer;\n\n";
    
    // Push constants (to know vertex stride)
    shader << "layout(push_constant) uniform PushConstants {\n";
    shader << "    mat4 mvp;\n";
    shader << "    uint vertex_count;\n";
    shader << "    uint primitive_count;\n";
    shader << "    uint vertex_stride_floats;\n";
    shader << "    uint index_offset_bytes;\n";
    shader << "} pc;\n\n";
    
    // Output
    shader << "layout(location = 0) out vec4 fragColor;\n\n";
    
    // Generate vertex reader functions (same as mesh shader)
    generateVertexReaders(shader, config);
    
    // Add index reading function if indices are present
    if (config.has_indices) {
        shader << "uint readIndex(uint indexNum) {\n";
        shader << "    uint byte_offset = pc.index_offset_bytes + indexNum * 4u;\n";
        shader << "    uint word_offset = byte_offset / 4u;\n";
        shader << "    return vertexBuffer.vertices[word_offset];\n";
        shader << "}\n\n";
    }
    
    // Main function
    shader << "void main() {\n";
    
    // Check if we have color attribute
    bool hasColor = false;
    for (const auto& attr : config.attributes) {
        if (strcmp(attr.name, "color") == 0) {
            hasColor = true;
            break;
        }
    }
    
    if (hasColor) {
        // Debug: Show barycentric coordinates to verify interpolation
        shader << "    // DEBUG: Visualize barycentric coordinates\n";
        shader << "    vec3 bary = gl_BaryCoordEXT;\n";
        shader << "    fragColor = vec4(bary.x, bary.y, bary.z, 1.0);\n";
        shader << "    \n";
        shader << "    // Original: Use hardware-interpolated color from mesh shader\n";
        shader << "    // fragColor = in_color;\n";
    } else {
        // Default white if no color
        shader << "    fragColor = vec4(1.0, 1.0, 1.0, 1.0);\n";
    }
    
    shader << "}\n";
    
    return shader.str();
}

void FixedMeshShaderGenerator::generateColorInterpolation(std::stringstream& shader, const ShaderConfig& config) {
    shader << "    // Manual barycentric interpolation\n";
    shader << "    // For a triangle, we need to read all 3 vertex colors\n";
    shader << "    vec4 color0 = read_color(vertexIndex - vertexIndex % 3u);\n";
    shader << "    vec4 color1 = read_color(vertexIndex - vertexIndex % 3u + 1u);\n";
    shader << "    vec4 color2 = read_color(vertexIndex - vertexIndex % 3u + 2u);\n";
    shader << "    \n";
    shader << "    // Use gl_BaryCoordEXT for proper interpolation\n";
    shader << "    vec3 bary = vec3(gl_BaryCoordEXT.x, gl_BaryCoordEXT.y, 1.0 - gl_BaryCoordEXT.x - gl_BaryCoordEXT.y);\n";
    shader << "    fragColor = vec4(color0.rgb * bary.x + color1.rgb * bary.y + color2.rgb * bary.z, 1.0);\n";
}

} // namespace Taffy