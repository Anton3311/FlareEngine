#ifndef MATH_H
#define MATH_H

const float PI = 3.1415926535897932384626433832795;
const float TWO_PI = 3.1415926535897932384626433832795 * 2.0f;
const float HALF_PI = 3.1415926535897932384626433832795 / 2.0f;

float InterleavedGradientNoise(vec2 screenSpacePosition)
{
	const float scale = 64.0;
	vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
	return -scale + 2.0 * scale * fract(magic.z * fract(dot(screenSpacePosition, magic.xy)));
}

float LuminanceFromRGB(vec3 rgb)
{
	return dot(rgb, vec3(0.2126, 0.7152, 0.0722));
}

// https://en.wikipedia.org/wiki/SRGB#Theory_of_the_transformation
vec3 LinearRGBToSRGB(vec3 rgb)
{
	bvec3 condition = greaterThan(rgb, vec3(0.0031308f));
	return mix(12.92f * rgb,
		1.055f * pow(rgb, vec3(1.0f / 2.4f)) - vec3(0.055f),
		condition);
}

vec3 SRGBToLinearRGB(vec3 srgb)
{
	bvec3 condition = greaterThan(srgb, vec3(0.04045f));
	return mix(srgb / 12.92f,
		pow((srgb + vec3(0.055f)) / 1.055f, vec3(2.4f)),
		condition);
}

#endif
