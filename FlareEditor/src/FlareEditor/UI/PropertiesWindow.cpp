#include "PropertiesWindow.h"

#include "Flare/Scene/Scene.h"
#include "Flare/Scripting/ScriptingEngine.h"

#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/MaterialsTable.h"

#include "FlareEditor/UI/EditorGUI.h"
#include "FlareEditor/UI/ECS/EntityProperties.h"
#include "FlareEditor/UI/SpriteEditor.h"
#include "FlareEditor/UI/SerializablePropertyRenderer.h"

#include "FlareEditor/EditorLayer.h"
#include "FlareEditor/AssetManager/EditorShaderCache.h"
#include "FlareEditor/AssetManager/MaterialImporter.h"
#include "FlareEditor/AssetManager/SpriteImporter.h"

#include <imgui.h>

namespace Flare
{
	TextureImportSettings s_SelectedTextureImportSettings;

	PropertiesWindow::PropertiesWindow(AssetManagerWindow& assetManagerWindow)
		: m_AssetManagerWindow(assetManagerWindow) {}

	void PropertiesWindow::OnAttach()
	{
		m_AssetManagerWindow.OnAssetSelectionChanged.Bind([this](AssetHandle handle)
		{
			if (!AssetManager::IsAssetHandleValid(handle))
				return;

			switch (AssetManager::GetAssetMetadata(handle)->Type)
			{
			case AssetType::Texture:
				s_SelectedTextureImportSettings = TextureImportSettings();
				TextureImporter::DeserializeImportSettings(handle, s_SelectedTextureImportSettings);
				break;
			case AssetType::Sprite:
			{
				Ref<Sprite> sprite = AssetManager::GetAsset<Sprite>(handle);
				FLARE_CORE_ASSERT(sprite);
				m_SpriteEditor = SpriteEditor(sprite);
				break;
			}
			}
		});
	}

	void PropertiesWindow::OnImGuiRender()
	{
		ImGui::Begin("Properties");

		if (Scene::GetActive() == nullptr)
		{
			ImGui::End();
			return;
		}

		World& world = Scene::GetActive()->GetECSWorld();
		const EditorSelection& selection = EditorLayer::GetInstance().Selection;

		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();

		if (selection.GetType() == EditorSelectionType::Entity)
		{
			Entity selectedEntity = selection.GetEntity();
			if (world.IsEntityAlive(selectedEntity))
			{
				EntityProperties properties(world);
				properties.OnRenderImGui(selectedEntity);
			}
		}
		else if (selection.GetType() == EditorSelectionType::Asset)
		{
			AssetHandle handle = selection.GetAssetHandle();
			if (AssetManager::IsAssetHandleValid(handle))
				RenderAssetProperties(handle);
		}

		ImGui::End();
	}

	void PropertiesWindow::RenderAssetProperties(AssetHandle handle)
	{
		FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(handle));

