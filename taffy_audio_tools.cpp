#include "include/taffy_audio_tools.h"
#include "include/taffy.h"
#include "include/asset.h"
#include <iostream>
#include <cstring>
#include <filesystem>
#include <ctime>

namespace tremor::taffy::tools {

bool createWaveformAudioAsset(const std::string& output_path, float frequency, float duration, uint32_t waveform_type) {
    const char* waveform_names[] = {"Sine", "Square", "Saw", "Triangle", "Noise"};
    const char* waveform_name = (waveform_type < 5) ? waveform_names[waveform_type] : "Unknown";
    
    std::cout << "🎵 Creating " << waveform_name << " wave audio asset..." << std::endl;
    std::cout << "   Frequency: " << frequency << " Hz" << std::endl;
    std::cout << "   Duration: " << duration << " seconds" << std::endl;
    std::cout << "   Waveform: " << waveform_name << " (type " << waveform_type << ")" << std::endl;
    
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
    audioChunk.parameter_count = 4;   // Frequency, amplitude, time, and waveform
    audioChunk.sample_rate = frequency;
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
    nodes[0].param_count = 2;     // Frequency and waveform parameters
    
    // Node 1: Amplifier
    nodes[1].id = 1;
    nodes[1].type = AudioChunk::NodeType::Amplifier;
    nodes[1].name_hash = fnv1a_hash("main_amplifier");
    nodes[1].position[0] = 300.0f;
    nodes[1].position[1] = 100.0f;
    nodes[1].input_count = 2;     // Audio input + amplitude control
    nodes[1].output_count = 1;    // Audio output
    nodes[1].param_offset = 2;    // Third parameter (amplitude)
    nodes[1].param_count = 1;     // Amplitude parameter
    
    // Node 2: Parameter (time) - for potential future modulation
    nodes[2].id = 2;
    nodes[2].type = AudioChunk::NodeType::Parameter;
    nodes[2].name_hash = fnv1a_hash("time_parameter");
    nodes[2].position[0] = 100.0f;
    nodes[2].position[1] = 200.0f;
    nodes[2].input_count = 0;
    nodes[2].output_count = 1;    // Time value output
    nodes[2].param_offset = 3;    // Fourth parameter (time)
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
    AudioChunk::Parameter params[4] = {};
    
    // Parameter 0: Frequency (for oscillator)
    params[0].name_hash = fnv1a_hash("frequency");
    params[0].default_value = frequency;
    params[0].min_value = 20.0f;
    params[0].max_value = 20000.0f;
    params[0].curve = 2.0f;  // Exponential curve for frequency
    params[0].flags = 0;
    
    // Parameter 1: Waveform type (for oscillator)
    params[1].name_hash = fnv1a_hash("waveform");
    params[1].default_value = static_cast<float>(waveform_type);
    params[1].min_value = 0.0f;
    params[1].max_value = 4.0f;  // 0-4 for our waveform types
    params[1].curve = 1.0f;  // Linear
    params[1].flags = 0;
    
    // Parameter 2: Amplitude (for amplifier)
    params[2].name_hash = fnv1a_hash("amplitude");
    params[2].default_value = 0.7f;
    params[2].min_value = 0.0f;
    params[2].max_value = 1.0f;
    params[2].curve = 1.0f;  // Linear
    params[2].flags = 0;
    
    // Parameter 3: Time (global)
    params[3].name_hash = fnv1a_hash("time");
    params[3].default_value = 0.0f;
    params[3].min_value = 0.0f;
    params[3].max_value = duration;
    params[3].curve = 1.0f;  // Linear
    params[3].flags = 0;
    
    // Write parameters
    std::memcpy(ptr, params, params_size);
    
    // Add the audio chunk to the asset
    std::string chunk_name = std::string(waveform_name) + "_wave_audio";
    asset.add_chunk(ChunkType::AUDI, chunk_data, chunk_name);
    
    // Save to file
    std::filesystem::create_directories(std::filesystem::path(output_path).parent_path());
    
    if (asset.save_to_file(output_path)) {
        std::cout << "✅ " << waveform_name << " wave audio asset created: " << output_path << std::endl;
        std::cout << "   📊 Total size: " << total_size << " bytes" << std::endl;
        std::cout << "   🎼 Audio graph: " << waveform_name << " Oscillator(" << frequency << "Hz) -> Amplifier(0.7) -> Output" << std::endl;
        return true;
    }
    
    return false;
}

// Backward compatibility - creates a sine wave
bool createSineWaveAudioAsset(const std::string& output_path, float frequency, float duration) {
    return createWaveformAudioAsset(output_path, frequency, duration, 0); // 0 = Sine wave
}

bool createMixerDemoAsset(const std::string& output_path, float duration) {
    std::cout << "🎛️ Creating mixer demo audio asset..." << std::endl;
    std::cout << "   Duration: " << duration << " seconds" << std::endl;
    std::cout << "   Mixing: Sine(440Hz) + Square(220Hz) + Triangle(880Hz)" << std::endl;
    
    using namespace Taffy;
    
    Asset asset;
    asset.set_creator("Taffy Mixer Demo Creator");
    asset.set_description("Mixer demo combining multiple waveforms");
    asset.set_feature_flags(FeatureFlags::Audio);
    
    // Create AudioChunk with mixer setup
    AudioChunk audioChunk{};
    audioChunk.node_count = 5;        // 3 Oscillators + 1 Mixer + 1 Amplifier
    audioChunk.connection_count = 4;  // 3 osc->mixer + mixer->amp
    audioChunk.pattern_count = 0;
    audioChunk.sample_count = 0;
    audioChunk.parameter_count = 10;  // freq*3 + waveform*3 + gain*3 + amplitude
    audioChunk.sample_rate = 48000;
    audioChunk.tick_rate = 0;
    
    // Calculate sizes
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
    AudioChunk::Node nodes[5] = {};
    
    // Node 0: Sine oscillator (440Hz)
    nodes[0].id = 0;
    nodes[0].type = AudioChunk::NodeType::Oscillator;
    nodes[0].name_hash = fnv1a_hash("sine_osc_440");
    nodes[0].position[0] = 100.0f;
    nodes[0].position[1] = 100.0f;
    nodes[0].input_count = 1;
    nodes[0].output_count = 1;
    nodes[0].param_offset = 0;  // freq, waveform
    nodes[0].param_count = 2;
    
    // Node 1: Square oscillator (220Hz)
    nodes[1].id = 1;
    nodes[1].type = AudioChunk::NodeType::Oscillator;
    nodes[1].name_hash = fnv1a_hash("square_osc_220");
    nodes[1].position[0] = 100.0f;
    nodes[1].position[1] = 200.0f;
    nodes[1].input_count = 1;
    nodes[1].output_count = 1;
    nodes[1].param_offset = 2;  // freq, waveform
    nodes[1].param_count = 2;
    
    // Node 2: Triangle oscillator (880Hz)
    nodes[2].id = 2;
    nodes[2].type = AudioChunk::NodeType::Oscillator;
    nodes[2].name_hash = fnv1a_hash("triangle_osc_880");
    nodes[2].position[0] = 100.0f;
    nodes[2].position[1] = 300.0f;
    nodes[2].input_count = 1;
    nodes[2].output_count = 1;
    nodes[2].param_offset = 4;  // freq, waveform
    nodes[2].param_count = 2;
    
    // Node 3: Mixer
    nodes[3].id = 3;
    nodes[3].type = AudioChunk::NodeType::Mixer;
    nodes[3].name_hash = fnv1a_hash("main_mixer");
    nodes[3].position[0] = 300.0f;
    nodes[3].position[1] = 200.0f;
    nodes[3].input_count = 3;     // 3 inputs from oscillators
    nodes[3].output_count = 1;
    nodes[3].param_offset = 6;     // gain_0, gain_1, gain_2, master_gain
    nodes[3].param_count = 4;
    
    // Node 4: Output Amplifier
    nodes[4].id = 4;
    nodes[4].type = AudioChunk::NodeType::Amplifier;
    nodes[4].name_hash = fnv1a_hash("output_amp");
    nodes[4].position[0] = 500.0f;
    nodes[4].position[1] = 200.0f;
    nodes[4].input_count = 2;
    nodes[4].output_count = 1;
    nodes[4].param_offset = 9;     // amplitude
    nodes[4].param_count = 1;
    
    // Write nodes
    std::memcpy(ptr, nodes, nodes_size);
    ptr += nodes_size;
    
    // Define connections
    AudioChunk::Connection connections[4] = {};
    
    // Oscillator connections to mixer
    connections[0].source_node = 0;  // Sine
    connections[0].source_output = 0;
    connections[0].dest_node = 3;    // Mixer
    connections[0].dest_input = 0;
    connections[0].strength = 1.0f;
    
    connections[1].source_node = 1;  // Square
    connections[1].source_output = 0;
    connections[1].dest_node = 3;    // Mixer
    connections[1].dest_input = 1;
    connections[1].strength = 1.0f;
    
    connections[2].source_node = 2;  // Triangle
    connections[2].source_output = 0;
    connections[2].dest_node = 3;    // Mixer
    connections[2].dest_input = 2;
    connections[2].strength = 1.0f;
    
    // Mixer to amplifier
    connections[3].source_node = 3;  // Mixer
    connections[3].source_output = 0;
    connections[3].dest_node = 4;    // Amplifier
    connections[3].dest_input = 0;
    connections[3].strength = 1.0f;
    
    // Write connections
    std::memcpy(ptr, connections, connections_size);
    ptr += connections_size;
    
    // Define parameters
    AudioChunk::Parameter params[10] = {};
    
    // Oscillator 0 (Sine) parameters
    params[0].name_hash = fnv1a_hash("frequency");
    params[0].default_value = 261.626f;
    params[0].min_value = 20.0f;
    params[0].max_value = 20000.0f;
    params[0].curve = 2.0f;
    
    params[1].name_hash = fnv1a_hash("waveform");
    params[1].default_value = 0.0f;  // Sine
    params[1].min_value = 0.0f;
    params[1].max_value = 4.0f;
    params[1].curve = 1.0f;
    
    // Oscillator 1 (Square) parameters
    params[2].name_hash = fnv1a_hash("frequency");
    params[2].default_value = 329.628f;
    params[2].min_value = 20.0f;
    params[2].max_value = 20000.0f;
    params[2].curve = 2.0f;
    
    params[3].name_hash = fnv1a_hash("waveform");
    params[3].default_value = 1.0f;  // Square
    params[3].min_value = 0.0f;
    params[3].max_value = 4.0f;
    params[3].curve = 1.0f;
    
    // Oscillator 2 (Triangle) parameters
    params[4].name_hash = fnv1a_hash("frequency");
    params[4].default_value = 391.995f;
    params[4].min_value = 20.0f;
    params[4].max_value = 20000.0f;
    params[4].curve = 2.0f;
    
    params[5].name_hash = fnv1a_hash("waveform");
    params[5].default_value = 3.0f;  // Triangle
    params[5].min_value = 0.0f;
    params[5].max_value = 4.0f;
    params[5].curve = 1.0f;
    
    // Mixer gain parameters
    params[6].name_hash = fnv1a_hash("gain_0");  // Sine gain
    params[6].default_value = 0.33f;
    params[6].min_value = 0.0f;
    params[6].max_value = 1.0f;
    params[6].curve = 1.0f;
    
    params[7].name_hash = fnv1a_hash("gain_1");  // Square gain
    params[7].default_value = 0.33f;
    params[7].min_value = 0.0f;
    params[7].max_value = 1.0f;
    params[7].curve = 1.0f;
    
    params[8].name_hash = fnv1a_hash("gain_2");  // Triangle gain
    params[8].default_value = 0.33f;
    params[8].min_value = 0.0f;
    params[8].max_value = 1.0f;
    params[8].curve = 1.0f;
    
    // Output amplifier parameter
    params[9].name_hash = fnv1a_hash("amplitude");
    params[9].default_value = 0.7f;  // Overall volume
    params[9].min_value = 0.0f;
    params[9].max_value = 1.0f;
    params[9].curve = 1.0f;
    
    // Write parameters
    std::memcpy(ptr, params, params_size);
    
    // Add the audio chunk to the asset
    asset.add_chunk(ChunkType::AUDI, chunk_data, "mixer_demo_audio");
    
    // Save to file
    std::filesystem::create_directories(std::filesystem::path(output_path).parent_path());
    
    if (asset.save_to_file(output_path)) {
        std::cout << "✅ Mixer demo audio asset created: " << output_path << std::endl;
        std::cout << "   📊 Total size: " << total_size << " bytes" << std::endl;
        std::cout << "   🎼 Audio graph:" << std::endl;
        std::cout << "      Sine(440Hz) ──┐" << std::endl;
        std::cout << "      Square(220Hz)─┼─→ Mixer → Amplifier → Output" << std::endl;
        std::cout << "      Triangle(880Hz)┘" << std::endl;
        std::cout << "   🎚️ Mix levels: Sine=50%, Square=30%, Triangle=20%" << std::endl;
        return true;
    }
    
    return false;
}

bool createADSRDemoAsset(const std::string& output_path, float duration) {
    std::cout << "🎹 Creating ADSR envelope demo audio asset..." << std::endl;
    std::cout << "   Duration: " << duration << " seconds" << std::endl;
    std::cout << "   Gate: On for 1s, off for rest (to hear release)" << std::endl;
    
    using namespace Taffy;
    
    Asset asset;
    asset.set_creator("Taffy ADSR Demo Creator");
    asset.set_description("ADSR envelope demonstration");
    asset.set_feature_flags(FeatureFlags::Audio);
    
    // Create AudioChunk with ADSR setup
    AudioChunk audioChunk{};
    audioChunk.node_count = 4;        // Gate parameter, Envelope, Oscillator, Amplifier
    audioChunk.connection_count = 3;  // gate->env, osc->amp, env->amp
    audioChunk.pattern_count = 0;
    audioChunk.sample_count = 0;
    audioChunk.parameter_count = 8;   // gate, attack, decay, sustain, release, freq, waveform, amplitude
    audioChunk.sample_rate = 48000;
    audioChunk.tick_rate = 0;
    
    // Calculate sizes
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
    AudioChunk::Node nodes[4] = {};
    
    // Node 0: Gate parameter (outputs 1 for first second, then 0)
    nodes[0].id = 0;
    nodes[0].type = AudioChunk::NodeType::Parameter;
    nodes[0].name_hash = fnv1a_hash("gate_generator");
    nodes[0].position[0] = 100.0f;
    nodes[0].position[1] = 100.0f;
    nodes[0].input_count = 0;
    nodes[0].output_count = 1;
    nodes[0].param_offset = 0;  // gate parameter
    nodes[0].param_count = 1;
    
    // Node 1: ADSR Envelope
    nodes[1].id = 1;
    nodes[1].type = AudioChunk::NodeType::Envelope;
    nodes[1].name_hash = fnv1a_hash("adsr_envelope");
    nodes[1].position[0] = 300.0f;
    nodes[1].position[1] = 100.0f;
    nodes[1].input_count = 1;     // Gate input
    nodes[1].output_count = 1;    // Envelope output
    nodes[1].param_offset = 1;    // attack, decay, sustain, release
    nodes[1].param_count = 4;
    
    // Node 2: Sine oscillator
    nodes[2].id = 2;
    nodes[2].type = AudioChunk::NodeType::Oscillator;
    nodes[2].name_hash = fnv1a_hash("tone_oscillator");
    nodes[2].position[0] = 300.0f;
    nodes[2].position[1] = 200.0f;
    nodes[2].input_count = 1;
    nodes[2].output_count = 1;
    nodes[2].param_offset = 5;    // frequency, waveform
    nodes[2].param_count = 2;
    
    // Node 3: Output Amplifier (modulated by envelope)
    nodes[3].id = 3;
    nodes[3].type = AudioChunk::NodeType::Amplifier;
    nodes[3].name_hash = fnv1a_hash("envelope_amp");
    nodes[3].position[0] = 500.0f;
    nodes[3].position[1] = 150.0f;
    nodes[3].input_count = 2;     // Audio input + envelope modulation
    nodes[3].output_count = 1;
    nodes[3].param_offset = 7;    // amplitude
    nodes[3].param_count = 1;
    
    // Write nodes
    std::memcpy(ptr, nodes, nodes_size);
    ptr += nodes_size;
    
    // Define connections
    AudioChunk::Connection connections[3] = {};
    
    // Gate to envelope
    connections[0].source_node = 0;  // Gate parameter
    connections[0].source_output = 0;
    connections[0].dest_node = 1;    // Envelope
    connections[0].dest_input = 0;
    connections[0].strength = 1.0f;
    
    // Oscillator to amplifier (audio input)
    connections[1].source_node = 2;  // Oscillator
    connections[1].source_output = 0;
    connections[1].dest_node = 3;    // Amplifier
    connections[1].dest_input = 0;
    connections[1].strength = 1.0f;
    
    // Envelope to amplifier (modulation input)
    connections[2].source_node = 1;  // Envelope
    connections[2].source_output = 0;
    connections[2].dest_node = 3;    // Amplifier
    connections[2].dest_input = 1;   // Modulation input
    connections[2].strength = 1.0f;
    
    // Write connections
    std::memcpy(ptr, connections, connections_size);
    ptr += connections_size;
    
    // Define parameters
    AudioChunk::Parameter params[8] = {};
    
    // Parameter 0: Gate (1 for first second, 0 after)
    params[0].name_hash = fnv1a_hash("gate");
    params[0].default_value = 1.0f;  // Start with gate on
    params[0].min_value = 0.0f;
    params[0].max_value = 1.0f;
    params[0].curve = 1.0f;
    
    // ADSR parameters
    params[1].name_hash = fnv1a_hash("attack");
    params[1].default_value = 0.1f;   // 100ms attack
    params[1].min_value = 0.001f;
    params[1].max_value = 2.0f;
    params[1].curve = 2.0f;  // Exponential
    
    params[2].name_hash = fnv1a_hash("decay");
    params[2].default_value = 0.2f;   // 200ms decay
    params[2].min_value = 0.001f;
    params[2].max_value = 2.0f;
    params[2].curve = 2.0f;
    
    params[3].name_hash = fnv1a_hash("sustain");
    params[3].default_value = 0.6f;   // 60% sustain level
    params[3].min_value = 0.0f;
    params[3].max_value = 1.0f;
    params[3].curve = 1.0f;
    
    params[4].name_hash = fnv1a_hash("release");
    params[4].default_value = 0.5f;   // 500ms release
    params[4].min_value = 0.001f;
    params[4].max_value = 3.0f;
    params[4].curve = 2.0f;
    
    // Oscillator parameters
    params[5].name_hash = fnv1a_hash("frequency");
    params[5].default_value = 440.0f;  // A4
    params[5].min_value = 20.0f;
    params[5].max_value = 20000.0f;
    params[5].curve = 2.0f;
    
    params[6].name_hash = fnv1a_hash("waveform");
    params[6].default_value = 0.0f;  // Sine
    params[6].min_value = 0.0f;
    params[6].max_value = 4.0f;
    params[6].curve = 1.0f;
    
    // Amplifier parameter
    params[7].name_hash = fnv1a_hash("amplitude");
    params[7].default_value = 0.8f;  // Overall volume
    params[7].min_value = 0.0f;
    params[7].max_value = 1.0f;
    params[7].curve = 1.0f;
    
    // Write parameters
    std::memcpy(ptr, params, params_size);
    
    // Add the audio chunk to the asset
    asset.add_chunk(ChunkType::AUDI, chunk_data, "adsr_demo_audio");
    
    // Save to file
    std::filesystem::create_directories(std::filesystem::path(output_path).parent_path());
    
    if (asset.save_to_file(output_path)) {
        std::cout << "✅ ADSR demo audio asset created: " << output_path << std::endl;
        std::cout << "   📊 Total size: " << total_size << " bytes" << std::endl;
        std::cout << "   🎼 Audio graph:" << std::endl;
        std::cout << "      Gate ────────────→ ADSR Envelope ─┐" << std::endl;
        std::cout << "                                        ↓ (modulation)" << std::endl;
        std::cout << "      Sine(440Hz) ────────────────→ Amplifier → Output" << std::endl;
        std::cout << "   ⏱️ ADSR: Attack=100ms, Decay=200ms, Sustain=60%, Release=500ms" << std::endl;
        std::cout << "   🎵 Note: Gate turns off after 1 second to demonstrate release phase" << std::endl;
        return true;
    }
    
    return false;
}

bool createFilterDemoAsset(const std::string& output_path, uint32_t filterType, float duration) {
    // Filter demo with oscillator -> filter -> amplifier
    // Also includes envelope for filter sweeps
    std::vector<uint8_t> audioChunk;
    
    // Audio chunk header
    Taffy::AudioChunk header;
    std::memset(&header, 0, sizeof(header));
    header.node_count = 5;         // oscillator, envelope, filter, amplifier, cutoff parameter
    header.connection_count = 4;   // osc->filter, env->filter(cutoff), filter->amp, param->env
    header.parameter_count = 11;   // frequency, waveform, attack, decay, sustain, release, cutoff, resonance, type, amplitude, gate
    header.sample_rate = 48000;
    
    // Create nodes
    std::vector<Taffy::AudioChunk::Node> nodes;
    
    // Node 0: Oscillator (sawtooth for rich harmonics)
    Taffy::AudioChunk::Node oscillator;
    std::memset(&oscillator, 0, sizeof(oscillator));
    oscillator.id = 0;
    oscillator.type = Taffy::AudioChunk::NodeType::Oscillator;
    oscillator.name_hash = Taffy::fnv1a_hash("saw_oscillator");
    oscillator.input_count = 0;
    oscillator.output_count = 1;
    oscillator.param_offset = 0;   // frequency, waveform
    oscillator.param_count = 2;
    nodes.push_back(oscillator);
    
    // Node 1: Envelope for filter sweep
    Taffy::AudioChunk::Node envelope;
    std::memset(&envelope, 0, sizeof(envelope));
    envelope.id = 1;
    envelope.type = Taffy::AudioChunk::NodeType::Envelope;
    envelope.name_hash = Taffy::fnv1a_hash("filter_envelope");
    envelope.input_count = 1;      // gate input
    envelope.output_count = 1;
    envelope.param_offset = 2;     // attack, decay, sustain, release
    envelope.param_count = 4;
    nodes.push_back(envelope);
    
    // Node 2: Filter
    Taffy::AudioChunk::Node filter;
    std::memset(&filter, 0, sizeof(filter));
    filter.id = 2;
    filter.type = Taffy::AudioChunk::NodeType::Filter;
    filter.name_hash = Taffy::fnv1a_hash("demo_filter");
    filter.input_count = 2;        // audio in, cutoff modulation
    filter.output_count = 1;
    filter.param_offset = 6;       // cutoff, resonance, type
    filter.param_count = 3;
    nodes.push_back(filter);
    
    // Node 3: Amplifier
    Taffy::AudioChunk::Node amplifier;
    std::memset(&amplifier, 0, sizeof(amplifier));
    amplifier.id = 3;
    amplifier.type = Taffy::AudioChunk::NodeType::Amplifier;
    amplifier.name_hash = Taffy::fnv1a_hash("output_amp");
    amplifier.input_count = 1;
    amplifier.output_count = 1;
    amplifier.param_offset = 9;    // amplitude
    amplifier.param_count = 1;
    nodes.push_back(amplifier);
    
    // Node 4: Gate parameter
    Taffy::AudioChunk::Node gate;
    std::memset(&gate, 0, sizeof(gate));
    gate.id = 4;
    gate.type = Taffy::AudioChunk::NodeType::Parameter;
    gate.name_hash = Taffy::fnv1a_hash("gate_param");
    gate.input_count = 0;
    gate.output_count = 1;
    gate.param_offset = 10;        // gate value
    gate.param_count = 1;
    nodes.push_back(gate);
    
    // Create connections
    std::vector<Taffy::AudioChunk::Connection> connections;
    
    // Oscillator -> Filter (audio input)
    Taffy::AudioChunk::Connection conn1;
    conn1.source_node = 0;
    conn1.source_output = 0;
    conn1.dest_node = 2;
    conn1.dest_input = 0;
    conn1.strength = 1.0f;
    connections.push_back(conn1);
    
    // Envelope -> Filter (cutoff modulation)
    Taffy::AudioChunk::Connection conn2;
    conn2.source_node = 1;
    conn2.source_output = 0;
    conn2.dest_node = 2;
    conn2.dest_input = 1;
    conn2.strength = 5000.0f;  // Modulation depth in Hz
    connections.push_back(conn2);
    
    // Filter -> Amplifier
    Taffy::AudioChunk::Connection conn3;
    conn3.source_node = 2;
    conn3.source_output = 0;
    conn3.dest_node = 3;
    conn3.dest_input = 0;
    conn3.strength = 1.0f;
    connections.push_back(conn3);
    
    // Gate -> Envelope
    Taffy::AudioChunk::Connection conn4;
    conn4.source_node = 4;
    conn4.source_output = 0;
    conn4.dest_node = 1;
    conn4.dest_input = 0;
    conn4.strength = 1.0f;
    connections.push_back(conn4);
    
    // Create parameters
    std::vector<Taffy::AudioChunk::Parameter> parameters;
    
    // Oscillator frequency
    Taffy::AudioChunk::Parameter freq;
    freq.name_hash = Taffy::fnv1a_hash("frequency");
    freq.default_value = 110.0f;   // A2 - low frequency for filter demo
    freq.min_value = 20.0f;
    freq.max_value = 20000.0f;
    parameters.push_back(freq);
    
    // Oscillator waveform (sawtooth)
    Taffy::AudioChunk::Parameter waveform;
    waveform.name_hash = Taffy::fnv1a_hash("waveform");
    waveform.default_value = 2.0f;  // Saw wave
    waveform.min_value = 0.0f;
    waveform.max_value = 4.0f;
    parameters.push_back(waveform);
    
    // Envelope parameters
    Taffy::AudioChunk::Parameter attack;
    attack.name_hash = Taffy::fnv1a_hash("attack");
    attack.default_value = 0.5f;    // 500ms attack
    attack.min_value = 0.001f;
    attack.max_value = 10.0f;
    parameters.push_back(attack);
    
    Taffy::AudioChunk::Parameter decay;
    decay.name_hash = Taffy::fnv1a_hash("decay");
    decay.default_value = 0.7f;     // 700ms decay
    decay.min_value = 0.001f;
    decay.max_value = 10.0f;
    parameters.push_back(decay);
    
    Taffy::AudioChunk::Parameter sustain;
    sustain.name_hash = Taffy::fnv1a_hash("sustain");
    sustain.default_value = 0.0f;   // No sustain - sweep down to base cutoff
    sustain.min_value = 0.0f;
    sustain.max_value = 1.0f;
    parameters.push_back(sustain);
    
    Taffy::AudioChunk::Parameter release;
    release.name_hash = Taffy::fnv1a_hash("release");
    release.default_value = 0.3f;    // 300ms release
    release.min_value = 0.001f;
    release.max_value = 10.0f;
    parameters.push_back(release);
    
    // Filter parameters
    Taffy::AudioChunk::Parameter cutoff;
    cutoff.name_hash = Taffy::fnv1a_hash("cutoff");
    cutoff.default_value = 200.0f;   // Base cutoff frequency
    cutoff.min_value = 20.0f;
    cutoff.max_value = 20000.0f;
    parameters.push_back(cutoff);
    
    Taffy::AudioChunk::Parameter resonance;
    resonance.name_hash = Taffy::fnv1a_hash("resonance");
    resonance.default_value = 5.0f;   // High resonance for dramatic effect
    resonance.min_value = 0.1f;
    resonance.max_value = 20.0f;
    parameters.push_back(resonance);
    
    Taffy::AudioChunk::Parameter type;
    type.name_hash = Taffy::fnv1a_hash("type");
    type.default_value = static_cast<float>(filterType);
    type.min_value = 0.0f;
    type.max_value = 2.0f;
    parameters.push_back(type);
    
    // Amplifier amplitude
    Taffy::AudioChunk::Parameter amplitude;
    amplitude.name_hash = Taffy::fnv1a_hash("amplitude");
    amplitude.default_value = 0.7f;
    amplitude.min_value = 0.0f;
    amplitude.max_value = 1.0f;
    parameters.push_back(amplitude);
    
    // Gate parameter (triggers envelope every second)
    Taffy::AudioChunk::Parameter gateParam;
    gateParam.name_hash = Taffy::fnv1a_hash("gate");
    gateParam.default_value = 1.0f;
    gateParam.min_value = 0.0f;
    gateParam.max_value = 1.0f;
    parameters.push_back(gateParam);
    
    // Build audio chunk data
    audioChunk.resize(sizeof(Taffy::AudioChunk) + 
                     nodes.size() * sizeof(Taffy::AudioChunk::Node) +
                     connections.size() * sizeof(Taffy::AudioChunk::Connection) +
                     parameters.size() * sizeof(Taffy::AudioChunk::Parameter));
    
    uint8_t* ptr = audioChunk.data();
    
    // Write header
    std::memcpy(ptr, &header, sizeof(header));
    ptr += sizeof(header);
    
    // Write nodes
    std::memcpy(ptr, nodes.data(), nodes.size() * sizeof(Taffy::AudioChunk::Node));
    ptr += nodes.size() * sizeof(Taffy::AudioChunk::Node);
    
    // Write connections
    std::memcpy(ptr, connections.data(), connections.size() * sizeof(Taffy::AudioChunk::Connection));
    ptr += connections.size() * sizeof(Taffy::AudioChunk::Connection);
    
    // Write parameters
    std::memcpy(ptr, parameters.data(), parameters.size() * sizeof(Taffy::AudioChunk::Parameter));
    
    // Create Taffy asset
    Taffy::Asset asset;
    const char* filterNames[] = {"lowpass", "highpass", "bandpass"};
    asset.set_creator("Taffy Filter Demo Creator");
    asset.set_description(std::string("Filter demonstration: ") + filterNames[filterType] + " with envelope sweep");
    asset.set_feature_flags(Taffy::FeatureFlags::Audio);
    
    // Add audio chunk with descriptive name
    asset.add_chunk(Taffy::ChunkType::AUDI, audioChunk, std::string("filter_") + filterNames[filterType] + "_demo_audio");
    
    // Save to file
    std::filesystem::create_directories(std::filesystem::path(output_path).parent_path());
    
    if (asset.save_to_file(output_path)) {
        std::cout << "✅ Filter demo audio asset created: " << output_path << std::endl;
        std::cout << "   🎼 Audio graph:" << std::endl;
        std::cout << "      Saw(110Hz) → Filter(" << filterNames[filterType] << ") → Amplifier → Output" << std::endl;
        std::cout << "                      ↑" << std::endl;
        std::cout << "                  Envelope" << std::endl;
        std::cout << "   🎚️ Filter sweep: 200Hz → 5200Hz → 200Hz" << std::endl;
        std::cout << "   ⚡ Resonance: 5.0 (high resonance for dramatic effect)" << std::endl;
        return true;
    }
    
    return false;
}

bool createDistortionDemoAsset(const std::string& output_path, uint32_t distortionType, float duration) {
    // Distortion demo with oscillator -> distortion -> amplifier
    std::vector<uint8_t> audioChunk;
    
    // Audio chunk header
    Taffy::AudioChunk header;
    std::memset(&header, 0, sizeof(header));
    header.node_count = 3;         // oscillator, distortion, amplifier
    header.connection_count = 2;   // osc->dist, dist->amp
    header.parameter_count = 6;    // frequency, waveform, drive, mix, type, amplitude
    header.sample_rate = 48000;
    
    // Create nodes
    std::vector<Taffy::AudioChunk::Node> nodes;
    
    // Node 0: Oscillator (sine wave for clean input)
    Taffy::AudioChunk::Node oscillator;
    std::memset(&oscillator, 0, sizeof(oscillator));
    oscillator.id = 0;
    oscillator.type = Taffy::AudioChunk::NodeType::Oscillator;
    oscillator.name_hash = Taffy::fnv1a_hash("input_oscillator");
    oscillator.input_count = 0;
    oscillator.output_count = 1;
    oscillator.param_offset = 0;   // frequency, waveform
    oscillator.param_count = 2;
    nodes.push_back(oscillator);
    
    // Node 1: Distortion
    Taffy::AudioChunk::Node distortion;
    std::memset(&distortion, 0, sizeof(distortion));
    distortion.id = 1;
    distortion.type = Taffy::AudioChunk::NodeType::Distortion;
    distortion.name_hash = Taffy::fnv1a_hash("demo_distortion");
    distortion.input_count = 1;    // audio in
    distortion.output_count = 1;
    distortion.param_offset = 2;   // drive, mix, type
    distortion.param_count = 3;
    nodes.push_back(distortion);
    
    // Node 2: Amplifier
    Taffy::AudioChunk::Node amplifier;
    std::memset(&amplifier, 0, sizeof(amplifier));
    amplifier.id = 2;
    amplifier.type = Taffy::AudioChunk::NodeType::Amplifier;
    amplifier.name_hash = Taffy::fnv1a_hash("output_amp");
    amplifier.input_count = 1;
    amplifier.output_count = 1;
    amplifier.param_offset = 5;    // amplitude
    amplifier.param_count = 1;
    nodes.push_back(amplifier);
    
    // Create connections
    std::vector<Taffy::AudioChunk::Connection> connections;
    
    // Oscillator -> Distortion
    Taffy::AudioChunk::Connection conn1;
    conn1.source_node = 0;
    conn1.source_output = 0;
    conn1.dest_node = 1;
    conn1.dest_input = 0;
    conn1.strength = 1.0f;
    connections.push_back(conn1);
    
    // Distortion -> Amplifier
    Taffy::AudioChunk::Connection conn2;
    conn2.source_node = 1;
    conn2.source_output = 0;
    conn2.dest_node = 2;
    conn2.dest_input = 0;
    conn2.strength = 1.0f;
    connections.push_back(conn2);
    
    // Create parameters
    std::vector<Taffy::AudioChunk::Parameter> parameters;
    
    // Oscillator frequency - use a low frequency to hear distortion clearly
    Taffy::AudioChunk::Parameter freq;
    freq.name_hash = Taffy::fnv1a_hash("frequency");
    // Higher frequency for beeper to sound more authentic
    freq.default_value = (distortionType == 5) ? 880.0f : 440.0f;   // A5 for beeper, A4 for others
    freq.min_value = 20.0f;
    freq.max_value = 20000.0f;
    parameters.push_back(freq);
    
    // Oscillator waveform (sine for clean input, except sawtooth for beeper)
    Taffy::AudioChunk::Parameter waveform;
    waveform.name_hash = Taffy::fnv1a_hash("waveform");
    // Use sawtooth for beeper to get more harmonics, sine for others
    waveform.default_value = (distortionType == 5) ? 2.0f : 0.0f;  // Saw for beeper, sine for others
    waveform.min_value = 0.0f;
    waveform.max_value = 4.0f;
    parameters.push_back(waveform);
    
    // Distortion drive
    Taffy::AudioChunk::Parameter drive;
    drive.name_hash = Taffy::fnv1a_hash("drive");
    // Different drive amounts for different distortion types - much higher!
    float driveAmounts[] = {10.0f, 5.0f, 8.0f, 6.0f, 12.0f, 2.0f}; // HardClip, SoftClip, Foldback, BitCrush, Overdrive, Beeper
    drive.default_value = driveAmounts[distortionType];
    drive.min_value = 0.1f;
    drive.max_value = 20.0f;
    parameters.push_back(drive);
    
    // Distortion mix (100% wet)
    Taffy::AudioChunk::Parameter mix;
    mix.name_hash = Taffy::fnv1a_hash("mix");
    mix.default_value = 1.0f;
    mix.min_value = 0.0f;
    mix.max_value = 1.0f;
    parameters.push_back(mix);
    
    // Distortion type
    Taffy::AudioChunk::Parameter type;
    type.name_hash = Taffy::fnv1a_hash("type");
    type.default_value = static_cast<float>(distortionType);
    type.min_value = 0.0f;
    type.max_value = 5.0f;
    parameters.push_back(type);
    
    // Amplifier amplitude - reduced to prevent clipping
    Taffy::AudioChunk::Parameter amplitude;
    amplitude.name_hash = Taffy::fnv1a_hash("amplitude");
    amplitude.default_value = 0.3f;
    amplitude.min_value = 0.0f;
    amplitude.max_value = 1.0f;
    parameters.push_back(amplitude);
    
    // Build audio chunk data
    audioChunk.resize(sizeof(Taffy::AudioChunk) + 
                     nodes.size() * sizeof(Taffy::AudioChunk::Node) +
                     connections.size() * sizeof(Taffy::AudioChunk::Connection) +
                     parameters.size() * sizeof(Taffy::AudioChunk::Parameter));
    
    uint8_t* ptr = audioChunk.data();
    
    // Write header
    std::memcpy(ptr, &header, sizeof(header));
    ptr += sizeof(header);
    
    // Write nodes
    std::memcpy(ptr, nodes.data(), nodes.size() * sizeof(Taffy::AudioChunk::Node));
    ptr += nodes.size() * sizeof(Taffy::AudioChunk::Node);
    
    // Write connections
    std::memcpy(ptr, connections.data(), connections.size() * sizeof(Taffy::AudioChunk::Connection));
    ptr += connections.size() * sizeof(Taffy::AudioChunk::Connection);
    
    // Write parameters
    std::memcpy(ptr, parameters.data(), parameters.size() * sizeof(Taffy::AudioChunk::Parameter));
    
    // Debug output
    std::cout << "🔍 Distortion demo parameters:" << std::endl;
    for (size_t i = 0; i < parameters.size(); ++i) {
        std::cout << "   Param " << i << ": hash=0x" << std::hex << parameters[i].name_hash << std::dec
                  << ", value=" << parameters[i].default_value << std::endl;
    }
    
    // Create Taffy asset
    Taffy::Asset asset;
    const char* distortionNames[] = {"hardclip", "softclip", "foldback", "bitcrush", "overdrive", "beeper"};
    asset.set_creator("Taffy Distortion Demo Creator");
    asset.set_description(std::string("Distortion demonstration: ") + distortionNames[distortionType]);
    asset.set_feature_flags(Taffy::FeatureFlags::Audio);
    
    // Add audio chunk with descriptive name
    asset.add_chunk(Taffy::ChunkType::AUDI, audioChunk, std::string("distortion_") + distortionNames[distortionType] + "_demo_audio");
    
    // Save to file
    std::filesystem::create_directories(std::filesystem::path(output_path).parent_path());
    
    if (asset.save_to_file(output_path)) {
        std::cout << "✅ Distortion demo audio asset created: " << output_path << std::endl;
        std::cout << "   🎼 Audio graph:" << std::endl;
        const char* waveformName = (distortionType == 5) ? "Saw" : "Sine";
        float freqValue = (distortionType == 5) ? 880.0f : 440.0f;
        std::cout << "      " << waveformName << "(" << freqValue << "Hz) → Distortion(" << distortionNames[distortionType] << ") → Amplifier → Output" << std::endl;
        std::cout << "   🎚️ Drive: " << driveAmounts[distortionType] << "x" << std::endl;
        std::cout << "   💥 100% wet signal for maximum effect" << std::endl;
        return true;
    }
    
    return false;
}

bool createSampleAudioAsset(const std::string& output_path,
                           const std::vector<float>& sampleData,
                           uint32_t sampleRate,
                           uint32_t channelCount,
                           float baseFrequency,
                           uint32_t loopStart,
                           uint32_t loopEnd) {
    
    std::cout << "🎵 Creating sample-based audio asset..." << std::endl;
    std::cout << "   Sample rate: " << sampleRate << " Hz" << std::endl;
    std::cout << "   Channels: " << channelCount << std::endl;
    std::cout << "   Samples: " << sampleData.size() / channelCount << std::endl;
    std::cout << "   Base frequency: " << baseFrequency << " Hz" << std::endl;
    if (loopEnd > loopStart) {
        std::cout << "   Loop: " << loopStart << " - " << loopEnd << std::endl;
    }
    
    using namespace Taffy;
    
    Asset asset;
    asset.set_creator("Taffy Sample Creator");
    asset.set_description("Sample-based audio asset");
    asset.set_feature_flags(FeatureFlags::Audio);
    
    // Create AudioChunk
    AudioChunk audioChunk{};
    audioChunk.node_count = 4;        // Gate, Sampler, Amplifier, Output
    audioChunk.connection_count = 3;  // Gate->Sampler, Sampler->Amp, Output
    audioChunk.pattern_count = 0;
    audioChunk.sample_count = 1;      // One sample
    audioChunk.parameter_count = 7;   // gate, sample_index, pitch, start_position, loop, amplitude, time
    audioChunk.sample_rate = sampleRate;
    audioChunk.tick_rate = 0;
    
    // Calculate sizes
    size_t header_size = sizeof(AudioChunk);
    size_t nodes_size = audioChunk.node_count * sizeof(AudioChunk::Node);
    size_t connections_size = audioChunk.connection_count * sizeof(AudioChunk::Connection);
    size_t params_size = audioChunk.parameter_count * sizeof(AudioChunk::Parameter);
    size_t wavetable_size = sizeof(AudioChunk::WaveTable);
    
    // Convert float samples to 16-bit for storage
    std::vector<int16_t> samples16(sampleData.size());
    for (size_t i = 0; i < sampleData.size(); ++i) {
        float clamped = std::max(-1.0f, std::min(1.0f, sampleData[i]));
        samples16[i] = static_cast<int16_t>(clamped * 32767.0f);
    }
    size_t sample_data_size = samples16.size() * sizeof(int16_t);
    
    size_t total_size = header_size + nodes_size + connections_size + params_size + 
                       wavetable_size + sample_data_size;
    
    // Create chunk data buffer
    std::vector<uint8_t> chunk_data(total_size);
    uint8_t* ptr = chunk_data.data();
    
    // Write header
    std::memcpy(ptr, &audioChunk, header_size);
    ptr += header_size;
    
    // Define nodes
    std::vector<AudioChunk::Node> nodes(4);
    
    // Node 0: Gate parameter
    nodes[0].id = 0;
    nodes[0].type = AudioChunk::NodeType::Parameter;
    nodes[0].name_hash = fnv1a_hash("gate_parameter");
    nodes[0].position[0] = 100.0f;
    nodes[0].position[1] = 100.0f;
    nodes[0].input_count = 0;
    nodes[0].output_count = 1;
    nodes[0].param_offset = 0;    // gate parameter
    nodes[0].param_count = 1;
    
    // Node 1: Sampler
    nodes[1].id = 1;
    nodes[1].type = AudioChunk::NodeType::Sampler;
    nodes[1].name_hash = fnv1a_hash("main_sampler");
    nodes[1].position[0] = 300.0f;
    nodes[1].position[1] = 100.0f;
    nodes[1].input_count = 2;     // Trigger input + pitch modulation
    nodes[1].output_count = 1;    // Audio output
    nodes[1].param_offset = 1;    // sample_index, pitch, start_position, loop
    nodes[1].param_count = 4;
    
    // Node 2: Amplifier
    nodes[2].id = 2;
    nodes[2].type = AudioChunk::NodeType::Amplifier;
    nodes[2].name_hash = fnv1a_hash("main_amplifier");
    nodes[2].position[0] = 500.0f;
    nodes[2].position[1] = 100.0f;
    nodes[2].input_count = 2;     // Audio input + amplitude control
    nodes[2].output_count = 1;    // Audio output
    nodes[2].param_offset = 5;    // amplitude parameter
    nodes[2].param_count = 1;
    
    // Node 3: Time parameter
    nodes[3].id = 3;
    nodes[3].type = AudioChunk::NodeType::Parameter;
    nodes[3].name_hash = fnv1a_hash("time_parameter");
    nodes[3].position[0] = 100.0f;
    nodes[3].position[1] = 200.0f;
    nodes[3].input_count = 0;
    nodes[3].output_count = 1;
    nodes[3].param_offset = 6;    // time parameter
    nodes[3].param_count = 1;
    
    // Write nodes
    std::memcpy(ptr, nodes.data(), nodes_size);
    ptr += nodes_size;
    
    // Define connections
    std::vector<AudioChunk::Connection> connections(3);
    
    // Connection 0: Gate -> Sampler trigger
    connections[0].source_node = 0;
    connections[0].source_output = 0;
    connections[0].dest_node = 1;
    connections[0].dest_input = 0;
    connections[0].strength = 1.0f;
    
    // Connection 1: Sampler -> Amplifier
    connections[1].source_node = 1;
    connections[1].source_output = 0;
    connections[1].dest_node = 2;
    connections[1].dest_input = 0;
    connections[1].strength = 1.0f;
    
    // Connection 2: Time -> could be used for modulation
    connections[2].source_node = 3;
    connections[2].source_output = 0;
    connections[2].dest_node = 1;
    connections[2].dest_input = 1;
    connections[2].strength = 0.0f;  // Not active by default
    
    // Write connections
    std::memcpy(ptr, connections.data(), connections_size);
    ptr += connections_size;
    
    // Define parameters
    std::vector<AudioChunk::Parameter> params(7);
    
    // Parameter 0: Gate
    params[0].name_hash = fnv1a_hash("gate");
    params[0].default_value = 0.0f;  // Start at 0 so first pulse creates rising edge
    params[0].min_value = 0.0f;
    params[0].max_value = 1.0f;
    params[0].curve = 1.0f;
    params[0].flags = 0;
    
    // Parameter 1: Sample index
    params[1].name_hash = fnv1a_hash("sample_index");
    params[1].default_value = 0.0f;  // First (and only) sample
    params[1].min_value = 0.0f;
    params[1].max_value = 0.0f;
    params[1].curve = 1.0f;
    params[1].flags = 0;
    
    // Parameter 2: Pitch
    params[2].name_hash = fnv1a_hash("pitch");
    params[2].default_value = 1.0f;  // Normal speed
    params[2].min_value = 0.25f;
    params[2].max_value = 4.0f;
    params[2].curve = 1.0f;
    params[2].flags = 0;
    
    // Parameter 3: Start position
    params[3].name_hash = fnv1a_hash("start_position");
    params[3].default_value = 0.0f;
    params[3].min_value = 0.0f;
    params[3].max_value = 1.0f;
    params[3].curve = 1.0f;
    params[3].flags = 0;
    
    // Parameter 4: Loop
    params[4].name_hash = fnv1a_hash("loop");
    params[4].default_value = (loopEnd > loopStart) ? 1.0f : 0.0f;
    params[4].min_value = 0.0f;
    params[4].max_value = 1.0f;
    params[4].curve = 1.0f;
    params[4].flags = 0;
    
    // Parameter 5: Amplitude
    params[5].name_hash = fnv1a_hash("amplitude");
    params[5].default_value = 0.7f;
    params[5].min_value = 0.0f;
    params[5].max_value = 1.0f;
    params[5].curve = 1.0f;
    params[5].flags = 0;
    
    // Parameter 6: Time
    params[6].name_hash = fnv1a_hash("time");
    params[6].default_value = 0.0f;
    params[6].min_value = 0.0f;
    params[6].max_value = 10.0f;
    params[6].curve = 1.0f;
    params[6].flags = 0;
    
    // Write parameters
    std::memcpy(ptr, params.data(), params_size);
    ptr += params_size;
    
    // Define wavetable
    AudioChunk::WaveTable wavetable{};
    wavetable.name_hash = fnv1a_hash("main_sample");
    wavetable.sample_count = sampleData.size() / channelCount;
    wavetable.channel_count = channelCount;
    wavetable.bit_depth = 16;
    wavetable.data_offset = header_size + nodes_size + connections_size + params_size + wavetable_size;
    wavetable.data_size = sample_data_size;
    wavetable.base_frequency = baseFrequency;
    wavetable.loop_start = loopStart;
    wavetable.loop_end = loopEnd;
    
    // Write wavetable
    std::memcpy(ptr, &wavetable, wavetable_size);
    ptr += wavetable_size;
    
    // Write sample data
    std::memcpy(ptr, samples16.data(), sample_data_size);
    
    // Add the audio chunk to the asset
    asset.add_chunk(ChunkType::AUDI, chunk_data, "sample_audio");
    
    // Save to file
    std::filesystem::create_directories(std::filesystem::path(output_path).parent_path());
    
    if (asset.save_to_file(output_path)) {
        std::cout << "✅ Sample audio asset created: " << output_path << std::endl;
        std::cout << "   📊 Total size: " << total_size << " bytes" << std::endl;
        std::cout << "   🎼 Audio graph: Gate → Sampler → Amplifier → Output" << std::endl;
        return true;
    }
    
    return false;
}

// Simple WAV file loader
bool loadWAVFile(const std::string& wavPath,
                 std::vector<float>& outData,
                 uint32_t& outSampleRate,
                 uint32_t& outChannelCount) {
    
    std::ifstream file(wavPath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "❌ Failed to open WAV file: " << wavPath << std::endl;
        return false;
    }
    
    // WAV header structure
    struct WAVHeader {
        char riff[4];        // "RIFF"
        uint32_t fileSize;
        char wave[4];        // "WAVE"
        char fmt[4];         // "fmt "
        uint32_t fmtSize;
        uint16_t format;     // 1 = PCM
        uint16_t channels;
        uint32_t sampleRate;
        uint32_t byteRate;
        uint16_t blockAlign;
        uint16_t bitsPerSample;
    };
    
    WAVHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(WAVHeader));
    
