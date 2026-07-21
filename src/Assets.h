#pragma once
#include <SDL3/SDL.h>
#include <string>
#include <glm/glm.hpp>

class Assets {
public:
    struct Mesh {
        SDL_GPUBuffer* vertexBuffer;
        SDL_GPUBuffer* indexBuffer;
        Uint32 numIndices;
    };

    struct Material {
        SDL_GPUTexture* diffuseTexture;
        glm::vec3 specularColor;
        float specularShininess;
    };

    struct Model {
        Mesh mesh;
        Material material;
    };


    static Mesh loadObjFile(SDL_GPUCopyPass* copyPass, const std::string& modelPath);
    static SDL_GPUTexture *loadTextureFile(SDL_GPUCopyPass *copyPass, const std::string &textureFile);
    //static Model loadModel(SDL_GPUCopyPass *copyPass, const std::string &meshPath, const std::string &modelPath);
private:
};