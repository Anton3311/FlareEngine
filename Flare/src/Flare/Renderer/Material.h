#pragma once

#include "Flare/AssetManager/Asset.h"
#include "Flare/Renderer/Shader.h"

namespace Flare
{
	class FLARE_API Material : public Asset
	{
	public:
		Material(Ref<Shader> shader);
		Material(AssetHandle shaderHandle);
		virtual ~Material();

		inline AssetHandle GetShaderHandle() const { return m_Shader->Handle; }
		inline Ref<Shader> GetShader() const { return m_Shader; }

		/*void SetInt(uint32_t index, int32_t value);
		void SetInt2(uint32_t index, glm::ivec2 value);
		void SetInt3(uint32_t index, const glm::ivec3& value);
		void SetInt4(uint32_t index, const glm::ivec4& value);

		void SetFloat(uint32_t index, float value);
		void SetFloat2(uint32_t index, glm::vec2 value);
		void SetFloat3(uint32_t index, const glm::vec3& value);
		void SetFloat4(uint32_t index, const glm::vec4& value);
		void SetMatrix4(uint32_t index, const glm::mat4& matrix);*/
		void SetIntArray(uint32_t index, const int32_t* values, uint32_t count);

		template<typename T>
		T ReadParameterValue(uint32_t index)
		{
			const ShaderParameters& parameters = m_Shader->GetParameters();
			FLARE_CORE_ASSERT((size_t)index < parameters.size());
			FLARE_CORE_ASSERT(sizeof(T) == parameters[index].Size);

			T value;

			memcpy_s(&value, sizeof(value), m_Buffer + parameters[index].Offset, parameters[index].Size);
			return value;
		}

		template<typename T>
		void WriteParameterValue(uint32_t index, const T& value)
		{
			const ShaderParameters& parameters = m_Shader->GetParameters();
			FLARE_CORE_ASSERT((size_t)index < parameters.size());
			FLARE_CORE_ASSERT(sizeof(T) == parameters[index].Size);

			memcpy_s(m_Buffer + parameters[index].Offset, sizeof(value), &value, parameters[index].Size);
		}

		void SetShaderParameters();
	private:
		void Initialize();
	private:
		Ref<Shader> m_Shader;

		size_t m_ShaderParametersCount;
		size_t m_BufferSize;
		uint8_t* m_Buffer;
	};
}