#include "Material.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/Shader.h"
#include "Flare/Renderer/Texture.h"
#include "Flare/Renderer/RendererAPI.h"
#include "Flare/Renderer/Renderer.h"

#include "Flare/Platform/OpenGL/OpenGLShader.h"

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

	void Material::Initialize()
	{
		if (m_Shader == nullptr)
			return;

		const ShaderProperties& properties = m_Shader->GetProperties();
		size_t samplers = 0;

		for (const auto& prop : properties)
		{
			m_BufferSize += prop.Size;

			if (prop.Type == ShaderDataType::Sampler)
				samplers++;
		}

		m_Textures.resize(samplers, TexturePropertyValue());

		if (properties.size() > 0)
			m_BufferSize += properties.back().Size;

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

	void Material::SetShaderProperties() const
	{
		if (!m_Shader)
			return;

		const ShaderProperties& properties = m_Shader->GetProperties();

		if (!m_Shader->IsLoaded())
			return;

		if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
		{
			Ref<OpenGLShader> glShader = As<OpenGLShader>(m_Shader);
			glShader->Bind();

			if (properties.size() == 0)
				return;
			
			FLARE_CORE_ASSERT(m_Buffer);
			for (uint32_t i = 0; i < (uint32_t)properties.size(); i++)
			{
				uint8_t* paramData = m_Buffer + properties[i].Offset;
				
				switch (properties[i].Type)
				{
				case ShaderDataType::Int:
					glShader->SetInt(i, *(int32_t*)paramData);
					break;
				case ShaderDataType::Sampler:
				{
					const auto& textureValue = m_Textures[properties[i].SamplerIndex];

					switch (textureValue.ValueType)
					{
					case TexturePropertyValue::Type::FrameBufferAttachment:
						FLARE_CORE_ASSERT(textureValue.FrameBuffer);
						textureValue.FrameBuffer->BindAttachmentTexture(textureValue.FrameBufferAttachmentIndex, properties[i].Location);
						break;
					case TexturePropertyValue::Type::Texture:
						if (textureValue.Texture)
							textureValue.Texture->Bind(properties[i].Location);
						break;
					default:
						FLARE_CORE_ASSERT(false);
					}

					break;
				}

				case ShaderDataType::Int2:
					glShader->SetInt2(i, *(glm::ivec2*)paramData);
					break;
				case ShaderDataType::Int3:
					glShader->SetInt3(i, *(glm::ivec3*)paramData);
					break;
				case ShaderDataType::Int4:
					glShader->SetInt4(i, *(glm::ivec4*)paramData);
					break;

				case ShaderDataType::Float:
					glShader->SetFloat(i, *(float*)paramData);
					break;
				case ShaderDataType::Float2:
					glShader->SetFloat2(i, *(glm::vec2*)paramData);
					break;
				case ShaderDataType::Float3:
					glShader->SetFloat3(i, *(glm::vec3*)paramData);
					break;
				case ShaderDataType::Float4:
					glShader->SetFloat4(i, *(glm::vec4*)paramData);
					break;

				case ShaderDataType::Matrix4x4:
					glShader->SetMatrix4(i, *(glm::mat4*)paramData);
					break;
				case ShaderDataType::SamplerArray:
				{
					uint32_t arraySize = (uint32_t)properties[i].Size / ShaderDataTypeSize(ShaderDataType::Sampler);
					glShader->SetIntArray(i, (int32_t*)paramData, arraySize);
					break;
				}
				}
			}
		}
	}

	template<>
	FLARE_API TexturePropertyValue& Material::GetPropertyValue(uint32_t index)
	{
		const ShaderProperties& properties = m_Shader->GetProperties();
		FLARE_CORE_ASSERT((size_t)index < properties.size());
		FLARE_CORE_ASSERT(properties[index].Type == ShaderDataType::Sampler);
		FLARE_CORE_ASSERT(properties[index].SamplerIndex < (uint32_t)m_Textures.size());

		return m_Textures[properties[index].SamplerIndex];
	}

	template<>
	FLARE_API TexturePropertyValue Material::ReadPropertyValue(uint32_t index)
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

		m_Textures[properties[index].SamplerIndex] = value;
	}
}
