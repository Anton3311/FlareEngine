#include "ImGuiLayerVulkan.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Core/Application.h"

#include "Flare/Renderer/DescriptorSet.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanFrameBuffer.h"
#include "Flare/Platform/Vulkan/VulkanRenderPass.h"
#include "Flare/Platform/Vulkan/VulkanTexture.h"

#include "FlareEditor/ImGui/ImGuiVulkanRenderer.h"

#include <ImGuizmo.h>

#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>

namespace Flare
{
	static void RendererCreateWindow(ImGuiViewport* viewport)
	{
		FLARE_PROFILE_FUNCTION();
		ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
		VkSurfaceKHR surface = VK_NULL_HANDLE;
		VK_CHECK_RESULT((VkResult)platformIO.Platform_CreateVkSurface(viewport,
			(ImU64)VulkanContext::GetInstance().GetVulkanInstance(),
			nullptr,
			(ImU64*)&surface));

		ImGuiVulkanRenderer* vulkanRenderer = new ImGuiVulkanRenderer(surface);
		viewport->RendererUserData = (void*)vulkanRenderer;

		vulkanRenderer->Create(viewport);
	}

	static void RendererDestroyWindow(ImGuiViewport* viewport)
	{
		FLARE_PROFILE_FUNCTION();
		ImGuiVulkanRenderer* vulkanRenderer = (ImGuiVulkanRenderer*)viewport->RendererUserData;
		delete vulkanRenderer;

		viewport->RendererUserData = nullptr;
	}

	static void RendererSetWindowSize(ImGuiViewport* viewport, ImVec2 size)
	{
		FLARE_PROFILE_FUNCTION();
		ImGuiVulkanRenderer* vulkanRenderer = (ImGuiVulkanRenderer*)viewport->RendererUserData;
		vulkanRenderer->SetSize(glm::uvec2((uint32_t)size.x, (uint32_t)size.y));
	}

	static void RendererSwapBuffers(ImGuiViewport* viewport, void* renderArgs)
	{
		FLARE_PROFILE_FUNCTION();
		ImGuiVulkanRenderer* vulkanRenderer = (ImGuiVulkanRenderer*)viewport->RendererUserData;
		vulkanRenderer->Present();
	}

	static void RendererRenderWindow(ImGuiViewport* viewport, void* renderArgs)
	{
		FLARE_PROFILE_FUNCTION();
		ImGuiVulkanRenderer* vulkanRenderer = (ImGuiVulkanRenderer*)viewport->RendererUserData;
		vulkanRenderer->RenderWindow(viewport, renderArgs);
	}

