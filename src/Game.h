#pragma once
#include <SDL3/SDL.h>
#include <glm/glm.hpp>

namespace Game {
//public:
    struct MatrixUniformBuffer {
        glm::mat4 mvp;
    };

    struct Model {
        SDL_GPUBuffer* vertexBuffer;
        SDL_GPUBuffer* indexBuffer;
        Uint32 numIndices;
        SDL_GPUTexture* colormapTexture;
    };

    void init();
    void updateCamera(float deltaTime);
    void update(float deltaTime);
    void render(SDL_GPUCommandBuffer *commandBuffer, SDL_GPUTexture *swapchainTexture);

    static MatrixUniformBuffer matrixUniform;
    

//private:
    Model loadModel(const char* meshPath, const char* modelPath);
    void setupPipeline();

    constexpr float EYE_HEIGHT = 1.0f;
    // radians per second
    constexpr float ROTATION_SPEED = SDL_PI_F / 4.0f;

    constexpr float MOUSE_SENSITIVITY = 0.001f;
    // staticconstexpr float MOUSE_SENSITIVITY = 0.01f;

    // units per second
    constexpr float CAMERA_SPEED = 5.0f;

    constexpr const char* MODEL_PATH = "Content/Meshes/sedan-sports.obj";
    constexpr const char* MESH_PATH = "Content/Meshes/colormap.png";

};

