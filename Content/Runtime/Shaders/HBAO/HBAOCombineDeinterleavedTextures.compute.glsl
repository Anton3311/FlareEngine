#begin compute
#version 450

const uint TILE_SIZE = 16;

layout(set = 3, binding = 0, r8) uniform readonly image2D u_AOTexture0;
layout(set = 3, binding = 1, r8) uniform readonly image2D u_AOTexture1;
layout(set = 3, binding = 2, r8) uniform readonly image2D u_AOTexture2;
layout(set = 3, binding = 3, r8) uniform readonly image2D  u_AOTexture3;

layout(set = 3, binding = 4, r8) uniform writeonly image2D  u_OutputImage;

layout(std140, push_constant) uniform Constants
{
	ivec2 u_OutputImageSize;
};

layout(local_size_x = TILE_SIZE, local_size_y = TILE_SIZE, local_size_z = 1) in;
void main()
{
	ivec2 pixelPosition = ivec2(gl_GlobalInvocationID.xy);
	if (pixelPosition.x * 2 >= u_OutputImageSize.x || pixelPosition.y * 2 >= u_OutputImageSize.y)
	{
		return;
	}

	ivec2 aoSampleCoordinates = pixelPosition;

	vec4 aoSample0 = imageLoad(u_AOTexture0, pixelPosition);
	vec4 aoSample1 = imageLoad(u_AOTexture1, pixelPosition);
	vec4 aoSample2 = imageLoad(u_AOTexture2, pixelPosition);
	vec4 aoSample3 = imageLoad(u_AOTexture3, pixelPosition);

	imageStore(u_OutputImage, pixelPosition * 2 + ivec2(0, 0), aoSample0);
	imageStore(u_OutputImage, pixelPosition * 2 + ivec2(1, 0), aoSample1);
	imageStore(u_OutputImage, pixelPosition * 2 + ivec2(0, 1), aoSample2);
	imageStore(u_OutputImage, pixelPosition * 2 + ivec2(1, 1), aoSample3);
}

#end
