DepthTest = false

#begin vertex
#version 450

#include "Common/Camera.glsl"

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec4 i_Color;
layout(location = 2) in vec2 i_UV;
layout(location = 3) in float i_TextureIndex;
#ifdef OPENGL
	layout(location = 4) in int i_EntityIndex;
#endif

layout(location = 0) out vec4 o_VertexColor;
layout(location = 1) out vec2 o_UV;
layout(location = 2) flat out float o_TextureIndex;
#ifdef OPENGL
	layout(location = 3) flat out int o_EntityIndex;
#endif

void main()
{
    o_VertexColor = i_Color;
    o_UV = i_UV;
    o_TextureIndex = i_TextureIndex;
#ifdef OPENGL
    o_EntityIndex = i_EntityIndex;
    gl_Position = u_Camera.ViewProjection * vec4(i_Position, 1.0);
#else
    gl_Position = vec4(i_Position, 1.0);
#endif
}

#end

#begin pixel
#version 450

#ifdef OPENGL
	layout(binding = 0) uniform sampler2D u_Textures[32];
#endif

layout(location = 0) in vec4 i_VertexColor;
layout(location = 1) in vec2 i_UV;
layout(location = 2) flat in float i_TextureIndex;
layout(location = 3) flat in int i_EntityIndex;

layout(location = 0) out vec4 o_Color;

#ifdef OPENGL
	layout(location = 2) out int o_EntityIndex;
#endif

void main()
{
#ifdef OPENGL
    o_Color = texture(u_Textures[int(i_TextureIndex)], i_UV);
    if (o_Color.a == 0)
        discard;

    o_Color *= i_VertexColor;
#endif

#ifdef VULKAN
	o_Color = i_VertexColor;
#endif

#ifdef OPENGL
    o_EntityIndex = i_EntityIndex;
#endif
}

#end
