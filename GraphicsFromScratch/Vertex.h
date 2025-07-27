#pragma once
#include <glm/glm.hpp>
#include <SDL3/SDL.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec4 color;
    glm::vec2 uv;

    bool operator==(const Vertex& other) const {
        return position == other.position
            && color == other.color
            && uv == other.uv;
    }
};

// ripped straight from vulkan
// https://vulkan-tutorial.com/code/28_model_loading.cpp
namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.uv) << 1);
        }
    };
}
