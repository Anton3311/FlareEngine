#type vertex
#version 330

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec4 i_Color;
layout(location = 2) in vec2 i_UV;

uniform mat4 u_Projection;

out vec4 VertexColor;
out vec2 UV;

void main()
{
    VertexColor = i_Color;
    UV = i_UV;
    gl_Position = u_Projection * vec4(i_Position, 1.0);
}

#type fragment
#version 330

uniform sampler2D u_Texture;

in vec2 UV;
in vec4 VertexColor;

out vec4 o_Color;

void main()
{
    o_Color = VertexColor * texture(u_Texture, UV);
}