    // Validate header
    if (std::strncmp(header.riff, "RIFF", 4) != 0 ||
        std::strncmp(header.wave, "WAVE", 4) != 0 ||
        std::strncmp(header.fmt, "fmt ", 4) != 0) {
        std::cerr << "❌ Invalid WAV file format" << std::endl;
        return false;
    }
    
    if (header.format != 1) {
        std::cerr << "❌ Only PCM WAV files are supported" << std::endl;
        return false;
    }
    
    // Skip any extra fmt data
    if (header.fmtSize > 16) {
        file.seekg(header.fmtSize - 16, std::ios::cur);
    }
    
    // Find data chunk
    char chunkId[4];
    uint32_t chunkSize;
    
    while (file.read(chunkId, 4)) {
        file.read(reinterpret_cast<char*>(&chunkSize), 4);
        
        if (std::strncmp(chunkId, "data", 4) == 0) {
            // Found data chunk
            break;
        } else {
            // Skip this chunk
            file.seekg(chunkSize, std::ios::cur);
        }
    }
    
    if (file.eof()) {
        std::cerr << "❌ No data chunk found in WAV file" << std::endl;
        return false;
    }
    
    // Read sample data
    outSampleRate = header.sampleRate;
    outChannelCount = header.channels;
    
    uint32_t bytesPerSample = header.bitsPerSample / 8;
    uint32_t totalSamples = chunkSize / bytesPerSample;
    outData.resize(totalSamples);
    
