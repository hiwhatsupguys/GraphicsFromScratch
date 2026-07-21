#include <SDL3/SDL.h>
#include <imgui.h>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <array>
#include <string>

#include "Game.h"
#include "Assets.h"
#include "Common.h"
#include "Shader.h"
#include "Vertex.h"

#include "Buffer.h"

// VVV CAUSES MEGA LINKER ERROR
//#include "ObjData.h"

//// Define static member declared in Game.h
//Game::MatrixUniformBuffer Game::matrixUniform{};


void Game::init() {
    setupPipeline();

    //SDL_Log("Size of FragGlobalUniformBuffer: %llu", sizeof(FragGlobalUniformBuffer));
    //SDL_Log("offset of lightPosition: %llu", offsetof(FragGlobalUniformBuffer, lightPosition));
    //SDL_Log("offset of lightColor: %llu", offsetof(FragGlobalUniformBuffer, lightColor));
    //SDL_Log("offset of lightIntensity: %llu", offsetof(FragGlobalUniformBuffer, lightIntensity));
    //SDL_Log("offset of viewPosition: %llu", offsetof(FragGlobalUniformBuffer, viewPosition));
    //SDL_Log("offset of ambientLightColor: %llu", offsetof(FragGlobalUniformBuffer, ambientLightColor));


    SDL_GPUCommandBuffer* uploadCommandBuffer =
        SDL_AcquireGPUCommandBuffer(globals.device);

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCommandBuffer);

    // LOAD MODELS HERE

    globals.colormapTexture = Assets::loadTextureFile(copyPass, MESH_PATH);

    //globals.model = Assets::loadModel(copyPass, MESH_PATH, MODEL_PATH);
    globals.gameState.models.insert(globals.gameState.models.end(), {
        {
            .mesh = Assets::loadObjFile(copyPass, "Content/Meshes/sedan-sports.obj"),
            .material = {
                .diffuseTexture = globals.colormapTexture, 
                .specularColor = {0.0f, 0.0f, 0.0f}, 
                .specularShininess = 80.0f,
            },
        },
        {
            .mesh = Assets::loadObjFile(copyPass, "Content/Meshes/van.obj"),
            .material = {
                .diffuseTexture = globals.colormapTexture, 
                .specularColor = {1.0f, 1.0f, 1.0f}, 
                .specularShininess = 160.0f,
            }
        },
    });

    globals.gameState.entities.insert(globals.gameState.entities.end(), {
        {
            .modelID = 0,
            .position = {},
            .rotation = glm::quat{glm::vec3{}}, // identity quaternion by default (no rotation). uses euler angles of 0,0,0
        },
        {
            .modelID = 1,
            .position = {3.0f, 0.0f, 0.0f},
            .rotation = glm::quat(glm::radians(glm::vec3{90, 45, 0})), // quaternion from euler angles
        },
    });

    globals.gameState.lightPosition = {3.0f, 3.0f, 3.0f};
    globals.gameState.lightColor = { 1.0f, 1.0f, 1.0f };
    globals.gameState.lightIntensity = 1.0f;

    globals.gameState.ambientLightColor = { 0.1f, 0.1f, 0.1f };

    // END LOAD MODELS HERE

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(uploadCommandBuffer);

    globals.gameState.clearColor = convertRGBToSRGB(SDL_FColor(0.45f, 0.55f, 0.60f, 1.00f));
    globals.gameState.clearColor = SDL_FColor(0, 0, 0, 0);
    // globals.clearColor = convertRGBToSRGB(SDL_FColor(0.5f, 0.5f, 0.5f, 1.0f));

    globals.gameState.camera.position = glm::vec3{ 0, EYE_HEIGHT, 3 };
    globals.gameState.camera.target = glm::vec3{ 0, EYE_HEIGHT, 0 };
}

