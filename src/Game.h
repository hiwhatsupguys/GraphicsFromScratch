#pragma once
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <string>

namespace Game {
//public:
    struct MatrixUniformBuffer {
        glm::mat4 mvp;
    };

    //struct Mesh {
    //    SDL_GPUBuffer* vertexBuffer;
    //    SDL_GPUBuffer* indexBuffer;
    //    Uint32 numIndices;
    //};

    //struct Model : public Mesh {
    //    SDL_GPUTexture* colormapTexture;
    //};

    void init();
    void setupPipeline();
    void updateCamera(float deltaTime);
    void update(float deltaTime);
    void render(SDL_GPUCommandBuffer *commandBuffer, SDL_GPUTexture *swapchainTexture);

    static MatrixUniformBuffer matrixUniform;

//private:

    constexpr float EYE_HEIGHT = 1.0f;
    // radians per secoar*d
    constexpr float ROTATION_SPEED = SDL_PI_F / 4.0f;

    constexpr float MOUSE_SENSITIVITY = 0.001f;
    // staticconstexpr float MOUSE_SENSITIVITY = 0.01f;

    // units per second
    constexpr float CAMERA_SPEED = 5.0f;

    inline const std::string MODEL_PATH = "Content/Meshes/sedan-sports.obj";
    inline const std::string MESH_PATH = "Content/Meshes/colormap.png";

};

