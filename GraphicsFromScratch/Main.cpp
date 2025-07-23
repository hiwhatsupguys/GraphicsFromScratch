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

#include <glm/glm.hpp>

SDL_Window *window;
SDL_GPUDevice *device;
SDL_GPUBuffer *vertexBuffer;
SDL_GPUBuffer *indexBuffer;
// buffer to move buffer content from cpu to gpu in a copy pass
SDL_GPUTransferBuffer *transferBuffer;
// specifies which shaders to use, how many buffers, vertex inputs, color
// blending
SDL_GPUGraphicsPipeline *fillPipeline;
//SDL_GPUTexture *texture;

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
        SDL_asprintf(&fullPath, "%sContent/Shaders/Compiled/SPIRV/%s.spv",
                     SDL_GetBasePath(), shaderFilename);
        format = SDL_GPU_SHADERFORMAT_SPIRV;
        entrypoint = "main";
    } else if (backendFormats & SDL_GPU_SHADERFORMAT_MSL) {
        SDL_asprintf(&fullPath, "%sContent/Shaders/Compiled/MSL/%s.msl",
                     SDL_GetBasePath(), shaderFilename);
        format = SDL_GPU_SHADERFORMAT_MSL;
        entrypoint = "main0";
    } else if (backendFormats & SDL_GPU_SHADERFORMAT_DXIL) {
        SDL_asprintf(&fullPath, "%sContent/Shaders/Compiled/DXIL/%s.dxil",
                     SDL_GetBasePath(), shaderFilename);
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

struct Vertex {
    glm::vec3 position;
    glm::u8vec4 color;
};

// rect
Vertex vertices[4] = {
    Vertex{{-0.5f, 0.5f, 0.0f}, {255, 0, 0, 255}},
    Vertex{{0.5f, 0.5f, 0.0f}, {0, 255, 0, 255}},
    Vertex{{0.5f, -0.5f, 0.0f}, {0, 0, 255, 255}},
    Vertex{{-0.5f, -0.5f, 0.0f}, {255, 255, 0, 255}},

};

Uint16 indices[6] = {0, 1, 2, 0, 2, 3};

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    /* Create the window */
    // window = SDL_CreateWindow("Graphics From Scratch", 800, 600,
    //                           SDL_WINDOW_FULLSCREEN);
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

    SDL_ClaimWindowForGPUDevice(device, window);

    SDL_GPUShader *vertexShader =
        LoadShader(device, "PositionColor.vert", 0, 0, 0, 0);
    if (!vertexShader) {
        SDL_Log("vertex shader failed ;(");
        return SDL_APP_FAILURE;
    }

    SDL_GPUShader *fragmentShader =
        LoadShader(device, "SolidColor.frag", 0, 0, 0, 0);
    if (!fragmentShader) {
        SDL_Log("fragment shader failed ;(");
        return SDL_APP_FAILURE;
    }

    SDL_GPUColorTargetDescription colorTargetDescriptions[1];
    colorTargetDescriptions[0] = {};
    colorTargetDescriptions[0].format =
        SDL_GetGPUSwapchainTextureFormat(device, window);

    SDL_GPUGraphicsPipelineTargetInfo targetInfo{};
    targetInfo.num_color_targets = 1;
    targetInfo.color_target_descriptions = colorTargetDescriptions;

    SDL_GPUVertexAttribute vertexAttributes[2];
    // location, buffer_slot, format, offset
    // location: float3 Position in Input in PositionColor.vert
    // buffer_slot: 0 for Input
    // format: float3 Position
    // offset: how many bytes over from the start of Input
    vertexAttributes[0] =
        SDL_GPUVertexAttribute{0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0};
    vertexAttributes[1] =
        SDL_GPUVertexAttribute{1, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
                               sizeof(Vertex::position)};

    // describe the vertex buffers
    SDL_GPUVertexBufferDescription vertexBufferDescriptions[1];
    // slot, pitch, input_rate, instance_step_rate
    // vertexBufferDescriptions[0] = SDL_GPUVertexBufferDescription{0,
    // sizeof(glm::vec3), SDL_GPU_VERTEXINPUTRATE_VERTEX, 0};
    vertexBufferDescriptions[0].slot = 0; // vertex buffer set to slot 0
    vertexBufferDescriptions[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBufferDescriptions[0].instance_step_rate = 0;
    vertexBufferDescriptions[0].pitch =
        sizeof(Vertex); // how many bytes to jump after each cycle

    SDL_GPUVertexInputState vertexInputState{};
    vertexInputState.num_vertex_buffers = 1;
    vertexInputState.vertex_buffer_descriptions = vertexBufferDescriptions;
    vertexInputState.num_vertex_attributes = 2;
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

    SDL_GPUBufferCreateInfo indexBufferCreateInfo{};
    indexBufferCreateInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    indexBufferCreateInfo.size = sizeof(indices);
    indexBuffer = SDL_CreateGPUBuffer(device, &indexBufferCreateInfo);

    // we have to use a transfer buffer to upload triangle data into the vertex
    // buffer
    SDL_GPUTransferBufferCreateInfo transferBufferCreateInfo{};
    transferBufferCreateInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferBufferCreateInfo.size =
        vertexBufferCreateInfo.size +
        indexBufferCreateInfo.size; // also the size of the vertex buffer
    transferBuffer =
        SDL_CreateGPUTransferBuffer(device, &transferBufferCreateInfo);

    void *transferData =
        (void *)SDL_MapGPUTransferBuffer(device, transferBuffer, false);

    SDL_memcpy(transferData, vertices, vertexBufferCreateInfo.size);
    // copy index buffer to next section
    // dest, source
    // Uint16* ptr = static_cast<void*>(static_cast<Uint16*>(transferData) +
    // vertexBufferCreateInfo.size);
    void *ptr = static_cast<void *>(static_cast<char *>(transferData) +
                                    vertexBufferCreateInfo.size);
    SDL_memcpy(ptr, indices, indexBufferCreateInfo.size);

    // transferData[0] = vertices[0];
    // transferData[1] = vertices[1];
    // transferData[2] = vertices[2];

    SDL_UnmapGPUTransferBuffer(device, transferBuffer);

    SDL_GPUCommandBuffer *uploadCommandBuffer =
        SDL_AcquireGPUCommandBuffer(device);

    // upload transfer data to vertex buffer
    SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(uploadCommandBuffer);

    SDL_GPUTransferBufferLocation transferLocation{};
    transferLocation.transfer_buffer = transferBuffer;
    transferLocation.offset = 0;

    SDL_GPUBufferRegion bufferRegion{};
    bufferRegion.buffer = vertexBuffer;
    bufferRegion.size = vertexBufferCreateInfo.size;
    bufferRegion.offset = 0; // maybe change
    SDL_UploadToGPUBuffer(copyPass, &transferLocation, &bufferRegion, false);

    transferLocation.offset =
        vertexBufferCreateInfo.size; // move over to the index buffer section
    bufferRegion.buffer = indexBuffer;
    bufferRegion.size = indexBufferCreateInfo.size;
    bufferRegion.offset = 0;
    SDL_UploadToGPUBuffer(copyPass, &transferLocation, &bufferRegion, false);

    SDL_EndGPUCopyPass(copyPass);

    SDL_SubmitGPUCommandBuffer(uploadCommandBuffer);

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
    SDL_ReleaseGPUTransferBuffer(device, transferBuffer);

    SDL_ReleaseGPUGraphicsPipeline(device, fillPipeline);

    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
}