    // Read and convert to float
    if (header.bitsPerSample == 8) {
        std::vector<uint8_t> data8(totalSamples);
        file.read(reinterpret_cast<char*>(data8.data()), chunkSize);
        for (size_t i = 0; i < totalSamples; ++i) {
            outData[i] = (static_cast<float>(data8[i]) - 128.0f) / 128.0f;
        }
    } else if (header.bitsPerSample == 16) {
        std::vector<int16_t> data16(totalSamples);
        file.read(reinterpret_cast<char*>(data16.data()), chunkSize);
        for (size_t i = 0; i < totalSamples; ++i) {
            outData[i] = static_cast<float>(data16[i]) / 32768.0f;
        }
    } else if (header.bitsPerSample == 24) {
        std::vector<uint8_t> data24(chunkSize);
        file.read(reinterpret_cast<char*>(data24.data()), chunkSize);
        for (size_t i = 0; i < totalSamples; ++i) {
            int32_t val = 0;
            std::memcpy(&val, &data24[i * 3], 3);
            if (val & 0x800000) val |= 0xFF000000;  // Sign extend
            outData[i] = static_cast<float>(val) / 8388608.0f;
        }
    } else if (header.bitsPerSample == 32) {
        if (header.format == 3) {
            // Float format
            file.read(reinterpret_cast<char*>(outData.data()), chunkSize);
        } else {
            // 32-bit integer
            std::vector<int32_t> data32(totalSamples);
            file.read(reinterpret_cast<char*>(data32.data()), chunkSize);
            for (size_t i = 0; i < totalSamples; ++i) {
                outData[i] = static_cast<float>(data32[i]) / 2147483648.0f;
            }
        }
    } else {
        std::cerr << "❌ Unsupported bit depth: " << header.bitsPerSample << std::endl;
        return false;
    }
    
