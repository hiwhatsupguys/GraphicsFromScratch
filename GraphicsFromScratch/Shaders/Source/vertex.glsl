// opengl version 4.6, doesn't matter since we're converting to Vulkan SPIRV anyway
#version 460

// vertex shader takes in position and color to map to correct variables
layout (location = 0) in vec3 a_position;
layout (location = 1) in vec4 a_color;

// varying color output to send to the fragment shader later
layout (location = 0) out vec4 v_color;

void main()
{
    // use gl_Position to set the vertex position to what was passed in the vertex buffer
    // same as vec4(a_position.x, a_position.y, a_position.z, 1.0f) where 1.0f is w (special camera perspecive value)
    gl_Position = vec4(a_position, 1.0f);
    // output the vertex color
    v_color = a_color;
}