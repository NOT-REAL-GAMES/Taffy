cmake_minimum_required(VERSION 3.16)
project(Taffy VERSION 0.1.0 LANGUAGES CXX)

# Add custom module path for finding packages
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

# Find SPIRV-Cross
find_package(SPIRVCross REQUIRED)

# Create Taffy library
add_library(Taffy STATIC
    Taffy.cpp
    include/tools.cpp  
    taffy_audio_tools.cpp  # Audio tools
    taffy_font_tools.cpp   # SDF font tools
)

# Create Taffy compiler executable (uses Taffy.cpp as main)
add_executable(taffy_compiler
    Taffy.cpp
    include/tools.cpp  
    taffy_audio_tools.cpp  # Audio tools
    taffy_font_tools.cpp   # SDF font tools
)

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

if(SHADERC_COMBINED_LIBRARY)
    set(SHADERC_LIBS ${SHADERC_COMBINED_LIBRARY})
elseif(SHADERC_LIBRARY)
    set(SHADERC_LIBS ${SHADERC_LIBRARY})
else()
    message(FATAL_ERROR "shaderc not found. Please install libshaderc-dev")
endif()

# Link with SPIRV-Cross and shaderc libraries
target_link_libraries(taffy_compiler PRIVATE 
    ${SPIRV_CROSS_LIBRARIES}
    ${SHADERC_LIBS}
)

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
    ${SPIRV_CROSS_INCLUDE_DIRS}
)

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

# Add SPIRV-Cross
if(SPIRVCross_FOUND)
    target_include_directories(Taffy PRIVATE ${SPIRV_CROSS_INCLUDE_DIRS})
    target_link_libraries(Taffy PRIVATE ${SPIRV_CROSS_LIBRARIES})
else()
    message(FATAL_ERROR "SPIRV-Cross not found. Please install spirv-cross-dev")
endif()

# Add shaderc to Taffy library as well
target_link_libraries(Taffy PRIVATE ${SHADERC_LIBS})

# Set compile features
target_compile_features(Taffy PUBLIC cxx_std_20)

# Define USE_SHADERC since we have it
target_compile_definitions(taffy_compiler PRIVATE USE_SHADERC)
target_compile_definitions(Taffy PRIVATE USE_SHADERC)

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

# Export for use by Tremor
set(TAFFY_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include PARENT_SCOPE)