		switch (AssetManager::GetAssetMetadata(handle)->Type)
		{
		case AssetType::Scene:
			break;
		case AssetType::Texture:
			RenderTextureSettingsEditor(handle, s_SelectedTextureImportSettings);
			break;
		case AssetType::Material:
			RenderMaterialEditor(handle);
			break;
		case AssetType::MaterialsTable:
		{
			Ref<MaterialsTable> table = AssetManager::GetAsset<MaterialsTable>(handle);
			FLARE_CORE_ASSERT(table);

			if (EditorGUI::BeginPropertyGrid())
			{
				for (size_t i = 0; i < table->Materials.size(); i++)
				{
					EditorGUI::PropertyIndex(i);
					EditorGUI::AssetField(table->Materials[i]);
				}

				EditorGUI::EndPropertyGrid();
			}

			break;
		}
		case AssetType::Sprite:
		{
			Ref<Sprite> sprite = AssetManager::GetAsset<Sprite>(handle);
			FLARE_CORE_ASSERT(sprite);

			if (ImGui::Button("Save"))
			{
				SpriteImporter::SerializeSprite(sprite, AssetManager::GetAssetMetadata(handle)->Path);
			}

			SerializablePropertyRenderer propertiesRenderer;
			propertiesRenderer.Serialize("Sprite", SerializationValue(*sprite));

			if (ImGui::Begin("Sprite Editor"))
			{
				m_SpriteEditor.OnImGuiRenderer();
				ImGui::End();
			}

			break;
		}
		}
	}

	bool PropertiesWindow::RenderTextureSettingsEditor(AssetHandle handle, TextureImportSettings& importSettings)
	{
		FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(handle));
		FLARE_CORE_ASSERT(AssetManager::GetAssetMetadata(handle)->Type == AssetType::Texture);

		if (ImGui::Button("Save"))
		{
			TextureImporter::SerialiazeImportSettings(handle, importSettings);
			As<EditorAssetManager>(AssetManager::GetInstance())->ReloadAsset(handle);
		}

		Ref<const Texture> texture = AssetManager::GetAsset<Texture>(handle);
		FLARE_CORE_ASSERT(texture);

		if (ImGui::BeginChild("Texture Settings"))
		{
			const ImGuiStyle& style = ImGui::GetStyle();
			if (EditorGUI::BeginPropertyGrid())
			{
				EditorGUI::PropertyName("Size");

				EditorGUI::MoveCursor(ImVec2(0, style.FramePadding.y));
				ImGui::Text("%u x %u", texture->GetWidth(), texture->GetHeight());
				EditorGUI::MoveCursor(ImVec2(0, style.FramePadding.y));

				EditorGUI::PropertyName("Format");

				EditorGUI::MoveCursor(ImVec2(0, style.FramePadding.y));
				ImGui::TextUnformatted(TextureFormatToString(texture->GetSpecifications().Format));
				EditorGUI::MoveCursor(ImVec2(0, style.FramePadding.y));

				{
					const char* previewText = TextureFilteringToString(importSettings.Filtering);

					EditorGUI::PropertyName("Filtering");
					ImGui::PushID(&importSettings.Filtering);
					if (ImGui::BeginCombo("", previewText, ImGuiComboFlags_HeightRegular))
					{
						bool selected = importSettings.Filtering == TextureFiltering::Linear;
						if (ImGui::Selectable(TextureFilteringToString(TextureFiltering::Linear), selected))
							importSettings.Filtering = TextureFiltering::Linear;

						if (selected)
							ImGui::SetItemDefaultFocus();

						selected = importSettings.Filtering == TextureFiltering::Closest;
						if (ImGui::Selectable(TextureFilteringToString(TextureFiltering::Closest), selected))
							importSettings.Filtering = TextureFiltering::Closest;

						if (selected)
							ImGui::SetItemDefaultFocus();

						ImGui::EndCombo();
					}
					ImGui::PopID();
				}

				{
					const char* previewText = TextureWrapToString(importSettings.WrapMode);
					EditorGUI::PropertyName("Wrap");

					ImGui::PushID(&importSettings.WrapMode);
					if (ImGui::BeginCombo("", previewText, ImGuiComboFlags_HeightRegular))
					{
						bool selected = importSettings.WrapMode == TextureWrap::Repeat;
						if (ImGui::Selectable(TextureWrapToString(TextureWrap::Repeat), selected))
							importSettings.WrapMode = TextureWrap::Repeat;

						if (selected)
							ImGui::SetItemDefaultFocus();

						selected = importSettings.WrapMode == TextureWrap::Clamp;
						if (ImGui::Selectable(TextureWrapToString(TextureWrap::Clamp), selected))
							importSettings.WrapMode = TextureWrap::Clamp;

						if (selected)
							ImGui::SetItemDefaultFocus();

						ImGui::EndCombo();
					}
					ImGui::PopID();
				}

				EditorGUI::BoolPropertyField("Generate Mip Maps", importSettings.GenerateMipMaps);

				EditorGUI::EndPropertyGrid();
			}

			// Preview

			float scrollbarWidth = ImGui::GetCurrentWindow()->ScrollbarSizes.x;

			ImVec2 availiableContentSize = ImGui::GetContentRegionAvail() - ImVec2(scrollbarWidth, 0.0f);
			float width = (float)texture->GetWidth();
			float height = (float)texture->GetHeight();

			float aspectRatio = width / height;

			ImGui::Image(
				(ImTextureID)texture->GetRendererId(),
				ImVec2(availiableContentSize.x, availiableContentSize.x / aspectRatio),
				ImVec2(0, 1), ImVec2(1, 0), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), style.Colors[ImGuiCol_Border]);

			ImGui::EndChild();
		}

		return false;
	}

	bool PropertiesWindow::RenderMaterialEditor(AssetHandle handle)
	{
		FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(handle));
		Ref<Material> material = AssetManager::GetAsset<Material>(handle);

		const AssetMetadata* metadata = AssetManager::GetAssetMetadata(handle);

		if (ImGui::Button("Save"))
			MaterialImporter::SerializeMaterial(material, metadata->Path);

		AssetHandle shaderHandle = NULL_ASSET_HANDLE;
		bool hasShader = material->GetShader() != nullptr;

		if (hasShader)
			shaderHandle = material->GetShader()->Handle;

		if (EditorGUI::BeginPropertyGrid())
		{
			if (EditorGUI::AssetField("Shader", shaderHandle))
			{
				if (AssetManager::IsAssetHandleValid(shaderHandle))
				{
					if (AssetManager::GetAssetMetadata(shaderHandle)->Type == AssetType::Shader)
					{
						Ref<Shader> newShader = AssetManager::GetAsset<Shader>(shaderHandle);
						hasShader = newShader != nullptr;
						material->SetShader(newShader);
					}
					else
						FLARE_CORE_ERROR("Provided asset is not a shader");
				}
			}

			ImGui::BeginDisabled(true);

			if (hasShader)
			{
				ShaderFeatures features = material->GetShader()->GetFeatures();

				EditorGUI::BoolPropertyField("Depth Test", features.DepthTesting);
				EditorGUI::BoolPropertyField("Depth Write", features.DepthWrite);
				EditorGUI::PropertyName("Culling Mode");

				const char* preview = CullingModeToString(features.Culling);

				ImGui::PushID("CullingMode");
				if (ImGui::BeginCombo("", preview, ImGuiComboFlags_HeightRegular))
				{
					if (ImGui::MenuItem("None"))
						features.Culling = CullingMode::None;
					if (ImGui::MenuItem("Front"))
						features.Culling = CullingMode::Front;
					if (ImGui::MenuItem("Back"))
						features.Culling = CullingMode::Back;

					ImGui::EndCombo();
				}

				ImGui::PopID();

				// Blend Mode

				preview = BlendModeToString(features.Blending);

				EditorGUI::PropertyName("Blend Mode");

				ImGui::PushID("BlendMode");
				if (ImGui::BeginCombo("", preview, ImGuiComboFlags_HeightRegular))
				{
					if (ImGui::MenuItem("Opaque"))
						features.Blending = BlendMode::Opaque;
					if (ImGui::MenuItem("Transparent"))
						features.Blending = BlendMode::Transparent;

					ImGui::EndCombo();
				}

				ImGui::PopID();

				// Depth Comaprison Function

				preview = DepthComparisonFunctionToString(features.DepthFunction);

				EditorGUI::PropertyName("Depth Function");

				ImGui::PushID("Depth Function");
				if (ImGui::BeginCombo("", preview, ImGuiComboFlags_HeightRegular))
				{
					if (ImGui::MenuItem("Less"))
						features.DepthFunction = DepthComparisonFunction::Less;
					if (ImGui::MenuItem("Greater"))
						features.DepthFunction = DepthComparisonFunction::Greater;
					if (ImGui::MenuItem("Less or equal"))
						features.DepthFunction = DepthComparisonFunction::LessOrEqual;
					if (ImGui::MenuItem("Greater or equal"))
						features.DepthFunction = DepthComparisonFunction::GreaterOrEqual;
					if (ImGui::MenuItem("Equal"))
						features.DepthFunction = DepthComparisonFunction::Equal;
					if (ImGui::MenuItem("Not equal"))
						features.DepthFunction = DepthComparisonFunction::NotEqual;
					if (ImGui::MenuItem("Never"))
						features.DepthFunction = DepthComparisonFunction::Never;
					if (ImGui::MenuItem("Always"))
						features.DepthFunction = DepthComparisonFunction::Always;

					ImGui::EndCombo();
				}

				ImGui::PopID();
			}

			ImGui::EndDisabled();

			EditorGUI::EndPropertyGrid();
		}

		EditorShaderCache* shaderCache = (EditorShaderCache*)ShaderCacheManager::GetInstance().get();

		Ref<const ShaderMetadata> shaderEntry = shaderCache->FindShaderMetadata(shaderHandle);
		if (shaderEntry)
		{
			const ShaderProperties& properties = shaderEntry->Properties;

			SerializablePropertyRenderer propertyRenderer;
			for (size_t propertyIndex = 0; propertyIndex < properties.size(); propertyIndex++)
			{
				const ShaderProperty& property = properties[propertyIndex];
				if (property.Hidden)
					continue;

				propertyRenderer.PropertyKey(property.Name);
				switch (property.Type)
				{
				case ShaderDataType::Int:
					propertyRenderer.Serialize(SerializationValue(material->GetPropertyValue<int32_t>((uint32_t)propertyIndex), property.Flags));
					break;
				case ShaderDataType::Int2:
					propertyRenderer.Serialize(SerializationValue(material->GetPropertyValue<glm::ivec2>((uint32_t)propertyIndex), property.Flags));
					break;
				case ShaderDataType::Int3:
					propertyRenderer.Serialize(SerializationValue(material->GetPropertyValue<glm::ivec3>((uint32_t)propertyIndex), property.Flags));
					break;
				case ShaderDataType::Int4:
					propertyRenderer.Serialize(SerializationValue(material->GetPropertyValue<glm::ivec4>((uint32_t)propertyIndex), property.Flags));
					break;

				case ShaderDataType::Float:
					propertyRenderer.Serialize(SerializationValue(material->GetPropertyValue<float>((uint32_t)propertyIndex), property.Flags));
					break;
				case ShaderDataType::Float2:
					propertyRenderer.Serialize(SerializationValue(material->GetPropertyValue<glm::vec2>((uint32_t)propertyIndex), property.Flags));
					break;
				case ShaderDataType::Float3:
					propertyRenderer.Serialize(SerializationValue(material->GetPropertyValue<glm::vec3>((uint32_t)propertyIndex), property.Flags));
					break;
				case ShaderDataType::Float4:
					propertyRenderer.Serialize(SerializationValue(material->GetPropertyValue<glm::vec4>((uint32_t)propertyIndex), property.Flags));
					break;

				case ShaderDataType::Sampler:
				{
					auto& value = material->GetPropertyValue<TexturePropertyValue>((uint32_t)propertyIndex);
					FLARE_CORE_ASSERT(value.ValueType == TexturePropertyValue::Type::Texture);

					propertyRenderer.Serialize(SerializationValue(value.Texture, property.Flags));
					break;
				}
				}
			}

			if (propertyRenderer.PropertiesGridStarted())
				EditorGUI::EndPropertyGrid();
		}

		return false;
	}
}