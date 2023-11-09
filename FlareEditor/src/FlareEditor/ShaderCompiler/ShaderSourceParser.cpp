#include "ShaderSourceParser.h"

#include "FlareCore/Core.h"
#include "FlareCore/Assert.h"

#include <string_view>
#include <vector>

namespace Flare
{
	ShaderSourceParser::ShaderSourceParser(const std::filesystem::path& shaderPath, std::string_view shaderSource, std::vector<ShaderError>& errors)
		: m_ShaderSource(shaderSource),
		m_ReadPosition(0),
		m_Errors(errors),
		m_ShaderPath(shaderPath),
		m_CurrentBlockIndex(UINT32_MAX) {}

	void ShaderSourceParser::Parse()
	{
		m_CurrentBlockIndex = CreateBlock();

		while (IsReadPositionValid())
		{
			std::optional<Identifier> identifier = ReadIdentifier();
			if (!identifier)
				break;

			if (identifier.value().Value == "#begin")
			{
				std::optional<Identifier> shaderTypeString = ReadIdentifier();
				if (!shaderTypeString)
					break;

				size_t shaderSourceStart = m_ReadPosition;

				std::optional<ShaderStageType> shaderType = {};
				if (shaderTypeString.value().Value == "vertex")
					shaderType = ShaderStageType::Vertex;
				else if (shaderTypeString.value().Value == "pixel")
					shaderType = ShaderStageType::Pixel;
				else
				{
					m_Errors.emplace_back(shaderTypeString->Position,
						fmt::format("Unknown shader type: '{}'", shaderTypeString->Value));
					break;
				}

				std::string_view endToken = "#end";
				if (!SkipUntil(endToken))
				{
					m_Errors.emplace_back(SourcePosition(UINT32_MAX, UINT32_MAX), "Missing '#end' at the end of shader block");
					break;
				}

				if (shaderType)
				{
					std::string_view shaderBlockSource = m_ShaderSource.substr(shaderSourceStart, m_ReadPosition - shaderSourceStart);
					ShaderSourceBlock& shaderBlock = m_SourceBlocks.emplace_back();
					shaderBlock.Stage = *shaderType;
					shaderBlock.Source = shaderBlockSource;
				}

				m_ReadPosition += endToken.size() + 1;
			}
			else
				ParseBlockElement(identifier.value());
		}
	}

	std::optional<Identifier> ShaderSourceParser::ReadIdentifier()
	{
		SkipWhitespace();

		SourcePosition position = m_CurrentPosition;

		size_t start = m_ReadPosition;
		while (IsReadPositionValid())
		{
			if (iswspace(m_ShaderSource[m_ReadPosition]))
				break;

			Advance();
		}

		size_t identifierLength = m_ReadPosition - start;
		if (identifierLength == 0)
			return {};

		std::string_view identifier = m_ShaderSource.substr(start, identifierLength);
		return Identifier(identifier, position);
	}

	uint32_t ShaderSourceParser::ParseBlock()
	{
		if (!IsReadPositionValid() || m_ShaderSource[m_ReadPosition] != '{')
		{
			m_Errors.emplace_back(m_CurrentPosition, "Shader block must start with '{'");
			return UINT32_MAX;
		}

		Advance();

		uint32_t blockIndex = CreateBlock();
		uint32_t previousBlockIndex = m_CurrentBlockIndex;
		m_CurrentBlockIndex = blockIndex;

		while (IsReadPositionValid() && m_ShaderSource[m_ReadPosition] != '}')
		{
			std::optional<Identifier> name = ReadIdentifier();
			if (!name)
				continue;

			ParseBlockElement(name.value());
			SkipWhitespace();
		}

		m_CurrentBlockIndex = previousBlockIndex;

		if (!IsReadPositionValid() || m_ShaderSource[m_ReadPosition] != '}')
		{
			m_Errors.emplace_back(m_CurrentPosition, "Shader block must end with '}'");
			return UINT32_MAX;
		}

		Advance();

		return blockIndex;
	}

	void ShaderSourceParser::ParseBlockElement(const Identifier& name)
	{
		std::optional<Identifier> nextToken = ReadIdentifier();
		if (!nextToken)
			return;

		if (nextToken.value().Value == "=")
		{
			BlockElement& element = m_Blocks[m_CurrentBlockIndex].Elements.emplace_back();
			element.Name = name;
			element.ChildBlockIndex = UINT32_MAX;

			SkipWhitespace();
			if (m_ShaderSource[m_ReadPosition] == '{')
			{
				uint32_t block = ParseBlock();

				element.Type = BlockElementType::Block;
				element.ChildBlockIndex = block;
			}
			else
			{
				std::optional<Identifier> value = ReadIdentifier();
				if (!value)
					return;

				element.Type = BlockElementType::Value;
				element.Value = value.value();
			}
		}
	}

	uint32_t ShaderSourceParser::CreateBlock()
	{
		uint32_t blockIndex = (uint32_t)m_Blocks.size();
		Block& block = m_Blocks.emplace_back();
		block.Index = blockIndex;
		return blockIndex;
	}

	bool ShaderSourceParser::SkipUntil(std::string_view token)
	{
		size_t matchPosition = 0;
		while (IsReadPositionValid())
		{
			if (token[matchPosition] == m_ShaderSource[m_ReadPosition])
			{
				matchPosition++;
				if (matchPosition == token.size())
				{
					m_ReadPosition -= token.size();
					return true;
				}
			}
			else
				matchPosition = 0;

			Advance();
		}

		return false;
	}

	void ShaderSourceParser::Advance()
	{
		if (m_ShaderSource[m_ReadPosition] == '\n')
		{
			m_CurrentPosition.Line++;
			m_CurrentPosition.Column = 1;
		}
		else
			m_CurrentPosition.Column++;
		m_ReadPosition++;
	}

	void ShaderSourceParser::SkipWhitespace()
	{
		while (IsReadPositionValid())
		{
			if (!iswspace(m_ShaderSource[m_ReadPosition]))
				break;

			Advance();
		}
	}
}