#include "VulkanPipeline.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanShader.h"

namespace Flare
{
	VulkanPipeline::VulkanPipeline(const PipelineSpecifications& specifications,
		const Ref<VulkanRenderPass>& renderPass,
		const Span<Ref<const DescriptorSetLayout>>& layouts,
		const Span<ShaderPushConstantsRange>& pushConstantsRanges)
		: m_Specifications(specifications), m_CompatbileRenderPass(renderPass)
	{
		CreatePipelineLayout(layouts, pushConstantsRanges);
		Create();
	}

	VulkanPipeline::VulkanPipeline(const PipelineSpecifications& specifications, const Ref<VulkanRenderPass>& renderPass)
		: m_Specifications(specifications), m_CompatbileRenderPass(renderPass), m_OwnsPipelineLayout(false)
	{
		FLARE_CORE_ASSERT(m_Specifications.Shader);
		FLARE_CORE_ASSERT(m_Specifications.Shader->IsLoaded());

		m_PipelineLayout = As<const VulkanShader>(m_Specifications.Shader)->GetPipelineLayout();

		Create();
	}

	VulkanPipeline::~VulkanPipeline()
	{
		FLARE_CORE_ASSERT(VulkanContext::GetInstance().IsValid());

		if (m_OwnsPipelineLayout)
			vkDestroyPipelineLayout(VulkanContext::GetInstance().GetDevice(), m_PipelineLayout, nullptr);

		vkDestroyPipeline(VulkanContext::GetInstance().GetDevice(), m_Pipeline, nullptr);
	}

	const PipelineSpecifications& Flare::VulkanPipeline::GetSpecifications() const
	{
		return m_Specifications;
	}

