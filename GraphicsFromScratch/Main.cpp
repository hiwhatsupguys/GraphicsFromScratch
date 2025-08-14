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

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

SDL_Window *window;
SDL_GPUDevice *device;
SDL_GPUBuffer *vertexBuffer;
SDL_GPUBuffer *indexBuffer;
// specifies which shaders to use, how many buffers, vertex inputs, color
// blending
SDL_GPUGraphicsPipeline *fillPipeline;

Uint64 currentTime;
Uint64 previousTime;
float deltaTime;

float rotation;

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

struct MatrixUniformBuffer {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
};

constexpr float ROUNDED_RECT_RADIUS = 0.2f;

//static MatrixUniformBuffer matrixUniform;

struct Vertex {
    glm::vec3 position;
    SDL_FColor color;
    glm::vec2 uv;
};

SDL_FColor WHITE = {1, 1, 1, 1};

// rect
//Vertex vertices[4] = {
//
//    Vertex{{-0.5f, 0.5f, 0.0f}, {1, 0, 0, 1}, {0, 0}},
//    Vertex{{0.5f, 0.5f, 0.0f}, {0, 1, 0, 1}, {1, 0}},
//    Vertex{{0.5f, -0.5f, 0.0f}, {0, 0, 1, 1}, {1, 1}},
//    Vertex{{-0.5f, -0.5f, 0.0f}, {1, 1, 0, 1}, {0, 1}},
//
//};

Vertex vertices[4] = {

    Vertex{{-0.5f, 0.5f, 0.0f}, WHITE, {0, 0}},
    Vertex{{0.5f, 0.5f, 0.0f}, WHITE, {1, 0}},
    Vertex{{0.5f, -0.5f, 0.0f}, WHITE, {1, 1}},
    Vertex{{-0.5f, -0.5f, 0.0f}, WHITE, {0, 1}},

};

