#version 450

layout(location = 0) in vec2 attr_pos;
layout(location = 1) in vec2 attr_uv;

layout(location = 0) out vec2 out_uv;

void main(){
    gl_Position = vec4(attr_pos, 0.0, 1.0);
    out_uv = attr_uv;
}