#version 450

layout(location = 0) in vec2 attr_uv;

layout(location = 0) out vec4 frag_color;

layout(binding = 0) uniform sampler2D u_screen;

void main(){
    frag_color = texture(u_screen, attr_uv);
}