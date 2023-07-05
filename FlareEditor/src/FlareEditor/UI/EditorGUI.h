#pragma once

#include "Flare.h"

namespace Flare
{
	class EditorGUI
	{
	public:
		static bool BeginPropertyGrid();
		static void EndPropertyGrid();

		static bool FloatPropertyField(const char* name, float& value);
		static bool Vector3PropertyField(const char* name, glm::vec3& value);
		static bool ColorPropertyField(const char* name, glm::vec4& color);
	private:
		static void RenderPropertyName(const char* name);
	};
}