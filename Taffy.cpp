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

namespace {

const char* packageProfileName(PackageProfile profile) {
	switch (profile) {
	case PackageProfile::Asset: return "asset";
	case PackageProfile::Scene: return "scene";
	case PackageProfile::Game: return "game";
	}
	return "unknown";
}

std::optional<PackageProfile> parsePackageProfile(const std::string& text) {
	if (text == "asset") return PackageProfile::Asset;
	if (text == "scene") return PackageProfile::Scene;
	if (text == "game") return PackageProfile::Game;
	return std::nullopt;
}

const char* chunkTypeName(ChunkType type) {
	switch (type) {
	case ChunkType::MANF: return "MANF";
	case ChunkType::BOOT: return "BOOT";
	case ChunkType::GEOM: return "GEOM";
	case ChunkType::MTRL: return "MTRL";
	case ChunkType::SHDR: return "SHDR";
	case ChunkType::TXTR: return "TXTR";
	case ChunkType::ANIM: return "ANIM";
	case ChunkType::SCPT: return "SCPT";
	case ChunkType::PHYS: return "PHYS";
	case ChunkType::AUDI: return "AUDI";
	case ChunkType::FONT: return "FONT";
	case ChunkType::OVRL: return "OVRL";
	case ChunkType::CHKO: return "CHKO";
	case ChunkType::FRAC: return "FRAC";
	case ChunkType::PART: return "PART";
	case ChunkType::SVGU: return "SVGU";
	case ChunkType::DEPS: return "DEPS";
	}
	return "UNKN";
}

const char* dependencyReferenceTypeName(DependencyChunk::ReferenceType type) {
	switch (type) {
	case DependencyChunk::ReferenceType::ExternalFile: return "external-file";
	case DependencyChunk::ReferenceType::ExternalTaf: return "external-taf";
	case DependencyChunk::ReferenceType::ExternalDirectory: return "external-directory";
	}
	return "unknown";
}

const char* dependencyUsageName(DependencyChunk::Usage usage) {
	switch (usage) {
	case DependencyChunk::Usage::Generic: return "generic";
	case DependencyChunk::Usage::Scene: return "scene";
	case DependencyChunk::Usage::Code: return "code";
	case DependencyChunk::Usage::UI: return "ui";
	case DependencyChunk::Usage::Audio: return "audio";
	case DependencyChunk::Usage::Texture: return "texture";
	case DependencyChunk::Usage::Geometry: return "geometry";
	case DependencyChunk::Usage::Overlay: return "overlay";
	}
	return "unknown";
}

std::optional<DependencyChunk::Usage> parseDependencyUsage(const std::string& text) {
	if (text == "generic") return DependencyChunk::Usage::Generic;
	if (text == "scene") return DependencyChunk::Usage::Scene;
	if (text == "code") return DependencyChunk::Usage::Code;
	if (text == "ui") return DependencyChunk::Usage::UI;
	if (text == "audio") return DependencyChunk::Usage::Audio;
	if (text == "texture") return DependencyChunk::Usage::Texture;
	if (text == "geometry") return DependencyChunk::Usage::Geometry;
	if (text == "overlay") return DependencyChunk::Usage::Overlay;
	return std::nullopt;
}

std::optional<DependencyChunk::ReferenceType> parseDependencyReferenceType(const std::string& text) {
	if (text == "file") return DependencyChunk::ReferenceType::ExternalFile;
	if (text == "taf") return DependencyChunk::ReferenceType::ExternalTaf;
	if (text == "dir") return DependencyChunk::ReferenceType::ExternalDirectory;
	return std::nullopt;
}

template <typename T>
std::vector<uint8_t> bytesOf(const T& value) {
	std::vector<uint8_t> data(sizeof(T));
	std::memcpy(data.data(), &value, sizeof(T));
	return data;
}

std::vector<DependencyChunk::Entry> parseDependencyEntries(const std::vector<uint8_t>& data) {
	if (data.size() < sizeof(DependencyChunk)) {
		return {};
	}

	DependencyChunk deps{};
	std::memcpy(&deps, data.data(), sizeof(DependencyChunk));
	const size_t expected = sizeof(DependencyChunk) +
		static_cast<size_t>(deps.dependency_count) * sizeof(DependencyChunk::Entry);
	if (data.size() < expected) {
		return {};
	}

	std::vector<DependencyChunk::Entry> entries(deps.dependency_count);
	if (!entries.empty()) {
		std::memcpy(entries.data(),
			data.data() + sizeof(DependencyChunk),
			entries.size() * sizeof(DependencyChunk::Entry));
	}
	return entries;
}

std::vector<uint8_t> buildDependencyChunkData(const std::vector<DependencyChunk::Entry>& entries) {
	DependencyChunk deps{};
	deps.dependency_count = static_cast<uint32_t>(entries.size());

	std::vector<uint8_t> data(sizeof(DependencyChunk) +
		entries.size() * sizeof(DependencyChunk::Entry));
	std::memcpy(data.data(), &deps, sizeof(DependencyChunk));
	if (!entries.empty()) {
		std::memcpy(data.data() + sizeof(DependencyChunk),
			entries.data(),
			entries.size() * sizeof(DependencyChunk::Entry));
	}
	return data;
}

bool upsertDependencyChunk(Asset& asset, const std::vector<DependencyChunk::Entry>& entries) {
	if (asset.has_chunk(ChunkType::DEPS)) {
		asset.remove_chunk(ChunkType::DEPS);
	}
	asset.add_chunk(ChunkType::DEPS, buildDependencyChunkData(entries), "dependencies");
	asset.set_dependency_count(static_cast<uint32_t>(entries.size()));
	return true;
}

bool createPackageSkeleton(const std::string& outputPath,
						   PackageProfile profile,
						   const std::string& packageName) {
	Asset asset;
	asset.set_creator("Taffy Runtime Packager");
	asset.set_description("Phase One runtime container package");

	ManifestChunk manifest{};
	manifest.schema_version = 1;
	manifest.profile = profile;
	std::strncpy(manifest.package_name, packageName.c_str(), sizeof(manifest.package_name) - 1);
	std::strncpy(manifest.package_version, "0.1.0", sizeof(manifest.package_version) - 1);
	std::strncpy(manifest.build_id, "phase-one", sizeof(manifest.build_id) - 1);
	manifest.required_engine_version[0] = 0;
	manifest.required_engine_version[1] = 1;
	manifest.required_engine_version[2] = 0;

	BootstrapChunk bootstrap{};
	std::strncpy(bootstrap.startup_scene, "default_scene", sizeof(bootstrap.startup_scene) - 1);
	std::strncpy(bootstrap.startup_code, "startup_code", sizeof(bootstrap.startup_code) - 1);
	std::strncpy(bootstrap.startup_ui, "default_ui", sizeof(bootstrap.startup_ui) - 1);
	std::strncpy(bootstrap.startup_audio, "default_audio", sizeof(bootstrap.startup_audio) - 1);

	asset.add_chunk(ChunkType::MANF, bytesOf(manifest), "manifest");
	asset.add_chunk(ChunkType::BOOT, bytesOf(bootstrap), "bootstrap");
	upsertDependencyChunk(asset, {});

	const auto parent = std::filesystem::path(outputPath).parent_path();
	if (!parent.empty()) {
		std::filesystem::create_directories(parent);
	}
	return asset.save_to_file(outputPath);
}

bool addExternalReference(const std::string& inputPath,
						  const std::string& outputPath,
						  const std::string& logicalName,
						  const std::string& externalPath,
						  DependencyChunk::Usage usage,
						  DependencyChunk::ReferenceType referenceType,
						  bool packageRelative,
						  bool optional) {
	Asset asset;
	if (!asset.load_from_file_safe(inputPath)) {
		return false;
	}

	std::vector<DependencyChunk::Entry> entries;
	if (auto depsData = asset.get_chunk_data(ChunkType::DEPS)) {
		entries = parseDependencyEntries(*depsData);
	}

	bool replaced = false;
	for (auto& entry : entries) {
		if (entry.logical_name == logicalName) {
			entry.reference_type = referenceType;
			entry.usage = usage;
			entry.flags = 0;
			if (optional) {
				entry.flags |= 1u << 0;
			}
			if (packageRelative) {
				entry.flags |= 1u << 1;
			}
			std::strncpy(entry.path, externalPath.c_str(), sizeof(entry.path) - 1);
			entry.path[sizeof(entry.path) - 1] = '\0';
			replaced = true;
			break;
		}
	}

	if (!replaced) {
		DependencyChunk::Entry entry{};
		entry.reference_type = referenceType;
		entry.usage = usage;
		if (optional) {
			entry.flags |= 1u << 0;
		}
		if (packageRelative) {
			entry.flags |= 1u << 1;
		}
		std::strncpy(entry.logical_name, logicalName.c_str(), sizeof(entry.logical_name) - 1);
		std::strncpy(entry.path, externalPath.c_str(), sizeof(entry.path) - 1);
		std::strncpy(entry.version_tag, "loose", sizeof(entry.version_tag) - 1);
		entries.push_back(entry);
	}

	upsertDependencyChunk(asset, entries);

	const auto parent = std::filesystem::path(outputPath).parent_path();
	if (!parent.empty()) {
		std::filesystem::create_directories(parent);
	}
	return asset.save_to_file(outputPath);
}

bool addScriptChunk(const std::string& inputPath,
					const std::string& outputPath,
					const std::string& chunkName,
					const std::string& scriptPath) {
	std::ifstream file(scriptPath, std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "❌ Failed to open script file: " << scriptPath << std::endl;
		return false;
	}

	std::string scriptText{
		std::istreambuf_iterator<char>(file),
		std::istreambuf_iterator<char>()
	};

	Asset asset;
	if (!asset.load_from_file_safe(inputPath)) {
		return false;
	}

	if (asset.has_chunk(ChunkType::SCPT)) {
		asset.remove_chunk(ChunkType::SCPT);
	}

	std::vector<uint8_t> chunkData(scriptText.begin(), scriptText.end());
	asset.add_chunk(ChunkType::SCPT, chunkData, chunkName);
	asset.set_feature_flags(asset.get_feature_flags() | FeatureFlags::Scripting);

	const auto parent = std::filesystem::path(outputPath).parent_path();
	if (!parent.empty()) {
		std::filesystem::create_directories(parent);
	}
	return asset.save_to_file(outputPath);
}

bool inspectPackage(const std::string& inputPath) {
	Asset asset;
	if (!asset.load_from_file_safe(inputPath)) {
		return false;
	}

	const auto header = asset.get_header();
	std::cout << "\nPackage Summary\n";
	std::cout << "---------------\n";
	std::cout << "Magic: " << std::string(header.magic, 4) << "\n";
	std::cout << "Version: " << header.version_major << "." << header.version_minor << "." << header.version_patch << "\n";
	std::cout << "Creator: " << header.creator << "\n";
	std::cout << "Description: " << header.description << "\n";
	std::cout << "Chunks: " << header.chunk_count << "\n";
	std::cout << "Dependencies declared: " << header.dependency_count << "\n";
	std::cout << "AI models declared: " << header.ai_model_count << "\n";
	std::cout << "Feature flags: 0x" << std::hex << static_cast<uint64_t>(header.feature_flags) << std::dec << "\n";

	if (auto manifestData = asset.get_chunk_data(ChunkType::MANF);
		manifestData && manifestData->size() >= sizeof(ManifestChunk)) {
		ManifestChunk manifest{};
		std::memcpy(&manifest, manifestData->data(), sizeof(ManifestChunk));
		std::cout << "\nManifest\n";
		std::cout << "--------\n";
		std::cout << "Profile: " << packageProfileName(manifest.profile) << "\n";
		std::cout << "Package: " << manifest.package_name << "\n";
		std::cout << "Package Version: " << manifest.package_version << "\n";
		std::cout << "Build ID: " << manifest.build_id << "\n";
		std::cout << "Required Engine: "
				  << manifest.required_engine_version[0] << "."
				  << manifest.required_engine_version[1] << "."
				  << manifest.required_engine_version[2] << "\n";
	} else {
		std::cout << "\nManifest: missing\n";
	}

	if (auto bootData = asset.get_chunk_data(ChunkType::BOOT);
		bootData && bootData->size() >= sizeof(BootstrapChunk)) {
		BootstrapChunk boot{};
		std::memcpy(&boot, bootData->data(), sizeof(BootstrapChunk));
		std::cout << "\nBootstrap\n";
		std::cout << "---------\n";
		std::cout << "Startup Scene: " << boot.startup_scene << "\n";
		std::cout << "Startup Code: " << boot.startup_code << "\n";
		std::cout << "Startup UI: " << boot.startup_ui << "\n";
		std::cout << "Startup Audio: " << boot.startup_audio << "\n";
	} else {
		std::cout << "Bootstrap: missing\n";
	}

	if (auto depsData = asset.get_chunk_data(ChunkType::DEPS)) {
		auto entries = parseDependencyEntries(*depsData);
		std::cout << "\nDependencies\n";
		std::cout << "------------\n";
		if (entries.empty()) {
			std::cout << "(none)\n";
		} else {
			for (const auto& entry : entries) {
				std::cout << entry.logical_name
						  << "  type=" << dependencyReferenceTypeName(entry.reference_type)
						  << "  usage=" << dependencyUsageName(entry.usage)
						  << "  path=" << entry.path;
				if ((entry.flags & (1u << 1)) != 0) {
					std::cout << "  relative-to-package";
				}
				if ((entry.flags & (1u << 0)) != 0) {
					std::cout << "  optional";
				}
				std::cout << "\n";
			}
		}
	} else {
		std::cout << "\nDependencies: missing\n";
	}

	std::cout << "\nChunk Directory\n";
	std::cout << "---------------\n";
	for (const auto& entry : asset.get_chunk_directory()) {
		std::cout << chunkTypeName(entry.type)
				  << "  name=" << entry.name
				  << "  size=" << entry.size
				  << "  offset=" << entry.offset
				  << "  flags=0x" << std::hex << entry.flags << std::dec
				  << "\n";
	}

	return true;
}

} // namespace


