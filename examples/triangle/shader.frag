#version 450

layout(location = 0) in vec3 o_color;

layout(location = 0) out vec4 frag_color;

void main(){
    frag_color = vec4(o_color, 1.0);
}