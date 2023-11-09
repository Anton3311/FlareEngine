#pragma once

#include "Flare/Renderer/Shader.h"

#include "FlareEditor/ShaderCompiler/ShaderSyntax.h"
#include "FlareEditor/ShaderCompiler/ShaderError.h"

#include <stdint.h>

#include <string_view>
#include <vector>
#include <optional>

namespace Flare
{
	class ShaderSourceParser
	{
	public:
		ShaderSourceParser(const std::filesystem::path& shaderPath,
			std::string_view shaderSource,
			std::vector<ShaderError>& errors);

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
		std::vector<ShaderError>& m_Errors;

		size_t m_ReadPosition;
		SourcePosition m_CurrentPosition;
		std::filesystem::path m_ShaderPath;
		std::string_view m_ShaderSource;

		uint32_t m_CurrentBlockIndex;
		std::vector<Block> m_Blocks;
		std::vector<ShaderSourceBlock> m_SourceBlocks;
	};
}