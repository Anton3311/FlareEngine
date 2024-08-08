#include "ShaderLibraryWindow.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Core/Application.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/ComputeShader.h"
#include "Flare/Renderer/Shader.h"
#include "Flare/Renderer/ShaderLibrary.h"
#include "Flare/Renderer/ShaderMetadata.h"

#include "FlareEditor/ImGui/ImGuiLayer.h"
#include "FlareEditor/UI/EditorGUI.h"
#include "FlareEditor/AssetManager/EditorAssetManager.h"

namespace Flare
{
	static ImGuiTreeNodeFlags s_DefaultTreeNodeFlags = ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanFullWidth;
	static ShaderLibraryWindow s_Instance;

	void ShaderLibraryWindow::OnRenderImGui()
	{
		FLARE_PROFILE_FUNCTION();
		if (m_Show && ImGui::Begin("Shader Library", &m_Show))
		{
			const ImGuiStyle& style = ImGui::GetStyle();
			for (const auto& [name, handle] : ShaderLibrary::GetNameToHandleMap())
			{
				if (ImGui::TreeNodeEx(name.c_str(), s_DefaultTreeNodeFlags))
				{
					ImGui::TreePop();
				}

				if (ImGui::IsItemClicked())
				{
					m_SelectedShader = handle;
				}
			}

			if (AssetManager::IsAssetHandleValid(m_SelectedShader))
			{
				ImGui::Begin("Shader Metadata");

				const AssetMetadata* metadata = AssetManager::GetAssetMetadata(m_SelectedShader);
				FLARE_CORE_ASSERT(metadata);

				if (EditorGUI::BeginPropertyGrid())
				{
					RenderShaderAssetMetadata(metadata);
					EditorGUI::EndPropertyGrid();
				}

				if (metadata->Type == AssetType::Shader)
				{
					Ref<Shader> shader = AssetManager::GetAsset<Shader>(m_SelectedShader);

					if (shader)
						RenderGraphicsShaderMetadata(shader->GetMetadata());
				}
				else if (metadata->Type == AssetType::ComputeShader)
				{
					Ref<ComputeShader> shader = AssetManager::GetAsset<ComputeShader>(m_SelectedShader);

					if (shader)
						RenderComputeShaderMetadata(shader->GetMetadata());
				}

				ImGui::End();
			}

			ImGui::End();
		}
	}

	void ShaderLibraryWindow::Show()
	{
		s_Instance.m_Show = true;
	}

	ShaderLibraryWindow& ShaderLibraryWindow::GetInstance()
	{
		return s_Instance;
	}

	inline static void RenderTextProperty(const char* name, const char* text)
	{
		const ImGuiStyle& style = ImGui::GetStyle();

		EditorGUI::PropertyName(name);
		EditorGUI::MoveCursor(glm::vec2(0.0f, style.FramePadding.y));
		ImGui::TextUnformatted(text);
		EditorGUI::MoveCursor(glm::vec2(0.0f, style.FramePadding.y));
	}

	void ShaderLibraryWindow::RenderShaderAssetMetadata(const AssetMetadata* metadata)
	{
		FLARE_PROFILE_FUNCTION();

		const ImGuiStyle& style = ImGui::GetStyle();

		EditorGUI::PropertyName("Name");
		EditorGUI::MoveCursor(glm::vec2(0.0f, style.FramePadding.y));
		ImGui::Text("%s", metadata->Name.c_str());
		EditorGUI::MoveCursor(glm::vec2(0.0f, style.FramePadding.y));

		EditorGUI::PropertyName("Handle");
		EditorGUI::MoveCursor(glm::vec2(0.0f, style.FramePadding.y));
		ImGui::Text("%llu", (uint64_t)metadata->Handle);
		EditorGUI::MoveCursor(glm::vec2(0.0f, style.FramePadding.y));

		EditorGUI::PropertyName("Path");
		EditorGUI::MoveCursor(glm::vec2(0.0f, style.FramePadding.y));
		ImGui::Text("%s", metadata->Path.generic_string().c_str());
		EditorGUI::MoveCursor(glm::vec2(0.0f, style.FramePadding.y));
	}

	void ShaderLibraryWindow::RenderGraphicsShaderMetadata(Ref<const GraphicsShaderMetadata> metadata)
	{
		FLARE_PROFILE_FUNCTION();

		RenderShaderMetadata(metadata);
	}

	void ShaderLibraryWindow::RenderComputeShaderMetadata(Ref<const ComputeShaderMetadata> metadata)
	{
		FLARE_PROFILE_FUNCTION();

		const ImGuiStyle& style = ImGui::GetStyle();

		if (EditorGUI::BeginPropertyGrid())
		{
			EditorGUI::PropertyName("Local Group Size");
			EditorGUI::MoveCursor(glm::vec2(0, style.FramePadding.y));
			glm::uvec3 localGroupSize = metadata->LocalGroupSize;
			ImGui::Text("X: %u Y: %u Z: %u", localGroupSize.x, localGroupSize.y, localGroupSize.z);
			EditorGUI::MoveCursor(glm::vec2(0, style.FramePadding.y));

			EditorGUI::EndPropertyGrid();
		}
	}

	static const char* DescriptorTypeToString(ShaderDescriptorType type)
	{
		switch (type)
		{
		case ShaderDescriptorType::SampledImage:
			return "Sampler";
		case ShaderDescriptorType::StorageBuffer:
			return "StorageBuffer";
		case ShaderDescriptorType::StorageImage:
			return "StorageImage";
		case ShaderDescriptorType::UniformBuffer:
			return "UniformBuffer";
		}

		FLARE_CORE_ASSERT(false);
		return nullptr;
	}

	void ShaderLibraryWindow::RenderShaderMetadata(Ref<const ShaderMetadata> metadata)
	{
		FLARE_PROFILE_FUNCTION();

		if (metadata == nullptr)
		{
			ImGui::Text("Shader metadata is not available");
			return;
		}

		if (ImGui::TreeNodeEx("Descriptor Properties", ImGuiTreeNodeFlags_FramePadding))
		{
			for (const ShaderDescriptorProperty& property : metadata->DescriptorProperties)
			{
				ImGui::Separator();

				ImGui::BeginDisabled(true);
				if (EditorGUI::BeginPropertyGrid())
				{
					RenderTextProperty("Name", property.Name.c_str());

					int32_t descriptorCount = property.DescriptorCount;
					EditorGUI::IntPropertyField("Descriptor Count", descriptorCount);

					int32_t set = property.Set;
					EditorGUI::IntPropertyField("Set", set);

					int32_t binding = property.Binding;
					EditorGUI::IntPropertyField("Binding", binding);

					RenderTextProperty("Type", DescriptorTypeToString(property.Type));

					EditorGUI::EndPropertyGrid();
				}

				ImGui::EndDisabled();
			}

			ImGui::TreePop();
		}
	}
}
