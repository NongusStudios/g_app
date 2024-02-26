#version 450

layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec2 a_uv;

struct Cell {
    int type;
    int has_moved;
};
layout (std140, set = 0, binding = 0) buffer CellBuffer { Cell cells[]; } cell_buffer;

void main(){
//      00101011
//    - 01000111
//      --------|
//              |
//              V
//      00101011
//    + 10111001
//      --------
//      11100100

}