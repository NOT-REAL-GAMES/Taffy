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

using namespace Taffy;



int main(int argc, char* argv[]) {
	using namespace tremor::taffy::tools;

	if (argc < 3) {
		return 1;
	}

	std::string outMaster = argv[1];
	std::string outOverlay = argv[2];
	std::string outFont = argv[3];

	Taffy::Asset ass;

	DataDrivenAssetCompiler compiler{};

	 compiler.createDataDrivenTriangle(outMaster);
	// Use the fixed triangle version that includes indices
	//extern bool createFixedTriangle(const std::string& output_path);
	//createFixedTriangle(outMaster);

	tremor::taffy::tools::createSDFFontAsset(
        outFont, "assets/fonts/BebasNeue-Regular.ttf", 128, 2048, 1.0f);

	return createHotPinkShaderOverlay(outOverlay);
			
	
}