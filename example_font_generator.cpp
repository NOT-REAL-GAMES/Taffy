// Example of using taffy_font_tools in an asset generator
#include "include/taffy_font_tools.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input.ttf> <output.taf> [font_size] [texture_size] [sdf_range]\n";
        return 1;
    }

    std::string inputFont = argv[1];
    std::string outputAsset = argv[2];
    
    // Default parameters
    int fontSize = (argc > 3) ? std::stoi(argv[3]) : 48;
    int textureSize = (argc > 4) ? std::stoi(argv[4]) : 512;
    float sdfRange = (argc > 5) ? std::stof(argv[5]) : 4.0f;
    
    std::cout << "Generating SDF font asset:\n";
    std::cout << "  Input font: " << inputFont << "\n";
    std::cout << "  Output: " << outputAsset << "\n";
    std::cout << "  Font size: " << fontSize << " px\n";
    std::cout << "  Texture size: " << textureSize << "x" << textureSize << "\n";
    std::cout << "  SDF range: " << sdfRange << " px\n";
    
    bool success = tremor::taffy::tools::createSDFFontAsset(
        outputAsset, inputFont, fontSize, textureSize, sdfRange);
    
    if (success) {
        std::cout << "✅ Font asset generated successfully!\n";
        return 0;
    } else {
        std::cerr << "❌ Failed to generate font asset\n";
        return 1;
    }
}