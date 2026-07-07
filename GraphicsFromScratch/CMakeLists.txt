# CMakeList.txt : CMake project for GraphicsFromScratch, include source and define
# project specific logic here.
#

# add every single .h and .cpp file in each subdirectory
file (GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "*.h")
file (GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "*.cpp")

add_executable(${EXECUTABLE_NAME} ${SOURCES} ${HEADERS})

# copy shaders to build directory
# add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
#   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/Content $<TARGET_FILE_DIR:${EXECUTABLE_NAME}>/Content
#   COMMENT "COPYING CONTENT DIRECTORY NEXT TO EXECUTABLE"
#  )

# set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_custom_target(compile_shaders ALL

    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/Content/Shaders/MyShaders/compile.sh"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Content/Shaders/MyShaders"
    COMMENT "Compiling shaders"
)

add_custom_target(copy_resources ALL
        #COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/Content/Shaders/Source/compile.sh"
        #COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/Content/Shaders/MyShaders/compile.sh"

        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Content
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Content


        COMMENT "Copying resources into binary directory"
)

# copy resources depends on compile shaders to run
# add_dependencies(${EXECUTABLE_NAME} copy_resources compile_shaders)

# On Windows: copy SDL runtime DLLs next to the executable so the loader can find them.
# The executable target is created in this directory, so attach the POST_BUILD command here.
if (WIN32)
  add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL3::SDL3> $<TARGET_FILE_DIR:${EXECUTABLE_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL3_image::SDL3_image> $<TARGET_FILE_DIR:${EXECUTABLE_NAME}>
    COMMENT "Copying SDL3 runtime DLLs next to ${EXECUTABLE_NAME}"
  )
endif()

# maybe put this at the bottom of the top-level cmakelists
target_include_directories(${EXECUTABLE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME})

target_compile_definitions(${EXECUTABLE_NAME} PUBLIC SDL_MAIN_USE_CALLBACKS)



# Link SDL to our executable. This also makes its include directory available to us. 
target_link_libraries(${EXECUTABLE_NAME} PUBLIC 
#	SDL3_ttf::SDL3_ttf      # remove if you are not using SDL_ttf
#	SDL3_mixer::SDL3_mixer  # remove if you are not using SDL_mixer
	SDL3_image::SDL3_image	# remove if you are not using SDL_image
    SDL3::SDL3              # If using satelite libraries, SDL must be the last item in the list. 
    glm::glm-header-only
    nlohmann_json::nlohmann_json
    ImGui
)


# add_executable (${EXECUTABLE_NAME} "Game.cpp"  "GameObjects/Block.h" "GameObjects/Block.cpp" "globals.h" "globals.cpp"   "GameObjects/GameObject.h" "GameObjects/GameObject.cpp" "Game.h" "Vector2D.h" "Vector2D.cpp"   "Collisions/Collisions.h" "Collisions/Collisions.cpp" "RenderTools.cpp" "RenderTools.h" "GameObjects/Ball.h" "GameObjects/Ball.cpp" "ShapeGenerators/CircleGenerator.h" "ShapeGenerators/RoundedRectGenerator.h" "Vertex.h" "ShapeGenerators/RectGenerator.h" "GameObjects/Polygon.h" "Collisions/CollisionManifold.h")
# target_include_directories(${EXECUTABLE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ShapeGenerators)
# target_include_directories(${EXECUTABLE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/GameObjects)
# target_include_directories(${EXECUTABLE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Collisions)