    std::cout << "✅ Loaded WAV file: " << wavPath << std::endl;
    std::cout << "   Sample rate: " << outSampleRate << " Hz" << std::endl;
    std::cout << "   Channels: " << outChannelCount << std::endl;
    std::cout << "   Samples: " << totalSamples / outChannelCount << std::endl;
    std::cout << "   Bit depth: " << header.bitsPerSample << std::endl;
    
    return true;
}

bool createBitcrushedImportAsset(const std::string& output_path) {
    std::cout << "🎵 Creating bit-crushed imported sample asset..." << std::endl;
    
    using namespace Taffy;
    
    // First, load the original imported sample to get the wavetable data
    Asset originalAsset;
    if (!originalAsset.load_from_file_safe("assets/audio/imported_sample.taf")) {
        std::cerr << "❌ Could not load imported_sample.taf - make sure you've imported your song first!" << std::endl;
        return false;
    }
    
    auto originalAudioData = originalAsset.get_chunk_data(ChunkType::AUDI);
    if (!originalAudioData) {
        std::cerr << "❌ No audio chunk in imported sample!" << std::endl;
        return false;
    }
    
    // Extract the wavetable from the original
    const uint8_t* origPtr = originalAudioData->data();
    AudioChunk origHeader;
    std::memcpy(&origHeader, origPtr, sizeof(AudioChunk));
    origPtr += sizeof(AudioChunk);
    
    // Skip to the wavetable
    size_t skipSize = origHeader.node_count * sizeof(AudioChunk::Node) +
                      origHeader.connection_count * sizeof(AudioChunk::Connection) +
                      origHeader.parameter_count * sizeof(AudioChunk::Parameter);
    origPtr += skipSize;
    
    // Read wavetable
    AudioChunk::WaveTable origWavetable;
    std::memcpy(&origWavetable, origPtr, sizeof(AudioChunk::WaveTable));
    origPtr += sizeof(AudioChunk::WaveTable);
    
    // Now create a new asset with bit crusher
    Asset asset;
    asset.set_creator("Taffy BitCrush Sample Creator");
    asset.set_description("Imported sample with bit crusher effect");
    asset.set_feature_flags(FeatureFlags::Audio);
    
    // Audio chunk header
    AudioChunk header;
    std::memset(&header, 0, sizeof(header));
    header.node_count = 5;         // gate parameter, sampler, distortion, amplifier, time parameter
    header.connection_count = 4;   // gate->sampler, sampler->distortion, distortion->amp, time->sampler
    header.parameter_count = 10;   // gate, sample_index, pitch, start_position, loop, drive, mix, type, amplitude, time
    header.sample_rate = origHeader.sample_rate;
    header.sample_count = 1;       // We have the imported sample
    
    // Calculate sizes
    size_t headerSize = sizeof(AudioChunk);
    size_t nodesSize = header.node_count * sizeof(AudioChunk::Node);
    size_t connectionsSize = header.connection_count * sizeof(AudioChunk::Connection);
    size_t paramsSize = header.parameter_count * sizeof(AudioChunk::Parameter);
    size_t wavetableSize = sizeof(AudioChunk::WaveTable);
    size_t totalSize = headerSize + nodesSize + connectionsSize + paramsSize + 
                       wavetableSize + origWavetable.data_size;
    
    // Create audio chunk data
    std::vector<uint8_t> audioChunk(totalSize);
    uint8_t* ptr = audioChunk.data();
    
    // Write header
    std::memcpy(ptr, &header, headerSize);
    ptr += headerSize;
    
    // Create nodes
    std::vector<AudioChunk::Node> nodes;
    
    // Node 0: Gate parameter
    AudioChunk::Node gate;
    std::memset(&gate, 0, sizeof(gate));
    gate.id = 0;
    gate.type = AudioChunk::NodeType::Parameter;
    gate.name_hash = fnv1a_hash("gate_parameter");
    gate.position[0] = 100.0f;
    gate.position[1] = 100.0f;
    gate.input_count = 0;
    gate.output_count = 1;
    gate.param_offset = 0;
    gate.param_count = 1;
    nodes.push_back(gate);
    
    // Node 1: Sampler
    AudioChunk::Node sampler;
    std::memset(&sampler, 0, sizeof(sampler));
    sampler.id = 1;
    sampler.type = AudioChunk::NodeType::Sampler;
    sampler.name_hash = fnv1a_hash("imported_sampler");
    sampler.position[0] = 300.0f;
    sampler.position[1] = 100.0f;
    sampler.input_count = 2;     // trigger, pitch mod
    sampler.output_count = 1;
    sampler.param_offset = 1;    // sample_index, pitch, start_position, loop
    sampler.param_count = 4;
    nodes.push_back(sampler);
    
    // Node 2: Bit Crusher (Distortion)
    AudioChunk::Node distortion;
    std::memset(&distortion, 0, sizeof(distortion));
    distortion.id = 2;
    distortion.type = AudioChunk::NodeType::Distortion;
    distortion.name_hash = fnv1a_hash("bit_crusher");
    distortion.position[0] = 500.0f;
    distortion.position[1] = 100.0f;
    distortion.input_count = 1;
    distortion.output_count = 1;
    distortion.param_offset = 5;   // drive, mix, type
    distortion.param_count = 3;
    nodes.push_back(distortion);
    
    // Node 3: Amplifier
    AudioChunk::Node amplifier;
    std::memset(&amplifier, 0, sizeof(amplifier));
    amplifier.id = 3;
    amplifier.type = AudioChunk::NodeType::Amplifier;
    amplifier.name_hash = fnv1a_hash("output_amp");
    amplifier.position[0] = 700.0f;
    amplifier.position[1] = 100.0f;
    amplifier.input_count = 2;
    amplifier.output_count = 1;
    amplifier.param_offset = 8;    // amplitude
    amplifier.param_count = 1;
    nodes.push_back(amplifier);
    
    // Node 4: Time parameter
    AudioChunk::Node timeParam;
    std::memset(&timeParam, 0, sizeof(timeParam));
    timeParam.id = 4;
    timeParam.type = AudioChunk::NodeType::Parameter;
    timeParam.name_hash = fnv1a_hash("time_parameter");
    timeParam.position[0] = 100.0f;
    timeParam.position[1] = 200.0f;
    timeParam.input_count = 0;
    timeParam.output_count = 1;
    timeParam.param_offset = 9;
    timeParam.param_count = 1;
    nodes.push_back(timeParam);
    
    // Write nodes
    std::memcpy(ptr, nodes.data(), nodes.size() * sizeof(AudioChunk::Node));
    ptr += nodes.size() * sizeof(AudioChunk::Node);
    
    // Create connections
    std::vector<AudioChunk::Connection> connections;
    
    // Gate -> Sampler trigger
    AudioChunk::Connection conn1;
    conn1.source_node = 0;
    conn1.source_output = 0;
    conn1.dest_node = 1;
    conn1.dest_input = 0;
    conn1.strength = 1.0f;
    connections.push_back(conn1);
    
    // Sampler -> Bit Crusher
    AudioChunk::Connection conn2;
    conn2.source_node = 1;
    conn2.source_output = 0;
    conn2.dest_node = 2;
    conn2.dest_input = 0;
    conn2.strength = 1.0f;
    connections.push_back(conn2);
    
    // Bit Crusher -> Amplifier
    AudioChunk::Connection conn3;
    conn3.source_node = 2;
    conn3.source_output = 0;
    conn3.dest_node = 3;
    conn3.dest_input = 0;
    conn3.strength = 1.0f;
    connections.push_back(conn3);
    
    // Time -> Sampler (pitch modulation, inactive)
    AudioChunk::Connection conn4;
    conn4.source_node = 4;
    conn4.source_output = 0;
    conn4.dest_node = 1;
    conn4.dest_input = 1;
    conn4.strength = 0.0f;
    connections.push_back(conn4);
    
    // Write connections
    std::memcpy(ptr, connections.data(), connections.size() * sizeof(AudioChunk::Connection));
    ptr += connections.size() * sizeof(AudioChunk::Connection);
    
    // Create parameters
    std::vector<AudioChunk::Parameter> parameters;
    
    // Gate (starts at 1.0 to trigger sample playback)
    AudioChunk::Parameter gateParam;
    gateParam.name_hash = fnv1a_hash("gate");
    gateParam.default_value = 1.0f;  // Start triggered
    gateParam.min_value = 0.0f;
    gateParam.max_value = 1.0f;
    gateParam.curve = 1.0f;
    parameters.push_back(gateParam);
    
    // Sample index
    AudioChunk::Parameter sampleIndex;
    sampleIndex.name_hash = fnv1a_hash("sample_index");
    sampleIndex.default_value = 0.0f;
    sampleIndex.min_value = 0.0f;
    sampleIndex.max_value = 0.0f;
    sampleIndex.curve = 1.0f;
    parameters.push_back(sampleIndex);
    
    // Pitch
    AudioChunk::Parameter pitch;
    pitch.name_hash = fnv1a_hash("pitch");
    pitch.default_value = 1.0f;
    pitch.min_value = 0.25f;
    pitch.max_value = 4.0f;
    pitch.curve = 1.0f;
    parameters.push_back(pitch);
    
    // Start position
    AudioChunk::Parameter startPos;
    startPos.name_hash = fnv1a_hash("start_position");
    startPos.default_value = 0.0f;
    startPos.min_value = 0.0f;
    startPos.max_value = 1.0f;
    startPos.curve = 1.0f;
    parameters.push_back(startPos);
    
    // Loop
    AudioChunk::Parameter loop;
    loop.name_hash = fnv1a_hash("loop");
    loop.default_value = 0.0f;
    loop.min_value = 0.0f;
    loop.max_value = 1.0f;
    loop.curve = 1.0f;
    parameters.push_back(loop);
    
    // Bit crusher drive
    AudioChunk::Parameter drive;
    drive.name_hash = fnv1a_hash("drive");
    drive.default_value = 1.0f;  // Moderate bit crushing
    drive.min_value = 0.1f;
    drive.max_value = 20.0f;
    drive.curve = 1.0f;
    parameters.push_back(drive);
    
    // Mix
    AudioChunk::Parameter mix;
    mix.name_hash = fnv1a_hash("mix");
    mix.default_value = 0.33f;  // 100% wet
    mix.min_value = 0.0f;
    mix.max_value = 1.0f;
    mix.curve = 1.0f;
    parameters.push_back(mix);
    
    // Type (3 = BitCrush)
    AudioChunk::Parameter type;
    type.name_hash = fnv1a_hash("type");
    type.default_value = 5.0f;  // BitCrush
    type.min_value = 0.0f;
    type.max_value = 5.0f;
    type.curve = 1.0f;
    parameters.push_back(type);
    
    // Amplitude
    AudioChunk::Parameter amplitude;
    amplitude.name_hash = fnv1a_hash("amplitude");
    amplitude.default_value = 1.0f;  // Reduced to prevent clipping
    amplitude.min_value = 0.0f;
    amplitude.max_value = 1.0f;
    amplitude.curve = 1.0f;
    parameters.push_back(amplitude);
    
    // Time
    AudioChunk::Parameter time;
    time.name_hash = fnv1a_hash("time");
    time.default_value = 0.0f;
    time.min_value = 0.0f;
    time.max_value = 10.0f;
    time.curve = 1.0f;
    parameters.push_back(time);
    
    // Write parameters
    std::memcpy(ptr, parameters.data(), parameters.size() * sizeof(AudioChunk::Parameter));
    ptr += parameters.size() * sizeof(AudioChunk::Parameter);
    
    // Copy wavetable with updated offset
    AudioChunk::WaveTable wavetable = origWavetable;
    wavetable.data_offset = headerSize + nodesSize + connectionsSize + paramsSize + wavetableSize;
    
    // Write wavetable
    std::memcpy(ptr, &wavetable, wavetableSize);
    ptr += wavetableSize;
    
    // Copy sample data
    std::memcpy(ptr, origPtr, origWavetable.data_size);
    
    // Add chunk to asset
    asset.add_chunk(ChunkType::AUDI, audioChunk, "bitcrushed_import_audio");
    
    // Save
    std::filesystem::create_directories(std::filesystem::path(output_path).parent_path());
    
    if (asset.save_to_file(output_path)) {
        std::cout << "✅ Bit-crushed import asset created: " << output_path << std::endl;
        std::cout << "   🎼 Audio graph:" << std::endl;
        std::cout << "      Gate → Sampler → BitCrusher → Amplifier → Output" << std::endl;
        std::cout << "   🎚️ Bit crusher drive: 6.0x" << std::endl;
        std::cout << "   💥 100% wet signal for bit crushing effect" << std::endl;
        std::cout << "   🎵 Your song will now be permanently bit-crushed in this TAF file!" << std::endl;
        return true;
    }
    
    return false;
}

