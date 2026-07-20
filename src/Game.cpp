#include <SDL3/SDL.h>
#include <imgui.h>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <array>

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

    SDL_Log("Size of FragUniformBuffer: %llu", sizeof(FragUniformBuffer));
    SDL_Log("offset of lightPosition: %llu", offsetof(FragUniformBuffer, lightPosition));
    SDL_Log("offset of lightColor: %llu", offsetof(FragUniformBuffer, lightColor));
    SDL_Log("offset of lightIntensity: %llu", offsetof(FragUniformBuffer, lightIntensity));


    SDL_GPUCommandBuffer* uploadCommandBuffer =
        SDL_AcquireGPUCommandBuffer(globals.device);

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCommandBuffer);

    // LOAD MODELS HERE

    globals.colormapTexture = Assets::loadTextureFile(copyPass, MESH_PATH);

    //globals.model = Assets::loadModel(copyPass, MESH_PATH, MODEL_PATH);
    globals.models.insert(globals.models.end(), {
        {Assets::loadObjFile(copyPass, "Content/Meshes/sedan-sports.obj"), globals.colormapTexture},
        {Assets::loadObjFile(copyPass, "Content/Meshes/van.obj"), globals.colormapTexture},
    });

    globals.entities.insert(globals.entities.end(), {
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

    globals.lightPosition = {3.0f, 3.0f, 3.0f};
    globals.lightColor = { 1.0f, 0.0f, 0.0f };
    globals.lightIntensity = 1.0f;

    // END LOAD MODELS HERE

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(uploadCommandBuffer);

    globals.clearColor = convertRGBToSRGB(SDL_FColor(0.45f, 0.55f, 0.60f, 1.00f));
    globals.clearColor = SDL_FColor(0, 0, 0, 0);
    // globals.clearColor = convertRGBToSRGB(SDL_FColor(0.5f, 0.5f, 0.5f, 1.0f));

    globals.camera.position = glm::vec3{ 0, EYE_HEIGHT, 3 };
    globals.camera.target = glm::vec3{ 0, EYE_HEIGHT, 0 };
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

    globals.fillPipeline = SDL_CreateGPUGraphicsPipeline(globals.device, &pipelineInfo);
    if (!globals.fillPipeline) {
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
    globals.sampler = SDL_CreateGPUSampler(globals.device, &samplerCreateInfo);
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
        globals.look.x = SDL_fmodf(globals.look.x - globals.mouseVelocityVector.x * MOUSE_SENSITIVITY,
            2.0f * SDL_PI_F);
        // clamp to -90, 90 (pitch)
        globals.look.y =
            SDL_clamp(globals.look.y - globals.mouseVelocityVector.y * MOUSE_SENSITIVITY,
                glm::radians(-89.0f), glm::radians(89.0f));

        //SDL_Log("%f, %f", look.yaw, look.pitch);
    }

    //glm::mat3 yawPitchRollMatrix =
    //    glm::yawPitchRoll(look.getValue().x, look.getValue().y, 0.0f);

    glm::mat3 yawPitchRollMatrix = glm::yawPitchRoll(globals.look.x, globals.look.y, 0.0f);

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

    globals.camera.position += movementDirection;
    globals.camera.target = globals.camera.position + forward;
}

void Game::update(float deltaTime) {
    {
        //static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        //ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        //ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Rotate Model", &globals.isRotating);

        ImGui::SliderFloat("float0", &globals.sliderFloat0, 0.0f, 0.01f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::SliderFloat("float1", &globals.sliderFloat1, 1.0f, 1000.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        //ImGui::SliderFloat("color slider", &globals.colorSlider, 0.0f, 1.0f); // Edit 3 floats representing a color
        ImGui::ColorEdit3("clear color", (float*)&globals.clearColor); // Edit 3 floats representing a color

        ImGui::SeparatorText("Light");
        ImGui::DragFloat3("Position", (float*)&globals.lightPosition);
        ImGui::ColorEdit3("Color", (float*)&globals.lightColor); 
        ImGui::SliderFloat("Intensity", &globals.lightIntensity, 0.0f, 5.0f);            // Edit 1 float using a slider from 0.0f to 1.0f

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
    if (globals.isRotating) {
        globals.entities[0].rotation *= glm::quat{ glm::vec3{0.0f, ROTATION_SPEED * deltaTime, 0.0f} };
    }
    //globals.rotation += globals.isRotating ? ROTATION_SPEED * deltaTime : 0.0f;

    Game::updateCamera(deltaTime);
}

void Game::render(SDL_GPUCommandBuffer *commandBuffer, SDL_GPUTexture *swapchainTexture) {
    // create matrices so the gpu can use them to transform the vertices to
    // go on the screen where they belong


    glm::mat4 projectionMatrix = glm::perspective(glm::radians(70.0f), globals.aspectRatio, 0.01f, 1000.0f);

     //glm::mat4 viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, -3.0f));
    glm::mat4 viewMatrix = glm::lookAt(globals.camera.position, globals.camera.target, glm::vec3{ 0.0f, 1.0f, 0.0f });

    fragUniform.lightPosition = globals.lightPosition;
    fragUniform.lightColor = globals.lightColor;
    fragUniform.lightIntensity = globals.lightIntensity;

    SDL_PushGPUVertexUniformData(commandBuffer, 0, &fragUniform, sizeof(fragUniform));

    // CREATE COLOR TARGET

    // color target: where to draw the things
    SDL_GPUColorTargetInfo colorTargetInfo{
        .texture = swapchainTexture,
        // replace previous color, clear (screen?) with color
        .clear_color = globals.clearColor,
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


    for (const Entity &entity : globals.entities) {
        // 3d (4d) rotation matrix
        //glm::mat4 modelMatrix = glm::rotate(glm::mat4(1.0f), globals.rotation, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), entity.position) * glm::toMat4(entity.rotation);

        // calculate mvp matrix to send to the vertex shader
        //matrixUniform.mvp = projectionMatrix * viewMatrix * modelMatrix;
        matrixUniform.vp = projectionMatrix * viewMatrix;
        matrixUniform.m = modelMatrix;

        Assets::Model& model = globals.models[entity.modelID];
        
        // bind the graphics pipeline
        SDL_BindGPUGraphicsPipeline(renderPass, globals.fillPipeline);

        // DRAW SOMETHING
        SDL_GPUBufferBinding vertexBufferBinding{
            .buffer = model.vertexBuffer,
            .offset = 0,
        };
        SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBufferBinding, 1);

        SDL_GPUBufferBinding indexBufferBinding{
            .buffer = model.indexBuffer,
            .offset = 0,
        };
        SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

        SDL_GPUTextureSamplerBinding textureSamplerBinding{
            .texture = model.colormapTexture,
            .sampler = globals.sampler,
        };
        SDL_BindGPUFragmentSamplers(renderPass, 0, &textureSamplerBinding, 1);

        // push vertex uniform data so the gpu can perform the matrix
        // multiplication
        SDL_PushGPUVertexUniformData(commandBuffer, 0, &matrixUniform, sizeof(matrixUniform));

        // fragment shader uniforms
        // SDL_PushGPUFragmentUniformData();

        SDL_DrawGPUIndexedPrimitives(renderPass, model.numIndices, 1, 0, 0, 0);
    }





    // END RENDER PASS

    SDL_EndGPURenderPass(renderPass);
}