void Game::setupPipeline() {
    SDL_GPUShader* vertexShader = Shader::loadShader(globals.device, "Full.vert");
    if (!vertexShader) {
        SDL_Log("vertex shader failed ;(: %s", SDL_GetError());
        std::exit(1);
    }

    SDL_GPUShader* fragmentShader = Shader::loadShader(globals.device, "Lighting.frag");
    if (!fragmentShader) {
        SDL_Log("fragment shader failed ;(: %s", SDL_GetError());
        std::exit(1);
    }

    std::array<SDL_GPUColorTargetDescription, 1> colorTargetDescriptions{ {
        {
        .format = globals.swapchainTextureFormat,
        .blend_state = {},
        }
    } };

    // describe the vertex buffers
    std::array<SDL_GPUVertexBufferDescription, 1> vertexBufferDescriptions{ {
        {
            // slot, pitch, input_rate, instance_step_rate
            .slot = 0, // vertex buffer set to slot 0
            .pitch =
                sizeof(Vertex), // how many bytes to jump after each cycle
            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            .instance_step_rate = 0,
        }
    } };


    // location, buffer_slot, format, offset
    // location: float3 Position in Input in PositionColor.vert
    // buffer_slot: 0 for Input
    // format: float3 Position
    // offset: how many bytes over from the start of Input
    std::array<SDL_GPUVertexAttribute, 4> vertexAttributes{{
        {
            .location = 0,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = offsetof(Vertex, position),
        },
        {
            .location = 1,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
            .offset = offsetof(Vertex, color),
        },
        {
            .location = 2,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
            .offset = offsetof(Vertex, uv),
        },
        {
            .location = 3,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = offsetof(Vertex, normal),
        },
    }};

    // MAKE PIPELINE
    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{
        .vertex_shader = vertexShader,
        .fragment_shader = fragmentShader,
        .vertex_input_state = {
            .vertex_buffer_descriptions = vertexBufferDescriptions.data(),
            .num_vertex_buffers = vertexBufferDescriptions.size(),
            .vertex_attributes = vertexAttributes.data(),
            .num_vertex_attributes = vertexAttributes.size(),
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state = {
            .fill_mode = SDL_GPU_FILLMODE_FILL,
            //.fill_mode = SDL_GPU_FILLMODE_LINE,
            .cull_mode = SDL_GPU_CULLMODE_BACK, // backface culling
        },
        .multisample_state = {},
        .depth_stencil_state = {
            .compare_op = SDL_GPU_COMPAREOP_LESS,
            .enable_depth_test = true,
            .enable_depth_write = true,
        },
        .target_info = {
            .color_target_descriptions = colorTargetDescriptions.data(),
            .num_color_targets = colorTargetDescriptions.size(),
            .depth_stencil_format = globals.depthTextureFormat,
            .has_depth_stencil_target = true,
        }
,
    };

    globals.gameState.pipeline = SDL_CreateGPUGraphicsPipeline(globals.device, &pipelineInfo);
    if (!globals.gameState.pipeline) {
        SDL_Log("Failed to create graphics pipeline, error: %s", SDL_GetError());
        std::exit(1);
    }

    SDL_ReleaseGPUShader(globals.device, vertexShader);
    SDL_ReleaseGPUShader(globals.device, fragmentShader);

    // LinearClamp
    SDL_GPUSamplerCreateInfo samplerCreateInfo{
        .min_filter = SDL_GPU_FILTER_LINEAR,
        .mag_filter = SDL_GPU_FILTER_LINEAR,
        .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    };
    globals.gameState.sampler = SDL_CreateGPUSampler(globals.device, &samplerCreateInfo);
}

void Game::updateCamera(float deltaTime) {
    glm::vec2 movementInput{
        // x
        globals.cameraRightPressed - globals.cameraLeftPressed,
        // z
        globals.cameraForwardPressed - globals.cameraBackwardPressed 
    };

    // y
    float yInput = globals.cameraUpPressed - globals.cameraDownPressed;

    // need to convert two angles (from mouse cumulative movement) to x, y,
    // and z on screen relative to camera position x: yaw, y: pitch


    // only changes look's start and end values if the mouse moved
    if (globals.mouseVelocityVector.x != 0.0f || globals.mouseVelocityVector.y != 0.0f) {

        // wrap angle (yaw)
        globals.gameState.look.x = SDL_fmodf(globals.gameState.look.x - globals.mouseVelocityVector.x * MOUSE_SENSITIVITY,
            2.0f * SDL_PI_F);
        // clamp to -90, 90 (pitch)
        globals.gameState.look.y =
            SDL_clamp(globals.gameState.look.y - globals.mouseVelocityVector.y * MOUSE_SENSITIVITY,
                glm::radians(-89.0f), glm::radians(89.0f));

        //SDL_Log("%f, %f", look.yaw, look.pitch);
    }

    //glm::mat3 yawPitchRollMatrix =
    //    glm::yawPitchRoll(look.getValue().x, look.getValue().y, 0.0f);

    glm::mat3 yawPitchRollMatrix = glm::yawPitchRoll(globals.gameState.look.x, globals.gameState.look.y, 0.0f);

    // glm::vec3 lookDirection{SDL_sinf(look.yaw), 0.0f,
    // SDL_cosf(look.yaw)};
    glm::vec3 forward = yawPitchRollMatrix * glm::vec3{ 0.0f, 0.0f, -1.0f };
    glm::vec3 right = yawPitchRollMatrix * glm::vec3{ 1.0f, 0.0f, 0.0f };

    glm::vec3 movementDirection =
        forward * movementInput.y + right * movementInput.x;
    movementDirection.y = yInput;

    // normalize vector
    if (movementDirection != glm::zero<glm::vec3>())
        movementDirection = glm::normalize(movementDirection);

    // deltatimeify
    movementDirection = movementDirection * CAMERA_SPEED * deltaTime;

    globals.gameState.camera.position += movementDirection;
    globals.gameState.camera.target = globals.gameState.camera.position + forward;
}

void Game::update(float deltaTime) {
    {
        //static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        //ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        //ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Rotate Model", &globals.gameState.isRotating);

        ImGui::SliderFloat("float0", &globals.sliderFloat0, 0.0f, 0.01f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::SliderFloat("float1", &globals.sliderFloat1, 1.0f, 1000.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        //ImGui::SliderFloat("color slider", &globals.colorSlider, 0.0f, 1.0f); // Edit 3 floats representing a color
        ImGui::ColorEdit3("clear color", (float*)&globals.gameState.clearColor); // Edit 3 floats representing a color

        ImGui::SeparatorText("Light");
        ImGui::DragFloat3("Position", (float*)&globals.gameState.lightPosition);
        ImGui::ColorEdit3("Color", (float*)&globals.gameState.lightColor); 
        ImGui::SliderFloat("Intensity", &globals.gameState.lightIntensity, 0.0f, 50.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("Ambient Light Color", (float*)&globals.gameState.ambientLightColor);            // Edit 1 float using a slider from 0.0f to 1.0f



        for (int i = 0; i < globals.gameState.entities.size(); i++) {
            Entity& entity = globals.gameState.entities[i];
            Assets::Model& model = globals.gameState.models[entity.modelID];
            Assets::Mesh& mesh = model.mesh;
            Assets::Material& material = model.material;

            ImGui::PushID(i);
            ImGui::SeparatorText(("Object " + std::to_string(i)).c_str());
            ImGui::ColorEdit3("Specular Color", (float*)&material.specularColor);
            ImGui::DragFloat("Shininess", &material.specularShininess, 1.0f, 1.0f, 500.0f);
            ImGui::PopID();
        }

        //if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
        //    counter++;
        //ImGui::SameLine();
        //ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / globals.io->Framerate, globals.io->Framerate);
        ImGui::End();
    }

    //globals.clearColor = convertRGBToSRGB(SDL_FColor{ globals.colorSlider, globals.colorSlider, globals.colorSlider, 1.0f });

    // rotation += deltaTime: 1 radian per second
    // want: pi radians per second
    if (globals.gameState.isRotating) {
        globals.gameState.entities[0].rotation *= glm::quat{ glm::vec3{0.0f, ROTATION_SPEED * deltaTime, 0.0f} };
    }
    //globals.rotation += globals.isRotating ? ROTATION_SPEED * deltaTime : 0.0f;

    Game::updateCamera(deltaTime);
}

void Game::render(SDL_GPUCommandBuffer *commandBuffer, SDL_GPUTexture *swapchainTexture) {
    // create matrices so the gpu can use them to transform the vertices to
    // go on the screen where they belong


    glm::mat4 projectionMatrix = glm::perspective(glm::radians(70.0f), globals.aspectRatio, 0.01f, 1000.0f);

     //glm::mat4 viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, -3.0f));
    glm::mat4 viewMatrix = glm::lookAt(globals.gameState.camera.position, globals.gameState.camera.target, glm::vec3{ 0.0f, 1.0f, 0.0f });


    globals.gameState.vertGlobalUniform = {
        .viewProjectionMat = projectionMatrix * viewMatrix // calculate vp matrix to send to the vertex shader
    }; 
    SDL_PushGPUVertexUniformData(commandBuffer, 0, &globals.gameState.vertGlobalUniform, sizeof(globals.gameState.vertGlobalUniform)); // push vertex uniform data so the gpu can perform the matrix multiplication

    globals.gameState.fragGlobalUniform = {
        .lightPosition = globals.gameState.lightPosition,
        .lightColor = globals.gameState.lightColor,
        .lightIntensity = globals.gameState.lightIntensity,
        .viewPosition = globals.gameState.camera.position,
        .ambientLightColor = globals.gameState.ambientLightColor,
    };

    SDL_PushGPUFragmentUniformData(commandBuffer, 0, &globals.gameState.fragGlobalUniform, sizeof(globals.gameState.fragGlobalUniform));

    // CREATE COLOR TARGET

    // color target: where to draw the things
    SDL_GPUColorTargetInfo colorTargetInfo{
        .texture = swapchainTexture,
        // replace previous color, clear (screen?) with color
        .clear_color = globals.gameState.clearColor,
        // .clearColor = SDL_FColor{0x71 / 255.0f, 0x79 / 255.0f, 0x7E / 255.0f, 255 / 255.0f},
        // discard previous content
        .load_op = SDL_GPU_LOADOP_CLEAR, // or SDL_GPU_LOADOP_LOAD to
        // keep the previous content
        .store_op = SDL_GPU_STOREOP_STORE, // store content to texture
    };


    SDL_GPUDepthStencilTargetInfo depthStencilTargetInfo{
        .texture = globals.depthTexture,
        .clear_depth = 1,
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_DONT_CARE,
    };


    // BEGIN RENDER PASS

    // color_target_infos: array of render targets, lets you render multiple
    // at the same time num_color_targets: size of array
    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, &depthStencilTargetInfo);

    // bind the graphics pipeline (since we're using this pipeline for all our models, we can just bind it once)
    SDL_BindGPUGraphicsPipeline(renderPass, globals.gameState.pipeline);


    for (const Entity &entity : globals.gameState.entities) {
        // 3d (4d) rotation matrix
        //glm::mat4 modelMatrix = glm::rotate(glm::mat4(1.0f), globals.rotation, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), entity.position) * glm::toMat4(entity.rotation);


        globals.gameState.vertLocalUniform = {
            .modelMat = modelMatrix,
            .normalMat = glm::inverse(glm::transpose(modelMatrix)),
        };
        SDL_PushGPUVertexUniformData(commandBuffer, 1, &globals.gameState.vertLocalUniform, sizeof(globals.gameState.vertLocalUniform));


        Assets::Model& model = globals.gameState.models[entity.modelID];

        globals.gameState.fragLocalUniform = {
            .materialSpecularColor = model.material.specularColor,
            .materialShininess = model.material.specularShininess,
        };
        SDL_PushGPUFragmentUniformData(commandBuffer, 1, &globals.gameState.fragLocalUniform, sizeof(globals.gameState.fragLocalUniform));
        

        // DRAW SOMETHING
        SDL_GPUBufferBinding vertexBufferBinding{
            .buffer = model.mesh.vertexBuffer,
            .offset = 0,
        };
        SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBufferBinding, 1);

        SDL_GPUBufferBinding indexBufferBinding{
            .buffer = model.mesh.indexBuffer,
            .offset = 0,
        };
        SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

        SDL_GPUTextureSamplerBinding textureSamplerBinding{
            .texture = model.material.diffuseTexture,
            .sampler = globals.gameState.sampler,
        };
        SDL_BindGPUFragmentSamplers(renderPass, 0, &textureSamplerBinding, 1);

        SDL_DrawGPUIndexedPrimitives(renderPass, model.mesh.numIndices, 1, 0, 0, 0);
    }





    // END RENDER PASS

    SDL_EndGPURenderPass(renderPass);
}
