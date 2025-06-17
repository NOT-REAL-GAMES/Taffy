/**
 * Complete Taffy Workflow - SPIR-V Cross + Multiple Compilation Options
 *
 * This version:
 * 1. Brings back SPIR-V Cross for runtime transpilation
 * 2. Offers multiple ways to compile GLSL → SPIR-V (no Vulkan SDK required!)
 * 3. Shows the complete workflow from GLSL to universal assets
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <string>
#include <cstring>
#include <memory>


 // 🔥 SPIR-V Cross headers for runtime transpilation
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <spirv_cross/spirv_hlsl.hpp>
#include <spirv_cross/spirv_msl.hpp>

// 🔥 Optional: Use shaderc for compilation (no Vulkan SDK needed!)
#ifdef USE_SHADERC
#include <shaderc/shaderc.hpp>
#endif

#include "include/taffy.h"
#include "include/overlay.h"
#include "include/asset.h"
#include "include/tools.h"
#include "include/taffy_font_tools.h"
#include "include/taffy_audio_tools.h"

using namespace Taffy;



int main(int argc, char* argv[]) {
	using namespace tremor::taffy::tools;

	if (argc < 3) {
		std::cout << "Usage: " << argv[0] << " <master_output> <overlay_output> <font_output> [audio_output_dir]" << std::endl;
		return 1;
	}

	std::string outMaster = argv[1];
	std::string outOverlay = argv[2];
	std::string outFont = argv[3];
	//std::string outCube = argv[4];
	std::string audioDir = (argc > 4) ? argv[4] : "assets/audio";

	Taffy::Asset ass;

	DataDrivenAssetCompiler compiler{};

	 compiler.createDataDrivenTriangle(outMaster);
	// Use the fixed triangle version that includes indices
	//extern bool createFixedTriangle(const std::string& output_path);
	//createFixedTriangle(outMaster);

	tremor::taffy::tools::createSDFFontAsset(
        outFont, "assets/fonts/BebasNeue-Regular.ttf", 128, 2048, 1.0f);

	// Generate audio assets if directory is specified
	if (argc >= 4) {
		std::cout << "\n🎵 Generating audio assets..." << std::endl;
		
		// Waveform types
		const char* waveform_names[] = {"sine", "square", "saw", "triangle", "noise"};
		const float test_frequencies[] = {440.0f, 440.0f, 440.0f, 440.0f, 0.0f};
		
		// Create each waveform type
		for (uint32_t waveform = 0; waveform < 5; ++waveform) {
			std::string filename = audioDir + "/" + waveform_names[waveform] + "_wave.taf";
			float frequency = test_frequencies[waveform];
			
			if (!createWaveformAudioAsset(filename, frequency, 2.0f, waveform)) {
				std::cerr << "❌ Failed to create " << filename << std::endl;
			}
		}
		
		// Also create the legacy sine wave file for backward compatibility
		createSineWaveAudioAsset(audioDir + "/sine_440hz.taf", 440.0f, 1.0f);
		
		// Create mixer demo
		createMixerDemoAsset(audioDir + "/mixer_demo.taf", 2.0f);
		
		// Create ADSR demo
		createADSRDemoAsset(audioDir + "/adsr_demo.taf", 3.0f);
		
		// Create filter demos
		createFilterDemoAsset(audioDir + "/filter_lowpass.taf", 0, 3.0f);
		createFilterDemoAsset(audioDir + "/filter_highpass.taf", 1, 3.0f);
		createFilterDemoAsset(audioDir + "/filter_bandpass.taf", 2, 3.0f);
		
		// Create distortion demos
		createDistortionDemoAsset(audioDir + "/distortion_hardclip.taf", 0, 3.0f);
		createDistortionDemoAsset(audioDir + "/distortion_softclip.taf", 1, 3.0f);
		createDistortionDemoAsset(audioDir + "/distortion_foldback.taf", 2, 3.0f);
		createDistortionDemoAsset(audioDir + "/distortion_bitcrush.taf", 3, 3.0f);
		createDistortionDemoAsset(audioDir + "/distortion_overdrive.taf", 4, 3.0f);
		createDistortionDemoAsset(audioDir + "/distortion_beeper.taf", 5, 3.0f);
		
		std::cout << "✅ Audio assets generated!" << std::endl;
	}

	return createHotPinkShaderOverlay(outOverlay);
			
	
}