	void VulkanPipeline::CreatePipelineLayout(const Span<Ref<const DescriptorSetLayout>>& layouts, const Span<ShaderPushConstantsRange>& pushConstantsRanges)
	{
		VkPipelineLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.flags = 0;

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts(layouts.GetSize());
		for (size_t i = 0; i < layouts.GetSize(); i++)
		{
			descriptorSetLayouts[i] = As<const VulkanDescriptorSetLayout>(layouts[i])->GetHandle();
		}

		layoutInfo.setLayoutCount = (uint32_t)descriptorSetLayouts.size();
		layoutInfo.pSetLayouts = descriptorSetLayouts.data();

		std::vector<VkPushConstantRange> ranges(pushConstantsRanges.GetSize());
		for (size_t i = 0; i < pushConstantsRanges.GetSize(); i++)
		{
			switch (pushConstantsRanges[i].Stage)
			{
			case ShaderStageType::Vertex:
				ranges[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
				break;
			case ShaderStageType::Pixel:
				ranges[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
				break;
			}

			ranges[i].offset = (uint32_t)pushConstantsRanges[i].Offset;
			ranges[i].size = (uint32_t)pushConstantsRanges[i].Size;
		}

		layoutInfo.pushConstantRangeCount = (uint32_t)ranges.size();
		layoutInfo.pPushConstantRanges = ranges.data();

		VK_CHECK_RESULT(vkCreatePipelineLayout(VulkanContext::GetInstance().GetDevice(), &layoutInfo, nullptr, &m_PipelineLayout));
	}

	void VulkanPipeline::Create()
	{
		FLARE_CORE_ASSERT(m_CompatbileRenderPass);
		FLARE_CORE_ASSERT(m_PipelineLayout);

		std::vector<VkPipelineColorBlendAttachmentState> attachmentsBlendStates(m_CompatbileRenderPass->GetColorAttachmnetsCount());
		for (size_t i = 0; i < attachmentsBlendStates.size(); i++)
		{
			VkPipelineColorBlendAttachmentState& attachmentBlendState = attachmentsBlendStates[i];
			attachmentBlendState = {};
			attachmentBlendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

			switch (m_Specifications.Blending)
			{
			case BlendMode::Opaque:
				attachmentBlendState.blendEnable = VK_FALSE;
				attachmentBlendState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
				attachmentBlendState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
				attachmentBlendState.colorBlendOp = VK_BLEND_OP_ADD;
				attachmentBlendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
				attachmentBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
				attachmentBlendState.alphaBlendOp = VK_BLEND_OP_ADD;
				break;
			case BlendMode::Transparent:
				attachmentBlendState.blendEnable = VK_TRUE;
				attachmentBlendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
				attachmentBlendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				attachmentBlendState.colorBlendOp = VK_BLEND_OP_ADD;
				attachmentBlendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
				attachmentBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
				attachmentBlendState.alphaBlendOp = VK_BLEND_OP_ADD;
				break;
			}
		}

		VkPipelineColorBlendStateCreateInfo colorBlendState{};
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.attachmentCount = (uint32_t)attachmentsBlendStates.size();
		colorBlendState.pAttachments = attachmentsBlendStates.data();
		colorBlendState.logicOpEnable = VK_FALSE;
		colorBlendState.logicOp = VK_LOGIC_OP_COPY;

		VkCompareOp depthComapreFunction = VK_COMPARE_OP_LESS;
		switch (m_Specifications.DepthFunction)
		{
		case DepthComparisonFunction::Less:
			depthComapreFunction = VK_COMPARE_OP_LESS;
			break;
		case DepthComparisonFunction::LessOrEqual:
			depthComapreFunction = VK_COMPARE_OP_LESS_OR_EQUAL;
			break;
		case DepthComparisonFunction::Always:
			depthComapreFunction = VK_COMPARE_OP_ALWAYS;
			break;
		case DepthComparisonFunction::Never:
			depthComapreFunction = VK_COMPARE_OP_NEVER;
			break;
		case DepthComparisonFunction::Greater:
			depthComapreFunction = VK_COMPARE_OP_GREATER;
			break;
		case DepthComparisonFunction::GreaterOrEqual:
			depthComapreFunction = VK_COMPARE_OP_GREATER_OR_EQUAL;
			break;
		case DepthComparisonFunction::Equal:
			depthComapreFunction = VK_COMPARE_OP_EQUAL;
			break;
		case DepthComparisonFunction::NotEqual:
			depthComapreFunction = VK_COMPARE_OP_NOT_EQUAL;
			break;
		default:
			FLARE_CORE_ASSERT(false);
		}

		VkPipelineDepthStencilStateCreateInfo depthStencilState{};
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilState.depthTestEnable = m_Specifications.DepthTest;
		depthStencilState.depthWriteEnable = m_Specifications.DepthWrite;
		depthStencilState.depthBoundsTestEnable = VK_FALSE;
		depthStencilState.minDepthBounds = 0.0f;
		depthStencilState.maxDepthBounds = 1.0f;
		depthStencilState.stencilTestEnable = VK_FALSE;
		depthStencilState.depthCompareOp = depthComapreFunction;
		depthStencilState.front = {};
		depthStencilState.back = {};

		VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.pDynamicStates = dynamicStates;
		dynamicState.dynamicStateCount = 2;

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		switch (m_Specifications.Topology)
		{
		case PrimitiveTopology::Triangles:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			break;
		case PrimitiveTopology::Lines:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
			break;
		default:
			FLARE_CORE_ASSERT(false);
		}

		VkPipelineMultisampleStateCreateInfo multisampleState{};
		multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleState.sampleShadingEnable = VK_FALSE;
		multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleState.minSampleShading = 1.0f;
		multisampleState.pSampleMask = nullptr;
		multisampleState.alphaToCoverageEnable = VK_FALSE;
		multisampleState.alphaToOneEnable = VK_FALSE;

		VkCullModeFlags cullMode = VK_CULL_MODE_NONE;
		switch (m_Specifications.Culling)
		{
		case CullingMode::None:
			cullMode = VK_CULL_MODE_NONE;
			break;
		case CullingMode::Back:
			cullMode = VK_CULL_MODE_BACK_BIT;
			break;
		case CullingMode::Front:
			cullMode = VK_CULL_MODE_FRONT_BIT;
			break;
		default:
			FLARE_CORE_ASSERT(false);
		}

		VkPipelineRasterizationStateCreateInfo rasterizationState{};
		rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationState.depthClampEnable = VK_FALSE;
		rasterizationState.rasterizerDiscardEnable = VK_FALSE;
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationState.lineWidth = 1.0f;
		rasterizationState.cullMode = cullMode;
		rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationState.depthBiasEnable = m_Specifications.DepthBiasEnabled;
		rasterizationState.depthBiasSlopeFactor = m_Specifications.DepthBiasSlopeFactor;
		rasterizationState.depthBiasConstantFactor = m_Specifications.DepthBiasConstantFactor;

		VkPipelineShaderStageCreateInfo stages[2] = {};
		stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stages[0].module = As<VulkanShader>(m_Specifications.Shader)->GetModuleForStage(ShaderStageType::Vertex);
		stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stages[0].pName = "main";
		stages[0].pSpecializationInfo = nullptr;
		stages[0].flags = 0;

		stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stages[1].module = As<VulkanShader>(m_Specifications.Shader)->GetModuleForStage(ShaderStageType::Pixel);
		stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stages[1].pName = "main";
		stages[1].pSpecializationInfo = nullptr;
		stages[1].flags = 0;

		std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions;

		for (size_t i = 0; i < m_Specifications.InputLayout.Elements.size(); i++)
		{
			auto& element = m_Specifications.InputLayout.Elements[i];
			auto it = std::find_if(vertexBindingDescriptions.begin(),
				vertexBindingDescriptions.end(),
				[&](const VkVertexInputBindingDescription& desc) -> bool
				{
					return desc.binding == element.Binding;
				});

			VkVertexInputBindingDescription* bindingDescription = nullptr;
			if (it == vertexBindingDescriptions.end())
			{
				auto& desc = vertexBindingDescriptions.emplace_back();
				desc.binding = element.Binding;
				desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
				desc.stride = 0;

				bindingDescription = &desc;
			}
			else
			{
				bindingDescription = &(*it);
			}

			element.Offset = bindingDescription->stride;
			bindingDescription->stride += ShaderDataTypeSize(element.Type);
		}

		const auto& vertexInputLayoutElements = m_Specifications.InputLayout.Elements;
		std::vector<VkVertexInputAttributeDescription> vertexAttributes(vertexInputLayoutElements.size());
		for (size_t i = 0; i < vertexInputLayoutElements.size(); i++)
		{
			VkVertexInputAttributeDescription& attribute = vertexAttributes[i];
			attribute.binding = vertexInputLayoutElements[i].Binding;
			attribute.format = ShaderDataTypeToVulkanFormat(vertexInputLayoutElements[i].Type);
			attribute.location = vertexInputLayoutElements[i].Location;
			attribute.offset = (uint32_t)vertexInputLayoutElements[i].Offset;
		}

		VkPipelineVertexInputStateCreateInfo vertexInputState{};
		vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputState.vertexBindingDescriptionCount = (uint32_t)vertexBindingDescriptions.size();
		vertexInputState.pVertexBindingDescriptions = vertexBindingDescriptions.data();
		vertexInputState.vertexAttributeDescriptionCount = (uint32_t)vertexAttributes.size();
		vertexInputState.pVertexAttributeDescriptions = vertexAttributes.data();

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = { 100, 100 };

		VkViewport viewport{};
		viewport.width = 100;
		viewport.height = 100;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		viewport.x = 0;
		viewport.y = 0;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.flags = 0;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;

		VkGraphicsPipelineCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		info.basePipelineHandle = VK_NULL_HANDLE;
		info.basePipelineIndex = -1;
		info.flags = 0;
		info.layout = m_PipelineLayout;
		info.pColorBlendState = &colorBlendState;
		info.pDepthStencilState = &depthStencilState;
		info.pDynamicState = &dynamicState;
		info.pInputAssemblyState = &inputAssembly;
		info.pMultisampleState = &multisampleState;
		info.pRasterizationState = &rasterizationState;
		info.stageCount = 2;
		info.pStages = stages;
		info.pTessellationState = nullptr;
		info.pVertexInputState = &vertexInputState;
		info.pViewportState = &viewportState;
		info.renderPass = m_CompatbileRenderPass->GetHandle();
		info.subpass = 0;

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(VulkanContext::GetInstance().GetDevice(), VK_NULL_HANDLE, 1, &info, nullptr, &m_Pipeline));

		VulkanContext::GetInstance().SetDebugName(VK_OBJECT_TYPE_PIPELINE, (uint64_t)m_Pipeline, m_Specifications.Shader->GetMetadata()->Name.c_str());
	}
}