Uint16 indices[6] = {0, 1, 2, 0, 2, 3};

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    /* Create the window */
    // window = SDL_CreateWindow("Graphics From Scratch", 800, 600,
    //                           SDL_WINDOW_FULLSCREEN);
    // SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    // SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);

    window = SDL_CreateWindow("Graphics From Scratch", 800, 600,
                              SDL_WINDOW_RESIZABLE);
    SDL_RaiseWindow(window);

    // create gpu device with shaders for vulkan or metal and choose the best
    // driver NULL chooses the best driver
    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV |
                                     SDL_GPU_SHADERFORMAT_MSL |
                                     SDL_GPU_SHADERFORMAT_DXIL,
                                 true, nullptr);

    SDL_Log("Using GPU device driver: %s", SDL_GetGPUDeviceDriver(device));
    SDL_Log("Base path: %s", SDL_GetBasePath());

    SDL_ClaimWindowForGPUDevice(device, window);

    SDL_GPUShader *vertexShader =
        LoadShader(device, "PositionColor.vert", 0, 1, 0, 0);
    if (!vertexShader) {
        SDL_Log("vertex shader failed ;(");
        return SDL_APP_FAILURE;
    }

    SDL_GPUShader *fragmentShader =
        LoadShader(device, "Fractal.frag", 0, 1, 0, 0);
    if (!fragmentShader) {
        SDL_Log("fragment shader failed ;(");
        return SDL_APP_FAILURE;
    }

    SDL_GPUColorTargetBlendState blendState{};
    blendState.enable_color_write_mask = true;
    blendState.color_write_mask = SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A;
    blendState.enable_blend = true;
    blendState.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    blendState.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    blendState.color_blend_op = SDL_GPU_BLENDOP_ADD;
    blendState.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blendState.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    blendState.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

    SDL_GPUColorTargetDescription colorTargetDescriptions[1];
    colorTargetDescriptions[0] = {};
    colorTargetDescriptions[0].blend_state = blendState;
    colorTargetDescriptions[0].format =
        SDL_GetGPUSwapchainTextureFormat(device, window);

    SDL_GPUGraphicsPipelineTargetInfo targetInfo{};
    targetInfo.num_color_targets = 1;
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

    // MAKE PIPELINE
    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.vertex_shader = vertexShader;
    pipelineInfo.fragment_shader = fragmentShader;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    // pipelineInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pipelineInfo.target_info = targetInfo;
    pipelineInfo.vertex_input_state = vertexInputState;

    fillPipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
    if (!fillPipeline) {
        SDL_Log("Failed to create graphics pipeline, error: %s",
                SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_ReleaseGPUShader(device, vertexShader);
    SDL_ReleaseGPUShader(device, fragmentShader);

    SDL_GPUBufferCreateInfo vertexBufferCreateInfo{};
    vertexBufferCreateInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertexBufferCreateInfo.size =
        sizeof(vertices); // size of the whole vertex buffer
    vertexBuffer = SDL_CreateGPUBuffer(device, &vertexBufferCreateInfo);
    SDL_SetGPUBufferName(device, vertexBuffer, "Vertex Buffer");

    SDL_GPUBufferCreateInfo indexBufferCreateInfo{};
    indexBufferCreateInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    indexBufferCreateInfo.size = sizeof(indices);
    indexBuffer = SDL_CreateGPUBuffer(device, &indexBufferCreateInfo);
    SDL_SetGPUBufferName(device, indexBuffer, "Index Buffer");

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

    SDL_memcpy(transferData, vertices, vertexBufferCreateInfo.size);
    // copy index buffer to next section
    // dest, source
    void *transferDataIndicesPtr = static_cast<void *>(
        static_cast<char *>(transferData) + vertexBufferCreateInfo.size);
    SDL_memcpy(transferDataIndicesPtr, indices, indexBufferCreateInfo.size);

    SDL_UnmapGPUTransferBuffer(device, transferBuffer);

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

    SDL_EndGPUCopyPass(copyPass);

    SDL_SubmitGPUCommandBuffer(uploadCommandBuffer);

    SDL_ReleaseGPUTransferBuffer(device, transferBuffer);

    currentTime = SDL_GetTicksNS();
    previousTime = currentTime;

    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS; /* end the program, reporting success to the OS.
                                 */
    }
    return SDL_APP_CONTINUE;
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate) {

    currentTime = SDL_GetTicksNS();
    deltaTime = (currentTime - previousTime) / 1e9f;
    // rotation += deltaTime: 1 radian per second
    // want: pi radians per second
    rotation += deltaTime * (SDL_PI_F) / 2;

    previousTime = currentTime;

    int windowWidth, windowHeight;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);

    // list of commands to send to the gpu for fast execution
    SDL_GPUCommandBuffer *commandBuffer = SDL_AcquireGPUCommandBuffer(device);

    // swapchain: basically loading the next frame as the previous one is being
    // drawn
    SDL_GPUTexture *swapchainTexture;
    // window width and height
    Uint32 width, height;
    // acquire the next swapchain texture
    // be careful because the texture might be NULL (ex. window is minimized)
    // SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window,
    //                                      &swapchainTexture, &width, &height);
    SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window,
                                          &swapchainTexture, &width, &height);
    float widthf = static_cast<float>(width);
    float heightf = static_cast<float>(height);

    // if the texture is NULL (ex. minimized), submit and return
    if (swapchainTexture == nullptr) {
        SDL_SubmitGPUCommandBuffer(commandBuffer);
        return SDL_APP_CONTINUE;
    }

    // CREATE COLOR TARGET

    // color target: where to draw the things
    SDL_GPUColorTargetInfo colorTargetInfo{};

    // replace previous color, clear (screen?) with color
    // colorTargetInfo.clear_color =
    //    SDL_FColor{0xFB / 255.0f, 0xEA / 255.0f, 0xFF / 255.0f, 255 / 255.0f};
    colorTargetInfo.clear_color =
        SDL_FColor{0x71 / 255.0f, 0x79 / 255.0f, 0x7E / 255.0f, 255 / 255.0f};
    // discard previuos content
    colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR; // or SDL_GPU_LOADOP_LOAD to
    // keep the previous content
    // store content to texture
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

    colorTargetInfo.texture = swapchainTexture;

    // BEGIN RENDER PASS

    // color_target_infos: array of render targets, lets you render multiple at
    // the same time num_color_targets: size of array
    SDL_GPURenderPass *renderPass =
        SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, nullptr);

    // bind the graphics pipeline
    SDL_BindGPUGraphicsPipeline(renderPass, fillPipeline);

    // DRAW SOMETHING
    SDL_GPUBufferBinding vertexBufferBinding{};
    vertexBufferBinding.buffer = vertexBuffer;
    vertexBufferBinding.offset = 0;
    SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBufferBinding, 1);

    SDL_GPUBufferBinding indexBufferBinding{};
    indexBufferBinding.buffer = indexBuffer;
    indexBufferBinding.offset = 0;
    SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding,
                           SDL_GPU_INDEXELEMENTSIZE_16BIT);

    float aspectRatio = widthf / heightf;

    float time = SDL_GetTicksNS() / 1e9f;

    SDL_DrawGPUIndexedPrimitives(renderPass, SDL_arraysize(indices), 1, 0, 0,
                                 0);

    // END RENDER PASS

    SDL_EndGPURenderPass(renderPass);

    // do now
    SDL_SubmitGPUCommandBuffer(commandBuffer);

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    SDL_ReleaseGPUBuffer(device, vertexBuffer);
    SDL_ReleaseGPUBuffer(device, indexBuffer);

    SDL_ReleaseGPUGraphicsPipeline(device, fillPipeline);

    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
}
