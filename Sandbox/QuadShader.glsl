#type vertex
#version 330

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec4 i_Color;
layout(location = 2) in vec2 i_UV;
layout(location = 3) in float i_TextureIndex;
layout(location = 4) in vec2 i_TextureTiling;

uniform mat4 u_Projection;

out vec4 VertexColor;
out vec2 UV;
flat out float TextureIndex;

void main()
{
    VertexColor = i_Color;
    UV = i_UV * i_TextureTiling;
    TextureIndex = i_TextureIndex;
    gl_Position = u_Projection * vec4(i_Position, 1.0);
}

#type fragment
#version 330

uniform sampler2D u_Textures[32];

in vec2 UV;
in vec4 VertexColor;
flat in float TextureIndex;

out vec4 o_Color;

void main()
{
    o_Color = texture(u_Textures[int(TextureIndex)], UV);
    o_Color *= VertexColor;
}