#begin compute
#version 450

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(rgba8, set = 3, binding = 0) uniform image2D u_Color;
layout(set = 3, binding = 1) uniform sampler2D u_AO;

layout(std140, push_constant) uniform Constants
{
	ivec2 u_ImageSize;
};

void main()
{
	ivec2 pixelCoordinates = ivec2(gl_GlobalInvocationID.xy);
	if (pixelCoordinates.x >= u_ImageSize.x || pixelCoordinates.y >= u_ImageSize.y)
		return;

	vec4 color = imageLoad(u_Color, pixelCoordinates);

	vec2 uv = vec2(float(pixelCoordinates.x) + 0.5f, float(pixelCoordinates.y) + 0.5f) / vec2(u_ImageSize);
	float ao = texture(u_AO, uv).r;

	color.rgb *= ao;

	imageStore(u_Color, pixelCoordinates, color);
}

#end
