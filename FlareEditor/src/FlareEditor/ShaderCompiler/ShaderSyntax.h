#pragma once

namespace Flare
{
	struct SourcePosition
	{
		SourcePosition()
			: Line(0), Column(0) {}
		SourcePosition(uint32_t line, uint32_t column)
			: Line(line), Column(column) {}

		uint32_t Line;
		uint32_t Column;
	};

	enum class BlockElementType
	{
		Value,
		Block,
	};

	struct Identifier
	{
		Identifier() = default;
		Identifier(std::string_view value, const SourcePosition& position)
			: Value(value), Position(position) {}

		SourcePosition Position;
		std::string_view Value;
	};

	struct BlockElement
	{
		Identifier Name;
		BlockElementType Type;
		uint32_t ChildBlockIndex;
		Identifier Value;
	};

	struct Block
	{
		uint32_t Index;
		std::vector<BlockElement> Elements;
	};

	struct ShaderSourceBlock
	{
		SourcePosition Position;
		std::string_view Source;
		ShaderStageType Stage;
	};
}