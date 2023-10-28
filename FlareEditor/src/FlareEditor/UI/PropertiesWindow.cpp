#include "PropertiesWindow.h"

#include "Flare/Scene/Scene.h"
#include "Flare/Scripting/ScriptingEngine.h"

#include "Flare/Renderer/Material.h"

#include "FlareEditor/UI/EditorGUI.h"
#include "FlareEditor/UI/ECS/EntityProperties.h"

#include "FlareEditor/EditorLayer.h"
#include "FlareEditor/AssetManager/MaterialImporter.h"

#include <imgui.h>

namespace Flare
{
	TextureImportSettings s_SelectedTextureImportSettings;

	PropertiesWindow::PropertiesWindow(AssetManagerWindow& assetManagerWindow)
		: m_AssetManagerWindow(assetManagerWindow) {}

	void PropertiesWindow::OnAttach()
	{
		m_AssetManagerWindow.OnAssetSelectionChanged.Bind([](AssetHandle handle)
		{
			if (!AssetManager::IsAssetHandleValid(handle))
				return;

			switch (AssetManager::GetAssetMetadata(handle)->Type)
			{
			case AssetType::Texture:
				s_SelectedTextureImportSettings = TextureImportSettings();
				TextureImporter::DeserializeImportSettings(handle, s_SelectedTextureImportSettings);
				break;
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
			RenderTextureImportSettingsEditor(handle, s_SelectedTextureImportSettings);
			break;
		case AssetType::Material:
			RenderMaterialEditor(handle);
			break;
		}
	}

	bool PropertiesWindow::RenderTextureImportSettingsEditor(AssetHandle handle, TextureImportSettings& importSettings)
	{
		FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(handle));
		FLARE_CORE_ASSERT(AssetManager::GetAssetMetadata(handle)->Type == AssetType::Texture);

		if (ImGui::Button("Save"))
		{
			TextureImporter::SerialiazeImportSettings(handle, importSettings);
			As<EditorAssetManager>(AssetManager::GetInstance())->ReloadAsset(handle);
		}

		if (ImGui::BeginChild("Texture Import Settings"))
		{
			if (EditorGUI::BeginPropertyGrid())
			{
				const char* previewText = TextureFilteringToString(importSettings.Filtering);

				EditorGUI::PropertyName("Filtering");

				{
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
					previewText = TextureWrapToString(importSettings.WrapMode);
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

				EditorGUI::EndPropertyGrid();
			}

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

		if (EditorGUI::BeginPropertyGrid())
		{
			AssetHandle shaderHandle = material->GetShader()->Handle;
			EditorGUI::AssetField("Shader", shaderHandle);

			if (shaderHandle != material->GetShader()->Handle)
			{
				if (AssetManager::IsAssetHandleValid(shaderHandle))
				{
					if (AssetManager::GetAssetMetadata(shaderHandle)->Type == AssetType::Shader)
						material->SetShader(AssetManager::GetAsset<Shader>(shaderHandle));
					else
						FLARE_CORE_ERROR("Provided asset is not a shader");
				}
			}

			EditorGUI::BoolPropertyField("Depth Test", material->Features.DepthTesting);
			EditorGUI::PropertyName("Culling Mode");

			const char* preview = CullingModeToString(material->Features.Culling);
			
			ImGui::PushID("CullingMode");
			if (ImGui::BeginCombo("", preview, ImGuiComboFlags_HeightRegular))
			{
				if (ImGui::MenuItem("None"))
					material->Features.Culling = CullingMode::None;
				if (ImGui::MenuItem("Front"))
					material->Features.Culling = CullingMode::Front;
				if (ImGui::MenuItem("Back"))
					material->Features.Culling = CullingMode::Back;

				ImGui::EndCombo();
			}

			ImGui::PopID();

			EditorGUI::EndPropertyGrid();
		}

		if (EditorGUI::BeginPropertyGrid())
		{
			Ref<Shader> shader = material->GetShader();
			const ShaderProperties& shaderProperties = shader->GetProperties();
			for (uint32_t i = 0; i < (uint32_t)shaderProperties.size(); i++)
			{
				switch (shaderProperties[i].Type)
				{
				case ShaderDataType::Int:
				{
					int32_t value = material->ReadPropertyValue<int32_t>(i);
					if (EditorGUI::IntPropertyField(shaderProperties[i].Name.c_str(), value))
						material->WritePropertyValue(i, value);

					break;
				}
				case ShaderDataType::Float:
				{
					float value = material->ReadPropertyValue<float>(i);
					if (EditorGUI::FloatPropertyField(shaderProperties[i].Name.c_str(), value))
						material->WritePropertyValue(i, value);

					break;
				}
				case ShaderDataType::Float2:
				{
					glm::vec2 value = material->ReadPropertyValue<glm::vec2>(i);
					if (EditorGUI::Vector2PropertyField(shaderProperties[i].Name.c_str(), value))
						material->WritePropertyValue(i, value);

					break;
				}
				case ShaderDataType::Float3:
				{
					glm::vec3 value = material->ReadPropertyValue<glm::vec3>(i);
					if (EditorGUI::Vector3PropertyField(shaderProperties[i].Name.c_str(), value))
						material->WritePropertyValue(i, value);

					break;
				}
				case ShaderDataType::Float4:
				{
					glm::vec4 value = material->ReadPropertyValue<glm::vec4>(i);
					if (EditorGUI::Vector4PropertyField(shaderProperties[i].Name.c_str(), value))
						material->WritePropertyValue(i, value);

					break;
				}
				default:
					FLARE_CORE_ASSERT("Unhandled shader data type");
				}
			}

			EditorGUI::EndPropertyGrid();
		}

		return false;
	}
}