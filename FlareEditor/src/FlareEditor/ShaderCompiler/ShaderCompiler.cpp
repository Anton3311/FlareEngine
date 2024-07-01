#include "ShaderCompiler.h"

#include "Flare/Renderer/ComputeShader.h"

#include "Flare/AssetManager/AssetManager.h"
#include "FlareEditor/AssetManager/EditorShaderCache.h"
#include "FlareEditor/ShaderCompiler/ShaderSourceParser.h"

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

#include <fstream>
#include <string>
#include <sstream>

namespace Flare
{
	struct PreprocessedShaderProgram
	{
		std::string Source;
		ShaderStageType Stage;
	};

	class ShaderIncluder : public shaderc::CompileOptions::IncluderInterface
	{
	public:
		virtual shaderc_include_result* GetInclude(const char* requested_source,
			shaderc_include_type type,
			const char* requesting_source,
			size_t include_depth)
		{
			std::filesystem::path requestingPath = requesting_source;
			std::filesystem::path parent = requestingPath.parent_path();
			std::filesystem::path includedFilePath = parent / requested_source;

			if (std::string_view(requested_source)._Starts_with("Packages"))
			{
				includedFilePath = std::filesystem::absolute(std::filesystem::path("../") / requested_source);
			}

			shaderc_include_result* includeData = new shaderc_include_result();
			includeData->content = nullptr;
			includeData->content_length = 0;
			includeData->source_name = nullptr;
			includeData->source_name_length = 0;
			includeData->user_data = nullptr;

			std::ifstream inputStream(includedFilePath);

			std::string_view path;
			if (inputStream.is_open())
			{
				std::string pathString = includedFilePath.string();
				path = pathString;

				char* sourceName = new char[path.size() + 1];
				sourceName[path.size()] = 0;

				memcpy_s(sourceName, path.size() + 1, pathString.c_str(), pathString.size());

				// NOTE: Include result owns the shader path string
				includeData->source_name = sourceName;
				includeData->source_name_length = pathString.size();
			}

			if (inputStream.is_open())
			{
				inputStream.seekg(0, std::ios::end);
				size_t size = inputStream.tellg();
				char* source = new char[size + 1];
				std::memset(source, 0, size + 1);

				inputStream.seekg(0, std::ios::beg);
				inputStream.read(source, size);

				includeData->content = source;
				includeData->content_length = strlen(source);
			}
			else
			{
				std::string_view message = "File not found.";

				includeData->content = message.data();
				includeData->content_length = message.size();
			}

			return includeData;
		}

		virtual void ReleaseInclude(shaderc_include_result* data)
		{
			if (data->source_name)
			{
				delete data->source_name;
				delete data->content;
			}

			delete data;
		}
	};

	static std::optional<std::vector<uint32_t>> CompileVulkanGlslToSpirv(const std::string& path, const std::string& source, shaderc_shader_kind programKind)
	{
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetSourceLanguage(shaderc_source_language_glsl);
		options.SetGenerateDebugInfo();
		options.SetIncluder(CreateScope<ShaderIncluder>());
		options.SetTargetEnvironment(shaderc_target_env_vulkan, 0);
		options.SetOptimizationLevel(shaderc_optimization_level_performance);

		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			//options.AddMacroDefinition("VULKAN");
			break;
		}

		shaderc::SpvCompilationResult shaderModule = compiler.CompileGlslToSpv(source.c_str(), source.size(), programKind, path.c_str(), "main", options);
		if (shaderModule.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			std::string_view stageName = "";
			switch (programKind)
			{
			case shaderc_vertex_shader:
				stageName = "Vertex";
				break;
			case shaderc_fragment_shader:
				stageName = "Pixel";
				break;
			}

			FLARE_CORE_ERROR("Failed to compile Vulkan GLSL '{}' Stage = {}", path, stageName);
			FLARE_CORE_ERROR("Shader Error: {0}", shaderModule.GetErrorMessage());
			return {};
		}

