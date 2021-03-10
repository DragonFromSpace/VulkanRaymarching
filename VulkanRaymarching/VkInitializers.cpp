#include "pch.h"
#include "VkInitializers.h"

VkCommandPoolCreateInfo vkInit::CommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags)
{
	VkCommandPoolCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.queueFamilyIndex = queueFamilyIndex;
	info.flags = flags;

	return info;
}

VkCommandBufferAllocateInfo vkInit::CommandBufferAllocateInfo(VkCommandPool pool, uint32_t bufferCount, VkCommandBufferLevel level)
{
	VkCommandBufferAllocateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.commandPool = pool;
	info.level = level;
	info.commandBufferCount = bufferCount;

	return info;
}

VkCommandBufferBeginInfo vkInit::CommandBufferBeginInfo(VkCommandBufferUsageFlags flags)
{
	VkCommandBufferBeginInfo info{};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.flags = flags;

	return info;
}

VkFramebufferCreateInfo vkInit::FramebufferCreateInfo(VkRenderPass renderpass, VkExtent2D extent)
{
	VkFramebufferCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	info.attachmentCount = 1;
	info.renderPass = renderpass;
	info.layers = 1;
	info.width = extent.width;
	info.height = extent.height;

	return info;
}

VkFenceCreateInfo vkInit::FenceCreateInfo(VkFenceCreateFlags flags)
{
	VkFenceCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	info.flags = flags;

	return info;
}

VkSemaphoreCreateInfo vkInit::SemaphoreCreateInfo(VkSemaphoreCreateFlags flags)
{
	VkSemaphoreCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	info.flags = flags;

	return info;
}

VkSubmitInfo vkInit::SubmitInfo(VkCommandBuffer* commandBuffer)
{
	VkSubmitInfo info{};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.commandBufferCount = 1;
	info.pCommandBuffers = commandBuffer;

	info.waitSemaphoreCount = 0;
	info.pWaitSemaphores = nullptr;
	info.signalSemaphoreCount = 0;
	info.pSignalSemaphores = nullptr;
	info.pWaitDstStageMask = nullptr;

	return info;
}

VkPresentInfoKHR vkInit::PresentInfoKHR()
{
	VkPresentInfoKHR info{};
	info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	return info;
}

VkRenderPassBeginInfo vkInit::RenderPassBeginInfo(VkRenderPass renderPass, VkExtent2D windowExtent, VkFramebuffer frameBuffer)
{
	VkRenderPassBeginInfo info{};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	info.renderPass = renderPass;
	info.renderArea.offset = {0,0};
	info.renderArea.extent = windowExtent;
	info.framebuffer = frameBuffer;

	return info;
}

VkImageCreateInfo vkInit::ImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent)
{
	VkImageCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.imageType = VK_IMAGE_TYPE_2D;
	info.format = format;
	info.extent = extent;
	
	info.mipLevels = 1;
	info.arrayLayers = 1;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.usage = usageFlags;

	return info;
}

VkImageViewCreateInfo vkInit::ImageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	info.format = format;
	info.subresourceRange.baseMipLevel = 0;
	info.subresourceRange.levelCount = 1;
	info.subresourceRange.baseArrayLayer = 0;
	info.subresourceRange.layerCount = 1;
	info.subresourceRange.aspectMask = aspectFlags;
	info.image = image;

	return info;
}

VkDescriptorSetLayoutBinding vkInit::DescriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding)
{
	VkDescriptorSetLayoutBinding info{};
	info.descriptorCount = 1;
	info.descriptorType = type;
	info.stageFlags = stageFlags;
	info.binding = binding;
	info.pImmutableSamplers = nullptr;

	return info;
}

VkWriteDescriptorSet vkInit::WriteDescriptorSetBuffer(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo* bufferInfo, uint32_t binding)
{
	VkWriteDescriptorSet info{};
	info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	info.dstBinding = binding;
	info.dstSet = dstSet;
	info.descriptorCount = 1;
	info.descriptorType = type;
	info.pBufferInfo = bufferInfo;

	return info;
}

