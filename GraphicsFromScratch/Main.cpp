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
#include <vector>

SDL_Window *window;
int windowWidth, windowHeight;
float aspectRatio;

SDL_GPUDevice *device;
// specifies which shaders to use, how many buffers, vertex inputs, color
// blending
SDL_GPUGraphicsPipeline *fillPipeline;
SDL_GPUTexture *depthTexture;
SDL_GPUTextureCreateInfo
    depthTextureCreateInfo{}; // for updating width and height later

constexpr SDL_GPUTextureFormat DEPTH_TEXTURE_FORMAT =
    // SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    // VVV ONLY WORKS FOR 4060
    // SDL_GPU_TEXTUREFORMAT_D24_UNORM;
    SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

SDL_GPUSampler *sampler;

struct Camera {
    glm::vec3 position;
    glm::vec3 target;
};
Camera camera;

struct Look {
    float yaw;
    float pitch;
};
Look look{0.0f, 0.0f};

glm::vec2 mouseVelocityVector{0.0f, 0.0f};
// constexpr float MOUSE_SENSITIVITY = 0.002f;
constexpr float MOUSE_SENSITIVITY = 0.01f;

// units per second
constexpr float CAMERA_SPEED = 5.0f;

Uint64 currentTime;
Uint64 previousTime;
// deltaTime in seconds
float deltaTime;

float rotation;

// radians per second
const float ROTATION_SPEED = SDL_PI_F / 4.0f;

bool cameraUpPressed;
bool cameraDownPressed;
bool cameraLeftPressed;
bool cameraRightPressed;
bool cameraForwardPressed;
bool cameraBackwardPressed;

const char *MODEL_PATH = "Content/Meshes/sedan-sports.obj";
const char *MESH_PATH = "Content/Meshes/colormap.png";

struct Model {
    SDL_GPUBuffer *vertexBuffer;
    SDL_GPUBuffer *indexBuffer;
    Uint32 numIndices;
    SDL_GPUTexture *colormapTexture;
};

Model model;

struct MatrixUniformBuffer {
    glm::mat4 mvp;
};

static MatrixUniformBuffer matrixUniform;

constexpr glm::vec4 WHITE{1, 1, 1, 1};

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

// ripped from SDL_gpu examples
SDL_GPUShader *LoadShader(SDL_GPUDevice *device, const char *shaderFilename,
                          const Uint32 samplerCount,
                          const Uint32 uniformBufferCount,
                          const Uint32 storageBufferCount,
                          const Uint32 storageTextureCount) {
    // Auto-detect the shader stage from the file name for convenience
    SDL_GPUShaderStage stage;
    if (SDL_strstr(shaderFilename, ".vert")) {
        stage = SDL_GPU_SHADERSTAGE_VERTEX;
    } else if (SDL_strstr(shaderFilename, ".frag")) {
        stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    } else {
        SDL_Log("Invalid shader stage!");
        return nullptr;
    }

    char *fullPath;
    SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(device);
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
    const char *entrypoint;

    if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV) {
        SDL_asprintf(&fullPath, "Content/Shaders/Compiled/SPIRV/%s.spv",
                     shaderFilename);
        // SDL_asprintf(&fullPath, "%sContent/Shaders/Compiled/SPIRV/%s.spv",
        //              SDL_GetBasePath(), shaderFilename);
        format = SDL_GPU_SHADERFORMAT_SPIRV;
        entrypoint = "main";
    } else if (backendFormats & SDL_GPU_SHADERFORMAT_MSL) {
        SDL_asprintf(&fullPath, "Content/Shaders/Compiled/MSL/%s.msl",
                     shaderFilename);
        // SDL_asprintf(&fullPath, "%sContent/Shaders/Compiled/MSL/%s.msl",
        //              SDL_GetBasePath(), shaderFilename);
        format = SDL_GPU_SHADERFORMAT_MSL;
        entrypoint = "main0";
    } else if (backendFormats & SDL_GPU_SHADERFORMAT_DXIL) {
        SDL_asprintf(&fullPath, "Content/Shaders/Compiled/DXIL/%s.dxil",
                     shaderFilename);
        // SDL_asprintf(&fullPath, "%sContent/Shaders/Compiled/DXIL/%s.dxil",
        //              SDL_GetBasePath(), shaderFilename);
        format = SDL_GPU_SHADERFORMAT_DXIL;
        entrypoint = "main";
    } else {
        SDL_Log("%s", "Unrecognized backend shader format!");
        return nullptr;
    }

    size_t codeSize;
    void *code = SDL_LoadFile(fullPath, &codeSize);
    if (code == nullptr) {
        SDL_Log("Failed to load shader from disk! %s", fullPath);
        return nullptr;
    }

    SDL_GPUShaderCreateInfo shaderInfo{};
    shaderInfo.code = (Uint8 *)code;
    shaderInfo.code_size = codeSize;
    shaderInfo.entrypoint = entrypoint;
    shaderInfo.format = format;
    shaderInfo.stage = stage;
    shaderInfo.num_samplers = samplerCount;
    shaderInfo.num_uniform_buffers = uniformBufferCount;
    shaderInfo.num_storage_buffers = storageBufferCount;
    shaderInfo.num_storage_textures = storageTextureCount;
    SDL_GPUShader *shader = SDL_CreateGPUShader(device, &shaderInfo);

    if (shader == nullptr) {
        SDL_Log("Failed to create shader!");
        SDL_free(code);
        return nullptr;
    }

    SDL_free(code);
    return shader;
}

