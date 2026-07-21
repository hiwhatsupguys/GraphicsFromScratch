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

#include "Game.h"
#include "Common.h"
#include "Assets.h"
#include <glm/glm.hpp>

#include <cmath>
#include <imgui.h>
#include <imgui_impl_sdlgpu3.h>
#include <imgui_impl_sdl3.h>

SDL_GPUTextureCreateInfo depthTextureCreateInfo{}; // for updating width and height later

Uint64 currentTime;
Uint64 previousTime;
// deltaTime in seconds
float deltaTime;

//const std::string BASE_PATH = SDL_GetBasePath();
//const std::string COMPILED_SHADER_PATH = BASE_PATH + "/Content/Shaders/Compiled/";

// https://github.com/ocornut/imgui/blob/master/examples/example_sdl3_sdlgpu3/main.cpp
void initImGui() {
    float mainScale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    globals.io = &ImGui::GetIO();
    globals.io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Scaling
    ImGuiStyle& style = ImGui::GetStyle();
    for (ImVec4 &color : style.Colors) {
        // apply gamma correction
        color.x = glm::pow(color.x, 2.2f);
        color.y = glm::pow(color.y, 2.2f);
        color.z = glm::pow(color.z, 2.2f);
    }
    style.ScaleAllSizes(mainScale);
    style.FontScaleDpi = mainScale;

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLGPU(globals.window);
    ImGui_ImplSDLGPU3_InitInfo init_info = {};
    init_info.Device = globals.device;
    init_info.ColorTargetFormat = globals.swapchainTextureFormat;
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;                      // Only used in multi-viewports mode.
    init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;  // Only used in multi-viewports mode.
    init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
    ImGui_ImplSDLGPU3_Init(&init_info);


}

