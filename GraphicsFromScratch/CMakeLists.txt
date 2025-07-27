# CMakeList.txt : CMake project for GraphicsFromScratch, include source and define
# project specific logic here.
#

# add every single .h and .cpp file in each subdirectory
file(GLOB_RECURSE EXECUTABLES
     "*.h"
     "*.cpp"
)

add_executable(${EXECUTABLE_NAME} ${EXECUTABLES})

# copy shaders to build directory
# add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
#   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/Content $<TARGET_FILE_DIR:${EXECUTABLE_NAME}>/Content
#   COMMENT "COPYING CONTENT DIRECTORY NEXT TO EXECUTABLE"
#  )

# set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_custom_target(copy_resources ALL
        #COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/Content/Shaders/Source/compile.sh"
        #COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/Content/Shaders/MyShaders/compile.sh"

        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Content
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Content


        COMMENT "Copying resources into binary directory"
)

# MyShaders is not compiling for some reason
#add_custom_target(compile_shaders ALL
#        COMMENT "Compiling Shaders"
#)

# copy resources depends on compile shaders to run
#add_dependencies(copy_resources compile_shaders )
add_dependencies(${PROJECT_NAME} copy_resources )


# add_executable (${EXECUTABLE_NAME} "Game.cpp"  "GameObjects/Block.h" "GameObjects/Block.cpp" "globals.h" "globals.cpp"   "GameObjects/GameObject.h" "GameObjects/GameObject.cpp" "Game.h" "Vector2D.h" "Vector2D.cpp"   "Collisions/Collisions.h" "Collisions/Collisions.cpp" "RenderTools.cpp" "RenderTools.h" "GameObjects/Ball.h" "GameObjects/Ball.cpp" "ShapeGenerators/CircleGenerator.h" "ShapeGenerators/RoundedRectGenerator.h" "Vertex.h" "ShapeGenerators/RectGenerator.h" "GameObjects/Polygon.h" "Collisions/CollisionManifold.h")
# target_include_directories(${EXECUTABLE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ShapeGenerators)
# target_include_directories(${EXECUTABLE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/GameObjects)
# target_include_directories(${EXECUTABLE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Collisions)
