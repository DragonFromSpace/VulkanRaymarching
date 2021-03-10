#pragma once
namespace vkInit
{
	VkCommandPoolCreateInfo CommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
	VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool pool, uint32_t bufferCount, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	VkCommandBufferBeginInfo CommandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);
	VkFramebufferCreateInfo FramebufferCreateInfo(VkRenderPass renderpass, VkExtent2D extent);
	VkFenceCreateInfo FenceCreateInfo(VkFenceCreateFlags flags = 0);
	VkSemaphoreCreateInfo SemaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);
	VkSubmitInfo SubmitInfo(VkCommandBuffer* commandBuffer);
	VkPresentInfoKHR PresentInfoKHR();
	VkRenderPassBeginInfo RenderPassBeginInfo(VkRenderPass renderPass, VkExtent2D windowExtent, VkFramebuffer frameBuffer);
	VkImageCreateInfo ImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
	VkImageViewCreateInfo ImageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);
	VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);
	VkWriteDescriptorSet WriteDescriptorSetBuffer(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo* bufferInfo, uint32_t binding);
	VkWriteDescriptorSet WriteDescriptorSetImage(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo* imageInfo, uint32_t binding);
	VkSamplerCreateInfo SamplerCreateInfo(VkFilter filters, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

	VkPipelineShaderStageCreateInfo PipelineShaderStageInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule);
	VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCrearteInfo();
	VkPipelineInputAssemblyStateCreateInfo PipelineInputAssemblyStateCreateInfo(VkPrimitiveTopology topology);
	VkPipelineRasterizationStateCreateInfo PipelineRasterizationStateCreateInfo(VkPolygonMode polyMode);
	VkPipelineMultisampleStateCreateInfo PipelineMultisamplingStateCreateInfo();
	VkPipelineColorBlendAttachmentState PipelineColorBlendAttachmentState();
	VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo();
	VkPipelineDepthStencilStateCreateInfo PipelineDepthStencilStateCreateInfo(bool depthTest, bool depthWrite, VkCompareOp compareOp);
}