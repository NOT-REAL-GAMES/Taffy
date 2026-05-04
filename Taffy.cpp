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
#include <spirv_cross.hpp>
#include <spirv_glsl.hpp>
#include <spirv_hlsl.hpp>
#include <spirv_msl.hpp>

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



void printUsage(const char* program_name) {
	std::cout << "Taffy Asset Compiler\n" << std::endl;
	std::cout << "Usage:" << std::endl;
	std::cout << "  " << program_name << " demo <master_output> <overlay_output> <font_output> [audio_output_dir]" << std::endl;
	std::cout << "    Generate demo assets" << std::endl;
	std::cout << "  " << program_name << " create <input.obj> <output.taf>" << std::endl;
	std::cout << "    Create Taffy asset from OBJ file" << std::endl;
}

int main(int argc, char* argv[]) {
	using namespace tremor::taffy::tools;

	if (argc < 2) {
		printUsage(argv[0]);
		return 1;
	}

	std::string command = argv[1];

	if (command == "create") {
		if (argc < 4) {
			std::cout << "Usage: " << argv[0] << " create <input.obj> <output.taf>" << std::endl;
			return 1;
		}

		// TODO: Implement OBJ to Taffy conversion
		std::cout << "OBJ import not yet implemented" << std::endl;
		return 1;
	}

	if (command == "demo") {
		if (argc < 5) {
			std::cout << "Usage: " << argv[0] << " demo <master_output> <overlay_output> <font_output> [audio_output_dir]" << std::endl;
			return 1;
		}

		// Existing demo generation code
		std::string outMaster = argv[2];
		std::string outOverlay = argv[3];
		std::string outFont = argv[4];
		std::string audioDir = (argc > 5) ? argv[5] : "assets/audio";

		Taffy::Asset ass;
		DataDrivenAssetCompiler compiler{};

		compiler.createDataDrivenTriangle(outMaster);

		tremor::taffy::tools::createSDFFontAsset(
			outFont, "assets/fonts/BebasNeue-Regular.ttf", 128, 2048, 1.0f);

		// Generate audio assets if directory is specified
		if (argc >= 5) {
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

	// Unknown command
	std::cerr << "❌ Unknown command: " << command << std::endl;
	printUsage(argv[0]);
	return 1;
}