void init() {

    window = SDL_CreateWindow("Graphics From Scratch", 800, 600,
                              SDL_WINDOW_RESIZABLE);
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
    SDL_Log("Base path: %s", SDL_GetBasePath());

    SDL_ClaimWindowForGPUDevice(device, window);

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

    camera.position = glm::vec3{0, 1, 3};
    camera.target = glm::vec3{0, 1, 0};
}

void setupPipeline() {
    // KEEP IN MIND THE UNIFORM BUFFER COUNT
    SDL_GPUShader *vertexShader =
        LoadShader(device, "PositionColorTexturePerspective.vert", 0, 1, 0, 0);
    if (!vertexShader) {
        SDL_Log("vertex shader failed ;(");
        SDL_Quit();
    }

    // KEEP IN MIND THE UNIFORM BUFFER COUNT
    SDL_GPUShader *fragmentShader =
        LoadShader(device, "CustomTexturedQuad.frag", 1, 0, 0, 0);
    // SDL_GPUShader *fragmentShader =
    //     LoadShader(device, "SolidColor.frag", 0, 0, 0, 0);
    if (!fragmentShader) {
        SDL_Log("fragment shader failed ;(");
        SDL_Quit();
    }
    SDL_GPUColorTargetDescription colorTargetDescriptions[1];
    colorTargetDescriptions[0] = {};
    colorTargetDescriptions[0].format =
        SDL_GetGPUSwapchainTextureFormat(device, window);

    SDL_GPUGraphicsPipelineTargetInfo targetInfo{};
    targetInfo.num_color_targets = 1;
    targetInfo.has_depth_stencil_target = true;
    targetInfo.depth_stencil_format = DEPTH_TEXTURE_FORMAT;
    targetInfo.color_target_descriptions = colorTargetDescriptions;

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
        SDL_GPUVertexAttribute{0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                               offsetof(Vertex, Vertex::position)};
    vertexAttributes[1] =
        SDL_GPUVertexAttribute{1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
                               offsetof(Vertex, Vertex::color)};
    vertexAttributes[2] = SDL_GPUVertexAttribute{
        2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Vertex, Vertex::uv)};

    SDL_GPUVertexInputState vertexInputState{};
    vertexInputState.num_vertex_buffers = 1;
    vertexInputState.vertex_buffer_descriptions = vertexBufferDescriptions;
    vertexInputState.num_vertex_attributes = SDL_arraysize(vertexAttributes);
    vertexInputState.vertex_attributes = vertexAttributes;

    SDL_GPUDepthStencilState depthStencil{};
    depthStencil.enable_depth_test = true;
    depthStencil.enable_depth_write = true;
    depthStencil.compare_op = SDL_GPU_COMPAREOP_LESS;

    //SDL_GPURasterizerState rasterizerState{};
    //rasterizerState.cull_mode = SDL_GPU_CULLMODE_BACK;

    // MAKE PIPELINE
    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.vertex_shader = vertexShader;
    pipelineInfo.fragment_shader = fragmentShader;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineInfo.depth_stencil_state = depthStencil;
     //pipelineInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
     //pipelineInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_LINE;
    pipelineInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
    pipelineInfo.target_info = targetInfo;
    pipelineInfo.vertex_input_state = vertexInputState;

    fillPipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
    if (!fillPipeline) {
        SDL_Log("Failed to create graphics pipeline, error: %s",
                SDL_GetError());
        SDL_Quit();
    }

    SDL_ReleaseGPUShader(device, vertexShader);
    SDL_ReleaseGPUShader(device, fragmentShader);

    // LinearClamp
    SDL_GPUSamplerCreateInfo samplerCreateInfo{};
    samplerCreateInfo.min_filter = SDL_GPU_FILTER_LINEAR;
    samplerCreateInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
    samplerCreateInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    samplerCreateInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerCreateInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerCreateInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler = SDL_CreateGPUSampler(device, &samplerCreateInfo);
}

