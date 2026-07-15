#include "Buffer.h"
#include <SDL3/SDL.h>
#include <span>
#include <cstddef>

#include "Assets.h"
#include "Common.h"
#include "Vertex.h"

SDL_GPUTexture* Buffer::uploadTexture(SDL_GPUCopyPass* copyPass, const Uint8* pixels, Uint32 width, Uint32 height) {
    SDL_GPUTextureCreateInfo textureCreateInfo{
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        //.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = static_cast<Uint32>(width),
        .height = static_cast<Uint32>(height),
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };
    SDL_GPUTexture* colormapTexture = SDL_CreateGPUTexture(globals.device, &textureCreateInfo);
    SDL_SetGPUTextureName(globals.device, colormapTexture, "Colormap Texture");

    // transfer texture
    SDL_GPUTransferBufferCreateInfo textureTransferBufferCreateInfo{
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = static_cast<Uint32>(width * height * 4u), // total number of bytes (area), 4 bytes per pixel
    };
    SDL_GPUTransferBuffer* textureTransferBuffer = SDL_CreateGPUTransferBuffer(globals.device, &textureTransferBufferCreateInfo);
    Uint8* textureTransferData = static_cast<Uint8*>(SDL_MapGPUTransferBuffer(globals.device, textureTransferBuffer, false));
    SDL_memcpy(textureTransferData, pixels, textureTransferBufferCreateInfo.size);
    SDL_UnmapGPUTransferBuffer(globals.device, textureTransferBuffer);

    SDL_GPUTextureTransferInfo textureTransferInfo{
        .transfer_buffer = textureTransferBuffer,
        .offset = 0,
    };

    SDL_GPUTextureRegion textureRegion{
        .texture = colormapTexture,
        .w = static_cast<Uint32>(width),
        .h = static_cast<Uint32>(height),
        .d = 1,
    };
    SDL_UploadToGPUTexture(copyPass, &textureTransferInfo, &textureRegion,
        false);

    SDL_ReleaseGPUTransferBuffer(globals.device, textureTransferBuffer);

    return colormapTexture;
}

// converts span of any type to a span of Uint8
template<typename ByteT, typename From>
std::span<ByteT> byteSpanCast(const std::span<From> &spanToConvert) {

    const std::span<const std::byte> byteSpan = std::as_bytes(spanToConvert);
    return std::span<ByteT>{
        reinterpret_cast<ByteT*>(byteSpan.data()),
        byteSpan.size()
    };
}

Assets::Mesh Buffer::uploadMesh(SDL_GPUCopyPass *copyPass, const std::span<const Vertex> vertexArray, const std::span<const Uint32> indexArray) {

    return uploadMeshBytes(copyPass, byteSpanCast<const Uint8>(vertexArray), byteSpanCast<const Uint8>(indexArray), static_cast<Uint32>(indexArray.size()));
}

Assets::Mesh Buffer::uploadMeshBytes(SDL_GPUCopyPass *copyPass, const std::span<const Uint8> vertices, const std::span<const Uint8> indices, Uint32 numIndices) {
    SDL_GPUBufferCreateInfo vertexBufferCreateInfo{
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = static_cast<Uint32>(vertices.size()), // size of the whole vertex buffer
    };
    SDL_GPUBuffer *vertexBuffer = SDL_CreateGPUBuffer(globals.device, &vertexBufferCreateInfo);
    SDL_SetGPUBufferName(globals.device, vertexBuffer, "Vertex Buffer");

    SDL_GPUBufferCreateInfo indexBufferCreateInfo{
        .usage = SDL_GPU_BUFFERUSAGE_INDEX,
        .size = static_cast<Uint32>(indices.size()), // size of the whole index buffer
    };
    SDL_GPUBuffer *indexBuffer = SDL_CreateGPUBuffer(globals.device, &indexBufferCreateInfo);
    SDL_SetGPUBufferName(globals.device, indexBuffer, "Index Buffer");

    // we have to use a transfer buffer to upload triangle data into the vertex
    // buffer
    // transfer buffer: special GPU memory that is used to transfer data from
    // the CPU to the GPU
    SDL_GPUTransferBufferCreateInfo transferBufferCreateInfo{
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = vertexBufferCreateInfo.size +
            indexBufferCreateInfo.size, // also the size of the vertex buffer
    };
    SDL_GPUTransferBuffer* transferBuffer =
        SDL_CreateGPUTransferBuffer(globals.device, &transferBufferCreateInfo);

    void* transferData =
        (void*)SDL_MapGPUTransferBuffer(globals.device, transferBuffer, false);

    SDL_memcpy(transferData, vertices.data(), vertexBufferCreateInfo.size);
    // copy index buffer to next section
    // dest, source
    void* transferDataIndicesPtr = static_cast<void*>(
        static_cast<char*>(transferData) + vertexBufferCreateInfo.size);
    SDL_memcpy(transferDataIndicesPtr, indices.data(),
        indexBufferCreateInfo.size);

    SDL_UnmapGPUTransferBuffer(globals.device, transferBuffer);

    SDL_GPUTransferBufferLocation transferBufferLocation{
        .transfer_buffer = transferBuffer,
        .offset = 0,
    };

    SDL_GPUBufferRegion bufferRegion{
        .buffer = vertexBuffer,
        .offset = 0,
        .size = vertexBufferCreateInfo.size,
    };
    SDL_UploadToGPUBuffer(copyPass, &transferBufferLocation, &bufferRegion,
        false);

    transferBufferLocation.offset = vertexBufferCreateInfo.size; // move over to the index buffer section
    bufferRegion.buffer = indexBuffer;
    bufferRegion.size = indexBufferCreateInfo.size;
    bufferRegion.offset = 0;

    SDL_UploadToGPUBuffer(copyPass, &transferBufferLocation, &bufferRegion,
        false);

    SDL_ReleaseGPUTransferBuffer(globals.device, transferBuffer);

    return Assets::Mesh{
         vertexBuffer,
         indexBuffer,
         numIndices
    };
}