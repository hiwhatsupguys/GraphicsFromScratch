#pragma once
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "Assets.h"
#include "Game.h"

class Globals {
public:
    SDL_Window* window;
    SDL_GPUDevice* device;
    glm::ivec2 windowSize;
    //int windowWidth, windowHeight;
    float aspectRatio;

    SDL_GPUTexture* depthTexture;

    SDL_GPUTextureFormat depthTextureFormat;
    // SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    // VVV ONLY WORKS FOR 4060
    // SDL_GPU_TEXTUREFORMAT_D24_UNORM;
    //SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

    SDL_GPUTextureFormat swapchainTextureFormat;

    SDL_GPUTexture* colormapTexture;

    glm::vec2 mouseVelocityVector{ 0.0f, 0.0f };

    ImGuiIO* io;

    Game::GameState gameState;


    bool cameraUpPressed;
    bool cameraDownPressed;
    bool cameraLeftPressed;
    bool cameraRightPressed;
    bool cameraForwardPressed;
    bool cameraBackwardPressed;

    float sliderFloat0 = 0.0f;
    float sliderFloat1 = 0.0f;

    void updateWindowSizeAndAspectRatio() {
        if (!SDL_GetWindowSize(window, &windowSize.x, &windowSize.y)) {
            SDL_Log("Couldn't get window size :(: %s", SDL_GetError());
            std::exit(1);
        }
        aspectRatio = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);
    }

    void tryDepthTextureFormat(SDL_GPUTextureFormat format) {
        if (SDL_GPUTextureSupportsFormat(device, format, SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET)) {
            depthTextureFormat = format;
        }
    }
private:
};

template <typename Vec4>
Vec4 convertRGBToSRGB(const Vec4 &color) {
    Vec4 newColor;
    for (int i = 0; i < 3; i++) {
        const float &pF = reinterpret_cast<const float*>(&color)[i];
        reinterpret_cast<float*>(&newColor)[i] = SDL_powf(pF, 2.2f);
    }
    return newColor;
}

extern Globals globals;