Model loadModel(const char *meshPath, const char *modelPath) {
    SDL_GPUBuffer *vertexBuffer;
    SDL_GPUBuffer *indexBuffer;
    SDL_GPUTexture *colormapTexture;

    // use tinyobjloader to load .obj as vectors of our custom vertices and
    // indices
    ObjData objData;
    objData.loadModel(modelPath);
    const std::vector<Vertex> &vertices = objData.vertices;
    const std::vector<Uint32> &indices = objData.indices;

    SDL_Surface *imageDataSurface = IMG_Load(meshPath);
    imageDataSurface =
        SDL_ConvertSurface(imageDataSurface, SDL_PIXELFORMAT_ABGR8888);
    if (!imageDataSurface) {
        SDL_Log("image load failed ;(");
        // return SDL_APP_FAILURE;
    }

    SDL_GPUBufferCreateInfo vertexBufferCreateInfo{};
    vertexBufferCreateInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertexBufferCreateInfo.size =
        vertices.size() *
        sizeof(vertices[0]); // size of the whole vertex buffer
    vertexBuffer = SDL_CreateGPUBuffer(device, &vertexBufferCreateInfo);
    SDL_SetGPUBufferName(device, vertexBuffer, "Vertex Buffer");

    SDL_GPUBufferCreateInfo indexBufferCreateInfo{};
    indexBufferCreateInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    indexBufferCreateInfo.size = indices.size() * sizeof(indices[0]);
    indexBuffer = SDL_CreateGPUBuffer(device, &indexBufferCreateInfo);
    SDL_SetGPUBufferName(device, indexBuffer, "Index Buffer");

    SDL_GPUTextureCreateInfo textureCreateInfo{};
    textureCreateInfo.type = SDL_GPU_TEXTURETYPE_2D;
    textureCreateInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    textureCreateInfo.width = imageDataSurface->w;
    textureCreateInfo.height = imageDataSurface->h;
    textureCreateInfo.layer_count_or_depth = 1;
    textureCreateInfo.num_levels = 1;
    textureCreateInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    colormapTexture = SDL_CreateGPUTexture(device, &textureCreateInfo);
    SDL_SetGPUTextureName(device, colormapTexture, "Colormap Texture");

    // we have to use a transfer buffer to upload triangle data into the vertex
    // buffer
    // transfer buffer: special GPU memory that is used to transfer data from
    // the CPU to the GPU
    SDL_GPUTransferBufferCreateInfo transferBufferCreateInfo{};
    transferBufferCreateInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferBufferCreateInfo.size =
        vertexBufferCreateInfo.size +
        indexBufferCreateInfo.size; // also the size of the vertex buffer
    SDL_GPUTransferBuffer *transferBuffer =
        SDL_CreateGPUTransferBuffer(device, &transferBufferCreateInfo);

    void *transferData =
        (void *)SDL_MapGPUTransferBuffer(device, transferBuffer, false);

    SDL_memcpy(transferData, vertices.data(), vertexBufferCreateInfo.size);
    // copy index buffer to next section
    // dest, source
    void *transferDataIndicesPtr = static_cast<void *>(
        static_cast<char *>(transferData) + vertexBufferCreateInfo.size);
    SDL_memcpy(transferDataIndicesPtr, indices.data(),
               indexBufferCreateInfo.size);

    SDL_UnmapGPUTransferBuffer(device, transferBuffer);

    // transfer texture
    SDL_GPUTransferBufferCreateInfo textureTransferBufferCreateInfo{};
    textureTransferBufferCreateInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    textureTransferBufferCreateInfo.size =
        imageDataSurface->w * imageDataSurface->h * 4; // 4 bytes per pixel
    SDL_GPUTransferBuffer *textureTransferBuffer =
        SDL_CreateGPUTransferBuffer(device, &textureTransferBufferCreateInfo);
    Uint8 *textureTransferData = static_cast<Uint8 *>(
        SDL_MapGPUTransferBuffer(device, textureTransferBuffer, false));
    SDL_memcpy(textureTransferData, imageDataSurface->pixels,
               textureTransferBufferCreateInfo.size);
    SDL_UnmapGPUTransferBuffer(device, textureTransferBuffer);

    // upload transfer data to vertex buffer
    SDL_GPUCommandBuffer *uploadCommandBuffer =
        SDL_AcquireGPUCommandBuffer(device);

    SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(uploadCommandBuffer);

    SDL_GPUTransferBufferLocation transferBufferLocation{};
    transferBufferLocation.transfer_buffer = transferBuffer;
    transferBufferLocation.offset = 0;

    SDL_GPUBufferRegion bufferRegion{};
    bufferRegion.buffer = vertexBuffer;
    bufferRegion.size = vertexBufferCreateInfo.size;
    bufferRegion.offset = 0;
    SDL_UploadToGPUBuffer(copyPass, &transferBufferLocation, &bufferRegion,
                          false);

    transferBufferLocation.offset =
        vertexBufferCreateInfo.size; // move over to the index buffer section
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

    return Model{vertexBuffer, indexBuffer, static_cast<Uint32>(indices.size()),
                 colormapTexture};
}

