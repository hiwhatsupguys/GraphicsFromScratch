#pragma once
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <string>
#include <vector>
#include "Assets.h"

namespace Game {
//public:
    // cool website for alignment: https://maraneshi.github.io/HLSL-ConstantBufferLayoutVisualizer/
    struct FragGlobalUniformBuffer {
        glm::vec3 lightPosition;
        alignas(16) glm::vec3 lightColor;
        float lightIntensity;
        glm::vec3 viewPosition;
        alignas(16) glm::vec3 ambientLightColor;
    };

    struct FragLocalUniformBuffer {
        glm::vec3 materialSpecularColor;
        float materialShininess;
    };

    struct VertGlobalUniformBuffer {
        glm::mat4 viewProjectionMat;
    };
    struct VertLocalUniformBuffer {
        glm::mat4 modelMat;
        glm::mat4 normalMat;
    };

    typedef Uint32 ModelID;

    struct Entity {
        ModelID modelID;
        glm::vec3 position;
        glm::quat rotation; // rotation quaternion
    };

    struct GameState {
        // specifies which shaders to use, how many buffers, vertex inputs, color
        // blending
        SDL_GPUGraphicsPipeline* pipeline;
        SDL_GPUSampler* sampler;

        struct Camera {
            glm::vec3 position;
            glm::vec3 target;
        } camera;

        typedef glm::vec2 Look;
        Look look{ 0.0f, 0.0f };

        //float colorSlider = 0.0f;
        SDL_FColor clearColor;
        //Assets::Model model;
        std::vector<Assets::Model> models;
        //std::unordered_set<Game::Entity> entities;
        std::vector<Game::Entity> entities;

        FragGlobalUniformBuffer fragGlobalUniform;
        FragLocalUniformBuffer fragLocalUniform;
        VertGlobalUniformBuffer vertGlobalUniform;
        VertLocalUniformBuffer vertLocalUniform;

        glm::vec3 lightPosition;
        glm::vec3 lightColor;
        float lightIntensity;

        glm::vec3 ambientLightColor;

        bool isRotating = true;

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


//private:


    constexpr float EYE_HEIGHT = 1.0f;
    // radians per secoar*d
    constexpr float ROTATION_SPEED = SDL_PI_F / 4.0f;

    constexpr float MOUSE_SENSITIVITY = 0.001f;
    // staticconstexpr float MOUSE_SENSITIVITY = 0.01f;

    // units per second
    constexpr float CAMERA_SPEED = 5.0f;

    //inline const std::string MODEL_PATH = "Content/Meshes/sedan-sports.obj";
    inline const std::string MESH_PATH = "Content/Meshes/colormap.png";

};

