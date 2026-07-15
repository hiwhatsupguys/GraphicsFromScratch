#pragma once
#include <SDL3/SDL.h>
#include <span>
#include "Vertex.h"
#include "Assets.h"

class Buffer {
public:

    static SDL_GPUTexture* uploadTexture(SDL_GPUCopyPass* copyPass, const Uint8* pixels, Uint32 width, Uint32 height);

    static Assets::Mesh uploadMesh(SDL_GPUCopyPass* copyPass, const std::span<const Vertex> vertexArray, const std::span<const Uint32> indexArray);
    static Assets::Mesh uploadMeshBytes(SDL_GPUCopyPass* copyPass, const std::span<const Uint8> vertices, const std::span<const Uint8> indices, Uint32 numIndices);

private:
};