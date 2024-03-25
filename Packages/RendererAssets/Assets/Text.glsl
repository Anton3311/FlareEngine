#begin vertex
#version 450

#include "Common/Camera.glsl"

layout(location = 0) in vec3 i_Position;
layout(location = 1) in vec4 i_Color;
layout(location = 2) in vec2 i_UV;
layout(location = 3) in int i_EntityIndex;

layout(location = 0) out vec2 UV;
layout(location = 1) out vec4 Color;

#ifdef OPENGL
	layout(location = 2) out flat int EntityIndex;
#endif

void main()
{
	gl_Position = u_Camera.ViewProjection * vec4(i_Position, 1.0);
	UV = i_UV;
	Color = i_Color;
#ifdef OPENGL
	EntityIndex = i_EntityIndex;
#endif
}

#end

#begin pixel
#version 450

layout(location = 0) in vec2 UV;
layout(location = 1) in vec4 Color;
#ifdef OPENGL
	layout(location = 2) in flat int EntityIndex;
#endif

layout(set = 1, binding = 0) uniform sampler2D u_MSDF;

layout(location = 0) out vec4 o_Color;

#ifdef OPENGL
	layout(location = 2) out int o_EntityIndex;
#endif

const float pxRange = 2.0;

float screenPxRange()
{
    vec2 unitRange = vec2(pxRange) / vec2(textureSize(u_MSDF, 0));
    vec2 screenTexSize = vec2(1.0) / fwidth(UV);
    return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

float median(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

void main() {
    vec3 msd = texture(u_MSDF, UV).rgb;
    float sd = median(msd.r, msd.g, msd.b);
    float screenPxDistance = screenPxRange()*(sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);

	if (opacity <= 0.001)
		discard;

    o_Color = mix(vec4(0.0), Color, opacity);
#ifdef OPENGL
	o_EntityIndex = EntityIndex;
#endif
}

#end
