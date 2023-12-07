#pragma once

#include "Flare/Math/Math.h"
#include "Flare/Renderer/UniformBuffer.h"

#include "FlareCore/Core.h"
#include "FlareCore/Log.h"

#include <glm/glm.hpp>

namespace Flare
{
	struct FrustumPlanes
	{
		static constexpr size_t PlanesCount = 6;
		static constexpr size_t NearPlaneIndex = 0;
		static constexpr size_t FarPlaneIndex = 1;

		inline bool ContainsPoint(const glm::vec3& point) const
		{
			for (size_t i = 0; i < 6; i++)
			{
				if (Planes[i].ValueAt(point) < 0.0f)
					return false;
			}

			return true;
		}

		Math::Plane Planes[PlanesCount];
	};

	struct CameraData
	{
		CameraData()
			: Position(0.0f),
			Unused(0.0f),
			Projection(0.0f),
			View(0.0f),
			ViewProjection(0.0f),
			InverseProjection(0.0f),
			InverseView(0.0f),
			InverseViewProjection(0.0f) {}

		void CalculateViewProjection()
		{
			ViewProjection = Projection * View;
			InverseView = glm::inverse(View);
			InverseViewProjection = glm::inverse(ViewProjection);
		}

		glm::vec3 Position;

		float Unused; // Is needed to align the struct to 16 byte boundary

		glm::mat4 Projection;
		glm::mat4 View;
		glm::mat4 ViewProjection;

		glm::mat4 InverseProjection;
		glm::mat4 InverseView;
		glm::mat4 InverseViewProjection;

		float Near;
		float Far;

		float Padding[2];

		glm::vec3 ViewDirection;
		float FOV;
	};

	struct LightData
	{
		glm::vec3 Color;
		float Intensity;

		glm::vec3 Direction;
		float Unused;
		
		glm::vec4 EnvironmentLight;

		float Near;
	};

	struct RenderData
	{
		CameraData Camera;
		FrustumPlanes CameraFrustumPlanes;
		LightData Light;
		CameraData LightView[4];
		bool IsEditorCamera = false;
	};
}