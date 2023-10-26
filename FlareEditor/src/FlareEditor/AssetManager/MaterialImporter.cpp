#include "MaterialImporter.h"

#include "Flare/AssetManager/AssetManager.h"
#include "Flare/Renderer/Shader.h"

#include "Flare/Serialization/Serialization.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

namespace Flare
{
	void MaterialImporter::SerializeMaterial(Ref<Material> material, const std::filesystem::path& path)
	{
		Ref<Shader> shader = material->GetShader();

		YAML::Emitter emitter;
		emitter << YAML::BeginMap;

		emitter << YAML::Key << "Shader" << YAML::Value << shader->Handle;

		emitter << YAML::Key << "Parameters" << YAML::BeginSeq;

		const ShaderParameters& parameters = shader->GetParameters();
		for (uint32_t index = 0; index < (uint32_t)parameters.size(); index++)
		{
			const ShaderParameter& parameter = parameters[index];

			if (parameter.Type == ShaderDataType::Matrix4x4)
				continue; // Skip matrices

			emitter << YAML::BeginMap;

			emitter << YAML::Key << "Name" << YAML::Value << parameter.Name;
			emitter << YAML::Key << "Type" << YAML::Value << (uint32_t)parameter.Type;
			emitter << YAML::Key << "Value" << YAML::Value;

			switch (parameter.Type)
			{
			case ShaderDataType::Int:
				emitter << material->ReadParameterValue<int32_t>(index);
				break;
			case ShaderDataType::Int2:
				emitter << material->ReadParameterValue<glm::ivec2>(index);
				break;
			case ShaderDataType::Int3:
				emitter << material->ReadParameterValue<glm::ivec3>(index);
				break;
			case ShaderDataType::Int4:
				emitter << material->ReadParameterValue<glm::ivec4>(index);
				break;

			case ShaderDataType::Float:
				emitter << material->ReadParameterValue<float>(index);
				break;
			case ShaderDataType::Float2:
				emitter << material->ReadParameterValue<glm::vec2>(index);
				break;
			case ShaderDataType::Float3:
				emitter << material->ReadParameterValue<glm::vec3>(index);
				break;
			case ShaderDataType::Float4:
				emitter << material->ReadParameterValue<glm::vec4>(index);
				break;
			}

			emitter << YAML::EndMap;
		}

		emitter << YAML::EndSeq; // Parameters

		emitter << YAML::Key << "Features" << YAML::BeginMap;
		emitter << YAML::Key << "CullingMode" << YAML::Value << CullingModeToString(material->Features.Culling);
		emitter << YAML::Key << "DepthTest" << YAML::Value << material->Features.DepthTesting;
		emitter << YAML::EndMap; // Features

		emitter << YAML::EndMap;

		std::ofstream output(path);
		output << emitter.c_str();
	}

	Ref<Material> MaterialImporter::ImportMaterial(const AssetMetadata& metadata)
	{
		Ref<Material> material = nullptr;

		try
		{
			std::optional<AssetHandle> shaderHandle;

			YAML::Node node = YAML::LoadFile(metadata.Path.generic_string());
			if (YAML::Node shaderNode = node["Shader"])
			{
				AssetHandle handle = shaderNode.as<AssetHandle>();
				if (!AssetManager::IsAssetHandleValid(handle))
				{
					FLARE_CORE_ERROR("Material asset has invalid shader handle");
				}
				else
				{
					material = CreateRef<Material>(handle);
					shaderHandle = handle;
				}
			}

			if (shaderHandle.has_value())
			{
				Ref<Shader> shader = AssetManager::GetAsset<Shader>(shaderHandle.value());
				const ShaderParameters& shaderParameters = shader->GetParameters();

				if (YAML::Node parameters = node["Parameters"])
				{
					for (YAML::Node parameter : parameters)
					{
						std::optional<uint32_t> index = {};
						std::optional<ShaderDataType> type = {};

						if (YAML::Node nameNode = parameter["Name"])
						{
							std::string name = nameNode.as<std::string>();
							index = shader->GetParameterIndex(name);
						}

						if (YAML::Node typeNode = parameter["Type"])
						{
							uint32_t dataType = typeNode.as<uint32_t>();
							type = (ShaderDataType)dataType;
						}

						YAML::Node valueNode = parameter["Value"];
						if (index.has_value() && type.has_value() && valueNode)
						{
							switch (type.value())
							{
							case ShaderDataType::Int:
								material->WriteParameterValue(index.value(), valueNode.as<int32_t>());
								break;
							case ShaderDataType::Int2:
								material->WriteParameterValue(index.value(), valueNode.as<glm::ivec2>());
								break;
							case ShaderDataType::Int3:
								material->WriteParameterValue(index.value(), valueNode.as<glm::ivec3>());
								break;
							case ShaderDataType::Int4:
								material->WriteParameterValue(index.value(), valueNode.as<glm::ivec4>());
								break;

							case ShaderDataType::Float:
								material->WriteParameterValue(index.value(), valueNode.as<float>());
								break;
							case ShaderDataType::Float2:
								material->WriteParameterValue(index.value(), valueNode.as<glm::vec2>());
								break;
							case ShaderDataType::Float3:
								material->WriteParameterValue(index.value(), valueNode.as<glm::vec3>());
								break;
							case ShaderDataType::Float4:
								auto a = valueNode.as<glm::vec4>();
								material->WriteParameterValue(index.value(), valueNode.as<glm::vec4>());
								break;
							}
						}
					}
				} // Parameters

				if (YAML::Node featuresNode = node["Features"])
				{
					if (YAML::Node cullingMode = featuresNode["CullingMode"])
						material->Features.Culling = CullinModeFromString(cullingMode.as<std::string>());
					if (YAML::Node depthTest = featuresNode["DepthTest"])
						material->Features.DepthTesting = depthTest.as<bool>();
				} // Features
			}
		}
		catch (std::exception& e)
		{
			FLARE_CORE_ERROR("Failed to import a material '{0}': ", metadata.Path.generic_string(), e.what());
		}

		return material;
	}
}