		return std::vector<uint32_t>(shaderModule.cbegin(), shaderModule.cend());
	}

	struct CrossCompilationOptions
	{
		shaderc_shader_kind ProgramKind;
		uint32_t LocationBase;
	};

	static uint32_t CountRegistersUsedByStruct(const spirv_cross::Compiler& compiler, const spirv_cross::SPIRType& structType)
	{
		const uint32_t fieldAlignment = 16; // in OpenGL everything is aligned to 16 bytes

		size_t structSize = compiler.get_declared_struct_size(structType);
		return (uint32_t)((structSize + fieldAlignment - 1) / fieldAlignment);
	}

	static std::optional<std::vector<uint32_t>> CompileSpirvToGlsl(const std::string& path, const std::vector<uint32_t>& spirvData, const CrossCompilationOptions& options)
	{
		Scope<spirv_cross::CompilerGLSL> glslCompiler = CreateScope<spirv_cross::CompilerGLSL>(spirvData);

		{
			uint32_t location = options.LocationBase;
			spirv_cross::Compiler crossCompiler(spirvData.data(), spirvData.size());

			const auto& resources = crossCompiler.get_shader_resources();
			auto updateResourcesLocations = [&](const spirv_cross::SmallVector<spirv_cross::Resource>& resources, bool isStruct = false)
			{
				for (const auto& resource : resources)
				{
					glslCompiler->set_decoration(resource.id, spv::DecorationLocation, location);
					const spirv_cross::SPIRType& baseType = crossCompiler.get_type(resource.base_type_id);

					uint32_t registers = glm::max((uint32_t)baseType.member_types.size(), 1u);

					// TODO: Propertly handle arrays, because spirv_cross::CompilerGLSL
					//       doesn't provided an array size for a resource type
					for (auto size : baseType.array)
						registers *= size;

					location += registers;
				}
			};

			updateResourcesLocations(resources.push_constant_buffers, true);
			updateResourcesLocations(resources.sampled_images);
		}

		std::string glsl = glslCompiler->compile();

		shaderc::Compiler compiler;
		shaderc::CompileOptions compilerOptions;
		compilerOptions.SetTargetEnvironment(shaderc_target_env_opengl, shaderc_env_version_opengl_4_5);
		compilerOptions.SetSourceLanguage(shaderc_source_language_glsl);
		compilerOptions.SetGenerateDebugInfo();
		compilerOptions.SetOptimizationLevel(shaderc_optimization_level_performance);

		shaderc::SpvCompilationResult shaderModule = compiler.CompileGlslToSpv(glsl, options.ProgramKind, path.c_str(), compilerOptions);
		if (shaderModule.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			std::string_view stageName = "";
			switch (options.ProgramKind)
			{
			case shaderc_vertex_shader:
				stageName = "Vertex";
				break;
			case shaderc_fragment_shader:
				stageName = "Pixel";
				break;
			}

			FLARE_CORE_ERROR("Failed to compile shader '{}' Stage = {}", path, stageName);
			FLARE_CORE_ERROR("Shader Error: {}", shaderModule.GetErrorMessage());
			return {};
		}

		return std::vector<uint32_t>(shaderModule.cbegin(), shaderModule.cend());
	}

	static void PrintError(const ShaderError& error)
	{
		if (error.Position.Line == UINT32_MAX || error.Position.Column == UINT32_MAX)
			FLARE_CORE_ERROR(error.Message);
		else
		{
			FLARE_CORE_ERROR("Line {} Column {}: {}",
				error.Position.Line,
				error.Position.Column,
				error.Message);
		}
	}

	static std::optional<bool> BoolFromString(std::string_view string)
	{
		if (string == "true")
			return true;
		if (string == "false")
			return false;
		return {};
	}

	static void ParseShaderMetadata(const ShaderSourceParser& parser,
		Ref<ShaderMetadata> metadata,
		std::vector<ShaderError>& errors,
		const std::unordered_map<std::string, size_t>& propertyNameToIndex)
	{
		ShaderFeatures& features = metadata->Features;
		const Block& rootBlock = parser.GetBlock(0);
		for (const auto& element : rootBlock.Elements)
		{
			if (element.Name.Value == "Culling")
			{
				auto culling = CullingModeFromString(element.Value.Value);
				if (culling)
					features.Culling = *culling;
				else
					errors.emplace_back(element.Value.Position, fmt::format("Unknown culling mode '{}'", element.Value.Value));
			}
			else if (element.Name.Value == "BlendMode")
			{
				auto blending = BlendModeFromString(element.Value.Value);
				if (blending)
					features.Blending = *blending;
				else
					errors.emplace_back(element.Value.Position, fmt::format("Unknown blending mode '{}'", element.Value.Value));
			}
			else if (element.Name.Value == "DepthTest")
			{
				std::optional<bool> value = BoolFromString(element.Value.Value);
				if (value)
					features.DepthTesting = *value;
				else
					errors.emplace_back(element.Value.Position, fmt::format("Expected bool, but got {}", element.Value.Value));
			}
			else if (element.Name.Value == "DepthWrite")
			{
				std::optional<bool> value = BoolFromString(element.Value.Value);
				if (value)
					features.DepthWrite = *value;
				else
					errors.emplace_back(element.Value.Position, fmt::format("Expected bool, but got {}", element.Value.Value));
			}
			else if (element.Name.Value == "DepthBias")
			{
				std::optional<bool> value = BoolFromString(element.Value.Value);
				if (value)
					features.DepthBiasEnabled = *value;
				else
					errors.emplace_back(element.Value.Position, fmt::format("Expected bool, but got {}", element.Value.Value));
			}
			else if (element.Name.Value == "DepthFunction")
			{
				auto depthFunction = DepthComparisonFunctionFromString(element.Value.Value);
				if (depthFunction)
					features.DepthFunction = *depthFunction;
				else
					errors.emplace_back(element.Value.Position, fmt::format("Unknown depth function '{}'", element.Value.Value));
			}
			else if (element.Name.Value == "Type")
			{
				if (element.Value.Value == "Surface")
					metadata->Type = ShaderType::Surface;
				else if (element.Value.Value == "2D")
					metadata->Type = ShaderType::_2D;
				else if (element.Value.Value == "FullscreenQuad")
					metadata->Type = ShaderType::FullscreenQuad;
				else if (element.Value.Value == "Debug")
					metadata->Type = ShaderType::Debug;
			}
			else if (element.Name.Value == "Name")
			{
				if (element.Type == BlockElementType::Value)
					metadata->Name = element.Value.Value;
				else
					errors.emplace_back(element.Value.Position, "Expected a string value");
			}
			else if (element.Name.Value == "Properties")
			{
				if (element.Type != BlockElementType::Block)
				{
					errors.emplace_back(element.Value.Position, fmt::format("Expected an array of properties, instead of a value"));
					continue;
				}

				const Block& block = parser.GetBlock(element.ChildBlockIndex);
				for (const BlockElement& property : block.Elements)
				{
					auto it = propertyNameToIndex.find(std::string(property.Name.Value));
					if (it == propertyNameToIndex.end())
						continue;

					ShaderProperty& shaderProperty = metadata->Properties[it->second];
					shaderProperty.Hidden = false;
					shaderProperty.DisplayName = shaderProperty.Name;

					if (property.Type != BlockElementType::Block)
						continue;

					const Block& childBlock = parser.GetBlock(property.ChildBlockIndex);
					for (const BlockElement& element : childBlock.Elements)
					{
						if (element.Name.Value == "Type")
						{
							if (element.Type != BlockElementType::Value)
							{
								errors.emplace_back(element.Value.Position, "Expected a property type not a block");
								continue;
							}

							std::string_view typeString = element.Value.Value;
							if (typeString == "Color")
							{
								switch (shaderProperty.Type)
								{
								case ShaderDataType::Float3:
								case ShaderDataType::Float4:
									shaderProperty.Flags |= SerializationValueFlags::Color;
									break;
								default:
									errors.emplace_back(element.Value.Position, "Property type 'Color' is only compatible with vec3 or vec4");
									break;
								}
							}
							else if (typeString == "HDR")
							{
								switch (shaderProperty.Type)
								{
								case ShaderDataType::Float3:
								case ShaderDataType::Float4:
									shaderProperty.Flags |= SerializationValueFlags::HDRColor;
									break;
								default:
									errors.emplace_back(element.Value.Position, "Property type 'HDR' is only compatible with vec3 or vec4");
									break;
								}
							}
							else
							{
								errors.emplace_back(element.Value.Position, fmt::format("Unknown shader property type '{}'", element.Value.Value));
								continue;
							}
						}

						if (element.Name.Value == "DisplayName")
						{
							if (element.Type == BlockElementType::Value)
								shaderProperty.DisplayName = element.Value.Value;
							else
								errors.emplace_back(element.Value.Position, "Expected a string value");
						}
					}
				}
			}
		}
	}

	static std::optional<ShaderDataType> SPIRVTypeToShaderDataType(const spirv_cross::SPIRType& type)
	{
		std::optional<ShaderDataType> shaderDataType;
		uint32_t componentsCount = type.vecsize;

		switch (type.basetype)
		{
		case spirv_cross::SPIRType::BaseType::Int:
			switch (componentsCount)
			{
			case 1:
				shaderDataType = ShaderDataType::Int;
				break;
			case 2:
				shaderDataType = ShaderDataType::Int2;
				break;
			case 3:
				shaderDataType = ShaderDataType::Int2;
				break;
			case 4:
				shaderDataType = ShaderDataType::Int2;
				break;
			default:
				FLARE_CORE_ERROR("Unsupported components count");
				return {};
			}

			break;
		case spirv_cross::SPIRType::BaseType::Float:
			switch (componentsCount)
			{
			case 1:
				shaderDataType = ShaderDataType::Float;
				break;
			case 2:
				shaderDataType = ShaderDataType::Float2;
				break;
			case 3:
				shaderDataType = ShaderDataType::Float3;
				break;
			case 4:
				if (type.columns == 4)
					shaderDataType = ShaderDataType::Matrix4x4;
				else
					shaderDataType = ShaderDataType::Float4;
				break;
			default:
				FLARE_CORE_ERROR("Unsupported components count");
				return {};
			}

			break;
		default:
			FLARE_CORE_ERROR("Unsupported shader data type");
			return {};
		}

		return shaderDataType;
	}

	static void ExtractVertexShaderInputs(spirv_cross::Compiler& compiler, std::vector<VertexShaderInput>& inputs)
	{
		const spirv_cross::ShaderResources& resources = compiler.get_shader_resources();
		for (const auto& vertexInput : resources.stage_inputs)
		{
			const auto& bufferType = compiler.get_type(vertexInput.base_type_id);
			uint32_t location = compiler.get_decoration(vertexInput.id, spv::DecorationLocation);

			std::optional<ShaderDataType> dataType = SPIRVTypeToShaderDataType(bufferType);

			if (!dataType)
				continue;

			auto& input = inputs.emplace_back();
			input.Location = location;
			input.Type = *dataType;
		}
	}

	static void ExtractShaderProperties(spirv_cross::Compiler& compiler,
										std::vector<ShaderProperty>& properties,
										ShaderPushConstantsRange& pushConstantsRange,
										uint32_t descriptorSetMask)
	{
		const spirv_cross::ShaderResources& resources = compiler.get_shader_resources();

		size_t lastPropertyOffset = 0;
		for (const auto& resource : resources.push_constant_buffers)
		{
			const auto& bufferType = compiler.get_type(resource.base_type_id);
			size_t memberCount = bufferType.member_types.size();

			pushConstantsRange.Size = compiler.get_declared_struct_size(bufferType);
			for (size_t i = 0; i < memberCount; i++)
			{
				std::optional<ShaderDataType> shaderDataType = SPIRVTypeToShaderDataType(compiler.get_type(bufferType.member_types[i]));

				if (!shaderDataType.has_value())
					continue;

				size_t explicitOffset = compiler.get_member_decoration(resource.base_type_id, (uint32_t)i, spv::DecorationOffset);
				size_t memberOffset = compiler.type_struct_member_offset(bufferType, (uint32_t)i);

				if (i == 0)
				{
					pushConstantsRange.Offset = memberOffset;
				}

				ShaderProperty& shaderProperty = properties.emplace_back();
				shaderProperty.Binding = UINT32_MAX;
				shaderProperty.Offset = memberOffset;
				shaderProperty.Type = shaderDataType.value();
				shaderProperty.Size = compiler.get_declared_struct_member_size(bufferType, (uint32_t)i);
				shaderProperty.Hidden = true;

				if (resource.name.empty())
				{
					shaderProperty.Name = resource.name;
				}
				else
				{
					const std::string& memberName = compiler.get_member_name(resource.base_type_id, (uint32_t)i);
					shaderProperty.Name = fmt::format("{}.{}", resource.name, memberName);
				}

				lastPropertyOffset = shaderProperty.Offset;
			}
		}

		uint32_t samplerIndex = 0;
		for (const auto& resource : resources.sampled_images)
		{
			const auto& samplerType = compiler.get_type(resource.type_id);
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			uint32_t descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);

			if (!HAS_BIT(descriptorSetMask, 1 << descriptorSet))
				continue;

			ShaderDataType type = ShaderDataType::Sampler;
			size_t size = ShaderDataTypeSize(type);
			if (samplerType.array.size() > 0)
			{
				if (samplerType.array.size() == 1)
				{
					type = ShaderDataType::SamplerArray;
					size *= samplerType.array[0];
				}
				else
				{
					FLARE_CORE_ERROR("Unsupported sampler array dimensions: {}", samplerType.array.size());
					continue;
				}
			}

			ShaderProperty& shaderProperty = properties.emplace_back();
			shaderProperty.Binding = binding;
			shaderProperty.Name = resource.name;
			shaderProperty.Offset = lastPropertyOffset;
			shaderProperty.Type = type;
			shaderProperty.Size = size;
			shaderProperty.Hidden = true;
			shaderProperty.SamplerIndex = samplerIndex;

			lastPropertyOffset += size;
			samplerIndex++;
		}
		
		for (const auto& resource : resources.storage_images)
		{
			const auto& samplerType = compiler.get_type(resource.type_id);
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			uint32_t descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);

			if (!HAS_BIT(descriptorSetMask, 1 << descriptorSet))
				continue;

			ShaderProperty& shaderProperty = properties.emplace_back();
			shaderProperty.Binding = binding;
			shaderProperty.Name = resource.name;
			shaderProperty.Offset = lastPropertyOffset;
			shaderProperty.Type = ShaderDataType::StorageImage;
			shaderProperty.Size = ShaderDataTypeSize(ShaderDataType::StorageImage);
			shaderProperty.Hidden = true;
			shaderProperty.SamplerIndex = samplerIndex;

			lastPropertyOffset += shaderProperty.Size;
			samplerIndex++;
		}
	}

	static void Reflect(spirv_cross::Compiler& compiler,
		ShaderStageType stage,
		std::unordered_map<std::string, size_t>& propertyNameToIndex,
		Ref<ShaderMetadata> metadata)
	{
		auto& pushConstantsRange = metadata->PushConstantsRanges.emplace_back();
		pushConstantsRange.Offset = 0;
		pushConstantsRange.Stage = stage;

		uint32_t materialDescriptorSetIndex = GetMaterialDescriptorSetIndex(metadata->Type);
		ExtractShaderProperties(compiler, metadata->Properties, pushConstantsRange, 1 << materialDescriptorSetIndex);

		for (size_t i = 0; i < metadata->Properties.size(); i++)
		{
			propertyNameToIndex[metadata->Properties[i].Name] = i;
		}
	}

	static void ExtractShaderOutputs(spirv_cross::Compiler& compiler, ShaderOutputs& outputs)
	{
		const spirv_cross::ShaderResources& resources = compiler.get_shader_resources();

		outputs.reserve(resources.stage_outputs.size());
		for (const auto& output : resources.stage_outputs)
		{
			uint32_t location = compiler.get_decoration(output.id, spv::DecorationLocation);
			outputs.push_back(location);
		}
	}

	static shaderc_shader_kind ShaderStageTypeToShaderCStageType(ShaderStageType stageType)
	{
		switch (stageType)
		{
		case ShaderStageType::Vertex:
			return shaderc_vertex_shader;
		case ShaderStageType::Pixel:
			return shaderc_fragment_shader;
		case ShaderStageType::Compute:
			return shaderc_compute_shader;
		}

		FLARE_CORE_ASSERT(false);
		return (shaderc_shader_kind)0;
	}

	static bool CompileGraphicsShader(AssetHandle shaderHandle, bool forceRecompile, const ShaderSourceParser& parser, std::vector<ShaderError>& errors)
	{
		const std::filesystem::path& shaderPath = AssetManager::GetAssetMetadata(shaderHandle)->Path;
		std::string pathString = shaderPath.string();
		std::vector<PreprocessedShaderProgram> programs;
		std::unordered_map<std::string, size_t> propertyNameToIndex;
		ShaderOutputs shaderOutputs;

		for (const auto& sourceBlock : parser.GetSourceBlocks())
		{
			bool isStageSupported = false;
			switch (sourceBlock.Stage)
			{
			case ShaderStageType::Vertex:
			case ShaderStageType::Pixel:
				isStageSupported = true;
				break;
			case ShaderStageType::Compute:
				isStageSupported = false;
				break;
			default:
				FLARE_CORE_ASSERT(false, "Unhandled ShaderStageType");
				break;
			}

			if (!isStageSupported)
			{
				errors.emplace_back(sourceBlock.Position, fmt::format(
					"{} stage is not supported by shaders used in graphics pipeline",
					ShaderStageTypeToString(sourceBlock.Stage)));
				return false;
			}

			PreprocessedShaderProgram& program = programs.emplace_back();
			program.Source = sourceBlock.Source;
			program.Stage = sourceBlock.Stage;
		}

		Ref<ShaderMetadata> metadata = CreateRef<ShaderMetadata>();
		metadata->Type = ShaderType::Surface;
		metadata->Name = shaderPath.filename().replace_extension().generic_string();

		for (const PreprocessedShaderProgram& program : programs)
		{
			shaderc_shader_kind shaderKind = ShaderStageTypeToShaderCStageType(program.Stage);

			CrossCompilationOptions options;
			options.LocationBase = 0;
			options.ProgramKind = shaderKind;

			if (shaderKind == shaderc_fragment_shader)
			{
				// NOTE: Uniform locations in pixel shaders start at 100,
				//       in order to avoid overlaps with vertex shader uniform locations

				// TODO: Find a better solution
				options.LocationBase = 100;
			}

			std::optional<std::vector<uint32_t>> compiledVulkanShader = {};

			if (!forceRecompile)
			{
				compiledVulkanShader = ShaderCacheManager::GetInstance()->FindCache(
					shaderHandle,
					ShaderTargetEnvironment::Vulkan,
					program.Stage);
			}

			if (!compiledVulkanShader)
			{
				compiledVulkanShader = CompileVulkanGlslToSpirv(pathString, program.Source, shaderKind);
				if (!compiledVulkanShader)
					return false;

				ShaderCacheManager::GetInstance()->SetCache(shaderHandle, ShaderTargetEnvironment::Vulkan, program.Stage, compiledVulkanShader.value());
			}

			try
			{
				spirv_cross::Compiler compiler(compiledVulkanShader.value());
				Reflect(compiler, program.Stage, propertyNameToIndex, metadata);

				switch (program.Stage)
				{
				case ShaderStageType::Vertex:
					ExtractVertexShaderInputs(compiler, metadata->VertexShaderInputs);
					break;
				case ShaderStageType::Pixel:
					ExtractShaderOutputs(compiler, shaderOutputs);
					break;
				}
			}
			catch (spirv_cross::CompilerError& e)
			{
				FLARE_CORE_ERROR("Shader reflection failed: {}", e.what());
			}
		}

		metadata->Outputs = std::move(shaderOutputs);
		ParseShaderMetadata(parser, metadata, errors, propertyNameToIndex);

		for (const auto& program : programs)
			metadata->Stages.push_back(program.Stage);

		EditorShaderCache::GetInstance().SetShaderEntry(shaderHandle, metadata);

		return true;
	}

	static void ReflectComputeShader(spirv_cross::Compiler& compiler, Ref<ComputeShaderMetadata> metadata)
	{
		metadata->LocalGroupSize.x = compiler.get_execution_mode_argument(spv::ExecutionModeLocalSize, 0);
		metadata->LocalGroupSize.y = compiler.get_execution_mode_argument(spv::ExecutionModeLocalSize, 1);
		metadata->LocalGroupSize.z = compiler.get_execution_mode_argument(spv::ExecutionModeLocalSize, 2);

		const auto& resources = compiler.get_shader_resources();
		for (const auto& resource : resources.push_constant_buffers)
		{
			const auto& bufferType = compiler.get_type(resource.base_type_id);
			size_t bufferSize = compiler.get_declared_struct_size(bufferType);
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			uint32_t membersCount = (uint32_t)bufferType.member_types.size();

			metadata->PushConstantsRange.Offset = 0;
			metadata->PushConstantsRange.Size = bufferSize;
			metadata->PushConstantsRange.Stage = ShaderStageType::Compute;
		}

		ExtractShaderProperties(compiler, metadata->Properties, metadata->PushConstantsRange, UINT32_MAX);
	}

	static bool CompileComputeShader(AssetHandle shaderHandle, bool forceRecompile, const ShaderSourceParser& parser, std::vector<ShaderError>& errors)
	{
		const std::filesystem::path& shaderPath = AssetManager::GetAssetMetadata(shaderHandle)->Path;
		std::string shaderPathString = shaderPath.generic_string();

		PreprocessedShaderProgram program{};

		for (const auto& sourceBlock : parser.GetSourceBlocks())
		{
			bool isStageSupported = sourceBlock.Stage == ShaderStageType::Compute;

			if (!isStageSupported)
			{
				errors.emplace_back(sourceBlock.Position, fmt::format(
					"{} stage is not supported by compute shaders",
					ShaderStageTypeToString(sourceBlock.Stage)));
				return false;
			}

			program.Source = sourceBlock.Source;
			program.Stage = sourceBlock.Stage;
		}
		
		Ref<ComputeShaderMetadata> metadata = CreateRef<ComputeShaderMetadata>();
		metadata->Name = shaderPath.filename().replace_extension().generic_string();

		shaderc_shader_kind shaderKind = ShaderStageTypeToShaderCStageType(program.Stage);
		std::optional<std::vector<uint32_t>> compiledVulkanShader;

		if (!forceRecompile)
		{
			compiledVulkanShader = ShaderCacheManager::GetInstance()->FindCache(
				shaderHandle,
				ShaderTargetEnvironment::Vulkan,
				program.Stage);
		}

		if (!compiledVulkanShader)
		{
			compiledVulkanShader = CompileVulkanGlslToSpirv(shaderPathString, program.Source, shaderKind);
			if (!compiledVulkanShader)
				return false;

			ShaderCacheManager::GetInstance()->SetCache(shaderHandle, ShaderTargetEnvironment::Vulkan, program.Stage, compiledVulkanShader.value());
		}

		try
		{
			spirv_cross::Compiler compiler(*compiledVulkanShader);
			ReflectComputeShader(compiler, metadata);
		}
		catch (spirv_cross::CompilerError& error)
		{
			FLARE_CORE_ERROR("Shader reflection failed: {}", error.what());
		}

		EditorShaderCache::GetInstance().SetComputeShaderEntry(shaderHandle, metadata);
		return true;
	}

	static void PrintShaderErrors(const std::vector<ShaderError>& errors, const std::filesystem::path& shaderPath)
	{
		FLARE_CORE_ERROR("Failed to compile shader '{}'", shaderPath.generic_string());
		for (const ShaderError& error : errors)
			PrintError(error);
	}
	
	bool ShaderCompiler::Compile(AssetHandle shaderHandle, bool forceRecompile)
	{
		FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(shaderHandle));

		const AssetMetadata* assetMetadata = AssetManager::GetAssetMetadata(shaderHandle);
		const std::filesystem::path& shaderPath = assetMetadata->Path;
		std::string source = "";
		{
			std::ifstream file(shaderPath);
			if (!file.is_open())
			{
				FLARE_CORE_ERROR("Failed not read shader file: {}", shaderPath.string());
				return false;
			}

			std::stringstream buffer;
			buffer << file.rdbuf();

			source = buffer.str();
		}

		std::vector<ShaderError> errors;
		ShaderSourceParser parser(shaderPath, source, errors);
		parser.Parse();

		if (errors.size() > 0)
		{
			PrintShaderErrors(errors, shaderPath);
			return false;
		}
		
		switch (assetMetadata->Type)
		{
		case AssetType::Shader:
			if (CompileGraphicsShader(shaderHandle, forceRecompile, parser, errors))
				return true;
			break;
		case AssetType::ComputeShader:
			if (CompileComputeShader(shaderHandle, forceRecompile, parser, errors))
				return true;
			break;
		default:
			FLARE_CORE_ASSERT(false);
		}

		PrintShaderErrors(errors, shaderPath);
		return false;
	}
}