bool createStreamingAudioAsset(const std::string& inputWavPath,
                               const std::string& outputPath,
                               uint32_t chunkSizeMs) {
    // Load WAV file header to get info without loading all data
    std::ifstream file(inputWavPath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open WAV file: " << inputWavPath << std::endl;
        return false;
    }
    
    // Read WAV header
    char riff[4];
    file.read(riff, 4);
    if (std::string(riff, 4) != "RIFF") {
        std::cerr << "Not a valid WAV file: missing RIFF header" << std::endl;
        return false;
    }
    
    uint32_t fileSize;
    file.read(reinterpret_cast<char*>(&fileSize), 4);
    
    char wave[4];
    file.read(wave, 4);
    if (std::string(wave, 4) != "WAVE") {
        std::cerr << "Not a valid WAV file: missing WAVE header" << std::endl;
        return false;
    }
    
    // Find and read fmt chunk
    uint32_t sampleRate = 0;
    uint16_t channelCount = 0;
    uint16_t bitsPerSample = 0;
    uint32_t dataSize = 0;
    uint64_t dataOffset = 0;
    
    while (!file.eof()) {
        char chunkID[4];
        file.read(chunkID, 4);
        if (file.eof()) break;
        
        uint32_t chunkSize;
        file.read(reinterpret_cast<char*>(&chunkSize), 4);
        
        if (std::string(chunkID, 4) == "fmt ") {
            uint16_t audioFormat;
            file.read(reinterpret_cast<char*>(&audioFormat), 2);
            file.read(reinterpret_cast<char*>(&channelCount), 2);
            file.read(reinterpret_cast<char*>(&sampleRate), 4);
            file.seekg(6, std::ios::cur); // Skip byte rate and block align
            file.read(reinterpret_cast<char*>(&bitsPerSample), 2);
            
            if (audioFormat != 1 && audioFormat != 3) { // 1 = PCM, 3 = float
                std::cerr << "Unsupported audio format: " << audioFormat << std::endl;
                return false;
            }
        } else if (std::string(chunkID, 4) == "data") {
            dataSize = chunkSize;
            dataOffset = file.tellg();
            break;
        } else {
            file.seekg(chunkSize, std::ios::cur);
        }
    }
    
    if (dataOffset == 0) {
        std::cerr << "No data chunk found in WAV file" << std::endl;
        return false;
    }
    
    file.close();
    
    // Calculate streaming parameters
    uint32_t bytesPerSample = bitsPerSample / 8;
    uint32_t totalSamples = dataSize / (bytesPerSample * channelCount);
    uint32_t samplesPerChunk = (sampleRate * chunkSizeMs) / 1000;
    uint32_t bytesPerChunk = samplesPerChunk * bytesPerSample * channelCount;
    uint32_t chunkCount = (totalSamples + samplesPerChunk - 1) / samplesPerChunk;
    
    std::cout << "Creating streaming audio asset:" << std::endl;
    std::cout << "  Input: " << inputWavPath << std::endl;
    std::cout << "  Sample rate: " << sampleRate << " Hz" << std::endl;
    std::cout << "  Channels: " << channelCount << std::endl;
    std::cout << "  Bits per sample: " << bitsPerSample << std::endl;
    std::cout << "  Total samples: " << totalSamples << " (" 
              << static_cast<float>(totalSamples) / sampleRate << " seconds)" << std::endl;
    std::cout << "  Chunk size: " << chunkSizeMs << " ms (" << samplesPerChunk << " samples)" << std::endl;
    std::cout << "  Total chunks: " << chunkCount << std::endl;
    
    // CREATE PROPER TAFFY ASSET using Asset API
    Taffy::Asset asset;
    asset.set_creator("Taffy Streaming Audio Creator");
    asset.set_description("Streaming audio from WAV file");
    asset.set_feature_flags(Taffy::FeatureFlags::Audio);
    
    // Create audio chunk in memory
    std::vector<uint8_t> audioChunkData;
    
    // Audio chunk header
    Taffy::AudioChunk audioHeader;
    audioHeader.node_count = 3;        // Parameter -> StreamingSampler -> Amplifier
    audioHeader.connection_count = 2;   // Two connections
    audioHeader.pattern_count = 0;     // No tracker patterns
    audioHeader.sample_count = 0;       // No embedded samples (using streaming)
    audioHeader.parameter_count = 5;    // gate, stream_index, pitch, start_position, amplitude
    audioHeader.sample_rate = sampleRate;
    audioHeader.tick_rate = 0;         // Not used for streaming
    audioHeader.streaming_count = 1;    // One streaming audio
    
    // Initialize all reserved fields to 0
    std::memset(audioHeader.reserved, 0, sizeof(audioHeader.reserved));
    
    audioChunkData.resize(sizeof(Taffy::AudioChunk));
    std::memcpy(audioChunkData.data(), &audioHeader, sizeof(Taffy::AudioChunk));
    
    // Node 0: Parameter (gate)
    Taffy::AudioChunk::Node paramNode;
    paramNode.id = 0;
    paramNode.type = Taffy::AudioChunk::NodeType::Parameter;
    paramNode.input_count = 0;
    paramNode.output_count = 1;
    paramNode.param_offset = 0;
    paramNode.param_count = 1; // gate
    audioChunkData.insert(audioChunkData.end(),
                         reinterpret_cast<uint8_t*>(&paramNode),
                         reinterpret_cast<uint8_t*>(&paramNode) + sizeof(paramNode));
    
    // Node 1: StreamingSampler
    Taffy::AudioChunk::Node streamNode;
    streamNode.id = 1;
    streamNode.type = Taffy::AudioChunk::NodeType::StreamingSampler;
    streamNode.input_count = 1;
    streamNode.output_count = 1;
    streamNode.param_offset = 1;
    streamNode.param_count = 3; // stream_index, pitch, start_position
    audioChunkData.insert(audioChunkData.end(),
                         reinterpret_cast<uint8_t*>(&streamNode),
                         reinterpret_cast<uint8_t*>(&streamNode) + sizeof(streamNode));
    
    // Node 2: Amplifier
    Taffy::AudioChunk::Node ampNode;
    ampNode.id = 2;
    ampNode.type = Taffy::AudioChunk::NodeType::Amplifier;
    ampNode.input_count = 2;
    ampNode.output_count = 1;
    ampNode.param_offset = 4;
    ampNode.param_count = 1; // amplitude
    audioChunkData.insert(audioChunkData.end(),
                         reinterpret_cast<uint8_t*>(&ampNode),
                         reinterpret_cast<uint8_t*>(&ampNode) + sizeof(ampNode));
    
    // Connection 0: Parameter (gate) -> StreamingSampler
    Taffy::AudioChunk::Connection conn1;
    conn1.source_node = 0;
    conn1.source_output = 0;
    conn1.dest_node = 1;
    conn1.dest_input = 0;
    conn1.strength = 1.0f;
    audioChunkData.insert(audioChunkData.end(),
                         reinterpret_cast<uint8_t*>(&conn1),
                         reinterpret_cast<uint8_t*>(&conn1) + sizeof(conn1));
    
    // Connection 1: StreamingSampler -> Amplifier
    Taffy::AudioChunk::Connection conn2;
    conn2.source_node = 1;
    conn2.source_output = 0;
    conn2.dest_node = 2;
    conn2.dest_input = 0;
    conn2.strength = 1.0f;
    audioChunkData.insert(audioChunkData.end(),
                         reinterpret_cast<uint8_t*>(&conn2),
                         reinterpret_cast<uint8_t*>(&conn2) + sizeof(conn2));
    
    // Parameters
    // gate
    Taffy::AudioChunk::Parameter gateParam;
    gateParam.name_hash = Taffy::fnv1a_hash("gate");
    gateParam.default_value = 0.0f;
    gateParam.min_value = 0.0f;
    gateParam.max_value = 1.0f;
    audioChunkData.insert(audioChunkData.end(),
                         reinterpret_cast<uint8_t*>(&gateParam),
                         reinterpret_cast<uint8_t*>(&gateParam) + sizeof(gateParam));
    
    // stream_index
    Taffy::AudioChunk::Parameter streamIndexParam;
    streamIndexParam.name_hash = Taffy::fnv1a_hash("stream_index");
    streamIndexParam.default_value = 0.0f;
    streamIndexParam.min_value = 0.0f;
    streamIndexParam.max_value = 10.0f;
    audioChunkData.insert(audioChunkData.end(),
                         reinterpret_cast<uint8_t*>(&streamIndexParam),
                         reinterpret_cast<uint8_t*>(&streamIndexParam) + sizeof(streamIndexParam));
    
    // pitch
    Taffy::AudioChunk::Parameter pitchParam;
    pitchParam.name_hash = Taffy::fnv1a_hash("pitch");
    pitchParam.default_value = 1.0f;
    pitchParam.min_value = 0.1f;
    pitchParam.max_value = 4.0f;
    audioChunkData.insert(audioChunkData.end(),
                         reinterpret_cast<uint8_t*>(&pitchParam),
                         reinterpret_cast<uint8_t*>(&pitchParam) + sizeof(pitchParam));
    
    // start_position
    Taffy::AudioChunk::Parameter startPosParam;
    startPosParam.name_hash = Taffy::fnv1a_hash("start_position");
    startPosParam.default_value = 0.0f;
    startPosParam.min_value = 0.0f;
    startPosParam.max_value = 1.0f;
    audioChunkData.insert(audioChunkData.end(),
                         reinterpret_cast<uint8_t*>(&startPosParam),
                         reinterpret_cast<uint8_t*>(&startPosParam) + sizeof(startPosParam));
    
    // amplitude
    Taffy::AudioChunk::Parameter ampParam;
    ampParam.name_hash = Taffy::fnv1a_hash("amplitude");
    ampParam.default_value = 1.0f;
    ampParam.min_value = 0.0f;
    ampParam.max_value = 2.0f;
    audioChunkData.insert(audioChunkData.end(),
                         reinterpret_cast<uint8_t*>(&ampParam),
                         reinterpret_cast<uint8_t*>(&ampParam) + sizeof(ampParam));
    
    // Streaming audio info
    Taffy::AudioChunk::StreamingAudio streamInfo;
    std::memset(&streamInfo, 0, sizeof(streamInfo)); // Initialize entire struct to 0
    streamInfo.name_hash = Taffy::fnv1a_hash("main_stream");
    streamInfo.sample_rate = sampleRate;
    streamInfo.channel_count = channelCount;
    streamInfo.bit_depth = bitsPerSample;
    streamInfo.total_samples = totalSamples;
    streamInfo.chunk_size = samplesPerChunk;
    streamInfo.chunk_count = chunkCount;
    // Data offset will be from the start of the audio chunk data
    streamInfo.data_offset = audioChunkData.size() + sizeof(streamInfo);
    streamInfo.format = (bitsPerSample == 32) ? 1 : 0; // 1 for float, 0 for PCM
    
    audioChunkData.insert(audioChunkData.end(),
                         reinterpret_cast<uint8_t*>(&streamInfo),
                         reinterpret_cast<uint8_t*>(&streamInfo) + sizeof(streamInfo));
    
    // Now add the actual audio data from the WAV file
    std::ifstream wavFile(inputWavPath, std::ios::binary);
    wavFile.seekg(dataOffset);
    
    // Read and append audio data in chunks to avoid loading entire file at once
    const size_t copyBufferSize = 1024 * 1024; // 1MB buffer
    std::vector<uint8_t> buffer(copyBufferSize);
    size_t remaining = dataSize;
    
    while (remaining > 0) {
        size_t toRead = std::min(remaining, copyBufferSize);
        wavFile.read(reinterpret_cast<char*>(buffer.data()), toRead);
        audioChunkData.insert(audioChunkData.end(), buffer.begin(), buffer.begin() + toRead);
        remaining -= toRead;
    }
    
    wavFile.close();
    
    // Add audio chunk to asset
    asset.add_chunk(Taffy::ChunkType::AUDI, audioChunkData, "streaming_audio");
    
    // Save asset
    std::cout << "💾 Saving streaming asset to: " << outputPath << std::endl;
    
    // Ensure directory exists
    auto parentPath = std::filesystem::path(outputPath).parent_path();
    if (!parentPath.empty() && !std::filesystem::exists(parentPath)) {
        std::filesystem::create_directories(parentPath);
    }
    
    if (!asset.save_to_file(outputPath)) {
        std::cerr << "❌ Failed to save streaming asset!" << std::endl;
        return false;
    }
    
    std::cout << "✅ Streaming audio asset created successfully!" << std::endl;
    std::cout << "   📊 Total TAF size: " << (audioChunkData.size() / (1024.0 * 1024.0)) << " MB" << std::endl;
    std::cout << "   🎵 Duration: " << static_cast<float>(totalSamples) / sampleRate << " seconds" << std::endl;
    std::cout << "   📦 Chunk size: " << chunkSizeMs << " ms" << std::endl;
    std::cout << "   🔄 Total chunks: " << chunkCount << std::endl;
    std::cout << "   📍 Audio data offset in chunk: " << streamInfo.data_offset << std::endl;
    
    return true;
}

} // namespace tremor::taffy::tools

