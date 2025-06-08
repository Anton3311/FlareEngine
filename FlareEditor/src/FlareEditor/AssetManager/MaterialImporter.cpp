#include "PCH.h"

#include "MaterialImporter.h"

#include "FlareCore/Log.h"
#include "FlareCore/Profiler/Profiler.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/Shader.h"
#include "Flare/Renderer/Texture.h"

#include "Flare/Serialization/Serialization.h"

#include "FlareEditor/AssetManager/EditorAssetManager.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

namespace Flare
{
	static const char* ShaderDataTypeToString(ShaderDataType type)
	{
		switch (type)
		{
#define TYPE_TO_STRING(name) case ShaderDataType::name: return #name;
			TYPE_TO_STRING(Int);
			TYPE_TO_STRING(Int2);
			TYPE_TO_STRING(Int3);
			TYPE_TO_STRING(Int4);

			TYPE_TO_STRING(UInt);
			TYPE_TO_STRING(UInt2);
			TYPE_TO_STRING(UInt3);
			TYPE_TO_STRING(UInt4);

			TYPE_TO_STRING(Float);
			TYPE_TO_STRING(Float2);
			TYPE_TO_STRING(Float3);
			TYPE_TO_STRING(Float4);

			TYPE_TO_STRING(Sampler);
			TYPE_TO_STRING(SamplerArray);
			TYPE_TO_STRING(StorageImage);
			TYPE_TO_STRING(Matrix4x4);
		default:
			FLARE_VERIFY_UNREACHABLE();
#undef TYPE_TO_STRING
		}

		return "";
	}

	static std::optional<ShaderDataType> ShaderDataTypeFromString(std::string_view string)
	{
#define TYPE_FROM_STRING(name) if (string == #name) return ShaderDataType::name;
		TYPE_FROM_STRING(Int);
		TYPE_FROM_STRING(Int2);
		TYPE_FROM_STRING(Int3);
		TYPE_FROM_STRING(Int4);

		TYPE_FROM_STRING(UInt);
		TYPE_FROM_STRING(UInt2);
		TYPE_FROM_STRING(UInt3);
		TYPE_FROM_STRING(UInt4);

		TYPE_FROM_STRING(Float);
		TYPE_FROM_STRING(Float2);
		TYPE_FROM_STRING(Float3);
		TYPE_FROM_STRING(Float4);

		TYPE_FROM_STRING(Sampler);
		TYPE_FROM_STRING(SamplerArray);
		TYPE_FROM_STRING(StorageImage);
		TYPE_FROM_STRING(Matrix4x4);
#undef TYPE_FROM_STRING

		return {};
	}

	static void SerializeMaterialProperties(const Ref<const Material>& material, YAML::Emitter& emitter)
	{
		FLARE_PROFILE_FUNCTION();
		Ref<const Shader> shader = material->GetShader();

		const ShaderProperties& properties = shader->GetProperties();
		for (uint32_t index = 0; index < (uint32_t)properties.size(); index++)
		{
			const ShaderProperty& property = properties[index];

			if (property.Type == ShaderDataType::Matrix4x4)
				continue;

			emitter << YAML::BeginMap;

			emitter << YAML::Key << "Name" << YAML::Value << property.Name;
			emitter << YAML::Key << "Type" << YAML::Value << ShaderDataTypeToString(property.Type);
			emitter << YAML::Key << "Value" << YAML::Value;

			switch (property.Type)
			{
			case ShaderDataType::Int:
				emitter << material->ReadPropertyValue<int32_t>(index);
				break;
			case ShaderDataType::Int2:
				emitter << material->ReadPropertyValue<glm::ivec2>(index);
				break;
			case ShaderDataType::Int3:
				emitter << material->ReadPropertyValue<glm::ivec3>(index);
				break;
			case ShaderDataType::Int4:
				emitter << material->ReadPropertyValue<glm::ivec4>(index);
				break;

			case ShaderDataType::Sampler:
			{
				const Ref<Texture>& texture = material->GetTextureProperty(index);

				if (texture == nullptr)
				{
					emitter << NULL_ASSET_HANDLE;
					break;
				}

				if (AssetManager::IsAssetHandleValid(texture->Handle))
				{
					emitter << texture->Handle;
				}
				else
				{
					emitter << NULL_ASSET_HANDLE;
				}

				break;
			}
			case ShaderDataType::Float:
				emitter << material->ReadPropertyValue<float>(index);
				break;
			case ShaderDataType::Float2:
				emitter << material->ReadPropertyValue<glm::vec2>(index);
				break;
			case ShaderDataType::Float3:
				emitter << material->ReadPropertyValue<glm::vec3>(index);
				break;
			case ShaderDataType::Float4:
				emitter << material->ReadPropertyValue<glm::vec4>(index);
				break;
			}

			emitter << YAML::EndMap;
		}
	}

	void MaterialImporter::SerializeMaterial(Ref<const Material> material, const std::filesystem::path& path)
	{
		FLARE_PROFILE_FUNCTION();
		Ref<const Shader> shader = material->GetShader();

		YAML::Emitter emitter;
		emitter << YAML::BeginMap;

		if (shader == nullptr)
			emitter << YAML::Key << "Shader" << YAML::Value << NULL_ASSET_HANDLE;
		else
			emitter << YAML::Key << "Shader" << YAML::Value << shader->Handle;

		emitter << YAML::Key << "Properties" << YAML::BeginSeq;

		if (shader != nullptr)
		{
			SerializeMaterialProperties(material, emitter);
		}

		emitter << YAML::EndSeq; // Properties

		emitter << YAML::EndMap;

		std::ofstream output(path);
		output << emitter.c_str();
	}

