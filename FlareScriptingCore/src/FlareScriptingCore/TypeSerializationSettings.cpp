#include "TypeSerializationSettings.h"

#define DATA_TYPE_TO_FIELD_TYPE_CONVERTER_IMPL(dataType, fieldType)                    \
	template<>                                                                         \
	FieldType DataTypeToFiedType::Get<dataType>() { return FieldType::fieldType; };

namespace Flare::Internal
{
	DATA_TYPE_TO_FIELD_TYPE_CONVERTER_IMPL(int32_t, Int32);
	DATA_TYPE_TO_FIELD_TYPE_CONVERTER_IMPL(uint32_t, UInt32);
	DATA_TYPE_TO_FIELD_TYPE_CONVERTER_IMPL(float, Float);
	DATA_TYPE_TO_FIELD_TYPE_CONVERTER_IMPL(glm::vec2, Float2);
	DATA_TYPE_TO_FIELD_TYPE_CONVERTER_IMPL(glm::vec3, Float3);
	DATA_TYPE_TO_FIELD_TYPE_CONVERTER_IMPL(glm::vec4, Float4);
}

#undef DATA_TYPE_TO_FIELD_TYPE_CONVERTER