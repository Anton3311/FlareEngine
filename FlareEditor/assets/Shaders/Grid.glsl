#type vertex
#version 450

struct CameraData
{
	vec3 Position;

	mat4 Projection;
	mat4 View;
	mat4 ViewProjection;

	mat4 InverseProjection;
	mat4 InverseView;
	mat4 InverseViewProjection;
};

layout(std140, binding = 0) uniform Camera
{
	CameraData u_Camera;
};

layout(std140, push_constant) uniform GridData
{
	vec3 Offset;
	float GridScale;
	float CellScale;
	float Thickness;
	vec3 Color;
	float FallOffThreshold;
} u_Data;

struct CellData
{
	float Thickness;
	float CellScale;
	float FallOffThreshold;
	vec4 Color;
};

layout(location = 0) in vec2 i_Position;
layout(location = 0) out vec2 o_Position;
layout(location = 1) out vec2 o_UV;
layout(location = 2) out CellData o_CellData;

void main()
{
	vec3 worldSpacePosition = u_Data.GridScale * vec3(i_Position.x, 0.0, i_Position.y) + u_Data.Offset;
	gl_Position = u_Camera.ViewProjection * vec4(worldSpacePosition, 1.0);

	o_Position = worldSpacePosition.xz;
	o_UV = i_Position;

	o_CellData.FallOffThreshold = u_Data.FallOffThreshold;
	o_CellData.CellScale = u_Data.CellScale;
	o_CellData.Color = vec4(u_Data.Color, 1.0);
	o_CellData.Thickness = u_Data.Thickness;
}

#type fragment
#version 450

struct CellData
{
	float Thickness;
	float CellScale;
	float FallOffThreshold;
	vec4 Color;
};

layout(location = 0) in vec2 i_Position;
layout(location = 1) in vec2 i_UV;
layout(location = 2) in CellData i_CellData;
layout(location = 0) out vec4 o_Color;

void main()
{
	float a = smoothstep(0.0, i_CellData.FallOffThreshold, 1.0 - min(1.0, length(i_UV)));
	vec4 color = vec4(i_CellData.Color.rgb, a);

	vec2 uv = i_Position * i_CellData.CellScale;

	vec2 uv1 = abs(fract(uv) * 2.0 - vec2(1.0));
	float dist = max(uv1.x, uv1.y);

	vec2 uv2 = abs(fract(uv * 5.0) * 2.0 - vec2(1.0));
	float dist2 = max(uv2.x, uv2.y);

	float step = smoothstep(1.0 - i_CellData.Thickness, 1.0, dist);
	float step2 = smoothstep(1.0 - i_CellData.Thickness * 2.0, 1.0, dist2);

	if (step >= 0.01)
	{
		o_Color = color;
		return;
	}

	if (step2 <= 0.01)
		discard;

	o_Color = vec4(color.rgb / 2.0, a);
}
