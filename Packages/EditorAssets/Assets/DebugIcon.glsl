DepthTest = true
BlendMode = Transparent

#begin vertex
#version 450

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec4 i_Color;
layout(location = 2) in vec2 i_UV;
layout(location = 3) in float i_TextureIndex;
layout(location = 4) in int i_EntityIndex;

layout(location = 0) out vec4 o_VertexColor;
layout(location = 1) out vec2 o_UV;
layout(location = 2) flat out float o_TextureIndex;
layout(location = 3) flat out int o_EntityIndex;

layout(std140, binding = 0) uniform Camera
{
	vec3 Position;
	float Near;
	vec3 ViewDirection;
	float Far;

	mat4 Projection;
	mat4 View;
	mat4 ViewProjection;

	mat4 InverseProjection;
	mat4 InverseView;
	mat4 InverseViewProjection;

	ivec2 ViewportSize;
	float FOV;
} u_Camera;

void main()
{
    o_VertexColor = i_Color;
    o_UV = i_UV;
    o_TextureIndex = i_TextureIndex;
    o_EntityIndex = i_EntityIndex;
    gl_Position = u_Camera.Projection * vec4(i_Position, 1.0);
}

#end

#begin pixel
#version 450

layout(binding = 0) uniform sampler2D u_Textures[32];

layout(location = 0) in vec4 i_VertexColor;
layout(location = 1) in vec2 i_UV;
layout(location = 2) flat in float i_TextureIndex;
layout(location = 3) flat in int i_EntityIndex;

layout(location = 0) out vec4 o_Color;
layout(location = 2) out int o_EntityIndex;

void main()
{
    o_Color = texture(u_Textures[int(i_TextureIndex)], i_UV);
    if (o_Color.a == 0)
        discard;

    o_Color *= i_VertexColor;
    o_EntityIndex = i_EntityIndex;
}

#end

