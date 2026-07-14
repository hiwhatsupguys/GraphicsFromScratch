#pragma once
#include <SDL3/SDL.h>
#include <string>
#include <format>
#include <nlohmann/json.hpp>
#include <fstream>
#include <ios>

using json = nlohmann::json;

const std::string COMPILED_SHADER_PATH = std::string(SDL_GetBasePath()) + "/Content/Shaders/Compiled/";


class Shader {
public:
    // ripped from SDL_gpu examples
    static SDL_GPUShader* loadShader(SDL_GPUDevice* device, const std::string& shaderFilename);
private:
    struct ShaderInfo {
        Uint32 samplers;
        Uint32 storageTextures;
        Uint32 storageBuffers;
        Uint32 uniformBuffers;
    };

    static ShaderInfo loadShaderInfoFromJson(const std::string& shaderFilename);

};

