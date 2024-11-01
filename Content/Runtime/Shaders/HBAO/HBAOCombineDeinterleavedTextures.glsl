#begin vertex
#version 450

layout(location = 0) in vec3 i_Position;

void main()
{
	gl_Position = vec4(i_Position, 1.0f);
}

#end

#begin pixel
#version 450

layout(set = 3, binding = 0) uniform sampler2D u_AOTexture0;
layout(set = 3, binding = 1) uniform sampler2D u_AOTexture1;
layout(set = 3, binding = 2) uniform sampler2D u_AOTexture2;
layout(set = 3, binding = 3) uniform sampler2D u_AOTexture3;

layout(location = 0) out float o_AO;

void main()
{
	ivec2 pixelPosition = ivec2(gl_FragCoord.xy);

	ivec2 aoSampleCoordinates = pixelPosition / 2;
	ivec2 offset = ivec2(pixelPosition) & 1;

	int textureIndex = offset.y * 2 + offset.x;

	float ao = 0.0f;
	switch (textureIndex)
	{
	case 0:
		ao = texelFetch(u_AOTexture0, aoSampleCoordinates, 0).r;
		break;
	case 1:
		ao = texelFetch(u_AOTexture1, aoSampleCoordinates, 0).r;
		break;
	case 2:
		ao = texelFetch(u_AOTexture2, aoSampleCoordinates, 0).r;
		break;
	case 3:
		ao = texelFetch(u_AOTexture3, aoSampleCoordinates, 0).r;
		break;
	}

	o_AO = ao;
}

#end
