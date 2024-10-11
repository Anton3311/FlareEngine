#include "PCH.h"

#include "ShaderConstantBuffer.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/ComputeShader.h"

namespace Flare
{
	ShaderConstantBuffer::ShaderConstantBuffer(Ref<const Shader> shader)
	{
		SetShader(shader);
	}

	ShaderConstantBuffer::ShaderConstantBuffer(Ref<const ComputeShader> computeShader)
	{
		SetShader(computeShader);
	}

	ShaderConstantBuffer::~ShaderConstantBuffer()
	{
		Release();
	}

	ShaderConstantBuffer::ShaderConstantBuffer(const ShaderConstantBuffer& other)
	{
		Release();

		m_ShaderMetadata = other.m_ShaderMetadata;
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

		m_ShaderMetadata = std::move(other.m_ShaderMetadata);
		m_BufferSize = other.m_BufferSize;
		m_Buffer = other.m_Buffer;

		other.m_BufferSize = 0;
		other.m_Buffer = nullptr;
	}

	ShaderConstantBuffer& ShaderConstantBuffer::operator=(const ShaderConstantBuffer& other)
	{
		Release();

		m_ShaderMetadata = other.m_ShaderMetadata;
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

		m_ShaderMetadata = std::move(other.m_ShaderMetadata);
		m_BufferSize = other.m_BufferSize;
		m_Buffer = other.m_Buffer;

		other.m_BufferSize = 0;
		other.m_Buffer = nullptr;

		return *this;
	}

	void ShaderConstantBuffer::SetShader(Ref<const Shader> shader)
	{
		Release();
		m_ShaderMetadata = shader->GetMetadata();

		Initialize();
	}

	void ShaderConstantBuffer::SetShader(Ref<const ComputeShader> computeShader)
	{
		Release();
		m_ShaderMetadata = computeShader->GetMetadata();

		Initialize();
	}

	void ShaderConstantBuffer::Release()
	{
		FLARE_PROFILE_FUNCTION();
		if (m_Buffer != nullptr)
			delete[] m_Buffer;

		m_Buffer = nullptr;
		m_BufferSize = 0;
		m_ShaderMetadata = nullptr;
	}

	void ShaderConstantBuffer::Initialize()
	{
		FLARE_PROFILE_FUNCTION();

		for (const auto& range : m_ShaderMetadata->PushConstantsRanges)
			m_BufferSize = glm::max(range.Offset + range.Size, m_BufferSize);

		if (m_BufferSize > 0)
		{
			m_Buffer = new uint8_t[m_BufferSize];
			std::memset(m_Buffer, 0, m_BufferSize);
		}
	}
}