VkWriteDescriptorSet vkInit::WriteDescriptorSetImage(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo* imageInfo, uint32_t binding)
{
	VkWriteDescriptorSet info{};
	info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	info.dstBinding = binding;
	info.dstSet = dstSet;
	info.descriptorCount = 1;
	info.descriptorType = type;
	info.pImageInfo = imageInfo;

	return info;
}

VkSamplerCreateInfo vkInit::SamplerCreateInfo(VkFilter filters, VkSamplerAddressMode samplerAddressMode)
{
	VkSamplerCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	info.magFilter = filters;
	info.minFilter = filters;
	info.addressModeU = samplerAddressMode;
	info.addressModeV = samplerAddressMode;
	info.addressModeW = samplerAddressMode;

	return info;
}

VkPipelineShaderStageCreateInfo vkInit::PipelineShaderStageInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule)
{
	VkPipelineShaderStageCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	info.stage = stage;
	info.module = shaderModule;
	info.pName = "main";

	return info;
}

VkPipelineVertexInputStateCreateInfo vkInit::PipelineVertexInputStateCrearteInfo()
{
	VkPipelineVertexInputStateCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	info.vertexAttributeDescriptionCount = 0;
	info.vertexBindingDescriptionCount = 0;

	return info;
}

VkPipelineInputAssemblyStateCreateInfo vkInit::PipelineInputAssemblyStateCreateInfo(VkPrimitiveTopology topology)
{
	VkPipelineInputAssemblyStateCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	info.topology = topology;
	info.primitiveRestartEnable = VK_FALSE;

	return info;
}

VkPipelineRasterizationStateCreateInfo vkInit::PipelineRasterizationStateCreateInfo(VkPolygonMode polyMode)
{
	VkPipelineRasterizationStateCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	info.depthClampEnable = VK_FALSE;
	info.rasterizerDiscardEnable = VK_FALSE;
	info.polygonMode = polyMode;
	info.lineWidth = 1.0f;
	info.cullMode = VK_CULL_MODE_BACK_BIT;
	info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	info.depthBiasEnable = VK_FALSE;
	info.depthBiasClamp = 0.0f;
	info.depthBiasConstantFactor = 0.0f;
	info.depthBiasSlopeFactor = 0.0f;

	return info;
}

VkPipelineMultisampleStateCreateInfo vkInit::PipelineMultisamplingStateCreateInfo()
{
	VkPipelineMultisampleStateCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	info.sampleShadingEnable = VK_FALSE;
	info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	info.minSampleShading = 1.0f;
	info.pSampleMask = nullptr;
	info.alphaToCoverageEnable = VK_FALSE;
	info.alphaToOneEnable = VK_FALSE;

	return info;
}

VkPipelineColorBlendAttachmentState vkInit::PipelineColorBlendAttachmentState()
{
	VkPipelineColorBlendAttachmentState info{};
	info.colorWriteMask = VK_COMPONENT_SWIZZLE_R | VK_COMPONENT_SWIZZLE_G | VK_COMPONENT_SWIZZLE_B | VK_COMPONENT_SWIZZLE_A;
	info.blendEnable = VK_FALSE;

	return info;
}

VkPipelineLayoutCreateInfo vkInit::PipelineLayoutCreateInfo()
{
	VkPipelineLayoutCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	info.flags = 0;
	info.setLayoutCount = 0;
	info.pSetLayouts = nullptr;
	info.pushConstantRangeCount = 0;
	info.pPushConstantRanges = nullptr;

	return info;
}

VkPipelineDepthStencilStateCreateInfo vkInit::PipelineDepthStencilStateCreateInfo(bool depthTest, bool depthWrite, VkCompareOp compareOp)
{
	VkPipelineDepthStencilStateCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	info.depthTestEnable = depthTest ? VK_TRUE : VK_FALSE;
	info.depthWriteEnable = depthWrite ? VK_TRUE : VK_FALSE;
	info.depthCompareOp = depthTest ? compareOp : VK_COMPARE_OP_ALWAYS;
	info.depthBoundsTestEnable = VK_FALSE;
	info.minDepthBounds = 0.0f;
	info.maxDepthBounds = 1.0f;
	info.stencilTestEnable = VK_FALSE;

	return info;
}