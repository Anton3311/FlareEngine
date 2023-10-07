#pragma once

#include "Flare/AssetManager/Asset.h"

namespace Flare
{
	class FLARE_API Material : public Asset
	{
	public:
		Material(AssetHandle shaderHandle);
		virtual ~Material();

		inline AssetHandle GetShaderHandle() const { return m_ShaderHandle; }

		void SetInt(uint32_t index, int32_t value);
		void SetInt2(uint32_t index, glm::ivec2 value);
		void SetInt3(uint32_t index, const glm::ivec3& value);
		void SetInt4(uint32_t index, const glm::ivec4& value);

		void SetFloat(uint32_t index, float value);
		void SetFloat2(uint32_t index, glm::vec2 value);
		void SetFloat3(uint32_t index, const glm::vec3& value);
		void SetFloat4(uint32_t index, const glm::vec4& value);
		void SetIntArray(uint32_t index, const int32_t* values, uint32_t count);
		void SetMatrix4(uint32_t index, const glm::mat4& matrix);

		template<typename T>
		T ReadParameterValue(uint32_t index)
		{
			FLARE_CORE_ASSERT((size_t)index < m_ShaderParametersCount);

			T value;

			const ShaderParameters& parameters = AssetManager::GetAsset<Shader>(m_ShaderHandle)->GetParameters();
			memcpy_s(&value, sizeof(value), m_Buffer + parameters[index].Offset, parameters[index].Size);

			return value;
		}

		void SetShaderParameters();
	private:
		AssetHandle m_ShaderHandle;

		size_t m_ShaderParametersCount;
		size_t m_BufferSize;
		uint8_t* m_Buffer;
	};
}