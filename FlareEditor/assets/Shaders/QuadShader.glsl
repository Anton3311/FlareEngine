#type vertex
#version 450

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec4 i_Color;
layout(location = 2) in vec2 i_UV;
layout(location = 3) in float i_TextureIndex;
layout(location = 4) in int i_EntityIndex;

struct CameraData
{
	vec3 Position;

	mat4 Projection;
	mat4 View;
	mat4 ViewProjection;

	mat4 InverseProjection;
	mat4 InverseView;
	mat4 InverseViewProjection;
};

layout(std140, binding = 0) uniform Camera
{
	CameraData u_Camera;
};

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
    gl_Position = u_Camera.ViewProjection * vec4(i_Position, 1.0);
}

#type fragment
#version 450

layout(binding = 0) uniform sampler2D u_Textures[32];

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
