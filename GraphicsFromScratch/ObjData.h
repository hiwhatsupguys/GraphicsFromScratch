#pragma once
#include <SDL3/SDL.h>
#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#include <tiny_obj_loader.h>
#include "Vertex.h"
#include <glm/glm.hpp>
#include <stdexcept>
#include <unordered_map>
#include <vector>

class ObjData {

  public:
    std::vector<Vertex> vertices;
    std::vector<Uint32> indices;

    // ripped straight from vulkan:
    // https://vulkan-tutorial.com/code/28_model_loading.cpp
    void loadModel(const char *pathname) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                              pathname)) {
            throw std::runtime_error(warn + err);
        }

        std::unordered_map<Vertex, Uint32> uniqueVertices{};

        for (const auto &shape : shapes) {
            for (const auto &index : shape.mesh.indices) {
                Vertex vertex{};

                vertex.position = {attrib.vertices[3 * index.vertex_index + 0],
                                   attrib.vertices[3 * index.vertex_index + 1],
                                   attrib.vertices[3 * index.vertex_index + 2]};

                vertex.uv = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

                vertex.color = {1.0f, 1.0f, 1.0f, 1.0f};
                //vertex.color = {1.0f, 1.0f, 1.0f};

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] =
                        static_cast<Uint32>(vertices.size());
                    vertices.push_back(vertex);
                }

                indices.push_back(uniqueVertices[vertex]);
            }
        }
    }
};
