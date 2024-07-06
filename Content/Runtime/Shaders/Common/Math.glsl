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

#endif