	void ImGuiLayerVulkan::InitializeRenderer()
	{
		FLARE_PROFILE_FUNCTION();

		{
			VkDescriptorPoolSize sizes[1];
			sizes[0].descriptorCount = 1;
			sizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

			VkDescriptorPoolCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			info.maxSets = 20;
			info.poolSizeCount = 1;
			info.pPoolSizes = sizes;
			info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

			VK_CHECK_RESULT(vkCreateDescriptorPool(VulkanContext::GetInstance().GetDevice(), &info, nullptr, &m_DescriptorPool));
		}

		Application& application = Application::GetInstance();
		Ref<Window> window = application.GetWindow();

		ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)window->GetNativeWindow(), true);

		ImGuiIO& io = ImGui::GetIO();
		io.BackendRendererName = "FlareEngine";
		io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasViewports;

		ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
		platformIO.Renderer_CreateWindow = RendererCreateWindow;
		platformIO.Renderer_DestroyWindow = RendererDestroyWindow;
		platformIO.Renderer_SetWindowSize = RendererSetWindowSize;
		platformIO.Renderer_SwapBuffers = RendererSwapBuffers;
		platformIO.Renderer_RenderWindow = RendererRenderWindow;

		VulkanContext::GetInstance().SetImageViewDeletionHandler([this](VkImageView imageView)
		{
			auto it = m_ImageToDescriptor.find((uint64_t)imageView);
			if (it != m_ImageToDescriptor.end())
			{
				vkFreeDescriptorSets(VulkanContext::GetInstance().GetDevice(), m_DescriptorPool, 1, &it->second);
				m_ImageToDescriptor.erase(it);
			}
		});

		m_MainViewportFrameResources.resize(VulkanContext::GetInstance().GetFrameInFlightCount());

		CreateDescriptorSetLayout();
		CreatePipelineLayout();
		CreatePipeline();
	}

	void ImGuiLayerVulkan::ShutdownRenderer()
	{
		FLARE_PROFILE_FUNCTION();
		VulkanContext::GetInstance().SetImageViewDeletionHandler(nullptr);

		ImGui::DestroyPlatformWindows();

		m_MainViewportFrameResources.clear();
		m_FontsTexture = nullptr;

		VkDevice device = VulkanContext::GetInstance().GetDevice();
		vkDestroyPipeline(device, m_Pipeline, nullptr);
		vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);

		vkFreeDescriptorSets(device, m_DescriptorPool, 1, &m_FontsTextureDescriptor);
		m_FontsTextureDescriptor = VK_NULL_HANDLE;

		ImGui_ImplGlfw_Shutdown();
		vkDestroyDescriptorPool(VulkanContext::GetInstance().GetDevice(), m_DescriptorPool, nullptr);
	}

	void ImGuiLayerVulkan::InitializeFonts()
	{
		FLARE_PROFILE_FUNCTION();
	}

	void ImGuiLayerVulkan::Begin()
	{
		FLARE_PROFILE_FUNCTION();
		if (m_FontsTexture == nullptr)
		{
			CreateFontsTexture();
		}

		ImGui_ImplGlfw_NewFrame();

		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
	}

	void ImGuiLayerVulkan::End()
	{
		FLARE_PROFILE_FUNCTION();
		ImGui::EndFrame();
		ImGui::Render();
	}

	void ImGuiLayerVulkan::RenderCurrentWindow()
	{
		FLARE_PROFILE_FUNCTION();
		ImDrawData* drawData = ImGui::GetDrawData();
		if (drawData == nullptr)
			return;

		VulkanContext& context = VulkanContext::GetInstance();

		Ref<VulkanCommandBuffer> commandBuffer = context.GetPrimaryCommandBuffer();
		commandBuffer->BeginRenderPass(context.GetColorOnlyPass(), context.GetSwapChainFrameBuffer(context.GetCurrentFrameInFlight()));

		ImGuiVulkanRenderer::RenderViewportData(
			drawData,
			commandBuffer->GetHandle(),
			m_MainViewportFrameResources[VulkanContext::GetInstance().GetCurrentFrameInFlight()]);

		commandBuffer->EndRenderPass();
	}

	void ImGuiLayerVulkan::RenderWindows()
	{
		FLARE_PROFILE_FUNCTION();

		RenderCurrentWindow();

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::RenderPlatformWindowsDefault();
		}
	}

	void ImGuiLayerVulkan::UpdateWindows()
	{
		FLARE_PROFILE_FUNCTION();

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
		}
	}

	ImTextureID ImGuiLayerVulkan::GetTextureId(const Ref<const Texture>& texture)
	{
		FLARE_CORE_ASSERT(texture);

		Ref<const VulkanTexture> vulkanTexture = As<const VulkanTexture>(texture);
		return GetImageId(vulkanTexture->GetImageViewHandle(), vulkanTexture->GetDefaultSampler());
	}

	ImTextureID ImGuiLayerVulkan::GetFrameBufferAttachmentId(const Ref<const FrameBuffer>& frameBuffer, uint32_t attachment)
	{
		FLARE_CORE_ASSERT(frameBuffer);
		FLARE_CORE_ASSERT(attachment < frameBuffer->GetAttachmentsCount());

		Ref<const VulkanFrameBuffer> vulkanFrameBuffer = As<const VulkanFrameBuffer>(frameBuffer);
		return GetImageId(vulkanFrameBuffer->GetAttachmentImageView(attachment), vulkanFrameBuffer->GetDefaultAttachmentSampler(attachment));
	}

	ImTextureID ImGuiLayerVulkan::GetImageId(VkImageView imageView, VkSampler defaultSampler)
	{
		auto it = m_ImageToDescriptor.find((uint64_t)imageView);
		if (it == m_ImageToDescriptor.end())
		{
			VkDevice device = VulkanContext::GetInstance().GetDevice();
			VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
			VkDescriptorSetAllocateInfo descriptorAllocation = {};
			descriptorAllocation.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descriptorAllocation.descriptorPool = m_DescriptorPool;
			descriptorAllocation.descriptorSetCount = 1;
			descriptorAllocation.pSetLayouts = &m_DescriptorSetLayout;
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorAllocation, &descriptorSet));

			VkDescriptorImageInfo descriptorImage[1] = {};
			descriptorImage[0].sampler = defaultSampler;
			descriptorImage[0].imageView = imageView;
			descriptorImage[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkWriteDescriptorSet write[1] = {};
			write[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write[0].dstSet = descriptorSet;
			write[0].descriptorCount = 1;
			write[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			write[0].pImageInfo = descriptorImage;
			vkUpdateDescriptorSets(device, 1, write, 0, nullptr);

			m_ImageToDescriptor.emplace((uint64_t)imageView, descriptorSet);

			FLARE_CORE_ASSERT(descriptorSet);

			return (ImTextureID)descriptorSet;
		}

		return (ImTextureID)it->second;
	}

	// From ImGui Vulkan implementation

	// glsl_shader.vert, compiled with:
	// # glslangValidator -V -x -o glsl_shader.vert.u32 glsl_shader.vert
	/*
	#version 450 core
	layout(location = 0) in vec2 aPos;
	layout(location = 1) in vec2 aUV;
	layout(location = 2) in vec4 aColor;
	layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;

	out gl_PerVertex { vec4 gl_Position; };
	layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;

	void main()
	{
		Out.Color = aColor;
		Out.UV = aUV;
		gl_Position = vec4(aPos * pc.uScale + pc.uTranslate, 0, 1);
	}
	*/
	static uint32_t __glsl_shader_vert_spv[] =
	{
		0x07230203,0x00010000,0x00080001,0x0000002e,0x00000000,0x00020011,0x00000001,0x0006000b,
		0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
		0x000a000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000b,0x0000000f,0x00000015,
		0x0000001b,0x0000001c,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
		0x00000000,0x00030005,0x00000009,0x00000000,0x00050006,0x00000009,0x00000000,0x6f6c6f43,
		0x00000072,0x00040006,0x00000009,0x00000001,0x00005655,0x00030005,0x0000000b,0x0074754f,
		0x00040005,0x0000000f,0x6c6f4361,0x0000726f,0x00030005,0x00000015,0x00565561,0x00060005,
		0x00000019,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x00000019,0x00000000,
		0x505f6c67,0x7469736f,0x006e6f69,0x00030005,0x0000001b,0x00000000,0x00040005,0x0000001c,
		0x736f5061,0x00000000,0x00060005,0x0000001e,0x73755075,0x6e6f4368,0x6e617473,0x00000074,
		0x00050006,0x0000001e,0x00000000,0x61635375,0x0000656c,0x00060006,0x0000001e,0x00000001,
		0x61725475,0x616c736e,0x00006574,0x00030005,0x00000020,0x00006370,0x00040047,0x0000000b,
		0x0000001e,0x00000000,0x00040047,0x0000000f,0x0000001e,0x00000002,0x00040047,0x00000015,
		0x0000001e,0x00000001,0x00050048,0x00000019,0x00000000,0x0000000b,0x00000000,0x00030047,
		0x00000019,0x00000002,0x00040047,0x0000001c,0x0000001e,0x00000000,0x00050048,0x0000001e,
		0x00000000,0x00000023,0x00000000,0x00050048,0x0000001e,0x00000001,0x00000023,0x00000008,
		0x00030047,0x0000001e,0x00000002,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,
		0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040017,
		0x00000008,0x00000006,0x00000002,0x0004001e,0x00000009,0x00000007,0x00000008,0x00040020,
		0x0000000a,0x00000003,0x00000009,0x0004003b,0x0000000a,0x0000000b,0x00000003,0x00040015,
		0x0000000c,0x00000020,0x00000001,0x0004002b,0x0000000c,0x0000000d,0x00000000,0x00040020,
		0x0000000e,0x00000001,0x00000007,0x0004003b,0x0000000e,0x0000000f,0x00000001,0x00040020,
		0x00000011,0x00000003,0x00000007,0x0004002b,0x0000000c,0x00000013,0x00000001,0x00040020,
		0x00000014,0x00000001,0x00000008,0x0004003b,0x00000014,0x00000015,0x00000001,0x00040020,
		0x00000017,0x00000003,0x00000008,0x0003001e,0x00000019,0x00000007,0x00040020,0x0000001a,
		0x00000003,0x00000019,0x0004003b,0x0000001a,0x0000001b,0x00000003,0x0004003b,0x00000014,
		0x0000001c,0x00000001,0x0004001e,0x0000001e,0x00000008,0x00000008,0x00040020,0x0000001f,
		0x00000009,0x0000001e,0x0004003b,0x0000001f,0x00000020,0x00000009,0x00040020,0x00000021,
		0x00000009,0x00000008,0x0004002b,0x00000006,0x00000028,0x00000000,0x0004002b,0x00000006,
		0x00000029,0x3f800000,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,
		0x00000005,0x0004003d,0x00000007,0x00000010,0x0000000f,0x00050041,0x00000011,0x00000012,
		0x0000000b,0x0000000d,0x0003003e,0x00000012,0x00000010,0x0004003d,0x00000008,0x00000016,
		0x00000015,0x00050041,0x00000017,0x00000018,0x0000000b,0x00000013,0x0003003e,0x00000018,
		0x00000016,0x0004003d,0x00000008,0x0000001d,0x0000001c,0x00050041,0x00000021,0x00000022,
		0x00000020,0x0000000d,0x0004003d,0x00000008,0x00000023,0x00000022,0x00050085,0x00000008,
		0x00000024,0x0000001d,0x00000023,0x00050041,0x00000021,0x00000025,0x00000020,0x00000013,
		0x0004003d,0x00000008,0x00000026,0x00000025,0x00050081,0x00000008,0x00000027,0x00000024,
		0x00000026,0x00050051,0x00000006,0x0000002a,0x00000027,0x00000000,0x00050051,0x00000006,
		0x0000002b,0x00000027,0x00000001,0x00070050,0x00000007,0x0000002c,0x0000002a,0x0000002b,
		0x00000028,0x00000029,0x00050041,0x00000011,0x0000002d,0x0000001b,0x0000000d,0x0003003e,
		0x0000002d,0x0000002c,0x000100fd,0x00010038
	};

	// glsl_shader.frag, compiled with:
	// # glslangValidator -V -x -o glsl_shader.frag.u32 glsl_shader.frag
	/*
	#version 450 core
	layout(location = 0) out vec4 fColor;
	layout(set=0, binding=0) uniform sampler2D sTexture;
	layout(location = 0) in struct { vec4 Color; vec2 UV; } In;
	void main()
	{
		fColor = In.Color * texture(sTexture, In.UV.st);
	}
	*/
	static uint32_t __glsl_shader_frag_spv[] =
	{
		0x07230203,0x00010000,0x00080001,0x0000001e,0x00000000,0x00020011,0x00000001,0x0006000b,
		0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
		0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000d,0x00030010,
		0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
		0x00000000,0x00040005,0x00000009,0x6c6f4366,0x0000726f,0x00030005,0x0000000b,0x00000000,
		0x00050006,0x0000000b,0x00000000,0x6f6c6f43,0x00000072,0x00040006,0x0000000b,0x00000001,
		0x00005655,0x00030005,0x0000000d,0x00006e49,0x00050005,0x00000016,0x78655473,0x65727574,
		0x00000000,0x00040047,0x00000009,0x0000001e,0x00000000,0x00040047,0x0000000d,0x0000001e,
		0x00000000,0x00040047,0x00000016,0x00000022,0x00000000,0x00040047,0x00000016,0x00000021,
		0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,
		0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,0x00000003,
		0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,0x00040017,0x0000000a,0x00000006,
		0x00000002,0x0004001e,0x0000000b,0x00000007,0x0000000a,0x00040020,0x0000000c,0x00000001,
		0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000001,0x00040015,0x0000000e,0x00000020,
		0x00000001,0x0004002b,0x0000000e,0x0000000f,0x00000000,0x00040020,0x00000010,0x00000001,
		0x00000007,0x00090019,0x00000013,0x00000006,0x00000001,0x00000000,0x00000000,0x00000000,
		0x00000001,0x00000000,0x0003001b,0x00000014,0x00000013,0x00040020,0x00000015,0x00000000,
		0x00000014,0x0004003b,0x00000015,0x00000016,0x00000000,0x0004002b,0x0000000e,0x00000018,
		0x00000001,0x00040020,0x00000019,0x00000001,0x0000000a,0x00050036,0x00000002,0x00000004,
		0x00000000,0x00000003,0x000200f8,0x00000005,0x00050041,0x00000010,0x00000011,0x0000000d,
		0x0000000f,0x0004003d,0x00000007,0x00000012,0x00000011,0x0004003d,0x00000014,0x00000017,
		0x00000016,0x00050041,0x00000019,0x0000001a,0x0000000d,0x00000018,0x0004003d,0x0000000a,
		0x0000001b,0x0000001a,0x00050057,0x00000007,0x0000001c,0x00000017,0x0000001b,0x00050085,
		0x00000007,0x0000001d,0x00000012,0x0000001c,0x0003003e,0x00000009,0x0000001d,0x000100fd,
		0x00010038
	};

	static VkShaderModule CreateShaderModule(const uint32_t* code, uint32_t codeSize)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.pCode = code;
		createInfo.codeSize = codeSize;

		VkShaderModule shaderModule = VK_NULL_HANDLE;
		VK_CHECK_RESULT(vkCreateShaderModule(VulkanContext::GetInstance().GetDevice(), &createInfo, nullptr, &shaderModule));

		return shaderModule;
	}
	
	void ImGuiLayerVulkan::CreatePipeline()
	{
		FLARE_PROFILE_FUNCTION();
		VkShaderModule vertexShaderModule = CreateShaderModule(__glsl_shader_vert_spv, sizeof(__glsl_shader_vert_spv));
		VkShaderModule fragmentShaderModule = CreateShaderModule(__glsl_shader_frag_spv, sizeof(__glsl_shader_frag_spv));

		VkPipelineShaderStageCreateInfo stage[2] = {};
		stage[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stage[0].module = vertexShaderModule;
		stage[0].pName = "main";
		stage[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stage[1].module = fragmentShaderModule;
		stage[1].pName = "main";

		VkVertexInputBindingDescription binding_desc[1] = {};
		binding_desc[0].stride = sizeof(ImDrawVert);
		binding_desc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription attribute_desc[3] = {};
		attribute_desc[0].location = 0;
		attribute_desc[0].binding = binding_desc[0].binding;
		attribute_desc[0].format = VK_FORMAT_R32G32_SFLOAT;
		attribute_desc[0].offset = IM_OFFSETOF(ImDrawVert, pos);
		attribute_desc[1].location = 1;
		attribute_desc[1].binding = binding_desc[0].binding;
		attribute_desc[1].format = VK_FORMAT_R32G32_SFLOAT;
		attribute_desc[1].offset = IM_OFFSETOF(ImDrawVert, uv);
		attribute_desc[2].location = 2;
		attribute_desc[2].binding = binding_desc[0].binding;
		attribute_desc[2].format = VK_FORMAT_R8G8B8A8_UNORM;
		attribute_desc[2].offset = IM_OFFSETOF(ImDrawVert, col);

		VkPipelineVertexInputStateCreateInfo vertex_info = {};
		vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_info.vertexBindingDescriptionCount = 1;
		vertex_info.pVertexBindingDescriptions = binding_desc;
		vertex_info.vertexAttributeDescriptionCount = 3;
		vertex_info.pVertexAttributeDescriptions = attribute_desc;

		VkPipelineInputAssemblyStateCreateInfo ia_info = {};
		ia_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		ia_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkPipelineViewportStateCreateInfo viewport_info = {};
		viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_info.viewportCount = 1;
		viewport_info.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo raster_info = {};
		raster_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		raster_info.polygonMode = VK_POLYGON_MODE_FILL;
		raster_info.cullMode = VK_CULL_MODE_NONE;
		raster_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		raster_info.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo ms_info = {};
		ms_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		ms_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState color_attachment[1] = {};
		color_attachment[0].blendEnable = VK_TRUE;
		color_attachment[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_attachment[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		color_attachment[0].colorBlendOp = VK_BLEND_OP_ADD;
		color_attachment[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		color_attachment[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		color_attachment[0].alphaBlendOp = VK_BLEND_OP_ADD;
		color_attachment[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		VkPipelineDepthStencilStateCreateInfo depth_info = {};
		depth_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

		VkPipelineColorBlendStateCreateInfo blend_info = {};
		blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		blend_info.attachmentCount = 1;
		blend_info.pAttachments = color_attachment;

		VkDynamicState dynamic_states[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamic_state = {};
		dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state.dynamicStateCount = (uint32_t)IM_ARRAYSIZE(dynamic_states);
		dynamic_state.pDynamicStates = dynamic_states;

		VkGraphicsPipelineCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		info.flags = 0;
		info.stageCount = 2;
		info.pStages = stage;
		info.pVertexInputState = &vertex_info;
		info.pInputAssemblyState = &ia_info;
		info.pViewportState = &viewport_info;
		info.pRasterizationState = &raster_info;
		info.pMultisampleState = &ms_info;
		info.pDepthStencilState = &depth_info;
		info.pColorBlendState = &blend_info;
		info.pDynamicState = &dynamic_state;
		info.layout = m_PipelineLayout;
		info.renderPass = VulkanContext::GetInstance().GetColorOnlyPass()->GetHandle();
		info.subpass = 0;

		VkDevice device = VulkanContext::GetInstance().GetDevice();
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, &m_Pipeline));

		vkDestroyShaderModule(device, vertexShaderModule, nullptr);
		vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
	}

	void ImGuiLayerVulkan::CreatePipelineLayout()
	{
		FLARE_PROFILE_FUNCTION();

		VkPushConstantRange push_constants[1] = {};
		push_constants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		push_constants[0].offset = sizeof(float) * 0;
		push_constants[0].size = sizeof(float) * 4;

		VkDescriptorSetLayout set_layout[1] = { m_DescriptorSetLayout };
		VkPipelineLayoutCreateInfo layout_info = {};
		layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layout_info.setLayoutCount = 1;
		layout_info.pSetLayouts = set_layout;
		layout_info.pushConstantRangeCount = 1;
		layout_info.pPushConstantRanges = push_constants;

		VK_CHECK_RESULT(vkCreatePipelineLayout(VulkanContext::GetInstance().GetDevice(), &layout_info, nullptr, &m_PipelineLayout));
	}

	void ImGuiLayerVulkan::CreateDescriptorSetLayout()
	{
		FLARE_PROFILE_FUNCTION();

        VkDescriptorSetLayoutBinding binding[1] = {};
        binding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding[0].descriptorCount = 1;
        binding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = 1;
        info.pBindings = binding;

        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(VulkanContext::GetInstance().GetDevice(), &info, nullptr, &m_DescriptorSetLayout));
	}

	void ImGuiLayerVulkan::CreateFontsTexture()
	{
		FLARE_PROFILE_FUNCTION();

		uint8_t* pixels = nullptr;
		int32_t width = 0;
		int32_t height = 0;

		auto& fonts = ImGui::GetIO().Fonts;
		fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
		size_t pixelDataSie = width * height * 4;

		TextureSpecifications specifications{};
		specifications.Width = (uint32_t)width;
		specifications.Height = (uint32_t)height;
		specifications.Format = TextureFormat::RGBA8;
		specifications.Filtering = TextureFiltering::Linear;
		specifications.Wrap = TextureWrap::Repeat;
		specifications.Usage = TextureUsage::Sampling;
		
		m_FontsTexture = Texture::Create(specifications, MemorySpan::FromRawBytes(pixels, pixelDataSie));
		m_FontsTextureDescriptor = (VkDescriptorSet)GetTextureId(m_FontsTexture);

		fonts->SetTexID(m_FontsTextureDescriptor);
	}
}
