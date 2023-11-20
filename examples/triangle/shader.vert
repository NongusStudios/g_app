#version 450

layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec3 a_color;

layout(location = 0) out vec3 o_color;

layout(push_constant) uniform constants {
    float offsetx;
    float offsety;
} push_constants;

void main(){
    gl_Position = vec4(a_pos.x + push_constants.offsetx, push_constants.offsety + a_pos.y, 0.0, 1.0);
    o_color = a_color;
}