namespace Taffy {


} // namespace Taffy

namespace tremor::taffy::tools {

    bool createStreamingTestAsset(const std::string& outputPath) {
        std::cout << "🎵 Creating test streaming audio asset..." << std::endl;
        std::cout << "   Output path: " << outputPath << std::endl;
        
        // Generate 10 seconds of 440Hz sine wave at 48kHz
        const uint32_t sampleRate = 48000;
        const uint32_t duration = 10; // seconds
        const uint32_t totalSamples = sampleRate * duration;
        const uint32_t chunkSize = sampleRate / 2; // 500ms chunks
        const uint32_t chunkCount = (totalSamples + chunkSize - 1) / chunkSize;
        
        std::cout << "   Total samples: " << totalSamples << std::endl;
        std::cout << "   Chunk size: " << chunkSize << " samples (500ms)" << std::endl;
        std::cout << "   Total chunks: " << chunkCount << std::endl;
        
        // Create TAF asset
        Taffy::Asset asset;
        asset.set_creator("Taffy Streaming Test Creator");
        asset.set_description("Test streaming audio with 10 second sine wave");
        asset.set_feature_flags(Taffy::FeatureFlags::Audio);
        
        // Create audio chunk
        std::vector<uint8_t> audioChunkData;
        
        // Audio chunk header
        Taffy::AudioChunk audioHeader;
        audioHeader.node_count = 3;        // StreamingSampler -> Amplifier -> Parameter
        audioHeader.connection_count = 2;   // Two connections
        audioHeader.pattern_count = 0;     // No tracker patterns
        audioHeader.sample_count = 0;       // No embedded samples
        audioHeader.parameter_count = 5;    // stream_index, pitch, start_position, amplitude, gate
        audioHeader.sample_rate = sampleRate;
        audioHeader.tick_rate = 0;         // Not used for streaming
        audioHeader.streaming_count = 1;    // One streaming audio
        
        // Initialize all reserved fields to 0
        std::memset(audioHeader.reserved, 0, sizeof(audioHeader.reserved));
        
        audioChunkData.resize(sizeof(Taffy::AudioChunk));
        std::memcpy(audioChunkData.data(), &audioHeader, sizeof(Taffy::AudioChunk));
        
        // Node 0: StreamingSampler
        Taffy::AudioChunk::Node streamNode;
        streamNode.id = 0;
        streamNode.type = Taffy::AudioChunk::NodeType::StreamingSampler;
        streamNode.input_count = 1;
        streamNode.output_count = 1;
        streamNode.param_offset = 0;
        streamNode.param_count = 3; // stream_index, pitch, start_position
        audioChunkData.insert(audioChunkData.end(), 
                             reinterpret_cast<uint8_t*>(&streamNode),
                             reinterpret_cast<uint8_t*>(&streamNode) + sizeof(streamNode));
        
        // Node 1: Amplifier
        Taffy::AudioChunk::Node ampNode;
        ampNode.id = 1;
        ampNode.type = Taffy::AudioChunk::NodeType::Amplifier;
        ampNode.input_count = 2;
        ampNode.output_count = 1;
        ampNode.param_offset = 3;
        ampNode.param_count = 1; // amplitude
        audioChunkData.insert(audioChunkData.end(),
                             reinterpret_cast<uint8_t*>(&ampNode),
                             reinterpret_cast<uint8_t*>(&ampNode) + sizeof(ampNode));
        
        // Node 2: Parameter (gate)
        Taffy::AudioChunk::Node paramNode;
        paramNode.id = 2;
        paramNode.type = Taffy::AudioChunk::NodeType::Parameter;
        paramNode.input_count = 0;
        paramNode.output_count = 1;
        paramNode.param_offset = 4;
        paramNode.param_count = 1; // gate
        audioChunkData.insert(audioChunkData.end(),
                             reinterpret_cast<uint8_t*>(&paramNode),
                             reinterpret_cast<uint8_t*>(&paramNode) + sizeof(paramNode));
        
        // Connection 0: StreamingSampler -> Amplifier
        Taffy::AudioChunk::Connection conn1;
        conn1.source_node = 0;
        conn1.source_output = 0;
        conn1.dest_node = 1;
        conn1.dest_input = 0;
        conn1.strength = 1.0f;
        audioChunkData.insert(audioChunkData.end(),
                             reinterpret_cast<uint8_t*>(&conn1),
                             reinterpret_cast<uint8_t*>(&conn1) + sizeof(conn1));
        
        // Connection 1: Parameter (gate) -> StreamingSampler
        Taffy::AudioChunk::Connection conn2;
        conn2.source_node = 2;
        conn2.source_output = 0;
        conn2.dest_node = 0;
        conn2.dest_input = 0;
        conn2.strength = 1.0f;
        audioChunkData.insert(audioChunkData.end(),
                             reinterpret_cast<uint8_t*>(&conn2),
                             reinterpret_cast<uint8_t*>(&conn2) + sizeof(conn2));
        
        // Parameters
        // stream_index
        Taffy::AudioChunk::Parameter streamIndexParam;
        streamIndexParam.name_hash = Taffy::fnv1a_hash("stream_index");
        streamIndexParam.default_value = 0.0f;
        streamIndexParam.min_value = 0.0f;
        streamIndexParam.max_value = 10.0f;
        audioChunkData.insert(audioChunkData.end(),
                             reinterpret_cast<uint8_t*>(&streamIndexParam),
                             reinterpret_cast<uint8_t*>(&streamIndexParam) + sizeof(streamIndexParam));
        
        // pitch
        Taffy::AudioChunk::Parameter pitchParam;
        pitchParam.name_hash = Taffy::fnv1a_hash("pitch");
        pitchParam.default_value = 1.0f;
        pitchParam.min_value = 0.1f;
        pitchParam.max_value = 4.0f;
        audioChunkData.insert(audioChunkData.end(),
                             reinterpret_cast<uint8_t*>(&pitchParam),
                             reinterpret_cast<uint8_t*>(&pitchParam) + sizeof(pitchParam));
        
        // start_position
        Taffy::AudioChunk::Parameter startPosParam;
        startPosParam.name_hash = Taffy::fnv1a_hash("start_position");
        startPosParam.default_value = 0.0f;
        startPosParam.min_value = 0.0f;
        startPosParam.max_value = 1.0f;
        audioChunkData.insert(audioChunkData.end(),
                             reinterpret_cast<uint8_t*>(&startPosParam),
                             reinterpret_cast<uint8_t*>(&startPosParam) + sizeof(startPosParam));
        
        // amplitude
        Taffy::AudioChunk::Parameter ampParam;
        ampParam.name_hash = Taffy::fnv1a_hash("amplitude");
        ampParam.default_value = 0.7f;
        ampParam.min_value = 0.0f;
        ampParam.max_value = 1.0f;
        audioChunkData.insert(audioChunkData.end(),
                             reinterpret_cast<uint8_t*>(&ampParam),
                             reinterpret_cast<uint8_t*>(&ampParam) + sizeof(ampParam));
        
        // gate
        Taffy::AudioChunk::Parameter gateParam;
        gateParam.name_hash = Taffy::fnv1a_hash("gate");
        gateParam.default_value = 0.0f;
        gateParam.min_value = 0.0f;
        gateParam.max_value = 1.0f;
        audioChunkData.insert(audioChunkData.end(),
                             reinterpret_cast<uint8_t*>(&gateParam),
                             reinterpret_cast<uint8_t*>(&gateParam) + sizeof(gateParam));
        
        // Streaming audio info
        Taffy::AudioChunk::StreamingAudio streamInfo;
        std::memset(&streamInfo, 0, sizeof(streamInfo)); // Initialize entire struct to 0
        streamInfo.name_hash = Taffy::fnv1a_hash("test_stream");
        streamInfo.sample_rate = sampleRate;
        streamInfo.channel_count = 1;
        streamInfo.bit_depth = 32;
        streamInfo.total_samples = totalSamples;
        streamInfo.chunk_size = chunkSize;
        streamInfo.chunk_count = chunkCount;
        // Data offset will be from the start of the audio chunk data
        streamInfo.data_offset = audioChunkData.size() + sizeof(streamInfo);
        streamInfo.format = 1; // Float format
        
        audioChunkData.insert(audioChunkData.end(),
                             reinterpret_cast<uint8_t*>(&streamInfo),
                             reinterpret_cast<uint8_t*>(&streamInfo) + sizeof(streamInfo));
        
        // Generate and append audio data
        std::cout << "   Generating sine wave data..." << std::endl;
        const float frequency = 440.0f;
        const float twoPi = 2.0f * 3.14159265f;
        
        for (uint32_t i = 0; i < totalSamples; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(sampleRate);
            float sample = std::sin(twoPi * frequency * t) * 0.8f;
            audioChunkData.insert(audioChunkData.end(),
                                 reinterpret_cast<uint8_t*>(&sample),
                                 reinterpret_cast<uint8_t*>(&sample) + sizeof(sample));
        }
        
        // Add audio chunk to asset
        asset.add_chunk(Taffy::ChunkType::AUDI, audioChunkData, "streaming_test_audio");
        
        // Save asset
        std::cout << "💾 Saving streaming asset to: " << outputPath << std::endl;
        
        // Ensure directory exists
        std::filesystem::create_directories(std::filesystem::path(outputPath).parent_path());
        
        if (!asset.save_to_file(outputPath)) {
            std::cerr << "❌ Failed to save streaming asset!" << std::endl;
            return false;
        }
        
        std::cout << "✅ Streaming test asset created successfully!" << std::endl;
        std::cout << "   📊 Total size: " << audioChunkData.size() << " bytes" << std::endl;
        std::cout << "   🎵 Duration: " << duration << " seconds" << std::endl;
        std::cout << "   📦 Audio data starts at offset: " << streamInfo.data_offset << std::endl;
        
        return true;
    }

} // namespace tremor::taffy::tools