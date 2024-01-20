#begin vertex
#version 450

#include "Common/Camera.glsl"

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec4 i_Color;
layout(location = 2) in vec2 i_UV;
layout(location = 3) in float i_TextureIndex;
layout(location = 4) in int i_EntityIndex;

layout(location = 0) out vec4 VertexColor;
layout(location = 1) out vec2 UV;
layout(location = 2) flat out float TextureIndex;
layout(location = 3) flat out int EntityIndex;

void main()
{
    VertexColor = i_Color;
    UV = i_UV;
    TextureIndex = i_TextureIndex;
    EntityIndex = i_EntityIndex;
    gl_Position = u_Camera.ViewProjection * vec4(i_Position, 1.0);
}

#end

#begin pixel
#version 450

layout(binding = 0) uniform sampler2D u_Textures[32];

layout(location = 0) in vec4 VertexColor;
layout(location = 1) in vec2 UV;
layout(location = 2) flat in float TextureIndex;
layout(location = 3) flat in int EntityIndex;

layout(location = 0) out vec4 o_Color;
layout(location = 2) out int o_EntityIndex;

void main()
{
    o_Color = texture(u_Textures[int(TextureIndex)], UV);
    if (o_Color.a == 0)
        discard;

    o_Color *= VertexColor;
    o_EntityIndex = EntityIndex;
}

#end
