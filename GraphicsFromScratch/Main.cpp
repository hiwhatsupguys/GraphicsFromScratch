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
struct Vector3D {
  float x, y, z;
};

// the vertex input layout
struct Vertex {
  Vector3D position;
  SDL_FColor color;
};

// a list of vertices
static Vertex vertices[]{
    {0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f},    // top vertex
    {-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f},  // bottom left vertex
    {0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f}    // bottom right vertex
};

SDL_Window *window = NULL;
SDL_GPUDevice *device;
SDL_GPUBuffer *vertexBuffer;
// buffer to move buffer content from cpu to gpu in a copy pass
SDL_GPUTransferBuffer *transferBuffer;
SDL_GPUGraphicsPipeline *graphicsPipeline;


/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
  /* Create the window */
  if (!(window = SDL_CreateWindow("Graphics From Scratch", 800, 600,
                                  SDL_WINDOW_FULLSCREEN))) {
    SDL_Log("Couldn't create window and renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  // create gpu device with shaders for vulkan or metal and choose the best
  // driver NULL chooses the best driver
  device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, NULL);
  SDL_ClaimWindowForGPUDevice(device, window);

  // vertex buffer info

  SDL_GPUBufferCreateInfo bufferInfo{};
  // total bytes of all vertices
  bufferInfo.size = sizeof(vertices);
  bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
  vertexBuffer = SDL_CreateGPUBuffer(device, &bufferInfo);

  SDL_GPUTransferBufferCreateInfo transferBufferInfo{};
  transferBufferInfo.size = sizeof(vertices);
  transferBufferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferBufferInfo);

  // fill the transfer buffer with vertex data

  Vertex *data =
      (Vertex *)SDL_MapGPUTransferBuffer(device, transferBuffer, false);

  data[0] = vertices[0];
  data[1] = vertices[1];
  data[2] = vertices[2];
  // or SDL_memcpy(data, vertices, sizeof(vertices));

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
  region.offset = 0;  // begin writing from the first vertex

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
    return SDL_APP_SUCCESS; /* end the program, reporting success to the OS. */
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
  colorTargetInfo.clear_color = {240 / 255.0f, 240 / 255.0f, 240 / 255.0f,
                                 255 / 255.0f};
  // discard previuos content
  colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;  // or SDL_GPU_LOADOP_LOAD to
                                                   // keep the previous content
  // store content to texture
  colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

  colorTargetInfo.texture = swapchainTexture;

  // BEGIN RENDER PASS

  // color_target_infos: array of render targets, lets you render multiple at
  // the same time num_color_targets: size of array
  SDL_GPURenderPass *renderPass =
      SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, NULL);

  // DRAW SOMETHING

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
