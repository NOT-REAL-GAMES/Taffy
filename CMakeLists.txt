cmake_minimum_required(VERSION 3.16)
project(Taffy VERSION 0.1.0 LANGUAGES CXX)

# Guard against multiple inclusion
if(TARGET Taffy)
    return()
endif()

# Add custom module path for finding packages
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

# Try to find SPIRV-Cross
find_package(SPIRVCross QUIET)

# If not found via find_package, try to locate it manually in Vulkan SDK
if(NOT SPIRVCross_FOUND)
    if(DEFINED ENV{VULKAN_SDK})
        set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
        
        # Look for SPIRV-Cross headers
        find_path(SPIRV_CROSS_INCLUDE_DIR
            NAMES spirv_cross/spirv_cross.hpp
            PATHS 
                ${VULKAN_SDK_PATH}/Include
                ${VULKAN_SDK_PATH}/include
            NO_DEFAULT_PATH
        )
        
        # Look for SPIRV-Cross libraries
        set(SPIRV_CROSS_LIB_NAMES 
            spirv-cross-core
            spirv-cross-glsl
            spirv-cross-hlsl
            spirv-cross-cpp
        )
        
        set(SPIRV_CROSS_LIBRARIES "")
        foreach(lib ${SPIRV_CROSS_LIB_NAMES})
            find_library(${lib}_LIB
                NAMES ${lib}
                PATHS
                    ${VULKAN_SDK_PATH}/Lib
                    ${VULKAN_SDK_PATH}/Lib32
                    ${VULKAN_SDK_PATH}/lib
                NO_DEFAULT_PATH
            )
            if(${lib}_LIB)
                list(APPEND SPIRV_CROSS_LIBRARIES ${${lib}_LIB})
            endif()
        endforeach()
        
        if(SPIRV_CROSS_INCLUDE_DIR AND SPIRV_CROSS_LIBRARIES)
            set(SPIRVCross_FOUND TRUE)
            set(SPIRV_CROSS_INCLUDE_DIRS ${SPIRV_CROSS_INCLUDE_DIR})
            message(STATUS "Found SPIRV-Cross in Vulkan SDK: ${VULKAN_SDK_PATH}")
        endif()
    endif()
endif()

# Make SPIRV-Cross optional - disable transpilation features if not found
if(NOT SPIRVCross_FOUND)
    message(WARNING "SPIRV-Cross not found. Shader transpilation features will be disabled.")
    message(WARNING "To enable transpilation, install SPIRV-Cross or ensure Vulkan SDK is properly installed.")
    set(SPIRV_CROSS_INCLUDE_DIRS "")
    set(SPIRV_CROSS_LIBRARIES "")
endif()

# Create Taffy library WITHOUT the main function
add_library(Taffy STATIC
    include/tools.cpp  
    taffy_audio_tools.cpp  # Audio tools
    taffy_font_tools.cpp   # SDF font tools
    taffy_streaming.cpp    # Streaming TAF support
)

# Create Taffy compiler executable (uses Taffy.cpp which has main)
add_executable(taffy_compiler
    Taffy.cpp
)

# Link the compiler with the Taffy library
target_link_libraries(taffy_compiler PRIVATE Taffy)

# Create RGB test triangle generator (commented out - file doesn't exist)
# add_executable(create_test_rgb_triangle
#     create_test_rgb_triangle.cpp
#     include/tools.cpp
#     include/mesh_shader_generator.cpp
# )

# Test indexed mesh shader generation
# add_executable(test_indexed_mesh
#     test_indexed_mesh.cpp
# )

# Find and link shaderc
find_library(SHADERC_COMBINED_LIBRARY NAMES shaderc_combined)
find_library(SHADERC_LIBRARY NAMES shaderc_shared shaderc)

# If not found in standard locations, check Vulkan SDK
if(NOT SHADERC_COMBINED_LIBRARY AND NOT SHADERC_LIBRARY)
    if(DEFINED ENV{VULKAN_SDK})
        set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
        
        find_library(SHADERC_LIBRARY
            NAMES shaderc_shared shaderc
            PATHS
                ${VULKAN_SDK_PATH}/Lib
                ${VULKAN_SDK_PATH}/Lib32
                ${VULKAN_SDK_PATH}/lib
            NO_DEFAULT_PATH
        )
        
        if(NOT SHADERC_LIBRARY)
            find_library(SHADERC_COMBINED_LIBRARY
                NAMES shaderc_combined
                PATHS
                    ${VULKAN_SDK_PATH}/Lib
                    ${VULKAN_SDK_PATH}/Lib32
                    ${VULKAN_SDK_PATH}/lib
                NO_DEFAULT_PATH
            )
        endif()
    endif()
endif()

if(SHADERC_COMBINED_LIBRARY)
    set(SHADERC_LIBS ${SHADERC_COMBINED_LIBRARY})
    set(SHADERC_FOUND TRUE)
    message(STATUS "Found shaderc (combined): ${SHADERC_COMBINED_LIBRARY}")
elseif(SHADERC_LIBRARY)
    set(SHADERC_LIBS ${SHADERC_LIBRARY})
    set(SHADERC_FOUND TRUE)
    message(STATUS "Found shaderc: ${SHADERC_LIBRARY}")
else()
    set(SHADERC_FOUND FALSE)
    set(SHADERC_LIBS "")
    message(WARNING "shaderc not found. Shader compilation features will be disabled.")
    message(WARNING "To enable shader compilation, install shaderc or ensure Vulkan SDK is properly installed.")
