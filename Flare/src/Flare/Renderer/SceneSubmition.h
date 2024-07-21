#pragma once

#include "Flare/Renderer/RenderData.h"
#include "Flare/Renderer/RendererSubmitionQueue.h"

#include "Flare/Renderer2D/Renderer2DFrameData.h"
#include "Flare/DebugRenderer/DebugRendererFrameData.h"

#include "Flare/Math/Math.h"

namespace Flare
{
	class Material;

	struct CameraSubmition
	{
		enum class ProjectionType
		{
			Perspective,
			Orthographic,
		};

		ProjectionType Projection = ProjectionType::Perspective;

		float FOVAngle = 0.0f;
		float Size = 0.0f;

		float NearPlane = 0.0f;
		float FarPlane = 0.0f;

		Math::Compact3DTransform Transform;
	};

	struct DirectionalLightSubmition
	{
		glm::vec3 Direction = glm::vec3(0.0f);
		glm::vec3 Color = glm::vec3(0.0f);
		float Intensity = 0.0f;

		Math::Basis LightBasis;
	};

	// TODO: Should replace ShadowMappingSettings in Renderer.h
	struct ShadowMappingSettings
	{
		static constexpr uint32_t MAX_CASCADES = 4;

		uint32_t CascadeCount = 0;

		float ConstantBias = 0.0f;
		float NormalBias = 0.0f;

		float LightSize = 0.0f;
		float Softness = 0.0f;

		float CascadeSplitDistances[MAX_CASCADES - 1] = { 0.0f };
		float MaxShadowDistance = 0.0f;
		float ShadowFadeDistance = 0.0f;
	};

	struct EnvironmentSubmition
	{
		glm::vec3 EnvironmentColor = glm::vec3(0.0f);
		float EnvironmentColorIntensity = 0.0f;
	};
	
	struct PointLightSubmition
	{
		glm::vec3 Position = glm::vec3(0.0f);
		float Padding = 0.0f;
		glm::vec3 Color = glm::vec3(0.0f);
		float Intensity = 0.0f;
	};

	struct SpotLightSubmition
	{
		glm::vec3 Position = glm::vec3(0.0f);
		float InnerAngleCos = 0.0f;
		glm::vec3 Direction = glm::vec3(0.0f);
		float OuterAngleCos = 0.0f;
		glm::vec3 Color = glm::vec3(0.0f);
		float Intensity = 0.0f;
	};

	struct DecalSubmition
	{
		Ref<const Material> Material = nullptr;
		Math::Compact3DTransform Transform;
	};

	struct FLARE_API SceneSubmition
	{
		void Clear();

		CameraSubmition Camera;

		DirectionalLightSubmition DirectionalLight;
		EnvironmentSubmition Environment;

		RendererSubmitionQueue OpaqueGeometrySubmitions;

		std::vector<PointLightSubmition> PointLights;
		std::vector<SpotLightSubmition> SpotLights;

		std::vector<DecalSubmition> DecalSubmitions;

		Renderer2DFrameData Renderer2DSubmition;
		DebugRendererFrameData DebugRendererSubmition;
	};
}
