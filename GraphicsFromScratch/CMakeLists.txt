# CMakeList.txt : CMake project for GraphicsFromScratch, include source and define
# project specific logic here.
#

add_executable(${EXECUTABLE_NAME} "Main.cpp"  )

# copy shaders to build directory
# add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
#  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/Shaders $<TARGET_FILE_DIR:${EXECUTABLE_NAME}>/Shaders
# )

add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/Content $<TARGET_FILE_DIR:${EXECUTABLE_NAME}>/Content
 )

# add_executable (${EXECUTABLE_NAME} "Game.cpp"  "GameObjects/Block.h" "GameObjects/Block.cpp" "globals.h" "globals.cpp"   "GameObjects/GameObject.h" "GameObjects/GameObject.cpp" "Game.h" "Vector2D.h" "Vector2D.cpp"   "Collisions/Collisions.h" "Collisions/Collisions.cpp" "RenderTools.cpp" "RenderTools.h" "GameObjects/Ball.h" "GameObjects/Ball.cpp" "ShapeGenerators/CircleGenerator.h" "ShapeGenerators/RoundedRectGenerator.h" "Vertex.h" "ShapeGenerators/RectGenerator.h" "GameObjects/Polygon.h" "Collisions/CollisionManifold.h")
# target_include_directories(${EXECUTABLE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ShapeGenerators)
# target_include_directories(${EXECUTABLE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/GameObjects)
# target_include_directories(${EXECUTABLE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Collisions)
