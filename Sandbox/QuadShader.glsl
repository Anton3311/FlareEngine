#type vertex
#version 330

layout(location = 0) in vec2 i_Position;
layout(location = 1) in vec4 i_Color;

out vec4 VertexColor;

void main()
{
    VertexColor = i_Color;
    gl_Position = vec4(i_Position, 0.0, 1.0);
}

#type fragment
#version 330

in vec4 VertexColor;

out vec4 o_Color;

void main()
{
    o_Color = VertexColor;
}