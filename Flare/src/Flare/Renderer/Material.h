#pragma once

#include "FlareCore/Serialization/TypeInitializer.h"

#include "Flare/AssetManager/Asset.h"
#include "Flare/Renderer/Shader.h"
#include "Flare/Renderer/RendererAPI.h"
#include "Flare/Renderer/ShaderMetadata.h"
#include "Flare/Renderer/FrameBuffer.h"

namespace Flare
{
	struct FLARE_API TexturePropertyValue
	{
		enum class Type : uint8_t
		{
			Texture,
			FrameBufferAttachment,
		};

		TexturePropertyValue()
		{
			Clear();
		}

		void SetTexture(const Ref<Texture>& texture);
		void SetFrameBuffer(const Ref<FrameBuffer>& frameBuffer, uint32_t attachment);
		void Clear();

		Type ValueType = Type::Texture;

		Ref<Texture> Texture = nullptr;
		Ref<FrameBuffer> FrameBuffer = nullptr;
		uint32_t FrameBufferAttachmentIndex = UINT32_MAX;
	};

	class FLARE_API Material : public Asset
	{
	public:
		FLARE_ASSET;
		FLARE_TYPE;

		Material();
		Material(Ref<Shader> shader);
		Material(AssetHandle shaderHandle);
		virtual ~Material();

		inline Ref<Shader> GetShader() const { return m_Shader; }
		virtual void SetShader(const Ref<Shader>& shader);

		void SetIntArray(uint32_t index, const int32_t* values, uint32_t count);

		template<typename T>
		T& GetPropertyValue(uint32_t index)
		{
			const ShaderProperties& properties = m_Shader->GetProperties();
			FLARE_CORE_ASSERT((size_t)index < properties.size());
			FLARE_CORE_ASSERT(sizeof(T) == properties[index].Size);
			FLARE_CORE_ASSERT(properties[index].Type != ShaderDataType::Sampler);
			FLARE_CORE_ASSERT(properties[index].Offset + properties[index].Size <= m_BufferSize);
			return *(T*)(m_Buffer + properties[index].Offset);
		}

		template<typename T>
		T ReadPropertyValue(uint32_t index) const
		{
			const ShaderProperties& properties = m_Shader->GetProperties();
			FLARE_CORE_ASSERT((size_t)index < properties.size());
			FLARE_CORE_ASSERT(sizeof(T) == properties[index].Size);

			FLARE_CORE_ASSERT(properties[index].Type != ShaderDataType::Sampler);
			FLARE_CORE_ASSERT(properties[index].Offset + properties[index].Size <= m_BufferSize);

			T value;

			memcpy_s(&value, sizeof(value), m_Buffer + properties[index].Offset, properties[index].Size);
			return value;
		}

		template<typename T>
		void WritePropertyValue(uint32_t index, const T& value)
		{
			const ShaderProperties& properties = m_Shader->GetProperties();
			FLARE_CORE_ASSERT((size_t)index < properties.size());
			FLARE_CORE_ASSERT(sizeof(T) == properties[index].Size);
			FLARE_CORE_ASSERT(properties[index].Offset + properties[index].Size <= m_BufferSize);

			FLARE_CORE_ASSERT(properties[index].Type != ShaderDataType::Sampler);
			memcpy_s(m_Buffer + properties[index].Offset, sizeof(value), &value, properties[index].Size);
		}

		inline uint8_t* GetPropertiesBuffer() { return m_Buffer; }
		inline const uint8_t* GetPropertiesBuffer() const { return m_Buffer; }
	public:
		static Ref<Material> Create();
		static Ref<Material> Create(Ref<Shader> shader);
		static Ref<Material> Create(AssetHandle shaderHandle);
	private:
		void Initialize();
	protected:
		Ref<Shader> m_Shader;

		std::vector<TexturePropertyValue> m_Textures;

		size_t m_BufferSize;
		uint8_t* m_Buffer;
	};

	template<>
	FLARE_API TexturePropertyValue& Material::GetPropertyValue(uint32_t index);

	template<>
	FLARE_API TexturePropertyValue Material::ReadPropertyValue(uint32_t index) const;

	template<>
	FLARE_API void Material::WritePropertyValue(uint32_t index, const TexturePropertyValue& value);
}