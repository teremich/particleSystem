#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 in_color;

out vec3 vert_color;

uniform float scale;
uniform mat4 proj, view, model;

void main() {
    gl_Position = proj * view * model * vec4(position, 1.);
    vert_color = in_color;
}