void printUsage(const char* program_name) {
	std::cout << "Taffy Asset Compiler\n" << std::endl;
	std::cout << "Usage:" << std::endl;
	std::cout << "  " << program_name << " demo <master_output> <overlay_output> <font_output> [audio_output_dir]" << std::endl;
	std::cout << "    Generate demo assets" << std::endl;
	std::cout << "  " << program_name << " create <input.obj> <output.taf>" << std::endl;
	std::cout << "    Create Taffy asset from OBJ file" << std::endl;
	std::cout << "  " << program_name << " cube <output.taf> [mesh]" << std::endl;
	std::cout << "    Create the built-in cube asset" << std::endl;
	std::cout << "  " << program_name << " sphere <output.taf> [mesh]" << std::endl;
	std::cout << "    Create the built-in sphere asset" << std::endl;
	std::cout << "  " << program_name << " init-package <output.taf> <asset|scene|game> <name>" << std::endl;
	std::cout << "    Create a skeletal runtime container with MANF and BOOT chunks" << std::endl;
	std::cout << "  " << program_name << " inspect <input.taf>" << std::endl;
	std::cout << "    Inspect header, manifest, bootstrap, and chunk directory" << std::endl;
	std::cout << "  " << program_name << " add-script-chunk <input.taf> <output.taf> <chunk_name> <script_file>" << std::endl;
	std::cout << "    Add or replace a SCPT chunk from a loose script file" << std::endl;
	std::cout << "  " << program_name << " add-external-ref <input.taf> <output.taf> <logical_name> <path> [usage] [file|taf|dir] [relative] [optional]" << std::endl;
	std::cout << "    Add or update a loose-file dependency reference in the DEPS chunk" << std::endl;
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

	if (command == "sphere") {
		if (argc < 3) {
			std::cout << "Usage: " << argv[0] << " sphere <output.taf> [mesh]" << std::endl;
			return 1;
		}

		std::string outMaster = argv[2];
		bool mesh = (argc >= 4 && std::string(argv[3]) == "mesh");
		
		DataDrivenAssetCompiler compiler{};

		return compiler.createDataDrivenSphere(outMaster, 12, 24, 64000, mesh) ? 0 : 1;
	}

	if (command == "cube") {
		if (argc < 3) {
			std::cout << "Usage: " << argv[0] << " cube <output.taf> [mesh]" << std::endl;
			return 1;
		}

		std::string outMaster = argv[2];
		bool mesh = (argc >= 4 && std::string(argv[3]) == "mesh");

		DataDrivenAssetCompiler compiler{};

		return compiler.createDataDrivenTriangle(outMaster, mesh) ? 0 : 1;
	}

	if (command == "init-package") {
		if (argc < 5) {
			std::cout << "Usage: " << argv[0] << " init-package <output.taf> <asset|scene|game> <name>" << std::endl;
			return 1;
		}

		auto profile = parsePackageProfile(argv[3]);
		if (!profile) {
			std::cerr << "❌ Invalid package profile: " << argv[3] << std::endl;
			return 1;
		}

		return createPackageSkeleton(argv[2], *profile, argv[4]) ? 0 : 1;
	}

	if (command == "inspect") {
		if (argc < 3) {
			std::cout << "Usage: " << argv[0] << " inspect <input.taf>" << std::endl;
			return 1;
		}

		return inspectPackage(argv[2]) ? 0 : 1;
	}

	if (command == "add-script-chunk") {
		if (argc < 6) {
			std::cout << "Usage: " << argv[0] << " add-script-chunk <input.taf> <output.taf> <chunk_name> <script_file>" << std::endl;
			return 1;
		}

		return addScriptChunk(argv[2], argv[3], argv[4], argv[5]) ? 0 : 1;
	}

	if (command == "add-external-ref") {
		if (argc < 6) {
			std::cout << "Usage: " << argv[0] << " add-external-ref <input.taf> <output.taf> <logical_name> <path> [usage] [file|taf|dir] [relative] [optional]" << std::endl;
			return 1;
		}

		const std::string usageText = (argc >= 7) ? argv[6] : "generic";
		auto usage = parseDependencyUsage(usageText);
		if (!usage) {
			std::cerr << "❌ Invalid dependency usage: " << usageText << std::endl;
			return 1;
		}

		const std::string refTypeText = (argc >= 8) ? argv[7] : "file";
		auto refType = parseDependencyReferenceType(refTypeText);
		if (!refType) {
			std::cerr << "❌ Invalid reference type: " << refTypeText << std::endl;
			return 1;
		}

		const bool packageRelative = (argc >= 9 && std::string(argv[8]) == "relative");
		const bool optional = (argc >= 10 && std::string(argv[9]) == "optional");

		return addExternalReference(argv[2], argv[3], argv[4], argv[5], *usage, *refType, packageRelative, optional) ? 0 : 1;
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

		compiler.createDataDrivenTriangle(outMaster, true);

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

        return createHotPinkShaderOverlay(outOverlay) ? 0 : 1;
    }

	// Unknown command
	std::cerr << "❌ Unknown command: " << command << std::endl;
	printUsage(argv[0]);
	return 1;
}
