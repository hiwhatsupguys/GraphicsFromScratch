#include "Assets.h"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <process.h>
#include <vector>
#include <string>
#include "Vertex.h"
#include "Buffer.h"
#include "ObjData.h"

Assets::Mesh Assets::loadObjFile(SDL_GPUCopyPass *copyPass, const std::string &modelPath) {
    // use tinyobjloader to load .obj as vectors of our custom vertices and
    // indices
    ObjData objData;
    objData.loadModel(modelPath.c_str());
    const std::vector<Vertex>& vertices = objData.vertices;
    const std::vector<Uint32>& indices = objData.indices;

    Mesh mesh = Buffer::uploadMesh(copyPass, vertices, indices);

    return mesh;
}

SDL_GPUTexture *Assets::loadTextureFile(SDL_GPUCopyPass *copyPass, const std::string &textureFile) {
    SDL_Surface* imageDataSurface = IMG_Load(textureFile.c_str());
    //imageDataSurface = SDL_ConvertSurface(imageDataSurface, SDL_PIXELFORMAT_ABGR8888);
    imageDataSurface = SDL_ConvertSurface(imageDataSurface, SDL_PIXELFORMAT_RGBA32);
    if (!imageDataSurface) {
        SDL_Log("image load failed ;(: %s", SDL_GetError());
        std::exit(1);
        // return SDL_APP_FAILURE;
    }

    SDL_GPUTexture* texture = Buffer::uploadTexture(copyPass, static_cast<Uint8*>(imageDataSurface->pixels), imageDataSurface->w, imageDataSurface->h);
    SDL_DestroySurface(imageDataSurface);

    return texture;
}

//Assets::Model Assets::loadModel(SDL_GPUCopyPass *copyPass, const std::string &meshPath, const std::string &modelPath) {
//
//    Mesh mesh = loadObjFile(copyPass, modelPath);
//
//
//    SDL_GPUTexture* colormapTexture = loadTextureFile(copyPass, meshPath);
//
//
//    return Model{ 
//        mesh,
//        colormapTexture
//    };
//}
