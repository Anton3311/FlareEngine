#include "Components.h"

#include "Flare/AssetManager/AssetManager.h"

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

	glm::mat4 CameraComponent::GetProjection() const
	{
		glm::uvec2 viewportSize = Renderer::GetMainViewport().GetSize();

		float halfSize = Size / 2;
		float aspectRation = (float)viewportSize.x / (float)viewportSize.y;

		if (Projection == CameraComponent::ProjectionType::Orthographic)
			return glm::ortho(-halfSize * aspectRation, halfSize * aspectRation, -halfSize, halfSize, Near, Far);
		else
			return glm::perspective<float>(glm::radians(FOV), aspectRation, Near, Far);
	}

	glm::vec3 CameraComponent::ScreenToWorld(glm::vec2 point) const
	{
		glm::mat4 projection = GetProjection();
		glm::mat4 inverseProjection = glm::inverse(projection);

		point.y = Renderer::GetMainViewport().GetSize().y - point.y;

		point = (point / (glm::vec2)Renderer::GetMainViewport().GetSize()) * 2.0f - glm::vec2(1.0f);
		return inverseProjection * glm::vec4(point, 0.0f, 1.0f);
	}

	glm::vec3 CameraComponent::ViewportToWorld(glm::vec2 point) const
	{
		glm::mat4 projection = GetProjection();
		glm::mat4 inverseProjection = glm::inverse(projection);
		return inverseProjection * glm::vec4(point, 0.0f, 1.0f);
	}

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



	FLARE_IMPL_COMPONENT(MeshComponent);
	MeshComponent::MeshComponent(MeshRenderFlags flags)
		: Mesh(nullptr), Material(NULL_ASSET_HANDLE), Flags(flags) {}

	MeshComponent::MeshComponent(const Ref<Flare::Mesh>& mesh, AssetHandle material, MeshRenderFlags flags)
		: Mesh(mesh), Material(material), Flags(flags) {}



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