#include "WindowsWindowControls.h"

#include "Flare/Core/Application.h"
#include "FlarePlatform/Window.h"

#include <imgui_internal.h>

namespace Flare
{
	bool WindowsWindowControls::BeginTitleBar()
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration 
			| ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		bool showTitleBar = ImGui::BeginViewportSideBar("TitleBar", viewport, ImGuiDir_Up, m_TitleBarHeight, flags);
		ImGui::PopStyleVar();

		return showTitleBar;
	}

	void WindowsWindowControls::EndTitleBar()
	{
		ImGui::End();
	}

	void WindowsWindowControls::RenderControls()
	{
		Ref<Window> window = Application::GetInstance().GetWindow();
		glm::uvec2 buttonSize = window->GetControlsButtonSize();

		auto* drawList = ImGui::GetWindowDrawList();
		const float iconSize = 0.35f;
		ImU32 iconColor;

		ImVec2 windowSize = ImVec2(window->GetProperties().Size.x, m_TitleBarHeight);

		ImGui::InvisibleButton("##TitleBarDragArea", ImVec2(windowSize.x - buttonSize.x * 3 - ImGui::GetCursorPosX(), m_TitleBarHeight));
		m_TitleBarHovered = ImGui::IsItemHovered();

		ImGui::SetCursorPos(ImVec2(windowSize.x - buttonSize.x * 3, 0));

		{
			if (RenderControlsButton("##min", iconColor))
				window->Hide();

			ImRect rect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
			ImVec2 itemSize = ImGui::GetItemRectSize();

			drawList->AddLine(rect.Min + ImVec2(itemSize.x * iconSize, itemSize.y / 2), rect.Min + ImVec2(itemSize.x * (1.0f - iconSize), itemSize.y / 2), iconColor);
		}

		{
			if (RenderControlsButton("##max", iconColor))
				window->SetMaximized(!window->GetProperties().IsMaximized);

			ImRect rect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
			ImVec2 itemSize = ImGui::GetItemRectSize();

			float quadHalfSize = rect.GetHeight() * 0.28f / 2.0f;
			ImVec2 rectCenter = (rect.Min + rect.Max) / 2.0f;

			ImVec2 quadMin = rectCenter - ImVec2(quadHalfSize, quadHalfSize);
			ImVec2 quadMax = rectCenter + ImVec2(quadHalfSize, quadHalfSize);

			ImVec2 points[4] =
			{
				quadMin, 
				ImVec2(quadMin.x, quadMax.y),
				quadMax,
				ImVec2(quadMax.x, quadMin.y),
			};

			if (window->GetProperties().IsMaximized)
			{
				const float offsetPercent = 0.08f;
				ImVec2 offset = ImVec2(-glm::ceil(buttonSize.y * offsetPercent), glm::ceil(buttonSize.y * offsetPercent));
				for (uint32_t i = 0; i < 4; i++)
					points[i] += offset;

				drawList->AddLine(points[0] - offset, points[3] - offset, iconColor);
				drawList->AddLine(points[2] - offset, points[3] - offset, iconColor);
				drawList->AddLine(points[0] - offset, points[0] + ImVec2(-offset.x, 0), iconColor);
				drawList->AddLine(points[2] - offset, points[2] + ImVec2(0, -offset.y), iconColor);
			}

			drawList->AddLine(points[0], points[1], iconColor);
			drawList->AddLine(points[1], points[2], iconColor);
			drawList->AddLine(points[2], points[3], iconColor);
			drawList->AddLine(points[3], points[0], iconColor);
		}

		{
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.1f, 1.0f));

			if (RenderControlsButton("##close", iconColor))
				Application::GetInstance().Close();

			ImRect rect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
			ImVec2 itemSize = ImGui::GetItemRectSize();

			float quadHalfSize = rect.GetHeight() * iconSize / 2.0f;
			ImVec2 rectCenter = (rect.Min + rect.Max) / 2.0f;

			ImVec2 quadMin = rectCenter - ImVec2(quadHalfSize, quadHalfSize);
			ImVec2 quadMax = rectCenter + ImVec2(quadHalfSize, quadHalfSize);

			drawList->AddLine(quadMin, quadMax, iconColor);
			drawList->AddLine(ImVec2(quadMin.x, quadMax.y), ImVec2(quadMax.x, quadMin.y), iconColor);

			ImGui::PopStyleColor();
		}
	}

	bool WindowsWindowControls::RenderControlsButton(const char* name, ImU32& iconColor)
	{
		bool result = false;

		ImVec2 cursorPosition = ImGui::GetCursorPos();
		Ref<Window> window = Application::GetInstance().GetWindow();
		glm::uvec2 buttonSize = window->GetControlsButtonSize();

		const ImGuiStyle& style = ImGui::GetStyle();

		auto* drawList = ImGui::GetWindowDrawList();
		ImU32 buttonColor = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_WindowBg]);
		iconColor = 0xffffffff;

		ImGui::InvisibleButton(name, ImVec2(buttonSize.x, buttonSize.y));
		if (ImGui::IsItemClicked())
		{
			buttonColor = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_ButtonActive]);
			result = true;
		}

		if (ImGui::IsItemHovered())
		{
			buttonColor = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_ButtonHovered]);
		}

		drawList->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), buttonColor);
		ImGui::SetCursorPos(cursorPosition + ImVec2(buttonSize.x, 0));

		return result;
	}
}