void updateCamera(float deltaTime) {
    glm::vec2 movementInput{// x
                            (cameraRightPressed - cameraLeftPressed),

                            // z
                            (cameraForwardPressed - cameraBackwardPressed)};

    // y
    float yInput =  cameraUpPressed - cameraDownPressed;

    // need to convert two angles (from mouse cumulative movement) to x, y,
    // and z on screen relative to camera position x: yaw, y: pitch

    // wrap angle
    look.yaw = SDL_fmodf(look.yaw - mouseVelocityVector.x * MOUSE_SENSITIVITY,
                         2.0f * SDL_PI_F);
    // clamp to -90, 90
    look.pitch =
        SDL_clamp(look.pitch - mouseVelocityVector.y * MOUSE_SENSITIVITY,
                  glm::radians(-89.0f), glm::radians(89.0f));

    // SDL_Log("%f, %f", look.yaw, look.pitch);

    glm::mat3 yawPitchRollMatrix =
        glm::yawPitchRoll(look.yaw, look.pitch, 0.0f);

    // glm::vec3 lookDirection{SDL_sinf(look.yaw), 0.0f,
    // SDL_cosf(look.yaw)};
    glm::vec3 forward = yawPitchRollMatrix * glm::vec3{0.0f, 0.0f, -1.0f};
    glm::vec3 right = yawPitchRollMatrix * glm::vec3{1.0f, 0.0f, 0.0f};

    glm::vec3 movementDirection =
        forward * movementInput.y + right * movementInput.x;
    movementDirection.y = yInput;

    // normalize vector
    if (movementDirection.x + movementDirection.y + movementDirection.z != 0.0f)
        movementDirection = glm::normalize(movementDirection);

    // deltatimeify
    movementDirection = movementDirection * CAMERA_SPEED * deltaTime;

    camera.position += movementDirection;
    camera.target = camera.position + forward;
}

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    /* Create the window */
    // window = SDL_CreateWindow("Graphics From Scratch", 800, 600,
    //                           SDL_WINDOW_FULLSCREEN);
    // SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    // SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);

    init();
    setupPipeline();
    model = loadModel(MESH_PATH, MODEL_PATH);

    currentTime = SDL_GetTicksNS();
    previousTime = currentTime;

    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc)
 * occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {

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
        if (SDL_GetWindowRelativeMouseMode(window))
            mouseVelocityVector = {event->motion.xrel, event->motion.yrel};
        // SDL_Log("%f, %f", mouseVelocityVector.x, mouseVelocityVector.y);
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
SDL_AppResult SDL_AppIterate(void *appstate) {

    currentTime = SDL_GetTicksNS();
    deltaTime = (currentTime - previousTime) / 1e9f;
    // rotation += deltaTime: 1 radian per second
    // want: pi radians per second
    rotation += ROTATION_SPEED * deltaTime;

    updateCamera(deltaTime);
    mouseVelocityVector = {0.0f, 0.0f};

    previousTime = currentTime;

    // already getting window size in SDL_AppEvent
    // SDL_GetWindowSize(window, &windowWidth, &windowHeight);

    // list of commands to send to the gpu for fast execution
    SDL_GPUCommandBuffer *commandBuffer = SDL_AcquireGPUCommandBuffer(device);

    // swapchain: basically loading the next frame as the previous one is
    // being drawn
    SDL_GPUTexture *swapchainTexture;
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
    // colorTargetInfo.clear_color =
    //    SDL_FColor{0xFB / 255.0f, 0xEA / 255.0f, 0xFF / 255.0f, 255 /
    //    255.0f};
    colorTargetInfo.clear_color =
        SDL_FColor{0x71 / 255.0f, 0x79 / 255.0f, 0x7E / 255.0f, 255 / 255.0f};
    // discard previous content
    colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR; // or SDL_GPU_LOADOP_LOAD to
                                                    // keep the previous content
    colorTargetInfo.store_op =
        SDL_GPU_STOREOP_STORE; // store content to texture
    colorTargetInfo.texture = swapchainTexture;

    SDL_GPUDepthStencilTargetInfo depthStencilTargetInfo{};
    depthStencilTargetInfo.texture = depthTexture;
    depthStencilTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    depthStencilTargetInfo.clear_depth = 1;
    depthStencilTargetInfo.store_op = SDL_GPU_STOREOP_DONT_CARE;

    // BEGIN RENDER PASS

    // color_target_infos: array of render targets, lets you render multiple
    // at the same time num_color_targets: size of array
    SDL_GPURenderPass *renderPass = SDL_BeginGPURenderPass(
        commandBuffer, &colorTargetInfo, 1, &depthStencilTargetInfo);

    // bind the graphics pipeline
    SDL_BindGPUGraphicsPipeline(renderPass, fillPipeline);

    // DRAW SOMETHING
    SDL_GPUBufferBinding vertexBufferBinding{};
    vertexBufferBinding.buffer = model.vertexBuffer;
    vertexBufferBinding.offset = 0;
    SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBufferBinding, 1);

    SDL_GPUBufferBinding indexBufferBinding{};
    indexBufferBinding.buffer = model.indexBuffer;
    indexBufferBinding.offset = 0;
    SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding,
                           SDL_GPU_INDEXELEMENTSIZE_32BIT);

    SDL_GPUTextureSamplerBinding textureSamplerBinding{};
    textureSamplerBinding.texture = model.colormapTexture;
    textureSamplerBinding.sampler = sampler;
    SDL_BindGPUFragmentSamplers(renderPass, 0, &textureSamplerBinding, 1);

    float time = SDL_GetTicksNS() / 1e9f;

    // create matrices so the gpu can use them to transform the vertices to
    // go on the screen where they belong
    glm::mat4 projectionMatrix =
        glm::perspective(glm::radians(70.0f), aspectRatio, 0.01f, 1000.0f);
    // glm::mat4 viewMatrix =
    //     glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, -3.0f));
    glm::mat4 viewMatrix = glm::lookAt(camera.position, camera.target,
                                       glm::vec3{0.0f, 1.0f, 0.0f});

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

    // END RENDER PASS

    SDL_EndGPURenderPass(renderPass);

    // do now
    SDL_SubmitGPUCommandBuffer(commandBuffer);

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    SDL_ReleaseGPUGraphicsPipeline(device, fillPipeline);

    SDL_ReleaseGPUTexture(device, model.colormapTexture);
    SDL_ReleaseGPUTexture(device, depthTexture);

    SDL_ReleaseGPUBuffer(device, model.vertexBuffer);
    SDL_ReleaseGPUBuffer(device, model.indexBuffer);

    SDL_ReleaseGPUSampler(device, sampler);

    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
}
