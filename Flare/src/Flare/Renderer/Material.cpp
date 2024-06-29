#include "Material.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/Shader.h"
#include "Flare/Renderer/Texture.h"
#include "Flare/Renderer/RendererAPI.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/Texture.h"

#include "Flare/Platform/Vulkan/VulkanMaterial.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Flare
{
	FLARE_IMPL_ASSET(Material);
	FLARE_IMPL_TYPE(Material);

	Material::Material()
		: Asset(AssetType::Material), m_Shader(nullptr), m_Buffer(nullptr), m_BufferSize(0) {}

	Material::Material(Ref<Shader> shader)
		: Asset(AssetType::Material),
		m_Shader(shader),
		m_Buffer(nullptr),
		m_BufferSize(0)
	{
		FLARE_CORE_ASSERT(shader);
		Initialize();
	}

	Material::Material(AssetHandle shaderHandle)
		: Asset(AssetType::Material),
		m_Shader(nullptr),
		m_Buffer(nullptr),
		m_BufferSize(0)
	{
		FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(shaderHandle));
		m_Shader = AssetManager::GetAsset<Shader>(shaderHandle);
		
		Initialize();
	}

	Ref<Material> Material::Create()
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanMaterial>();
		}

		FLARE_CORE_ASSERT(false);
		return nullptr;
	}

	Ref<Material> Material::Create(Ref<Shader> shader)
	{
		Ref<Material> material = nullptr;
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			material = CreateRef<VulkanMaterial>();
			break;
		}

		FLARE_CORE_ASSERT(material);
		
		material->SetShader(shader);

		return material;
	}

	Ref<Material> Material::Create(AssetHandle shaderHandle)
	{
		Ref<Material> material = nullptr;
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			material = CreateRef<VulkanMaterial>();
			break;
		}

		FLARE_CORE_ASSERT(material);
		
		FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(shaderHandle));
		material->SetShader(AssetManager::GetAsset<Shader>(shaderHandle));

		return material;
	}

	void Material::Initialize()
	{
		if (m_Shader == nullptr)
			return;

		const ShaderProperties& properties = m_Shader->GetProperties();
		size_t samplers = 0;

		for (const auto& prop : properties)
		{
			if (prop.Type == ShaderDataType::Sampler)
			{
				samplers++;
			}
			else
			{
				m_BufferSize = glm::max(prop.Offset + prop.Size, m_BufferSize);
			}
		}

		m_Textures.resize(samplers, nullptr);

		if (m_BufferSize != 0)
		{
			m_Buffer = new uint8_t[m_BufferSize];
			std::memset(m_Buffer, 0, m_BufferSize);
		}
	}

	Material::~Material()
	{
		if (m_Buffer != nullptr)
			delete[] m_Buffer;
	}

	void Material::SetShader(const Ref<Shader>& shader)
	{
		if (m_Buffer != nullptr)
		{
			delete[] m_Buffer;
			m_Buffer = nullptr;
			m_BufferSize = 0;
		}

		m_Textures.clear();
		m_Shader = shader;
		Initialize();
	}

	void Material::SetIntArray(uint32_t index, const int32_t* values, uint32_t count)
	{
		const ShaderProperties& properties = m_Shader->GetProperties();
		FLARE_CORE_ASSERT((size_t)index < properties.size());
		memcpy_s(m_Buffer + properties[index].Offset, properties[index].Size, values, sizeof(*values) * count);
	}
	
	const Ref<Texture>& Material::GetTextureProperty(uint32_t propertyIndex) const
	{
		const ShaderProperties& properties = m_Shader->GetProperties();
		FLARE_CORE_ASSERT((size_t)propertyIndex < properties.size());
		FLARE_CORE_ASSERT(properties[propertyIndex].Type == ShaderDataType::Sampler);
		
		return m_Textures[properties[propertyIndex].SamplerIndex];
	}

	void Material::SetTextureProperty(uint32_t propertyIndex, Ref<Texture> texture)
	{
		const ShaderProperties& properties = m_Shader->GetProperties();
		FLARE_CORE_ASSERT((size_t)propertyIndex < properties.size());
		FLARE_CORE_ASSERT(properties[propertyIndex].Type == ShaderDataType::Sampler);
		
		const Ref<Texture>& oldTexture = m_Textures[properties[propertyIndex].SamplerIndex];
		if (oldTexture.get() == texture.get())
			return;

		m_Textures[properties[propertyIndex].SamplerIndex] = texture;
		m_IsDirty = true;
	}
}