	static void DeserializeMaterialProperty(const AssetMetadata& metadata, Ref<Material> material, const YAML::Node& parameter)
	{
		FLARE_PROFILE_FUNCTION();

		Ref<Shader> shader = material->GetShader();

		YAML::Node nameNode = parameter["Name"];
		if (!nameNode)
			return;

		std::string propertyName = nameNode.as<std::string>();
		std::optional<uint32_t> index = shader->GetPropertyIndex(propertyName);

		if (!index.has_value())
		{
			FLARE_CORE_ERROR("Material '{}' {} doesn't have a property named '{}'", metadata.Name, metadata.Handle, propertyName);
			return;
		}

		YAML::Node typeNode = parameter["Type"];
		if (!typeNode)
			return;

		std::string typeName = typeNode.as<std::string>();
		std::optional<ShaderDataType> type = ShaderDataTypeFromString(typeName);

		if (!type)
		{
			FLARE_CORE_ERROR("Failed to deserialize property of material '{}' with handle {}", metadata.Name, metadata.Handle);
			FLARE_CORE_ERROR("Property '{}' in has invalid data type {}", propertyName, typeName);
			return;
		}

		YAML::Node valueNode = parameter["Value"];
		if (!valueNode)
			return;

		if (*type != shader->GetProperties()[*index].Type)
		{
			FLARE_CORE_ERROR("Property named '{}' of material '{}' {} has a data type that doesn't match the one specified in the shader",
				propertyName,
				metadata.Name,
				metadata.Handle);
			return;
		}

		switch (type.value())
		{
		case ShaderDataType::Int:
			material->WritePropertyValue(index.value(), valueNode.as<int32_t>());
			break;
		case ShaderDataType::Int2:
			material->WritePropertyValue(index.value(), valueNode.as<glm::ivec2>());
			break;
		case ShaderDataType::Int3:
			material->WritePropertyValue(index.value(), valueNode.as<glm::ivec3>());
			break;
		case ShaderDataType::Int4:
			material->WritePropertyValue(index.value(), valueNode.as<glm::ivec4>());
			break;

		case ShaderDataType::UInt:
			material->WritePropertyValue(index.value(), valueNode.as<uint32_t>());
			break;
		case ShaderDataType::UInt2:
			material->WritePropertyValue(index.value(), valueNode.as<glm::uvec2>());
			break;
		case ShaderDataType::UInt3:
			material->WritePropertyValue(index.value(), valueNode.as<glm::uvec3>());
			break;
		case ShaderDataType::UInt4:
			material->WritePropertyValue(index.value(), valueNode.as<glm::uvec4>());
			break;

		case ShaderDataType::Float:
			material->WritePropertyValue(index.value(), valueNode.as<float>());
			break;
		case ShaderDataType::Float2:
			material->WritePropertyValue(index.value(), valueNode.as<glm::vec2>());
			break;
		case ShaderDataType::Float3:
			material->WritePropertyValue(index.value(), valueNode.as<glm::vec3>());
			break;
		case ShaderDataType::Float4:
			auto a = valueNode.as<glm::vec4>();
			material->WritePropertyValue(index.value(), valueNode.as<glm::vec4>());
			break;

		case ShaderDataType::Sampler:
		{
			AssetHandle handle = valueNode.as<AssetHandle>();
			if (AssetManager::IsAssetHandleValid(handle))
				material->SetTextureProperty(*index, AssetManager::GetAsset<Texture>(handle));

			break;
		}
		}
	}

	Ref<Material> MaterialImporter::ImportMaterial(const AssetMetadata& metadata)
	{
		FLARE_PROFILE_FUNCTION();
		if (metadata.Source == AssetSource::Memory)
		{
			AssetManager::GetInstance().As<EditorAssetManager>()->LoadAsset(metadata.Parent);
			return AssetManager::GetAsset<Material>(metadata.Handle);
		}

		Ref<Material> material = Material::Create();

		try
		{
			std::optional<AssetHandle> shaderHandle;

			YAML::Node node = YAML::LoadFile(metadata.Path.generic_string());
			if (YAML::Node shaderNode = node["Shader"])
			{
				AssetHandle handle = shaderNode.as<AssetHandle>();
				if (!AssetManager::IsAssetHandleValid(handle))
				{
					// No valid shader, return an empty material
					return material;
				}

				Ref<Shader> shader = AssetManager::GetAsset<Shader>(handle);
				if (!shader)
				{
					FLARE_CORE_ERROR("Failed to load shader (Handle={}) for material (Handle={}, Path={})",
						(uint64_t)handle,
						metadata.Handle,
						metadata.Path.string());

					// No valid shader, return an empty material
					return material;
				}

				material->SetShader(shader);
				shaderHandle = handle;
			}

			if (!shaderHandle.has_value())
				return nullptr;

			Ref<Shader> shader = AssetManager::GetAsset<Shader>(shaderHandle.value());
			if (!shader)
				return material;

			const ShaderProperties& shaderProperties = shader->GetProperties();
			if (YAML::Node parameters = node["Properties"])
			{
				for (YAML::Node parameter : parameters)
				{
					DeserializeMaterialProperty(metadata, material, parameter);
				}
			}
		}
		catch (std::exception& e)
		{
			FLARE_CORE_ERROR("Failed to import a material '{}': {}", metadata.Path.generic_string(), e.what());
		}

		return material;
	}
}
