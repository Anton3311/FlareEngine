#include "Material.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/Shader.h"
#include "Flare/Renderer/Texture.h"
#include "Flare/Renderer/RendererAPI.h"
#include "Flare/Renderer/Renderer.h"

#include "Flare/Platform/Vulkan/VulkanMaterial.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Flare
{
	void TexturePropertyValue::SetTexture(const Ref<Flare::Texture>& texture)
	{
		ValueType = Type::Texture;
		Texture = texture;
		FrameBuffer = nullptr;
		FrameBufferAttachmentIndex = UINT32_MAX;
	}

	void TexturePropertyValue::SetFrameBuffer(const Ref<Flare::FrameBuffer>& frameBuffer, uint32_t attachment)
	{
		FLARE_CORE_ASSERT(frameBuffer);
		FLARE_CORE_ASSERT(attachment < frameBuffer->GetAttachmentsCount());

		ValueType = Type::FrameBufferAttachment;
		Texture = nullptr;
		FrameBuffer = frameBuffer;
		FrameBufferAttachmentIndex = attachment;
	}

	void TexturePropertyValue::Clear()
	{
		ValueType = Type::Texture;
		Texture = nullptr;
		FrameBuffer = nullptr;
		FrameBufferAttachmentIndex = UINT32_MAX;
	}

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
		case RendererAPI::API::OpenGL:
			return CreateRef<Material>();
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
		case RendererAPI::API::OpenGL:
			material = CreateRef<Material>();
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
		case RendererAPI::API::OpenGL:
			material = CreateRef<Material>();
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
				samplers++;
		}

		m_Textures.resize(samplers, TexturePropertyValue());

		if (properties.size() > 0)
			m_BufferSize = properties.back().Offset + properties.back().Size;

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

	template<>
	FLARE_API TexturePropertyValue& Material::GetPropertyValue(uint32_t index)
	{
		const ShaderProperties& properties = m_Shader->GetProperties();
		FLARE_CORE_ASSERT((size_t)index < properties.size());
		FLARE_CORE_ASSERT(properties[index].Type == ShaderDataType::Sampler);
		FLARE_CORE_ASSERT(properties[index].SamplerIndex < (uint32_t)m_Textures.size());

		m_IsDirty = true;
		return m_Textures[properties[index].SamplerIndex];
	}

	template<>
	FLARE_API TexturePropertyValue Material::ReadPropertyValue(uint32_t index) const
	{
		const ShaderProperties& properties = m_Shader->GetProperties();
		FLARE_CORE_ASSERT((size_t)index < properties.size());
		FLARE_CORE_ASSERT(properties[index].Type == ShaderDataType::Sampler);
		FLARE_CORE_ASSERT(properties[index].SamplerIndex < (uint32_t)m_Textures.size());

		return m_Textures[properties[index].SamplerIndex];
	}

	template<>
	FLARE_API void Material::WritePropertyValue(uint32_t index, const TexturePropertyValue& value)
	{
		const ShaderProperties& properties = m_Shader->GetProperties();
		FLARE_CORE_ASSERT((size_t)index < properties.size());
		FLARE_CORE_ASSERT(properties[index].Type == ShaderDataType::Sampler);
		FLARE_CORE_ASSERT(properties[index].SamplerIndex < (uint32_t)m_Textures.size());

		m_IsDirty = true;
		m_Textures[properties[index].SamplerIndex] = value;
	}
}
