#pragma once
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include "Game.h"

class Globals {
public:
    SDL_Window* window;
    SDL_GPUDevice* device;
    int windowWidth, windowHeight;
    float aspectRatio;

    // specifies which shaders to use, how many buffers, vertex inputs, color
    // blending
    SDL_GPUGraphicsPipeline* fillPipeline;
    SDL_GPUTexture* depthTexture;

    SDL_GPUTextureFormat depthTextureFormat;
    // SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    // VVV ONLY WORKS FOR 4060
    // SDL_GPU_TEXTUREFORMAT_D24_UNORM;
    //SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

    SDL_GPUTextureFormat swapchainTextureFormat;
    SDL_GPUSampler* sampler;

    glm::vec2 mouseVelocityVector{ 0.0f, 0.0f };

    ImGuiIO* io;

    struct Camera {
        glm::vec3 position;
        glm::vec3 target;
    } camera;

    typedef glm::vec2 Look;
    Look look{ 0.0f, 0.0f };

    SDL_FColor clearColor;
    Game::Model model;

    float rotation;
    bool isRotating;

    bool cameraUpPressed;
    bool cameraDownPressed;
    bool cameraLeftPressed;
    bool cameraRightPressed;
    bool cameraForwardPressed;
    bool cameraBackwardPressed;

    float sliderFloat0 = 0.0f;
    float sliderFloat1 = 0.0f;

    void updateWindowSizeAndAspectRatio() {
        if (!SDL_GetWindowSize(window, &windowWidth, &windowHeight)) {
            SDL_Log("Couldn't get window size :(: %s", SDL_GetError());
            std::exit(1);
        }
        aspectRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
    }

    void tryDepthTextureFormat(SDL_GPUTextureFormat format) {
        if (SDL_GPUTextureSupportsFormat(device, format, SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET)) {
            depthTextureFormat = format;
        }
    }
private:
};

extern Globals globals;
