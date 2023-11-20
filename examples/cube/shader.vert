#version 450

layout(set = 0, binding = 0) uniform TransformData {
    mat4 model;
    mat4 projection;
} transform_data;

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_color;

layout(location = 0) out vec3 f_color;

void main(){
    gl_Position = transform_data.projection * transform_data.model * vec4(a_pos, 1.0);
    f_color = a_color;
}