#pragma once

#include "Flare/Renderer/Shader.h"

#include <stdint.h>

#include <string_view>
#include <vector>
#include <optional>

namespace Flare
{
	struct SourcePosition
	{
		SourcePosition()
			: Line(0), Column(0) {}

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

	class ShaderSourceParser
	{
	public:
		ShaderSourceParser(std::string_view shaderSource);

		void Parse();

		inline const std::vector<ShaderSourceBlock>& GetSourceBlocks() const { return m_SourceBlocks; }
		inline const Block& GetBlock(size_t index) const
		{
			FLARE_CORE_ASSERT(index < m_Blocks.size());
			return m_Blocks[index];
		}
	private:
		std::optional<Identifier> ReadIdentifier();
		
		uint32_t ParseBlock();
		void ParseBlockElement(const Identifier& name);

		uint32_t CreateBlock();
		bool SkipUntil(std::string_view token);
		void Advance();
		void SkipWhitespace();
		bool IsReadPositionValid() const { return m_ReadPosition < m_ShaderSource.size(); }
	private:
		size_t m_ReadPosition;
		SourcePosition m_CurrentPosition;
		std::string_view m_ShaderSource;

		uint32_t m_CurrentBlockIndex;
		std::vector<Block> m_Blocks;
		std::vector<ShaderSourceBlock> m_SourceBlocks;
	};
}