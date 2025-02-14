#begin compute
#version 450

#include "../Common/Camera.glsl"

layout(set = 3, binding = 0) uniform sampler2D u_DepthTexture;
layout(set = 3, binding = 1, rgba32f) uniform writeonly image2D u_LinearDepth;

layout(std140, push_constant) uniform Constants
{
	ivec2 u_ImageSize;
};

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
	ivec2 texelPosition = ivec2(gl_GlobalInvocationID.xy);
	if (texelPosition.x >= u_ImageSize.x || texelPosition.y >= u_ImageSize.y)
		return;

	float linearDepth = LinearizeDepth(texelFetch(u_DepthTexture, texelPosition, 0).r, u_Camera.Near, u_Camera.Far);
	imageStore(u_LinearDepth, ivec2(texelPosition), vec4(linearDepth));
}

#end
