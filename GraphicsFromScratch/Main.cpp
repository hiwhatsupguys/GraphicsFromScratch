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

struct UniformBuffer {
    float time;
    // more properties can be added here
};

static UniformBuffer timeUniform{};

// the vertex input layout
struct Vertex {
    // Vector3D vec3;
    float x, y, z;
    float r, g, b, a;
};

// a list of vertices
static Vertex vertices[]{
    {0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f},   // top vertex
    {-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f}, // bottom left vertex
    {0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f}   // bottom right vertex
};

SDL_Window *window;
SDL_GPUDevice *device;
SDL_GPUBuffer *vertexBuffer;
// buffer to move buffer content from cpu to gpu in a copy pass
SDL_GPUTransferBuffer *transferBuffer;
// specifies which shaders to use, how many buffers, vertex inputs, color
// blending
SDL_GPUGraphicsPipeline *graphicsPipeline;

SDL_GPUShader* LoadShader(
	SDL_GPUDevice* device,
	const char* shaderFilename,
	const Uint32 samplerCount,
	const Uint32 uniformBufferCount,
	const Uint32 storageBufferCount,
	const Uint32 storageTextureCount
) {
	// Auto-detect the shader stage from the file name for convenience
	SDL_GPUShaderStage stage;
	if (SDL_strstr(shaderFilename, ".vert"))
	{
		stage = SDL_GPU_SHADERSTAGE_VERTEX;
	}
	else if (SDL_strstr(shaderFilename, ".frag"))
	{
		stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	}
	else
	{
		SDL_Log("Invalid shader stage!");
		return nullptr;
	}

	char fullPath[256];
	SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(device);
	SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
	const char *entrypoint;

	if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV) {
		SDL_snprintf(fullPath, sizeof(fullPath), "%sContent/Shaders/Compiled/SPIRV/%s.spv", SDL_GetBasePath(), shaderFilename);
		format = SDL_GPU_SHADERFORMAT_SPIRV;
		entrypoint = "main";
	} else if (backendFormats & SDL_GPU_SHADERFORMAT_MSL) {
		SDL_snprintf(fullPath, sizeof(fullPath), "%sContent/Shaders/Compiled/MSL/%s.msl", SDL_GetBasePath(), shaderFilename);
		format = SDL_GPU_SHADERFORMAT_MSL;
		entrypoint = "main0";
	} else if (backendFormats & SDL_GPU_SHADERFORMAT_DXIL) {
		SDL_snprintf(fullPath, sizeof(fullPath), "%sContent/Shaders/Compiled/DXIL/%s.dxil", SDL_GetBasePath(), shaderFilename);
		format = SDL_GPU_SHADERFORMAT_DXIL;
		entrypoint = "main";
	} else {
		SDL_Log("%s", "Unrecognized backend shader format!");
		return nullptr;
	}

	size_t codeSize;
	void* code = SDL_LoadFile(fullPath, &codeSize);
	if (code == nullptr)
	{
		SDL_Log("Failed to load shader from disk! %s", fullPath);
		return nullptr;
	}

    SDL_GPUShaderCreateInfo shaderInfo{};
		shaderInfo.code = (Uint8*)code;
		shaderInfo.code_size = codeSize;
		shaderInfo.entrypoint = entrypoint;
		shaderInfo.format = format;
		shaderInfo.stage = stage;
		shaderInfo.num_samplers = samplerCount;
		shaderInfo.num_uniform_buffers = uniformBufferCount;
		shaderInfo.num_storage_buffers = storageBufferCount;
        shaderInfo.num_storage_textures = storageTextureCount;
	SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shaderInfo);

	if (shader == nullptr)
	{
		SDL_Log("Failed to create shader!");
		SDL_free(code);
		return nullptr;
	}

	SDL_free(code);
	return shader;
}

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
    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL | SDL_GPU_SHADERFORMAT_DXIL, true, nullptr);

    SDL_Log("Using GPU device driver: %s", SDL_GetGPUDeviceDriver(device));

    SDL_ClaimWindowForGPUDevice(device, window);

    // load vertex shader code from file
    size_t vertexCodeSize;
    void *vertexCode =
        SDL_LoadFile("Shaders/Compiled/vertex.spv", &vertexCodeSize);
    if (!vertexCode) {
        SDL_Log("Error: %s", SDL_GetError());
        SDL_Log("base path: %s", SDL_GetBasePath());
        return SDL_APP_FAILURE;
    }

    // create vertex shader
    SDL_GPUShaderCreateInfo vertexInfo{};
    vertexInfo.code = (Uint8 *)vertexCode; // convert to byte array
    vertexInfo.code_size = vertexCodeSize;
    vertexInfo.entrypoint =
        "main"; // name of the function in the shader ("main"
    // may be problematic on .msl shaders)
    vertexInfo.format = SDL_GPU_SHADERFORMAT_SPIRV; // loading spv shaders
    vertexInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;  // vertex shader
    vertexInfo.num_samplers = 0;
    vertexInfo.num_storage_buffers = 0;
    vertexInfo.num_storage_textures = 0;
    vertexInfo.num_uniform_buffers = 0;
    SDL_GPUShader *vertexShader = SDL_CreateGPUShader(device, &vertexInfo);

    // free the file
    SDL_free(vertexCode);

    // load fragment shader code from file
    size_t fragmentCodeSize;
    void *fragmentCode =
        SDL_LoadFile("Shaders/Compiled/fragment.spv", &fragmentCodeSize);
    if (!fragmentCode) {
        SDL_Log("Error: %s", SDL_GetError());
        SDL_Log("base path: %s", SDL_GetBasePath());
        return SDL_APP_FAILURE;
    }

    // create fragment shader
    SDL_GPUShaderCreateInfo fragmentInfo{};
    fragmentInfo.code = (Uint8 *)fragmentCode; // convert to byte array
    fragmentInfo.code_size = fragmentCodeSize;
    fragmentInfo.entrypoint =
        "main"; // name of the function in the shader ("main" may be problematic
    // on .msl shaders)
    fragmentInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;  // loading spv shaders
    fragmentInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT; // fragment shader
    fragmentInfo.num_samplers = 0;
    fragmentInfo.num_storage_buffers = 0;
    fragmentInfo.num_storage_textures = 0;
    fragmentInfo.num_uniform_buffers = 1; // 1 uniform buffer for time

    SDL_GPUShader *fragmentShader = SDL_CreateGPUShader(device, &fragmentInfo);

    // free the file
    SDL_free(fragmentCode);

    // create graphics pipeline
    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.vertex_shader = vertexShader;
    pipelineInfo.fragment_shader = fragmentShader;
    // draw triangles
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    // describe the vertex buffers
    SDL_GPUVertexBufferDescription vertexBufferDescriptions[1];
    vertexBufferDescriptions[0].slot = 0; // vertex buffer set to slot 0
    vertexBufferDescriptions[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBufferDescriptions[0].instance_step_rate = 0;
    vertexBufferDescriptions[0].pitch =
        sizeof(Vertex); // how many bytes to jump after each cycle

    pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
    pipelineInfo.vertex_input_state.vertex_buffer_descriptions =
        vertexBufferDescriptions;

    // describe the vertex attributes

    SDL_GPUVertexAttribute vertexAttributes[2];

    // a_position
    vertexAttributes[0].buffer_slot = 0; // fetch data from the buffer slot at 0
    vertexAttributes[0].location = 0;    // layout (location = 0) in the shader
    vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3; // vec3
    vertexAttributes[0].offset =
        0; // start from the first byte from the current buffer position

    // a_color
    vertexAttributes[1].buffer_slot = 0; // fetch data from the buffer slot at 0
    vertexAttributes[1].location = 1;    // layout (location = 1) in the shader
    vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4; // vec4
    vertexAttributes[1].offset =
        sizeof(float) * 3; // 4th float from current buffer position since we're
    // already doing 3 floats from Vector2D

    pipelineInfo.vertex_input_state.num_vertex_attributes = 2;
    pipelineInfo.vertex_input_state.vertex_attributes = vertexAttributes;

    // describe color target

    SDL_GPUColorTargetDescription colorTargetDescriptions[1];
    colorTargetDescriptions[0] = {};
    colorTargetDescriptions[0].format =
        SDL_GetGPUSwapchainTextureFormat(device, window);

    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.color_target_descriptions =
        colorTargetDescriptions;

    // finally create graphics pipeline
    graphicsPipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);

    // pipeline already created, so we're done with the shaders
    SDL_ReleaseGPUShader(device, vertexShader);
    SDL_ReleaseGPUShader(device, fragmentShader);

    // create vertex buffer

    SDL_GPUBufferCreateInfo bufferInfo{};
    // total bytes of all vertices
    bufferInfo.size = sizeof(vertices);
    bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertexBuffer = SDL_CreateGPUBuffer(device, &bufferInfo);

    // create transfer buffer to upload vertex buffer
    SDL_GPUTransferBufferCreateInfo transferInfo{};
    transferInfo.size = sizeof(vertices);
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);

    // fill the transfer buffer with vertex data

    Vertex *data =
        (Vertex *)SDL_MapGPUTransferBuffer(device, transferBuffer, false);

    SDL_memcpy(data, (void *)vertices, sizeof(vertices));

    // data[0] = vertices[0];
    // data[1] = vertices[1];
    // data[2] = vertices[2];

    // unmap pointer when done updating transfer buffer
    SDL_UnmapGPUTransferBuffer(device, transferBuffer);

    // START A COPY PASS

    // list of commands to send to the gpu for fast execution
    SDL_GPUCommandBuffer *commandBuffer = SDL_AcquireGPUCommandBuffer(device);

    // start a copy pass
    SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(commandBuffer);

    // where is the data
    SDL_GPUTransferBufferLocation location{};
    location.transfer_buffer = transferBuffer;
    location.offset = 0; // start from the beginning

    // where to upload the data
    SDL_GPUBufferRegion region{};
    region.buffer = vertexBuffer;
    region.size = sizeof(vertices); // size of data in bytes
    region.offset = 0;              // begin writing from the first vertex

    // upload the data
    SDL_UploadToGPUBuffer(copyPass, &location, &region, true);

    // end copy pass
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);

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
    //SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window,
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
    colorTargetInfo.clear_color =
        SDL_FColor{0xFB / 255.0f, 0xEA / 255.0f, 0xFF / 255.0f, 255 / 255.0f};
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
    SDL_BindGPUGraphicsPipeline(renderPass, graphicsPipeline);

    // bind the vertex buffer
    SDL_GPUBufferBinding bufferBindings[1];
    bufferBindings[0].buffer = vertexBuffer; // index 0 is slot 0 (?)
    bufferBindings[0].offset = 0;            // start from first byte

    SDL_BindGPUVertexBuffers(renderPass, 0, bufferBindings,
                             1); // bind one buffer starting from slot 0 (?)

    timeUniform.time = (float)SDL_GetTicksNS() / 1e9f; // time in seconds
    SDL_PushGPUFragmentUniformData(commandBuffer, 0, &timeUniform,
                                   sizeof(UniformBuffer));

    // DRAW SOMETHING
    SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);

    // END RENDER PASS

    SDL_EndGPURenderPass(renderPass);

    // do now
    SDL_SubmitGPUCommandBuffer(commandBuffer);

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    SDL_ReleaseGPUBuffer(device, vertexBuffer);
    SDL_ReleaseGPUTransferBuffer(device, transferBuffer);

    SDL_ReleaseGPUGraphicsPipeline(device, graphicsPipeline);

    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
}