void initSDL() {

    globals.window = SDL_CreateWindow("Graphics From Scratch", 800, 600,
        SDL_WINDOW_RESIZABLE);

    SDL_SetWindowPosition(globals.window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    SDL_RaiseWindow(globals.window);
    // confines mouse to window
    SDL_SetWindowRelativeMouseMode(globals.window, true);

    globals.updateWindowSizeAndAspectRatio();

    // create gpu device with shaders for vulkan or metal and choose the best
    // driver NULL chooses the best driver
    //globals.device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV |
    //    SDL_GPU_SHADERFORMAT_MSL |
    //    SDL_GPU_SHADERFORMAT_DXIL,
    //    true, nullptr);
     //globals.device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXIL, true, nullptr);
    globals.device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr);

    SDL_Log("Using graphics device: %s", SDL_GetStringProperty(SDL_GetGPUDeviceProperties(globals.device), SDL_PROP_GPU_DEVICE_NAME_STRING, nullptr));

    SDL_Log("Using GPU device driver: %s", SDL_GetGPUDeviceDriver(globals.device));
    SDL_Log("Base path: %s", SDL_GetBasePath());

    SDL_ClaimWindowForGPUDevice(globals.device, globals.window);

    //SDL_SetGPUSwapchainParameters(globals.device, globals.window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR, SDL_GPU_PRESENTMODE_VSYNC);
    SDL_SetGPUSwapchainParameters(globals.device, globals.window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR, SDL_GPU_PRESENTMODE_IMMEDIATE);

    globals.swapchainTextureFormat = SDL_GetGPUSwapchainTextureFormat(globals.device, globals.window);

    globals.tryDepthTextureFormat(SDL_GPU_TEXTUREFORMAT_D32_FLOAT);
    globals.tryDepthTextureFormat(SDL_GPU_TEXTUREFORMAT_D24_UNORM);

    if (globals.depthTextureFormat == SDL_GPU_TEXTUREFORMAT_INVALID) {
        SDL_Log("No supported depth texture format found!");
        std::exit(1);
    }

    // WILL CAUSE PROBLEMS DUE TO RESIZING (not anymore we just update it on
    // window resize)
    depthTextureCreateInfo = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = globals.depthTextureFormat,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,

        //// WILL CAUSE PROBLEMS DUE TO RESIZING (not anymore we just update it on
        //// window resize)
        .width = static_cast<Uint32>(globals.windowSize.x),
        .height = static_cast<Uint32>(globals.windowSize.y),
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };

    globals.depthTexture = SDL_CreateGPUTexture(globals.device, &depthTextureCreateInfo);
    SDL_SetGPUTextureName(globals.device, globals.depthTexture, "Depth Texture");

}

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    /* Create the window */
    // window = SDL_CreateWindow("Graphics From Scratch", 800, 600,
    //                           SDL_WINDOW_FULLSCREEN);
    // SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    // SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);

    initSDL();
    Game::init();
    initImGui();


    currentTime = SDL_GetPerformanceCounter();
    previousTime = currentTime;

    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc)
 * occurs. */
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {

    ImGui_ImplSDL3_ProcessEvent(event);

    switch (event->type) {
    case SDL_EVENT_QUIT:
    return SDL_APP_SUCCESS; /* end the program, reporting success to the
                               OS.*/
    break;

    case SDL_EVENT_WINDOW_RESIZED:
    // recreate the depth texture every time the window is resized
    // scrap old depth texture with old width and height
    SDL_ReleaseGPUTexture(globals.device, globals.depthTexture);
    // get new width and height
    globals.updateWindowSizeAndAspectRatio();

    // update width and height
    depthTextureCreateInfo.width = static_cast<Uint32>(globals.windowSize.x);
    depthTextureCreateInfo.height = static_cast<Uint32>(globals.windowSize.y);

    // recreate depthTexture
    globals.depthTexture = SDL_CreateGPUTexture(globals.device, &depthTextureCreateInfo);
    SDL_SetGPUTextureName(globals.device, globals.depthTexture, "Depth Texture");

    if (!globals.depthTexture) {

        SDL_Log("Couldn't recreate depth texture after window resize ;(: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    if (event->button.button == SDL_BUTTON_LEFT) {
        if (!globals.io->WantCaptureMouse)
            SDL_SetWindowRelativeMouseMode(globals.window, true);
    }
    break;

    case SDL_EVENT_MOUSE_MOTION:
    if (SDL_GetWindowRelativeMouseMode(globals.window)) {
        globals.mouseVelocityVector += glm::vec2{ event->motion.xrel, event->motion.yrel };
    }
    break;

    case SDL_EVENT_KEY_DOWN:
    switch (event->key.key) {
    case SDLK_W:
    globals.cameraForwardPressed = true;
    break;
    case SDLK_S:
    globals.cameraBackwardPressed = true;
    break;
    case SDLK_A:
    globals.cameraLeftPressed = true;
    break;
    case SDLK_D:
    globals.cameraRightPressed = true;
    break;
    case SDLK_SPACE:
    globals.cameraUpPressed = true;
    break;
    case SDLK_LSHIFT:
    globals.cameraDownPressed = true;
    break;
    case SDLK_ESCAPE:
    SDL_SetWindowRelativeMouseMode(globals.window, false);
    break;
    case SDLK_F11:
    // https://discourse.libsdl.org/t/how-to-check-if-a-window-is-fullscreen/35246/2
    bool isFullscreen = SDL_GetWindowFlags(globals.window) & SDL_WINDOW_FULLSCREEN;
    SDL_SetWindowFullscreen(globals.window, !isFullscreen);
    //SDL_Log("Fullscreen mode toggled to %d", SDL_GetWindowFullscreenMode(globals.window));
    globals.updateWindowSizeAndAspectRatio();
    break;
    }
    break;

    case SDL_EVENT_KEY_UP:
    switch (event->key.key) {
    case SDLK_W:
    globals.cameraForwardPressed = false;
    break;
    case SDLK_S:
    globals.cameraBackwardPressed = false;
    break;
    case SDLK_A:
    globals.cameraLeftPressed = false;
    break;
    case SDLK_D:
    globals.cameraRightPressed = false;
    break;
    case SDLK_SPACE:
    globals.cameraUpPressed = false;
    break;
    case SDLK_LSHIFT:
    globals.cameraDownPressed = false;
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

    // Start the Dear ImGui frame
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    Game::update(deltaTime);

    // list of commands to send to the gpu for fast execution
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(globals.device);

    // swapchain: basically loading the next frame as the previous one is
    // being drawn
    SDL_GPUTexture* swapchainTexture;
    // window width and height
    Uint32 width, height;
    // acquire the next swapchain texture
    // be careful because the texture might be NULL (ex. window is minimized)
    SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, globals.window, &swapchainTexture, &width, &height);

    float widthf = static_cast<float>(width);
    float heightf = static_cast<float>(height);

    // Rendering
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

    // if the texture is NULL (ex. minimized), submit and return
    if (swapchainTexture == nullptr) {
        SDL_SubmitGPUCommandBuffer(commandBuffer);
        //SDL_Log("swapchain texture is null");
        return SDL_APP_CONTINUE;
    }
    Game::render(commandBuffer, swapchainTexture);


    // BEGIN IMGUI SETUP
    if (!is_minimized) {
        ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, commandBuffer);

        SDL_GPUColorTargetInfo imColorTargetInfo{
            .texture = swapchainTexture,
            .load_op = SDL_GPU_LOADOP_LOAD, 
            .store_op = SDL_GPU_STOREOP_STORE, 
        };

        // BEGIN IMGUI RENDER PASS
        SDL_GPURenderPass* imRenderPass = SDL_BeginGPURenderPass(commandBuffer, &imColorTargetInfo, 1, nullptr);

        ImGui_ImplSDLGPU3_RenderDrawData(draw_data, commandBuffer, imRenderPass);

        // END IMGUI RENDER PASS
        SDL_EndGPURenderPass(imRenderPass);
    }

    // do now
    SDL_SubmitGPUCommandBuffer(commandBuffer);

    globals.mouseVelocityVector = { 0.0f, 0.0f };

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void* appstate, SDL_AppResult result) {

    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui::DestroyContext();

    SDL_ReleaseGPUGraphicsPipeline(globals.device, globals.gameState.pipeline);

    SDL_ReleaseGPUTexture(globals.device, globals.colormapTexture);

    for (Assets::Model &model : globals.gameState.models) {
        SDL_ReleaseGPUBuffer(globals.device, model.mesh.vertexBuffer);
        SDL_ReleaseGPUBuffer(globals.device, model.mesh.indexBuffer);
    }

    SDL_ReleaseGPUTexture(globals.device, globals.depthTexture);
    SDL_ReleaseGPUSampler(globals.device, globals.gameState.sampler);

    SDL_DestroyGPUDevice(globals.device);
    SDL_DestroyWindow(globals.window);
}

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
