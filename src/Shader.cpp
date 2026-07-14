#include "Shader.h"

// ripped from SDL_gpu examples
SDL_GPUShader* Shader::loadShader(SDL_GPUDevice* device, const std::string& shaderFilename) {
    // Auto-detect the shader stage from the file name for convenience
    SDL_GPUShaderStage stage;
    if (shaderFilename.contains(".vert"))
        stage = SDL_GPU_SHADERSTAGE_VERTEX;
    else if (shaderFilename.contains(".frag"))
        stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    else {
        SDL_Log("Invalid shader stage!");
        std::exit(1);
        return nullptr;
    }

    std::string fullPath;
    SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(device);
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
    const char* entrypoint;

    if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV) {

        fullPath = std::format("{}/SPIRV/{}.spv", COMPILED_SHADER_PATH, shaderFilename);
        format = SDL_GPU_SHADERFORMAT_SPIRV;
        entrypoint = "main";

    }
    else if (backendFormats & SDL_GPU_SHADERFORMAT_MSL) {

        fullPath = std::format("{}/MSL/{}.msl", COMPILED_SHADER_PATH, shaderFilename);
        format = SDL_GPU_SHADERFORMAT_MSL;
        entrypoint = "main0";

    }
    else if (backendFormats & SDL_GPU_SHADERFORMAT_DXIL) {

        fullPath = std::format("{}/DXIL/{}.dxil", COMPILED_SHADER_PATH, shaderFilename);
        format = SDL_GPU_SHADERFORMAT_DXIL;
        entrypoint = "main";

    }
    else {
        SDL_Log("Unrecognized backend shader format!");
        std::exit(1);
        return nullptr;
    }

    ShaderInfo info = loadShaderInfoFromJson(shaderFilename);

    std::ifstream shaderFile{ fullPath, std::ios::binary };
    if (!shaderFile)
        throw std::runtime_error{ "Failed to load shader from disk! " + fullPath };

    std::vector<Uint8> code{ std::istreambuf_iterator(shaderFile), {} };

    SDL_GPUShaderCreateInfo shaderInfo{
        .code_size = code.size(),
        .code = (Uint8*)code.data(),
        .entrypoint = entrypoint,
        .format = format,
        .stage = stage,
        .num_samplers = info.samplers,
        .num_storage_textures = info.storageTextures,
        .num_storage_buffers = info.storageBuffers,
        .num_uniform_buffers = info.uniformBuffers
    };
    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shaderInfo);

    if (shader == nullptr) {
        SDL_Log("Failed to create shader!: %s", SDL_GetError());
        std::exit(1);
    }

    return shader;
}

Shader::ShaderInfo Shader::loadShaderInfoFromJson(const std::string& shaderFilename) {
    std::string jsonFilePath = COMPILED_SHADER_PATH + "/JSON/" + shaderFilename + ".json";
    std::ifstream jsonFile{ jsonFilePath };
    if (!jsonFile) {
        SDL_Log("json file not found :(");
        std::exit(1);
    }
    json data = json::parse(jsonFile);
    ShaderInfo shaderInfo = {
        .samplers = data["samplers"],
        .storageTextures = data["storage_textures"],
        .storageBuffers = data["storage_buffers"],
        .uniformBuffers = data["uniform_buffers"],
    };

    return shaderInfo;

}