endif()

# Link with shaderc libraries if found
if(SHADERC_FOUND)
    target_link_libraries(taffy_compiler PRIVATE ${SHADERC_LIBS})
    target_link_libraries(Taffy PUBLIC ${SHADERC_LIBS})
endif()

# Link with SPIRV-Cross if found
if(SPIRVCross_FOUND)
    target_link_libraries(taffy_compiler PRIVATE ${SPIRV_CROSS_LIBRARIES})
    target_link_libraries(Taffy PUBLIC ${SPIRV_CROSS_LIBRARIES})
endif()

# target_link_libraries(create_test_rgb_triangle PRIVATE
#     Taffy
#     ${SPIRV_CROSS_LIBRARIES}
#     ${SHADERC_LIBS}
# )

# target_link_libraries(test_indexed_mesh PRIVATE
#     ${SPIRV_CROSS_LIBRARIES}
#     ${SHADERC_LIBS}
# )

# Set include directories for compiler
target_include_directories(taffy_compiler PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add SPIRV-Cross includes if found
if(SPIRVCross_FOUND)
    target_include_directories(taffy_compiler PRIVATE ${SPIRV_CROSS_INCLUDE_DIRS})
endif()

# target_include_directories(create_test_rgb_triangle PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/include
#     ${CMAKE_CURRENT_SOURCE_DIR}
#     ${SPIRV_CROSS_INCLUDE_DIRS}
# )

# target_include_directories(test_indexed_mesh PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/include
#     ${CMAKE_CURRENT_SOURCE_DIR}
#     ${SPIRV_CROSS_INCLUDE_DIRS}
# )

# Set include directories
target_include_directories(Taffy PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add SPIRV-Cross if found
if(SPIRVCross_FOUND)
    target_include_directories(Taffy PRIVATE ${SPIRV_CROSS_INCLUDE_DIRS})
    target_compile_definitions(Taffy PRIVATE HAS_SPIRV_CROSS)
endif()

# Set compile features
target_compile_features(Taffy PUBLIC cxx_std_20)

# Define USE_SHADERC if we have it
if(SHADERC_FOUND)
    target_compile_definitions(taffy_compiler PRIVATE USE_SHADERC)
    target_compile_definitions(Taffy PRIVATE USE_SHADERC)
endif()

# Add HAS_SPIRV_CROSS definition if found
if(SPIRVCross_FOUND)
    target_compile_definitions(taffy_compiler PRIVATE HAS_SPIRV_CROSS)
endif()

# Set output directory for executables
set_target_properties(taffy_compiler PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
# set_target_properties(create_test_rgb_triangle PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
# )
# set_target_properties(test_indexed_mesh PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
# )

# Platform-specific settings
if(WIN32)
    target_compile_definitions(Taffy PRIVATE WIN32_LEAN_AND_MEAN)
endif()

# Find SDL2 (optional for future graphical editor)
set(SDL2_FOUND FALSE)
if(WIN32 AND DEFINED ENV{VULKAN_SDK})
    set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
    
    # Look for SDL2 in Vulkan SDK Third-Party folder
    find_path(SDL2_INCLUDE_DIR
        NAMES SDL.h
        PATHS 
            ${VULKAN_SDK_PATH}/Include
            ${VULKAN_SDK_PATH}/Include/SDL2
        NO_DEFAULT_PATH
    )
    
    find_library(SDL2_LIBRARY
        NAMES SDL2
        PATHS
            ${VULKAN_SDK_PATH}/Bin
            ${VULKAN_SDK_PATH}/Lib
        NO_DEFAULT_PATH
    )
    
    find_library(SDL2MAIN_LIBRARY
        NAMES SDL2main
        PATHS
            ${VULKAN_SDK_PATH}/Bin
            ${VULKAN_SDK_PATH}/Lib
        NO_DEFAULT_PATH
    )
    
    if(SDL2_INCLUDE_DIR AND SDL2_LIBRARY)
        set(SDL2_FOUND TRUE)
        set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIR})
        set(SDL2_LIBRARIES ${SDL2_LIBRARY})
        if(SDL2MAIN_LIBRARY)
            list(APPEND SDL2_LIBRARIES ${SDL2MAIN_LIBRARY})
        endif()
        message(STATUS "Found SDL2 in Vulkan SDK: ${VULKAN_SDK_PATH}")
        message(STATUS "  Include: ${SDL2_INCLUDE_DIRS}")
        message(STATUS "  Libraries: ${SDL2_LIBRARIES}")
        
        # Create imported target for modern CMake
        add_library(SDL2::SDL2 INTERFACE IMPORTED)
        set_target_properties(SDL2::SDL2 PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES "${SDL2_LIBRARIES}"
        )
    endif()
else()
    # On non-Windows platforms, use standard find_package
    find_package(SDL2 QUIET)
endif()

if(SDL2_FOUND)
    message(STATUS "SDL2 found - graphical editor features will be available")
    target_compile_definitions(Taffy PRIVATE HAS_SDL2)
    target_compile_definitions(taffy_compiler PRIVATE HAS_SDL2)
else()
    message(STATUS "SDL2 not found - graphical editor features will be disabled")
endif()

# Export for use by Tremor
set(TAFFY_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include PARENT_SCOPE)