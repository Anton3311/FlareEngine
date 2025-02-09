#pragma once

#include "Flare/Math/Math.h"

#include "FlareCore/Core.h"
#include "FlareCore/Log.h"

#include <glm/glm.hpp>

namespace Flare
{
	struct FLARE_API FrustumPlanes
	{
		static constexpr size_t PlanesCount = 6;
		static constexpr size_t NearPlaneIndex = 0;
		static constexpr size_t FarPlaneIndex = 1;
		static constexpr size_t LeftPlaneIndex = 2;
		static constexpr size_t RightPlaneIndex = 3;
		static constexpr size_t TopPlaneIndex = 4;
		static constexpr size_t BottomPlaneIndex = 5;

		void SetFromViewAndProjection(const glm::mat4& view, const glm::mat4& inverseViewProjection, glm::vec3 viewDirection);

		inline bool ContainsPoint(const glm::vec3& point) const
		{
			for (size_t i = 0; i < 6; i++)
			{
				if (Planes[i].SignedDistance(point) < 0.0f)
					return false;
			}

			return true;
		}

		Math::Plane Planes[PlanesCount];
	};

	struct FLARE_API RenderView
	{
		RenderView() = default;

		void SetViewAndProjection(const glm::mat4& projection, const glm::mat4& view);

		glm::vec3 Position = glm::vec3(0.0f);
		float Near = 0.0f;
		glm::vec3 ViewDirection = glm::vec3(0.0f);
		float Far = 0.0f;

		glm::mat4 Projection = glm::mat4(0.0f);
		glm::mat4 View = glm::mat4(0.0f);
		glm::mat4 ViewProjection = glm::mat4(0.0f);

		glm::mat4 InverseProjection = glm::mat4(0.0f);
		glm::mat4 InverseView = glm::mat4(0.0f);
		glm::mat4 InverseViewProjection = glm::mat4(0.0f);

		glm::ivec2 ViewportSize = glm::ivec2(0);
		float FOV = 0.0f;
	};

	struct LightData
	{
		glm::vec3 Color = glm::vec3(0.0f);
		float Intensity = 0.0f;

		glm::vec3 Direction = glm::vec3(0.0f);
		float Near = 0.0f;
		
		glm::vec4 EnvironmentLight = glm::vec4(0.0f);

		uint32_t PointLightsCount = 0;
		uint32_t SpotLightsCount = 0;

		uint32_t FirstShadowCastingSpotlight = 0;
		uint32_t ShadowCastingSpotlightCount = 0;

		// Making it a `bool` type, causes errors when the `LightData` is copied to the GPU.
		// spirv-cross says that this member of an equivalent struct inside the shader is actually a uint.
		//
		//    struct LightData {
		//        float4 u_LightColor;       // abs offset =  0, rel offset =  0, size = 16, padded size = 16
		//        float3 u_LightDirection;   // abs offset = 16, rel offset = 16, size = 12, padded size = 12
		//        float  u_LightNear;        // abs offset = 28, rel offset = 28, size =  4, padded size =  4
		//        float4 u_EnvironmentLight; // abs offset = 32, rel offset = 32, size = 16, padded size = 16
		//        uint   u_PointLightsCount; // abs offset = 48, rel offset = 48, size =  4, padded size =  4
		//        uint   u_SpotLightsCount;  // abs offset = 52, rel offset = 52, size =  4, padded size =  4
		//        uint   u_AOEnabled;        // abs offset = 56, rel offset = 56, size =  4, padded size =  8
		//    } ;
		uint32_t AOEnabled = false;
	};
}