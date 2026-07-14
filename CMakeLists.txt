# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 23)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("GraphicsFromScratch")

set(EXECUTABLE_NAME main)


# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# This assumes the SDL source is available in vendored/SDL
# add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
find_package(SDL3 REQUIRED)
# add_subdirectory(vendored/glm EXCLUDE_FROM_ALL)
find_package(glm CONFIG REQUIRED)
find_package(nlohmann_json REQUIRED)
# find_package(SDL3_ttf REQUIRED)

set(IMGUI_PATH  "/repos/imgui")
set(IMGUI_BACKENDS_PATH  ${IMGUI_PATH}/backends)
file(GLOB IMGUI_SOURCES ${IMGUI_PATH}/*.cpp ${IMGUI_BACKENDS_PATH}/imgui_impl_sdlgpu3.cpp ${IMGUI_BACKENDS_PATH}/imgui_impl_sdl3.cpp)
add_library(ImGui STATIC ${IMGUI_SOURCES})
target_include_directories(ImGui PUBLIC ${IMGUI_PATH} ${IMGUI_BACKENDS_PATH})
target_link_libraries(ImGui PUBLIC 
    SDL3::SDL3
)

# SDL_image (used for loading various image formats)
# # set(SDLIMAGE_VENDORED ON)

# set(SDLIMAGE_VENDORED OFF)
set(SDLIMAGE_AVIF OFF)	# disable formats we don't use to make the build faster and smaller.
set(SDLIMAGE_BMP OFF)
set(SDLIMAGE_JPEG OFF)
set(SDLIMAGE_WEBP OFF)
find_package(SDL3_image REQUIRED)
# add_subdirectory(vendored/SDL_image EXCLUDE_FROM_ALL)

# Include sub-projects.
add_subdirectory ("src")

