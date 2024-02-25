#version 450

layout(location = 0) in vec2 frag_uv;
layout(location = 0) out vec4 frag_color;

layout(binding = 0) uniform sampler2D screen_contents;

void main(){
    frag_color = texture(screen_contents, frag_uv);
}