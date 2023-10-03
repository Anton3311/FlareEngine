local vulkan_sdk = os.getenv("VULKAN_SDK")

if vulkan_sdk == nil then
	print("Couldn't find Vulkan SDK")
end

print(vulkan_sdk)

local vulkan_include = vulkan_sdk .. "/include/"
local vulkan_lib = vulkan_sdk .. "/lib/"

INCLUDE_DIRS = {
	GLAD = "%{wks.location}/Flare/vendor/GLAD/include",
	GLFW = "%{wks.location}/Flare/vendor/GLFW/include",
	glm = "%{wks.location}/Flare/vendor/glm/",
	stb_image = "%{wks.location}/Flare/vendor/stb_image",
	spdlog = "%{wks.location}/Flare/vendor/spdlog/include",
	imgui = "%{wks.location}/Flare/vendor/ImGUI/",
	imguizmo = "%{wks.location}/Flare/vendor/ImGuizmo/",
	yaml_cpp = "%{wks.location}/Flare/vendor/yaml-cpp/include/",
	vulkan_sdk = vulkan_include,
}

LIBRARIES = {
	vulkan = vulkan_lib .. "vulkan-1.lib",
	shaderc_debug = vulkan_lib .. "shaderc_sharedd.lib",
	spriv_cross_debug = vulkan_lib .. "spirv-cross-cored.lib",
	spriv_cross_glsl_debug = vulkan_lib .. "spirv-cross-glsld.lib",
	spriv_tools_debug = vulkan_lib .. "spirv-toolsd.lib",

	shaderc_release = vulkan_lib .. "shaderc_shared.lib",
	spriv_cross_release = vulkan_lib .. "spirv-cross-core.lib",
	spriv_cross_glsl_release = vulkan_lib .. "spirv-cross-glsl.lib",
}
