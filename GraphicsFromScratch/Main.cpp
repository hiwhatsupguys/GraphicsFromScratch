/*
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/

#define SDL_MAIN_USE_CALLBACKS 1 /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>

#include "ObjData.h"
#include "Vertex.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <string>
#include <cmath>
#include <ios>
#include <iterator>
#include <fstream>
#include <format>
#include <vector>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include <imgui.h>
#include <imgui_impl_sdlgpu3.h>
#include <imgui_impl_sdl3.h>

using json = nlohmann::json;


SDL_Window* window;
int windowWidth, windowHeight;
float aspectRatio;

SDL_GPUDevice* device;
// specifies which shaders to use, how many buffers, vertex inputs, color
// blending
SDL_GPUGraphicsPipeline* fillPipeline;
SDL_GPUTexture* depthTexture;
SDL_GPUTextureCreateInfo
depthTextureCreateInfo{}; // for updating width and height later

constexpr SDL_GPUTextureFormat DEPTH_TEXTURE_FORMAT =
// SDL_GPU_TEXTUREFORMAT_D16_UNORM;
// VVV ONLY WORKS FOR 4060
// SDL_GPU_TEXTUREFORMAT_D24_UNORM;
SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

SDL_GPUSampler* sampler;

struct Camera {
    glm::vec3 position;
    glm::vec3 target;
};
Camera camera;



// https://www.youtube.com/watch?v=Lw8LPXPyrl0
//template<typename T>
//struct Interpolated {
//    T start{};
//    T end{};
//    float startTime{};
//    float speed = 8.0f;
//
//    Interpolated(T const& initialValue = {}) : start{ initialValue }, end{ start } {}
//    static float getCurrentTime() { return static_cast<float>(SDL_GetPerformanceCounter() / static_cast<float>(SDL_GetPerformanceFrequency())); }
//    float getElapsedSeconds() const { return getCurrentTime() - startTime; }
//    void setValue(T const &newValue) {
//        start = getValue();
//        end = newValue;
//        startTime = getCurrentTime();
//    }
//    T getValue() const {
//        const float elapsed = getElapsedSeconds();
//        float t = elapsed;
//        t *= speed;
//
//        // easings.net
//
//
//        if (t >= 1.0f)
//            return end;
//
//        // lerp
//        return start + (end - start) * t;
//    }
//
//    void setDuration(float duration) {
//        speed = 1.0f / duration;
//    
//    }
//
//    operator T() const {
//        return getValue();
//    }
//    void operator=(T const& newValue) {
//        setValue(newValue);
//    }
//};

glm::vec2 mouseVelocityVector{ 0.0f, 0.0f };
constexpr float MOUSE_SENSITIVITY = 0.002f;
//constexpr float MOUSE_SENSITIVITY = 0.01f;

// units per second
constexpr float CAMERA_SPEED = 5.0f;

Uint64 currentTime;
Uint64 previousTime;
// deltaTime in seconds
float deltaTime;

constexpr float SECONDS_PER_FPS_CHECK = 1.0f;
const Uint64 COUNTS_PER_FPS_CHECK = SECONDS_PER_FPS_CHECK * SDL_GetPerformanceFrequency();
float averageFPSOverCheckInterval = 0.0f;
float fpsSum = 0.0f;
Uint64 lastFPSCheckTime = 0;
Uint64 numFPSChecksInInterval = 0;

float rotation;

// radians per second
constexpr float ROTATION_SPEED = SDL_PI_F / 4.0f;

//struct Look {
//    float yaw;
//    float pitch;
//};

typedef glm::vec2 Look;
Look look{ 0.0f, 0.0f };

// pitch, yaw
//typedef glm::vec2 Look;
// Interpolated<Look> look{ Look{0.0f, 0.0f} };

bool show_demo_window = true;
bool show_another_window = false;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
ImGuiIO* io;

bool cameraUpPressed;
bool cameraDownPressed;
bool cameraLeftPressed;
bool cameraRightPressed;
bool cameraForwardPressed;
bool cameraBackwardPressed;

const char* MODEL_PATH = "Content/Meshes/sedan-sports.obj";
const char* MESH_PATH = "Content/Meshes/colormap.png";

struct Model {
    SDL_GPUBuffer* vertexBuffer;
    SDL_GPUBuffer* indexBuffer;
    Uint32 numIndices;
    SDL_GPUTexture* colormapTexture;
};

Model model;

struct MatrixUniformBuffer {
    glm::mat4 mvp;
};

static MatrixUniformBuffer matrixUniform;

struct ShaderInfo {
    Uint32 samplers;
    Uint32 storageTextures;
    Uint32 storageBuffers;
    Uint32 uniformBuffers;
};

const std::string BASE_PATH = SDL_GetBasePath();
const std::string COMPILED_SHADER_PATH = BASE_PATH + "/Content/Shaders/Compiled/";

constexpr glm::vec4 WHITE{ 1, 1, 1, 1 };

//// rect
// std::vector<Vertex> vertices{
//
//     Vertex{{-0.5f, 0.5f, 0.0f}, WHITE, {0, 0}},
//     Vertex{{0.5f, 0.5f, 0.0f}, WHITE, {1, 0}},
//     Vertex{{0.5f, -0.5f, 0.0f}, WHITE, {1, 1}},
//     Vertex{{-0.5f, -0.5f, 0.0f}, WHITE, {0, 1}},
// };
//
// std::vector<Uint32> indices{0, 1, 2, 0, 2, 3};

//// rect
// Vertex vertices[4] = {
//
//     Vertex{{-0.5f, 0.5f, 0.0f}, WHITE, {0, 0}},
//     Vertex{{0.5f, 0.5f, 0.0f}, WHITE, {1, 0}},
//     Vertex{{0.5f, -0.5f, 0.0f}, WHITE, {1, 1}},
//     Vertex{{-0.5f, -0.5f, 0.0f}, WHITE, {0, 1}},
//
// };
//
// Uint32 indices[6] = {0, 1, 2, 0, 2, 3};

ShaderInfo loadShaderInfoFromJson(const std::string& shaderFilename) {
    std::string jsonFilePath = COMPILED_SHADER_PATH + "/JSON/" + shaderFilename + ".json";
    std::ifstream jsonFile{ jsonFilePath };
    if (!jsonFile) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "json file not found :(");
        std::exit(1);
    }
    json data = json::parse(jsonFile);
    ShaderInfo shaderInfo = {
        .samplers = data["samplers"],
        .storageTextures = data["storage_textures"],
        .storageBuffers = data["storage_buffers"],
        .uniformBuffers = data["uniform_buffers"],
    };

    return shaderInfo;

}


// ripped from SDL_gpu examples
SDL_GPUShader* LoadShader(SDL_GPUDevice* device, const std::string& shaderFilename) {
    // Auto-detect the shader stage from the file name for convenience
    SDL_GPUShaderStage stage;
    if (shaderFilename.contains(".vert"))
        stage = SDL_GPU_SHADERSTAGE_VERTEX;
    else if (shaderFilename.contains(".frag"))
        stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    else {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Invalid shader stage!");
        std::exit(1);
        return nullptr;
    }

    std::string fullPath;
    SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(device);
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
    const char* entrypoint;

    if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV) {

        fullPath = std::format("{}/SPIRV/{}.spv", COMPILED_SHADER_PATH, shaderFilename);
        format = SDL_GPU_SHADERFORMAT_SPIRV;
        entrypoint = "main";

    }
    else if (backendFormats & SDL_GPU_SHADERFORMAT_MSL) {

        fullPath = std::format("{}/MSL/{}.msl", COMPILED_SHADER_PATH, shaderFilename);
        format = SDL_GPU_SHADERFORMAT_MSL;
        entrypoint = "main0";

    }
    else if (backendFormats & SDL_GPU_SHADERFORMAT_DXIL) {

        fullPath = std::format("{}/DXIL/{}.dxil", COMPILED_SHADER_PATH, shaderFilename);
        format = SDL_GPU_SHADERFORMAT_DXIL;
        entrypoint = "main";

    }
    else {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unrecognized backend shader format!");
        std::exit(1);
        return nullptr;
    }

    ShaderInfo info = loadShaderInfoFromJson(shaderFilename);

    std::ifstream shaderFile{ fullPath, std::ios::binary };
    if (!shaderFile)
        throw std::runtime_error{ "Failed to load shader from disk! " + fullPath };

    std::vector<Uint8> code{ std::istreambuf_iterator(shaderFile), {} };

    SDL_GPUShaderCreateInfo shaderInfo{};
    shaderInfo.code = (Uint8*)code.data();
    shaderInfo.code_size = code.size();
    shaderInfo.entrypoint = entrypoint;
    shaderInfo.format = format;
    shaderInfo.stage = stage;
    shaderInfo.num_samplers = info.samplers;
    shaderInfo.num_storage_textures = info.storageTextures;
    shaderInfo.num_storage_buffers = info.storageBuffers;
    shaderInfo.num_uniform_buffers = info.uniformBuffers;
    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shaderInfo);

    if (shader == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create shader!");
        std::exit(1);
    }

    return shader;
}

// https://github.com/ocornut/imgui/blob/master/examples/example_sdl3_sdlgpu3/main.cpp
void initImGui() {
    float mainScale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io = &ImGui::GetIO();
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(mainScale);
    style.FontScaleDpi = mainScale;

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLGPU(window);
    ImGui_ImplSDLGPU3_InitInfo init_info = {};
    init_info.Device = device;
    init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(device, window);
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;                      // Only used in multi-viewports mode.
    init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;  // Only used in multi-viewports mode.
    init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
    ImGui_ImplSDLGPU3_Init(&init_info);

}

void init() {

    window = SDL_CreateWindow("Graphics From Scratch", 800, 600,
        SDL_WINDOW_RESIZABLE);

    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    SDL_RaiseWindow(window);
    // confines mouse to window
    SDL_SetWindowRelativeMouseMode(window, true);

    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
    aspectRatio =
        static_cast<float>(windowWidth) / static_cast<float>(windowHeight);

    // create gpu device with shaders for vulkan or metal and choose the best
    // driver NULL chooses the best driver
    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV |
        SDL_GPU_SHADERFORMAT_MSL |
        SDL_GPU_SHADERFORMAT_DXIL,
        true, nullptr);

    SDL_Log("Using GPU device driver: %s", SDL_GetGPUDeviceDriver(device));
    SDL_Log("Base path: %s", BASE_PATH.c_str());

    SDL_ClaimWindowForGPUDevice(device, window);

    //SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR, SDL_GPU_PRESENTMODE_VSYNC);
    SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR, SDL_GPU_PRESENTMODE_IMMEDIATE);


    // SDL_GPUTextureCreateInfo depthTextureCreateInfo{};
    depthTextureCreateInfo.type = SDL_GPU_TEXTURETYPE_2D;
    depthTextureCreateInfo.format = DEPTH_TEXTURE_FORMAT;

    // WILL CAUSE PROBLEMS DUE TO RESIZING (not anymore we just update it on
    // window resize)
    depthTextureCreateInfo.width = windowWidth;
    depthTextureCreateInfo.height = windowHeight;
    depthTextureCreateInfo.layer_count_or_depth = 1;
    depthTextureCreateInfo.num_levels = 1;
    depthTextureCreateInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    depthTexture = SDL_CreateGPUTexture(device, &depthTextureCreateInfo);
    SDL_SetGPUTextureName(device, depthTexture, "Depth Texture");

    camera.position = glm::vec3{ 0, 1, 3 };
    camera.target = glm::vec3{ 0, 1, 0 };

    initImGui();

}

void setupPipeline() {
    // KEEP IN MIND THE UNIFORM BUFFER COUNT
    SDL_GPUShader* vertexShader = LoadShader(device, "PositionColorTexturePerspective.vert");
    if (!vertexShader) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "vertex shader failed ;(");
        std::exit(1);
    }

    // KEEP IN MIND THE UNIFORM BUFFER COUNT
    SDL_GPUShader* fragmentShader =
        LoadShader(device, "CustomTexturedQuad.frag");
    // SDL_GPUShader *fragmentShader =
    //     LoadShader(device, "SolidColor.frag", 0, 0, 0, 0);
    if (!fragmentShader) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "fragment shader failed ;(");
        std::exit(1);
    }

    SDL_GPUColorTargetDescription colorTargetDescriptions[1];
    colorTargetDescriptions[0] = {};
    colorTargetDescriptions[0].format =
        SDL_GetGPUSwapchainTextureFormat(device, window);

    SDL_GPUGraphicsPipelineTargetInfo targetInfo{
        .color_target_descriptions = colorTargetDescriptions,
        .num_color_targets = 1,
        .depth_stencil_format = DEPTH_TEXTURE_FORMAT,
        .has_depth_stencil_target = true,
    };

    // describe the vertex buffers
    SDL_GPUVertexBufferDescription vertexBufferDescriptions[1];
    // slot, pitch, input_rate, instance_step_rate
    vertexBufferDescriptions[0].slot = 0; // vertex buffer set to slot 0
    vertexBufferDescriptions[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBufferDescriptions[0].instance_step_rate = 0;
    vertexBufferDescriptions[0].pitch =
        sizeof(Vertex); // how many bytes to jump after each cycle

    SDL_GPUVertexAttribute vertexAttributes[3];
    // location, buffer_slot, format, offset
    // location: float3 Position in Input in PositionColor.vert
    // buffer_slot: 0 for Input
    // format: float3 Position
    // offset: how many bytes over from the start of Input
    vertexAttributes[0] =
        SDL_GPUVertexAttribute{ 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                               offsetof(Vertex, Vertex::position) };
    vertexAttributes[1] =
        SDL_GPUVertexAttribute{ 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
                               offsetof(Vertex, Vertex::color) };
    vertexAttributes[2] = SDL_GPUVertexAttribute{
        2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Vertex, Vertex::uv) };

    SDL_GPUVertexInputState vertexInputState{
        .vertex_buffer_descriptions = vertexBufferDescriptions,
        .num_vertex_buffers = 1,
        .vertex_attributes = vertexAttributes,
        .num_vertex_attributes = SDL_arraysize(vertexAttributes),
    };

    SDL_GPUDepthStencilState depthStencil{};
    depthStencil.enable_depth_test = true;
    depthStencil.enable_depth_write = true;
    depthStencil.compare_op = SDL_GPU_COMPAREOP_LESS;

    // MAKE PIPELINE
    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{
        .vertex_shader = vertexShader,
        .fragment_shader = fragmentShader,
        .vertex_input_state = vertexInputState,
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state = {
            //.fill_mode = SDL_GPU_FILLMODE_FILL,
            //.fill_mode = SDL_GPU_FILLMODE_LINE,
            .cull_mode = SDL_GPU_CULLMODE_BACK,
        },
        .multisample_state = {},
        .depth_stencil_state = depthStencil,
        .target_info = targetInfo,
    };

    fillPipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
    if (!fillPipeline) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create graphics pipeline, error: %s",
            SDL_GetError());
        std::exit(1);
    }

    SDL_ReleaseGPUShader(device, vertexShader);
    SDL_ReleaseGPUShader(device, fragmentShader);

    // LinearClamp
    SDL_GPUSamplerCreateInfo samplerCreateInfo{
        .min_filter = SDL_GPU_FILTER_LINEAR,
        .mag_filter = SDL_GPU_FILTER_LINEAR,
        .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    };
    sampler = SDL_CreateGPUSampler(device, &samplerCreateInfo);
}

Model loadModel(const char* meshPath, const char* modelPath) {
    SDL_GPUBuffer* vertexBuffer;
    SDL_GPUBuffer* indexBuffer;
    SDL_GPUTexture* colormapTexture;

    // use tinyobjloader to load .obj as vectors of our custom vertices and
    // indices
    ObjData objData;
    objData.loadModel(modelPath);
    const std::vector<Vertex>& vertices = objData.vertices;
    const std::vector<Uint32>& indices = objData.indices;

    SDL_Surface* imageDataSurface = IMG_Load(meshPath);
    imageDataSurface =
        SDL_ConvertSurface(imageDataSurface, SDL_PIXELFORMAT_ABGR8888);
    if (!imageDataSurface) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "image load failed ;(");
        std::exit(1);
        // return SDL_APP_FAILURE;
    }

    SDL_GPUBufferCreateInfo vertexBufferCreateInfo{
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = static_cast<Uint32>(vertices.size() *
            sizeof(vertices[0])), // size of the whole vertex buffer
    };
    vertexBuffer = SDL_CreateGPUBuffer(device, &vertexBufferCreateInfo);
    SDL_SetGPUBufferName(device, vertexBuffer, "Vertex Buffer");

    SDL_GPUBufferCreateInfo indexBufferCreateInfo{
        .usage = SDL_GPU_BUFFERUSAGE_INDEX,
        .size = static_cast<Uint32>(indices.size() *
        sizeof(indices[0])), // size of the whole index buffer
    };
    indexBuffer = SDL_CreateGPUBuffer(device, &indexBufferCreateInfo);
    SDL_SetGPUBufferName(device, indexBuffer, "Index Buffer");

    SDL_GPUTextureCreateInfo textureCreateInfo{
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = static_cast<Uint32>(imageDataSurface->w),
        .height = static_cast<Uint32>(imageDataSurface->h),
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };
    colormapTexture = SDL_CreateGPUTexture(device, &textureCreateInfo);
    SDL_SetGPUTextureName(device, colormapTexture, "Colormap Texture");

    // we have to use a transfer buffer to upload triangle data into the vertex
    // buffer
    // transfer buffer: special GPU memory that is used to transfer data from
    // the CPU to the GPU
    SDL_GPUTransferBufferCreateInfo transferBufferCreateInfo{
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = vertexBufferCreateInfo.size +
            indexBufferCreateInfo.size, // also the size of the vertex buffer
    };
    SDL_GPUTransferBuffer* transferBuffer =
        SDL_CreateGPUTransferBuffer(device, &transferBufferCreateInfo);

    void* transferData =
        (void*)SDL_MapGPUTransferBuffer(device, transferBuffer, false);

    SDL_memcpy(transferData, vertices.data(), vertexBufferCreateInfo.size);
    // copy index buffer to next section
    // dest, source
    void* transferDataIndicesPtr = static_cast<void*>(
        static_cast<char*>(transferData) + vertexBufferCreateInfo.size);
    SDL_memcpy(transferDataIndicesPtr, indices.data(),
        indexBufferCreateInfo.size);

    SDL_UnmapGPUTransferBuffer(device, transferBuffer);

    // transfer texture
    SDL_GPUTransferBufferCreateInfo textureTransferBufferCreateInfo{
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = static_cast<Uint32>(imageDataSurface->w * imageDataSurface->h * 4), // 4 bytes per pixel
    };
    SDL_GPUTransferBuffer* textureTransferBuffer =
        SDL_CreateGPUTransferBuffer(device, &textureTransferBufferCreateInfo);
    Uint8* textureTransferData = static_cast<Uint8*>(
        SDL_MapGPUTransferBuffer(device, textureTransferBuffer, false));
    SDL_memcpy(textureTransferData, imageDataSurface->pixels,
        textureTransferBufferCreateInfo.size);
    SDL_UnmapGPUTransferBuffer(device, textureTransferBuffer);

    // upload transfer data to vertex buffer
    SDL_GPUCommandBuffer* uploadCommandBuffer =
        SDL_AcquireGPUCommandBuffer(device);

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCommandBuffer);

    SDL_GPUTransferBufferLocation transferBufferLocation{};
    transferBufferLocation.transfer_buffer = transferBuffer;
    transferBufferLocation.offset = 0;

    SDL_GPUBufferRegion bufferRegion{};
    bufferRegion.buffer = vertexBuffer;
    bufferRegion.offset = 0;
    bufferRegion.size = vertexBufferCreateInfo.size;
    SDL_UploadToGPUBuffer(copyPass, &transferBufferLocation, &bufferRegion,
        false);

    transferBufferLocation.offset = vertexBufferCreateInfo.size; // move over to the index buffer section
    bufferRegion.buffer = indexBuffer;
    bufferRegion.size = indexBufferCreateInfo.size;
    bufferRegion.offset = 0;
    SDL_UploadToGPUBuffer(copyPass, &transferBufferLocation, &bufferRegion,
        false);

    SDL_GPUTextureTransferInfo textureTransferInfo{};
    textureTransferInfo.transfer_buffer = textureTransferBuffer;
    textureTransferInfo.offset = 0;
    SDL_GPUTextureRegion textureRegion{};
    textureRegion.texture = colormapTexture;
    textureRegion.w = imageDataSurface->w;
    textureRegion.h = imageDataSurface->h;
    textureRegion.d = 1;
    SDL_UploadToGPUTexture(copyPass, &textureTransferInfo, &textureRegion,
        false);

    SDL_EndGPUCopyPass(copyPass);

    SDL_SubmitGPUCommandBuffer(uploadCommandBuffer);

    SDL_DestroySurface(imageDataSurface);
    SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
    SDL_ReleaseGPUTransferBuffer(device, textureTransferBuffer);

    return Model{ vertexBuffer, indexBuffer, static_cast<Uint32>(indices.size()),
                 colormapTexture };
}

void updateCamera(float deltaTime) {
    glm::vec2 movementInput{// x
                            (cameraRightPressed - cameraLeftPressed),

                            // z
                            (cameraForwardPressed - cameraBackwardPressed) };

    // y
    float yInput = cameraUpPressed - cameraDownPressed;

    // need to convert two angles (from mouse cumulative movement) to x, y,
    // and z on screen relative to camera position x: yaw, y: pitch


    // only changes look's start and end values if the mouse moved
    if (mouseVelocityVector.x != 0.0f || mouseVelocityVector.y != 0.0f) {

        // wrap angle (yaw)
        look.x = SDL_fmodf(look.x - mouseVelocityVector.x * MOUSE_SENSITIVITY,
            2.0f * SDL_PI_F);
        // clamp to -90, 90 (pitch)
        look.y =
            SDL_clamp(look.y - mouseVelocityVector.y * MOUSE_SENSITIVITY,
                glm::radians(-89.0f), glm::radians(89.0f));

        //SDL_Log("%f, %f", look.yaw, look.pitch);
    }

    //glm::mat3 yawPitchRollMatrix =
    //    glm::yawPitchRoll(look.getValue().x, look.getValue().y, 0.0f);

    glm::mat3 yawPitchRollMatrix =
        glm::yawPitchRoll(look.x, look.y, 0.0f);

    // glm::vec3 lookDirection{SDL_sinf(look.yaw), 0.0f,
    // SDL_cosf(look.yaw)};
    glm::vec3 forward = yawPitchRollMatrix * glm::vec3{ 0.0f, 0.0f, -1.0f };
    glm::vec3 right = yawPitchRollMatrix * glm::vec3{ 1.0f, 0.0f, 0.0f };

    glm::vec3 movementDirection =
        forward * movementInput.y + right * movementInput.x;
    movementDirection.y = yInput;

    // normalize vector
    if (movementDirection != glm::zero<glm::vec3>())
        movementDirection = glm::normalize(movementDirection);

    // deltatimeify
    movementDirection = movementDirection * CAMERA_SPEED * deltaTime;

    camera.position += movementDirection;
    camera.target = camera.position + forward;
}

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    /* Create the window */
    // window = SDL_CreateWindow("Graphics From Scratch", 800, 600,
    //                           SDL_WINDOW_FULLSCREEN);
    // SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    // SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);

    init();
    setupPipeline();
    model = loadModel(MESH_PATH, MODEL_PATH);

    currentTime = SDL_GetPerformanceCounter();
    previousTime = currentTime;

    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc)
 * occurs. */
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {


    switch (event->type) {
    case SDL_EVENT_QUIT:
    return SDL_APP_SUCCESS; /* end the program, reporting success to the
                               OS.*/
    break;

    case SDL_EVENT_WINDOW_RESIZED:
    // recreate the depth texture every time the window is resized
    // scrap old depth texture with old width and height
    SDL_ReleaseGPUTexture(device, depthTexture);
    // get new width and height
    if (!SDL_GetWindowSize(window, &windowWidth, &windowHeight)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't get window size :(");
        return SDL_APP_FAILURE;
    }

    // update width and height
    depthTextureCreateInfo.width = windowWidth;
    depthTextureCreateInfo.height = windowHeight;
    aspectRatio =
        static_cast<float>(windowWidth) / static_cast<float>(windowHeight);

    // recreate depthTexture
    depthTexture = SDL_CreateGPUTexture(device, &depthTextureCreateInfo);
    SDL_SetGPUTextureName(device, depthTexture, "Depth Texture");

    if (!depthTexture) {

        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Couldn't recreate depth texture after window resize ;(");
        return SDL_APP_FAILURE;
    }
    break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    SDL_SetWindowRelativeMouseMode(window, true);
    break;

    case SDL_EVENT_MOUSE_MOTION:
    if (SDL_GetWindowRelativeMouseMode(window)) {
        mouseVelocityVector += glm::vec2{ event->motion.xrel, event->motion.yrel };
    }
    break;

    case SDL_EVENT_KEY_DOWN:
    switch (event->key.key) {
    case SDLK_W:
    cameraForwardPressed = true;
    break;
    case SDLK_S:
    cameraBackwardPressed = true;
    break;
    case SDLK_A:
    cameraLeftPressed = true;
    break;
    case SDLK_D:
    cameraRightPressed = true;
    break;
    case SDLK_SPACE:
    cameraUpPressed = true;
    break;
    case SDLK_LSHIFT:
    cameraDownPressed = true;
    break;
    case SDLK_ESCAPE:
    SDL_SetWindowRelativeMouseMode(window, false);
    break;
    case SDLK_F11:
    SDL_SetWindowFullscreen(window, !SDL_GetWindowFullscreenMode(window));
    break;
    }
    break;

    case SDL_EVENT_KEY_UP:
    switch (event->key.key) {
    case SDLK_W:
    cameraForwardPressed = false;
    break;
    case SDLK_S:
    cameraBackwardPressed = false;
    break;
    case SDLK_A:
    cameraLeftPressed = false;
    break;
    case SDLK_D:
    cameraRightPressed = false;
    break;
    case SDLK_SPACE:
    cameraUpPressed = false;
    break;
    case SDLK_LSHIFT:
    cameraDownPressed = false;
    break;
    }
    break;
    }

    return SDL_APP_CONTINUE;
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void* appstate) {

    currentTime = SDL_GetPerformanceCounter();
    deltaTime = static_cast<float>(currentTime - previousTime) / static_cast<float>(SDL_GetPerformanceFrequency());
    previousTime = currentTime;

    float fps = 1.0f / deltaTime;
    fpsSum += fps;
    numFPSChecksInInterval++;

    //SDL_Log("seconds since last fps check: %f", static_cast<float>(currentTime - lastFPSCheckTime) / static_cast<float>(SDL_GetPerformanceFrequency()));
    if (currentTime - lastFPSCheckTime > COUNTS_PER_FPS_CHECK) {
        averageFPSOverCheckInterval = fpsSum / numFPSChecksInInterval;

        numFPSChecksInInterval = 0;
        fpsSum = 0.0f;
        lastFPSCheckTime = currentTime;
        SDL_Log("average FPS over %.1f second(s): %.1f", SECONDS_PER_FPS_CHECK, averageFPSOverCheckInterval);
    }

    updateCamera(deltaTime);

    // rotation += deltaTime: 1 radian per second
    // want: pi radians per second
    rotation += ROTATION_SPEED * deltaTime;

    // Start the Dear ImGui frame
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();


    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);


    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io->Framerate, io->Framerate);
        ImGui::End();
    }


    // 3. Show another simple window.
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }

    // Rendering
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);


    // already getting window size in SDL_AppEvent
    // SDL_GetWindowSize(window, &windowWidth, &windowHeight);

    // list of commands to send to the gpu for fast execution
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);

    // swapchain: basically loading the next frame as the previous one is
    // being drawn
    SDL_GPUTexture* swapchainTexture;
    // window width and height
    Uint32 width, height;
    // acquire the next swapchain texture
    // be careful because the texture might be NULL (ex. window is
    // minimized) SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer,
    // window,
    //                                      &swapchainTexture, &width,
    //                                      &height);
    SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window,
        &swapchainTexture, &width, &height);
    // if the texture is NULL (ex. minimized), submit and return
    if (swapchainTexture == nullptr) {
        SDL_SubmitGPUCommandBuffer(commandBuffer);
        return SDL_APP_CONTINUE;
    }

    float widthf = static_cast<float>(width);
    float heightf = static_cast<float>(height);

    // CREATE COLOR TARGET

    // color target: where to draw the things
    SDL_GPUColorTargetInfo colorTargetInfo{};

    // replace previous color, clear (screen?) with color
    colorTargetInfo.clear_color = { 0.0f, 0.2f, 0.4f };
    //colorTargetInfo.clear_color =
    //    SDL_FColor{0x71 / 255.0f, 0x79 / 255.0f, 0x7E / 255.0f, 255 / 255.0f};
    // discard previous content
    colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR; // or SDL_GPU_LOADOP_LOAD to
    // keep the previous content
    colorTargetInfo.store_op =
        SDL_GPU_STOREOP_STORE; // store content to texture
    colorTargetInfo.texture = swapchainTexture;

    SDL_GPUDepthStencilTargetInfo depthStencilTargetInfo{
        .texture = depthTexture,
        .clear_depth = 1,
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_DONT_CARE,
    };

    ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, commandBuffer);

    // BEGIN RENDER PASS

    // color_target_infos: array of render targets, lets you render multiple
    // at the same time num_color_targets: size of array
    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(
        commandBuffer, &colorTargetInfo, 1, &depthStencilTargetInfo);


    // bind the graphics pipeline
    SDL_BindGPUGraphicsPipeline(renderPass, fillPipeline);

    // DRAW SOMETHING
    SDL_GPUBufferBinding vertexBufferBinding{
        .buffer = model.vertexBuffer,
        .offset = 0,
    };
    SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBufferBinding, 1);

    SDL_GPUBufferBinding indexBufferBinding{
        .buffer = model.indexBuffer,
        .offset = 0,
    };
    SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding,
        SDL_GPU_INDEXELEMENTSIZE_32BIT);

    SDL_GPUTextureSamplerBinding textureSamplerBinding{
        .texture = model.colormapTexture,
        .sampler = sampler,
    };
    SDL_BindGPUFragmentSamplers(renderPass, 0, &textureSamplerBinding, 1);

    // create matrices so the gpu can use them to transform the vertices to
    // go on the screen where they belong
    glm::mat4 projectionMatrix =
        glm::perspective(glm::radians(70.0f), aspectRatio, 0.01f, 1000.0f);
    // glm::mat4 viewMatrix =
    //     glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, -3.0f));
    glm::mat4 viewMatrix = glm::lookAt(camera.position, camera.target,
        glm::vec3{ 0.0f, 1.0f, 0.0f });

    // rotation matrix
    glm::mat4 modelMatrix =
        glm::rotate(glm::mat4(1.0f), rotation, glm::vec3(0.0f, 1.0f, 0.0f));

    // calculate mvp matrix to send to the vertex shader
    matrixUniform.mvp = projectionMatrix * viewMatrix * modelMatrix;

    // push vertex uniform data so the gpu can perform the matrix
    // multiplication
    SDL_PushGPUVertexUniformData(commandBuffer, 0, &matrixUniform,
        sizeof(matrixUniform));

    // fragment shader uniforms
    // SDL_PushGPUFragmentUniformData();

    SDL_DrawGPUIndexedPrimitives(renderPass, model.numIndices, 1, 0, 0, 0);

    ImGui_ImplSDLGPU3_RenderDrawData(draw_data, commandBuffer, renderPass);

    // END RENDER PASS

    SDL_EndGPURenderPass(renderPass);

    // do now
    SDL_SubmitGPUCommandBuffer(commandBuffer);

    mouseVelocityVector = { 0.0f, 0.0f };

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void* appstate, SDL_AppResult result) {

    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui::DestroyContext();

    SDL_ReleaseGPUGraphicsPipeline(device, fillPipeline);

    SDL_ReleaseGPUTexture(device, model.colormapTexture);
    SDL_ReleaseGPUTexture(device, depthTexture);

    SDL_ReleaseGPUBuffer(device, model.vertexBuffer);
    SDL_ReleaseGPUBuffer(device, model.indexBuffer);

    SDL_ReleaseGPUSampler(device, sampler);

    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
}
