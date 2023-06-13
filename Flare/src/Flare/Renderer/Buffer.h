#pragma once

#include "Flare/Core/Core.h"

#include <stdint.h>
#include <string>
#include <string_view>
#include <vector>

namespace Flare
{
	enum class ShaderDataType
	{
		Int,
		Float,
		Float2,
		Float3,
		Float4,
	};

	constexpr size_t ShaderDataTypeSize(ShaderDataType dataType)
	{
		switch (dataType)
		{
		case ShaderDataType::Int:
		case ShaderDataType::Float:
			return 4;
		case ShaderDataType::Float2:
			return 4 * 2;
		case ShaderDataType::Float3:
			return 4 * 3;
		case ShaderDataType::Float4:
			return 4 * 4;
		}

		return 0;
	}

	constexpr uint32_t ShaderDataTypeComponentCount(ShaderDataType dataType)
	{
		switch (dataType)
		{
		case ShaderDataType::Int:
		case ShaderDataType::Float:
			return 1;
		case ShaderDataType::Float2:
			return 2;
		case ShaderDataType::Float3:
			return 3;
		case ShaderDataType::Float4:
			return 4;
		}

		return 0;
	}

	struct BufferLayoutElement
	{
		BufferLayoutElement() = default;
		BufferLayoutElement(std::string_view name, ShaderDataType type, bool normalized = false)
			: Name(name), DataType(type), Size(ShaderDataTypeSize(type)), 
			ComponentsCount(ShaderDataTypeComponentCount(type)), IsNormalized(normalized), Offset(0) {}

		std::string Name;
		ShaderDataType DataType;
		uint32_t Size;
		uint32_t Offset;
		uint32_t ComponentsCount;
		bool IsNormalized;
	};

	class BufferLayout
	{
	public:
		BufferLayout() = default;
		BufferLayout(const std::initializer_list<BufferLayoutElement>& elements)
			: m_Elements(elements), m_Stride(0)
		{
			CalculateOffsetsAndStrides();
		}

		inline const std::vector<BufferLayoutElement> GetElements() const { return m_Elements; }
		inline uint32_t GetStride() const { return m_Stride; }
	private:
		void CalculateOffsetsAndStrides()
		{
			for (auto& elem : m_Elements)
			{
				elem.Offset = m_Stride;
				m_Stride += elem.Size;
			}
		}
	private:
		uint32_t m_Stride;
		std::vector<BufferLayoutElement> m_Elements;
	};

	class VertexBuffer
	{
	public:
		virtual const BufferLayout& GetLayout() const = 0;
		virtual void SetLayout(const BufferLayout& layout) = 0;

		virtual void Bind() = 0;
		virtual void SetData(const void* data, size_t size) = 0;
	public:
		static Ref<VertexBuffer> Create(size_t size);
		static Ref<VertexBuffer> Create(size_t size, const void* data);
	};

	class IndexBuffer
	{
	public:
		virtual void Bind() = 0;
		virtual void SetData(const void* indices, size_t count) = 0;
		
		virtual size_t GetCount() const = 0;
	public:
		static Ref<IndexBuffer> Create(size_t count);
		static Ref<IndexBuffer> Create(size_t count, const void* data);
	};
}