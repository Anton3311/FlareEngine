Type = 2D
DepthTest = false

#begin vertex
#version 450

#include "Common/Camera.glsl"

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec4 i_Color;
layout(location = 2) in vec2 i_UV;
layout(location = 3) in int i_TextureIndex;
#ifdef OPENGL
	layout(location = 4) in int i_EntityIndex;
#endif

layout(location = 0) out vec4 o_VertexColor;
layout(location = 1) out vec2 o_UV;
layout(location = 2) flat out int o_TextureIndex;
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
#endif

    gl_Position = u_Camera.ViewProjection * vec4(i_Position, 1.0);
}

#end

#begin pixel
#version 450

layout(set = 1, binding = 0) uniform sampler2D u_Textures[32];

layout(location = 0) in vec4 i_VertexColor;
layout(location = 1) in vec2 i_UV;
layout(location = 2) flat in int i_TextureIndex;
layout(location = 3) flat in int i_EntityIndex;

layout(location = 0) out vec4 o_Color;

#ifdef OPENGL
	layout(location = 2) out int o_EntityIndex;
#endif

void main()
{
	switch (i_TextureIndex)
	{
	case 0: o_Color = texture(u_Textures[0], i_UV); break;
	case 1: o_Color = texture(u_Textures[1], i_UV); break;
	case 2: o_Color = texture(u_Textures[2], i_UV); break;
	case 3: o_Color = texture(u_Textures[3], i_UV); break;
	case 4: o_Color = texture(u_Textures[4], i_UV); break;
	case 5: o_Color = texture(u_Textures[5], i_UV); break;
	case 6: o_Color = texture(u_Textures[6], i_UV); break;
	case 7: o_Color = texture(u_Textures[7], i_UV); break;
	case 8: o_Color = texture(u_Textures[8], i_UV); break;
	case 9: o_Color = texture(u_Textures[9], i_UV); break;
	case 10: o_Color = texture(u_Textures[10], i_UV); break;
	case 11: o_Color = texture(u_Textures[11], i_UV); break;
	case 12: o_Color = texture(u_Textures[12], i_UV); break;
	case 13: o_Color = texture(u_Textures[13], i_UV); break;
	case 14: o_Color = texture(u_Textures[14], i_UV); break;
	case 15: o_Color = texture(u_Textures[15], i_UV); break;
	case 16: o_Color = texture(u_Textures[16], i_UV); break;
	case 17: o_Color = texture(u_Textures[17], i_UV); break;
	case 18: o_Color = texture(u_Textures[18], i_UV); break;
	case 19: o_Color = texture(u_Textures[19], i_UV); break;
	case 20: o_Color = texture(u_Textures[20], i_UV); break;
	case 21: o_Color = texture(u_Textures[21], i_UV); break;
	case 22: o_Color = texture(u_Textures[22], i_UV); break;
	case 23: o_Color = texture(u_Textures[23], i_UV); break;
	case 24: o_Color = texture(u_Textures[24], i_UV); break;
	case 25: o_Color = texture(u_Textures[25], i_UV); break;
	case 26: o_Color = texture(u_Textures[26], i_UV); break;
	case 27: o_Color = texture(u_Textures[27], i_UV); break;
	case 28: o_Color = texture(u_Textures[28], i_UV); break;
	case 29: o_Color = texture(u_Textures[29], i_UV); break;
	case 30: o_Color = texture(u_Textures[30], i_UV); break;
	case 31: o_Color = texture(u_Textures[31], i_UV); break;
	}

    if (o_Color.a == 0)
        discard;

    o_Color *= i_VertexColor;

#ifdef OPENGL
    o_EntityIndex = i_EntityIndex;
#endif
}

#end
