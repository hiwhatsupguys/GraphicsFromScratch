// opengl 4.6
#version 460

// input passed from the vertex shader, matching at location 0
layout (location = 0) in vec4 v_color;
// output FragColor saves final color to resulting pixel
layout (location = 0) out vec4 FragColor;

layout (std140, set = 3, binding = 0) uniform UniformBlock {
  float time;
};


void main() {

// float pulse = sin(time * 2.0) * 0.5 + 0.5; // range [0, 1]
float pulse = cos(time * 2.0) * 0.5 + 0.5; // range [0, 1]
// float pulse = 0;

  // actually set the color of the pixel
  FragColor = vec4(v_color.rgb * pulse, v_color.a);
}
