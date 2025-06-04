#include "include/tools.h"
#include "include/quan.h"
#include "include/asset.h"
#include <iostream>
#include <cstring>
#include <filesystem>

namespace tremor::taffy::tools {

bool createSineWaveAudioAsset(const std::string& output_path, float frequency, float duration) {
    std::cout << "🎵 Creating sine wave audio asset..." << std::endl;
    std::cout << "   Frequency: " << frequency << " Hz" << std::endl;
    std::cout << "   Duration: " << duration << " seconds" << std::endl;
    
    using namespace Taffy;
    
    Asset asset;
    asset.set_creator("Taffy Audio Test Creator");
    asset.set_description("Simple sine wave test audio");
    asset.set_feature_flags(FeatureFlags::Audio);
    
    // Create AudioChunk
    AudioChunk audioChunk{};
    audioChunk.node_count = 3;        // Oscillator -> Amplifier -> Output
    audioChunk.connection_count = 2;  // Two connections
    audioChunk.pattern_count = 0;     // No patterns for simple sine
    audioChunk.sample_count = 0;      // No samples, pure synthesis
    audioChunk.parameter_count = 3;   // Frequency, amplitude, and time
    audioChunk.sample_rate = 48000;
    audioChunk.tick_rate = 0;         // Not using tracker timing
    
    // Calculate sizes for the chunk data
    size_t header_size = sizeof(AudioChunk);
    size_t nodes_size = audioChunk.node_count * sizeof(AudioChunk::Node);
    size_t connections_size = audioChunk.connection_count * sizeof(AudioChunk::Connection);
    size_t params_size = audioChunk.parameter_count * sizeof(AudioChunk::Parameter);
    size_t total_size = header_size + nodes_size + connections_size + params_size;
    
    // Create chunk data buffer
    std::vector<uint8_t> chunk_data(total_size);
    uint8_t* ptr = chunk_data.data();
    
    // Write header
    std::memcpy(ptr, &audioChunk, header_size);
    ptr += header_size;
    
    // Define nodes
    AudioChunk::Node nodes[3] = {};
    
    // Node 0: Sine oscillator
    nodes[0].id = 0;
    nodes[0].type = AudioChunk::NodeType::Oscillator;
    nodes[0].name_hash = fnv1a_hash("sine_oscillator");
    nodes[0].position[0] = 100.0f;
    nodes[0].position[1] = 100.0f;
    nodes[0].input_count = 1;     // Frequency input
    nodes[0].output_count = 1;    // Audio output
    nodes[0].param_offset = 0;    // First parameter
    nodes[0].param_count = 1;     // Frequency parameter
    
    // Node 1: Amplifier
    nodes[1].id = 1;
    nodes[1].type = AudioChunk::NodeType::Amplifier;
    nodes[1].name_hash = fnv1a_hash("main_amplifier");
    nodes[1].position[0] = 300.0f;
    nodes[1].position[1] = 100.0f;
    nodes[1].input_count = 2;     // Audio input + amplitude control
    nodes[1].output_count = 1;    // Audio output
    nodes[1].param_offset = 1;    // Second parameter
    nodes[1].param_count = 1;     // Amplitude parameter
    
    // Node 2: Parameter (time) - for potential future modulation
    nodes[2].id = 2;
    nodes[2].type = AudioChunk::NodeType::Parameter;
    nodes[2].name_hash = fnv1a_hash("time_parameter");
    nodes[2].position[0] = 100.0f;
    nodes[2].position[1] = 200.0f;
    nodes[2].input_count = 0;
    nodes[2].output_count = 1;    // Time value output
    nodes[2].param_offset = 2;    // Third parameter
    nodes[2].param_count = 1;     // Time parameter
    
    // Write nodes
    std::memcpy(ptr, nodes, nodes_size);
    ptr += nodes_size;
    
    // Define connections
    AudioChunk::Connection connections[2] = {};
    
    // Connection 0: Oscillator -> Amplifier (audio)
    connections[0].source_node = 0;
    connections[0].source_output = 0;
    connections[0].dest_node = 1;
    connections[0].dest_input = 0;
    connections[0].strength = 1.0f;
    
    // Connection 1: Time parameter -> could be used for frequency modulation
    connections[1].source_node = 2;
    connections[1].source_output = 0;
    connections[1].dest_node = 0;
    connections[1].dest_input = 0;
    connections[1].strength = 0.0f;  // Not active by default
    
    // Write connections
    std::memcpy(ptr, connections, connections_size);
    ptr += connections_size;
    
    // Define parameters
    AudioChunk::Parameter params[3] = {};
    
    // Parameter 0: Frequency
    params[0].name_hash = fnv1a_hash("frequency");
    params[0].default_value = frequency;
    params[0].min_value = 20.0f;
    params[0].max_value = 20000.0f;
    params[0].curve = 2.0f;  // Exponential curve for frequency
    params[0].flags = 0;
    
    // Parameter 1: Amplitude
    params[1].name_hash = fnv1a_hash("amplitude");
    params[1].default_value = 0.5f;
    params[1].min_value = 0.0f;
    params[1].max_value = 1.0f;
    params[1].curve = 1.0f;  // Linear
    params[1].flags = 0;
    
    // Parameter 2: Time
    params[2].name_hash = fnv1a_hash("time");
    params[2].default_value = 0.0f;
    params[2].min_value = 0.0f;
    params[2].max_value = duration;
    params[2].curve = 1.0f;  // Linear
    params[2].flags = 0;
    
    // Write parameters
    std::memcpy(ptr, params, params_size);
    
    // Add the audio chunk to the asset
    asset.add_chunk(ChunkType::AUDI, chunk_data, "sine_wave_audio");
    
    // Save to file
    std::filesystem::create_directories(std::filesystem::path(output_path).parent_path());
    
    if (asset.save_to_file(output_path)) {
        std::cout << "✅ Sine wave audio asset created: " << output_path << std::endl;
        std::cout << "   📊 Total size: " << total_size << " bytes" << std::endl;
        std::cout << "   🎼 Audio graph: Oscillator(" << frequency << "Hz) -> Amplifier(0.5) -> Output" << std::endl;
        return true;
    }
    
    return false;
}

bool createHotPinkShaderOverlay(const std::string& output_path) {
    // Temporary stub - we'll need to properly fix tools.cpp later
    std::cout << "⚠️  createHotPinkShaderOverlay is temporarily stubbed out" << std::endl;
    return false;
}

} // namespace tremor::taffy::tools

namespace Taffy {

bool DataDrivenAssetCompiler::createDataDrivenTriangle(const std::string& output_path) {
    // Temporary stub - we'll need to properly fix tools.cpp later
    std::cout << "⚠️  createDataDrivenTriangle is temporarily stubbed out" << std::endl;
    return false;
}

} // namespace Taffy