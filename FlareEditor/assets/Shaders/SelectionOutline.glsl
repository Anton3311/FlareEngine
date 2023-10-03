#type vertex
#version 420 

layout(location = 0) in vec2 i_Position;

struct Vertex
{
	vec2 UV;
};

layout(location = 0) out Vertex VertexData;

void main()
{
	gl_Position = vec4(i_Position, 0.0, 1.0);
	VertexData.UV = i_Position / 2.0 + vec2(0.5);
}

#type fragment
#version 420

struct Vertex
{
	vec2 UV;
};

layout(location = 0) in Vertex VertexData;
layout(location = 0) out vec4 o_Color;

layout(std140, push_constant) uniform OutlineData
{
	vec4 u_OutlineColor;
	int u_SelectedId;
	vec2 u_OutlineThickness;
};

layout(binding = 0) uniform isampler2D u_IdsTexture;

void main()
{
	int currentId = texture(u_IdsTexture, VertexData.UV).r;
	int offsettedId0 = texture(u_IdsTexture, VertexData.UV + u_OutlineThickness).r;
	int offsettedId1 = texture(u_IdsTexture, VertexData.UV + vec2(-u_OutlineThickness.x, u_OutlineThickness.y)).r;
	int offsettedId2 = texture(u_IdsTexture, VertexData.UV - u_OutlineThickness).r;
	int offsettedId3 = texture(u_IdsTexture, VertexData.UV + vec2(u_OutlineThickness.x, -u_OutlineThickness.y)).r;

	bool outline = currentId != offsettedId0
		|| currentId != offsettedId1
		|| currentId != offsettedId2
		|| currentId != offsettedId3;

	if (currentId == u_SelectedId && outline)
	{
		o_Color = u_OutlineColor;
		return;
	}
	discard;
}
