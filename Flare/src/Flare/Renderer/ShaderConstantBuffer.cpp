#include "ShaderConstantBuffer.h"

namespace Flare
{
	ShaderConstantBuffer::ShaderConstantBuffer(Ref<Shader> shader)
	{
		SetShader(shader);
	}

	ShaderConstantBuffer::~ShaderConstantBuffer()
	{
		Release();
	}

	ShaderConstantBuffer::ShaderConstantBuffer(const ShaderConstantBuffer& other)
	{
		Release();

		m_Shader = other.m_Shader;
		m_BufferSize = other.m_BufferSize;

		if (m_BufferSize > 0)
		{
			m_Buffer = new uint8_t[m_BufferSize];

			std::memcpy(m_Buffer, other.m_Buffer, m_BufferSize);
		}
	}

	ShaderConstantBuffer::ShaderConstantBuffer(ShaderConstantBuffer&& other) noexcept
	{
		Release();

		m_Shader = std::move(other.m_Shader);
		m_BufferSize = other.m_BufferSize;
		m_Buffer = other.m_Buffer;

		other.m_BufferSize = 0;
		other.m_Buffer = nullptr;
	}

	ShaderConstantBuffer& ShaderConstantBuffer::operator=(const ShaderConstantBuffer& other)
	{
		Release();

		m_Shader = other.m_Shader;
		m_BufferSize = other.m_BufferSize;

		if (m_BufferSize > 0)
		{
			m_Buffer = new uint8_t[m_BufferSize];

			std::memcpy(m_Buffer, other.m_Buffer, m_BufferSize);
		}

		return *this;
	}

	ShaderConstantBuffer& ShaderConstantBuffer::operator=(ShaderConstantBuffer&& other) noexcept
	{
		Release();

		m_Shader = std::move(other.m_Shader);
		m_BufferSize = other.m_BufferSize;
		m_Buffer = other.m_Buffer;

		other.m_BufferSize = 0;
		other.m_Buffer = nullptr;

		return *this;
	}

	void ShaderConstantBuffer::SetShader(Ref<const Shader> shader)
	{
		Release();

		m_Shader = shader;

		const ShaderProperties& properties = m_Shader->GetProperties();
		for (const auto& property : properties)
		{
			if (property.Type == ShaderDataType::Sampler)
				continue;

			m_BufferSize = glm::max(property.Offset + property.Size, m_BufferSize);
		}

		if (m_BufferSize > 0)
		{
			m_Buffer = new uint8_t[m_BufferSize];
			std::memset(m_Buffer, 0, m_BufferSize);
		}
	}

	void ShaderConstantBuffer::Release()
	{
		if (m_Buffer != nullptr)
			delete[] m_Buffer;

		m_Buffer = nullptr;
		m_BufferSize = 0;
		m_Shader = nullptr;
	}
}
