#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <typeinfo>
#include <string_view>
#include <cstddef>

namespace Flare::Internal
{
	enum class FieldType
	{
		None,

		Int32,
		UInt32,

		Float,
		Float2,
		Float3,
		Float4,

		Color4,
	};

	struct DataTypeToFiedType
	{
		template<typename T>
		static constexpr FieldType Get() { return FieldType::None; }
	};

#define DATA_TYPE_TO_FIELD_TYPE_CONVERTER(dataType)  \
	template<>                                       \
	FieldType DataTypeToFiedType::Get<dataType>();

	DATA_TYPE_TO_FIELD_TYPE_CONVERTER(int32_t);
	DATA_TYPE_TO_FIELD_TYPE_CONVERTER(uint32_t);
	DATA_TYPE_TO_FIELD_TYPE_CONVERTER(float);
	DATA_TYPE_TO_FIELD_TYPE_CONVERTER(glm::vec2);
	DATA_TYPE_TO_FIELD_TYPE_CONVERTER(glm::vec3);
	DATA_TYPE_TO_FIELD_TYPE_CONVERTER(glm::vec4);

#undef DATA_TYPE_TO_FIELD_TYPE_CONVERTER

	struct Field
	{
		Field(const std::string& name, size_t offset, FieldType type)
			: Name(name), Offset(offset), Type(type) {}

		std::string Name;
		size_t Offset;
		FieldType Type;
	};

	class TypeSerializationSettings
	{
	public:
		template<typename T>
		void AddField(std::string_view fieldName, size_t fieldOffset)
		{
			FieldType type = DataTypeToFiedType::Get<T>();
			if (type != FieldType::None)
				m_Fields.emplace_back(std::string(fieldName), fieldOffset, type);
		}

		const std::vector<Field>& GetFields() const { return m_Fields; }
	private:
		std::vector<Field> m_Fields;
	};
}

#ifndef FLARE_SCRIPTING_CORE_NO_MACROS
	#define FLARE_GET_FIELD_TYPE(typeName, fieldName) decltype(((typeName*) nullptr)->fieldName)
	#define FLARE_SERIALIZE_FIELD(settings, typeName, fieldName) settings.AddField<FLARE_GET_FIELD_TYPE(typeName, fieldName)> \
		(#fieldName, offsetof(typeName, fieldName));
#endif