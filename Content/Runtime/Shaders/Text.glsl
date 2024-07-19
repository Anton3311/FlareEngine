Type = 2D
DepthTest = false
Blending = Transparent

#begin vertex
#version 450

#include "Common/Camera.glsl"

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec4 i_Color;
layout(location = 2) in vec2 i_UV;

layout(location = 0) out vec2 o_UV;
layout(location = 1) out vec4 o_Color;

void main()
{
	gl_Position = u_Camera.ViewProjection * vec4(i_Position, 1.0);
	o_UV = i_UV;
	o_Color = i_Color;
}

#end

#begin pixel
#version 450

layout(location = 0) in vec2 i_UV;
layout(location = 1) in vec4 i_Color;

layout(set = 1, binding = 0) uniform sampler2D u_MSDF;

layout(location = 0) out vec4 o_Color;

const float pxRange = 2.0;

float ScreenPxRange()
{
    vec2 unitRange = vec2(pxRange) / vec2(textureSize(u_MSDF, 0));
    vec2 screenTexSize = vec2(1.0) / fwidth(i_UV);
    return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

float Median(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

void main() {
    vec3 msd = texture(u_MSDF, i_UV).rgb;
    float sd = Median(msd.r, msd.g, msd.b);
    float screenPxDistance = ScreenPxRange() * (sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);

	if (opacity <= 0.001)
		discard;

    o_Color = vec4(i_Color.rgb, i_Color.a * opacity);
}

#end
