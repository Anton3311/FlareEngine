#include "PCH.h"

#include "Components.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/Renderer.h"

namespace Flare
{
	FLARE_IMPL_COMPONENT(NameComponent);
	NameComponent::NameComponent() {}

	NameComponent::NameComponent(std::string_view name)
		: Value(name) {}

	NameComponent::~NameComponent()
	{
	}



	FLARE_IMPL_COMPONENT(CameraComponent);
	CameraComponent::CameraComponent()
		: Projection(ProjectionType::Perspective),
		  Size(10.0f),
		  FOV(60.0f),
		  Near(0.1f),
		  Far(1000.0f) {}

	CameraComponent::CameraComponent(ProjectionType projection)
		: Projection(projection),
		  Size(10.0f),
		  FOV(60.0f),
		  Near(0.1f),
		  Far(1000.0f) {}

	glm::mat4 CameraComponent::GetProjection(glm::uvec2 viewportSize) const
	{
		float viewportAspectRatio = static_cast<float>(viewportSize.x) / static_cast<float>(viewportSize.y);
		if (Projection == ProjectionType::Orthographic)
		{
			float halfSize = Size / 2.0f;
			return glm::orthoRH_ZO(
				-halfSize * viewportAspectRatio,
				+halfSize * viewportAspectRatio,
				-halfSize,
				+halfSize,
				Near,
				Far);
		}
		else
		{
			return glm::perspectiveRH_ZO(
				glm::radians(FOV),
				viewportAspectRatio,
				Near,
				Far);
		}
	}

	FLARE_IMPL_COMPONENT(CameraOutput);

	FLARE_IMPL_COMPONENT(SpriteComponent);
	SpriteComponent::SpriteComponent()
		: Color(glm::vec4(1.0f)),
		  Tilling(glm::vec2(1.0f)),
		  Sprite(nullptr),
		  Flags(SpriteRenderFlags::None) {}

	SpriteComponent::SpriteComponent(AssetHandle sprite)
		: Color(glm::vec4(1.0f)),
		Tilling(glm::vec2(1.0f)),
		Sprite(AssetManager::GetAsset<Flare::Sprite>(sprite)),
		Flags(SpriteRenderFlags::None) {}

	SpriteComponent::SpriteComponent(const Ref<Flare::Sprite>& sprite)
		: Sprite(sprite), Color(1.0f), Tilling(1.0f), Flags(SpriteRenderFlags::None) {}

	FLARE_IMPL_COMPONENT(SpriteLayer);
	SpriteLayer::SpriteLayer()
		: Layer(0) {}

	SpriteLayer::SpriteLayer(int32_t layer)
		: Layer(layer) {}

	FLARE_IMPL_COMPONENT(MaterialComponent);
	MaterialComponent::MaterialComponent()
		: Material(NULL_ASSET_HANDLE) {}

	MaterialComponent::MaterialComponent(AssetHandle handle)
		: Material(handle) {}



	FLARE_IMPL_COMPONENT(TextComponent);
	TextComponent::TextComponent()
		: Color(1.0f), Font(nullptr) {}

	TextComponent::TextComponent(std::string_view text, const glm::vec4& color, const Ref<Flare::Font>& font)
		: Text(text), Color(color), Font(font) {}



	FLARE_IMPL_COMPONENT(MeshRenderer);
	FLARE_IMPL_COMPONENT(Decal);



	FLARE_IMPL_COMPONENT(DirectionalLight);
	DirectionalLight::DirectionalLight()
		: Color(1.0f), Intensity(1.0f) {}

	DirectionalLight::DirectionalLight(const glm::vec3& color, float intensity)
		: Color(color), Intensity(intensity) {}



	FLARE_IMPL_COMPONENT(PointLight);
	FLARE_IMPL_COMPONENT(SpotLight);


	FLARE_IMPL_COMPONENT(Environment);
	Environment::Environment()
		: EnvironmentColor(0.0f), EnvironmentColorIntensity(0.0f) {}

	Environment::Environment(glm::vec3 color, float intensity)
		: EnvironmentColor(color), EnvironmentColorIntensity(intensity) {}
}