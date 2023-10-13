#include "Material.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/Shader.h"
#include "Flare/Renderer/RendererAPI.h"

#include "Flare/Platform/OpenGL/OpenGLShader.h"

namespace Flare
{
	Material::Material(AssetHandle shaderHandle)
		: Asset(AssetType::Material),
		m_ShaderHandle(shaderHandle),
		m_Buffer(nullptr),
		m_BufferSize(0),
		m_ShaderParametersCount(0)
	{
		FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(shaderHandle));
		Ref<Shader> shader = AssetManager::GetAsset<Shader>(shaderHandle);

		const ShaderParameters& parameters = shader->GetParameters();
		m_ShaderParametersCount = parameters.size();

		for (const auto& param : parameters)
			m_BufferSize += param.Offset;

		if (parameters.size() > 0)
			m_BufferSize += parameters.back().Size;

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

	void Material::SetInt(uint32_t index, int32_t value)
	{
		FLARE_CORE_ASSERT((size_t)index < m_ShaderParametersCount);
		const ShaderParameters& parameters = AssetManager::GetAsset<Shader>(m_ShaderHandle)->GetParameters();
		memcpy_s(m_Buffer + parameters[index].Offset, parameters[index].Size, &value, sizeof(value));
	}

	void Material::SetInt2(uint32_t index, glm::ivec2 value)
	{
		FLARE_CORE_ASSERT((size_t)index < m_ShaderParametersCount);
		const ShaderParameters& parameters = AssetManager::GetAsset<Shader>(m_ShaderHandle)->GetParameters();
		memcpy_s(m_Buffer + parameters[index].Offset, parameters[index].Size, &value, sizeof(value));
	}

	void Material::SetInt3(uint32_t index, const glm::ivec3& value)
	{
		FLARE_CORE_ASSERT((size_t)index < m_ShaderParametersCount);
		const ShaderParameters& parameters = AssetManager::GetAsset<Shader>(m_ShaderHandle)->GetParameters();
		memcpy_s(m_Buffer + parameters[index].Offset, parameters[index].Size, &value, sizeof(value));
	}

	void Material::SetInt4(uint32_t index, const glm::ivec4& value)
	{
		FLARE_CORE_ASSERT((size_t)index < m_ShaderParametersCount);
		const ShaderParameters& parameters = AssetManager::GetAsset<Shader>(m_ShaderHandle)->GetParameters();
		memcpy_s(m_Buffer + parameters[index].Offset, parameters[index].Size, &value, sizeof(value));
	}

	void Material::SetFloat(uint32_t index, float value)
	{
		FLARE_CORE_ASSERT((size_t)index < m_ShaderParametersCount);
		const ShaderParameters& parameters = AssetManager::GetAsset<Shader>(m_ShaderHandle)->GetParameters();
		memcpy_s(m_Buffer + parameters[index].Offset, parameters[index].Size, &value, sizeof(value));
	}

	void Material::SetFloat2(uint32_t index, glm::vec2 value)
	{
		FLARE_CORE_ASSERT((size_t)index < m_ShaderParametersCount);
		const ShaderParameters& parameters = AssetManager::GetAsset<Shader>(m_ShaderHandle)->GetParameters();
		memcpy_s(m_Buffer + parameters[index].Offset, parameters[index].Size, &value, sizeof(value));
	}

	void Material::SetFloat3(uint32_t index, const glm::vec3& value)
	{
		FLARE_CORE_ASSERT((size_t)index < m_ShaderParametersCount);
		const ShaderParameters& parameters = AssetManager::GetAsset<Shader>(m_ShaderHandle)->GetParameters();
		memcpy_s(m_Buffer + parameters[index].Offset, parameters[index].Size, &value, sizeof(value));
	}

	void Material::SetFloat4(uint32_t index, const glm::vec4& value)
	{
		FLARE_CORE_ASSERT((size_t)index < m_ShaderParametersCount);
		const ShaderParameters& parameters = AssetManager::GetAsset<Shader>(m_ShaderHandle)->GetParameters();
		memcpy_s(m_Buffer + parameters[index].Offset, parameters[index].Size, &value, sizeof(value));
	}

	void Material::SetIntArray(uint32_t index, const int32_t* values, uint32_t count)
	{
		FLARE_CORE_ASSERT((size_t)index < m_ShaderParametersCount);
		const ShaderParameters& parameters = AssetManager::GetAsset<Shader>(m_ShaderHandle)->GetParameters();
		memcpy_s(m_Buffer + parameters[index].Offset, parameters[index].Size, values, sizeof(*values) * count);
	}

	void Material::SetMatrix4(uint32_t index, const glm::mat4& matrix)
	{
		FLARE_CORE_ASSERT((size_t)index < m_ShaderParametersCount);
	}

	void Material::SetShaderParameters()
	{
		FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(m_ShaderHandle));
		Ref<Shader> shader = AssetManager::GetAsset<Shader>(m_ShaderHandle);

		const ShaderParameters& parameters = shader->GetParameters();

		if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
		{
			Ref<OpenGLShader> glShader = As<OpenGLShader>(shader);
			glShader->Bind();

			if (m_ShaderParametersCount == 0)
				return;
			
			FLARE_CORE_ASSERT(m_Buffer);
			for (uint32_t i = 0; i < (uint32_t)parameters.size(); i++)
			{
				uint8_t* paramData = m_Buffer + parameters[i].Offset;
				
				switch (parameters[i].Type)
				{
				case ShaderDataType::Int:
				case ShaderDataType::Sampler:
					glShader->SetInt(i, *(int32_t*)paramData);
					break;

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
					uint32_t arraySize = (uint32_t)parameters[i].Size / ShaderDataTypeSize(ShaderDataType::Sampler);
					glShader->SetIntArray(i, (int32_t*)paramData, arraySize);
					break;
				}
				}
			}
		}
	}
}
