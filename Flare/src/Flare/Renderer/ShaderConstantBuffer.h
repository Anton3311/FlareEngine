#pragma once

#include "FlareCore/Core.h"

#include "Flare/Renderer/Shader.h"
#include "Flare/Renderer/ShaderMetadata.h"

namespace Flare
{
	class ComputeShader;
	class Shader;
	class FLARE_API ShaderConstantBuffer
	{
	public:
		ShaderConstantBuffer() = default;
		ShaderConstantBuffer(Ref<const Shader> shader);
		ShaderConstantBuffer(Ref<const ComputeShader> computeShader);
		~ShaderConstantBuffer();

		ShaderConstantBuffer(const ShaderConstantBuffer& other);
		ShaderConstantBuffer(ShaderConstantBuffer&& other) noexcept;

		ShaderConstantBuffer& operator=(const ShaderConstantBuffer& other);
		ShaderConstantBuffer& operator=(ShaderConstantBuffer&& other) noexcept;

		void SetShader(Ref<const Shader> shader);
		void SetShader(Ref<const ComputeShader> computeShader);

		Ref<const ShaderMetadata> GetShaderMetadata() const { return m_ShaderMetadata; }

		template<typename T>
		T& GetProperty(size_t index)
		{
			FLARE_CORE_ASSERT(m_Buffer != nullptr);

			const ShaderProperties& shaderProperties = m_ShaderMetadata->Properties;
			FLARE_CORE_ASSERT(index < shaderProperties.size());
			FLARE_CORE_ASSERT(sizeof(T) == shaderProperties[index].Size);

			return *(T*)(m_Buffer + shaderProperties[index].Offset);
		}

		template<typename T>
		const T& GetProperty(size_t index) const
		{
			FLARE_CORE_ASSERT(m_Buffer != nullptr);

			const ShaderProperties& shaderProperties = m_ShaderMetadata->Properties;
			FLARE_CORE_ASSERT(index < shaderProperties.size());
			FLARE_CORE_ASSERT(sizeof(T) == shaderProperties[index].Size);

			return *(T*)(m_Buffer + shaderProperties[index].Offset);
		}

		template<typename T>
		void SetProperty(size_t index, const T& value) const
		{
			FLARE_CORE_ASSERT(m_Buffer != nullptr);

			const ShaderProperties& shaderProperties = m_ShaderMetadata->Properties;
			FLARE_CORE_ASSERT(index < shaderProperties.size());
			FLARE_CORE_ASSERT(sizeof(T) == shaderProperties[index].Size);

			size_t offset = shaderProperties[index].Offset;
			memcpy_s(m_Buffer + offset, m_BufferSize - offset, &value, sizeof(value));
		}

		uint8_t* GetBuffer() { return m_Buffer; }
		const uint8_t* GetBuffer() const { return m_Buffer; }
		size_t GetBufferSize() const { return m_BufferSize; }

		void Release();
	private:
		void Initialize();
	private:
		Ref<const ShaderMetadata> m_ShaderMetadata = nullptr;

		uint8_t* m_Buffer = nullptr;
		size_t m_BufferSize = 0;
	};
}
