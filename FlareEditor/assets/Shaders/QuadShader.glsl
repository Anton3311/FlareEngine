#type vertex
#version 330

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec4 i_Color;
layout(location = 2) in vec2 i_UV;
layout(location = 3) in float i_TextureIndex;
layout(location = 4) in int i_EntityIndex;

uniform mat4 u_Projection;

out vec4 VertexColor;
out vec2 UV;
flat out float TextureIndex;
flat out int EntityIndex;

void main()
{
    VertexColor = i_Color;
    UV = i_UV;
    TextureIndex = i_TextureIndex;
    EntityIndex = i_EntityIndex;
    gl_Position = u_Projection * vec4(i_Position, 1.0);
}

#type fragment
#version 330

uniform sampler2D u_Textures[32];

in vec2 UV;
in vec4 VertexColor;
flat in float TextureIndex;
flat in int EntityIndex;

out vec4 o_Color;
out int o_EntityIndex;

void main()
{
    o_Color = texture(u_Textures[int(TextureIndex)], UV);
    if (o_Color.a == 0)
        discard;

    o_Color *= VertexColor;
    o_EntityIndex = EntityIndex;
}
