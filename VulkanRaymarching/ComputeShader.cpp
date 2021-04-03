#include "pch.h"
#include "ComputeShader.h"
#include "VkEngine.h"

ComputeShader::ComputeShader(const VkDevice& device, const std::string& computeShaderFile)
	: Shader(device, computeShaderFile)
{
}

void ComputeShader::SetSkyboxTexture(VkImageView* skyboxTexture) 
{
	m_SkyboxTexture = skyboxTexture; 
}

void ComputeShader::SetSwapchainImage(VkImageView* swapchainImage)
{
	m_SwapchainImage = swapchainImage; 
}

void ComputeShader::InitDescriptors(int overlappingFrames, VkEngine* engine)
{
	//Create descriptor pool
	std::vector<VkDescriptorPoolSize> sizes =
	{
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10},
		{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10},
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10}
	};

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = 0;
	poolInfo.maxSets = 10;
	poolInfo.poolSizeCount = (uint32_t)sizes.size();
	poolInfo.pPoolSizes = sizes.data();

	if (vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
		throw std::runtime_error("ComputeShader::InitDescriptors() >> Failed to create descriptor pool!");

	engine->m_DeletionQueue.PushFunction([=]() {vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr); });

	//Create setLayoutBinding
	VkDescriptorSetLayoutBinding outputImageBinding = vkInit::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0);
	VkDescriptorSetLayoutBinding skyboxImageBinding = vkInit::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT, 1);
	VkDescriptorSetLayoutBinding dimensionsBinding = vkInit::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 2);
	VkDescriptorSetLayoutBinding sceneDataBinding = vkInit::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 3);
	VkDescriptorSetLayoutBinding lightDataBinding = vkInit::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 4);
	VkDescriptorSetLayoutBinding layoutBindings[] = { outputImageBinding, skyboxImageBinding, dimensionsBinding, sceneDataBinding, lightDataBinding };

	VkDescriptorSetLayoutCreateInfo setInfo{};
	setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	setInfo.flags = 0;
	setInfo.bindingCount = 5;
	setInfo.pBindings = layoutBindings;
	vkCreateDescriptorSetLayout(m_Device, &setInfo, nullptr, &m_descriptorSetLayout);

	engine->m_DeletionQueue.PushFunction([=]() {vkDestroyDescriptorSetLayout(m_Device, m_descriptorSetLayout, nullptr); });

	//Create sampler for the textures
	VkSamplerCreateInfo samplerInfo = vkInit::SamplerCreateInfo(VK_FILTER_LINEAR);
	VkSampler blockySampler;
	vkCreateSampler(m_Device, &samplerInfo, nullptr, &blockySampler);
	engine->m_DeletionQueue.PushFunction([=]()
		{
			vkDestroySampler(m_Device, blockySampler, nullptr);
		});

	m_FrameData.resize(overlappingFrames);
	for (int i = 0; i < overlappingFrames; ++i)
	{
		//Create buffers
		m_FrameData[i].dimensionsBuffer = engine->CreateBuffer(sizeof(uint32_t) * 2, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		m_FrameData[i].sceneBuffer = engine->CreateBuffer(sizeof(glm::mat4) * 3 + sizeof(float), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		m_FrameData[i].lightBuffer = engine->CreateBuffer(sizeof(glm::vec4) * 2, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	
		//allocate descriptorset
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorSetCount = 1;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.pSetLayouts = &m_descriptorSetLayout;
		if (vkAllocateDescriptorSets(m_Device, &allocInfo, &m_FrameData[i].descriptorSet) != VK_SUCCESS)
			throw std::runtime_error("ComputeShader::InitDescriptors() >> Failed to allocate descriptor set!");
	
		//Write buffer to descriptor set
		VkDescriptorImageInfo outputImageInfo{};
		outputImageInfo.imageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		outputImageInfo.imageView = m_SwapchainImage[i];
		outputImageInfo.sampler = VK_NULL_HANDLE;

		VkDescriptorImageInfo skyboxImageInfo{};
		skyboxImageInfo.sampler = blockySampler;
		skyboxImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		skyboxImageInfo.imageView = *m_SkyboxTexture;

		VkDescriptorBufferInfo dimensionsBufferInfo{};
		dimensionsBufferInfo.buffer = m_FrameData[i].dimensionsBuffer.buffer;
		dimensionsBufferInfo.offset = 0;
		dimensionsBufferInfo.range = sizeof(DimensionsBufferData);

		VkDescriptorBufferInfo sceneBufferInfo{};
		sceneBufferInfo.buffer = m_FrameData[i].sceneBuffer.buffer;
		sceneBufferInfo.offset = 0;
		sceneBufferInfo.range = sizeof(SceneBufferData);

		VkDescriptorBufferInfo lightBufferInfo{};
		lightBufferInfo.buffer = m_FrameData[i].lightBuffer.buffer;
		lightBufferInfo.offset = 0;
		lightBufferInfo.range = sizeof(LightBufferData);

		//Write texture to the descriptor set
		VkWriteDescriptorSet imageOutputSetWrite = vkInit::WriteDescriptorSetImage(VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_FrameData[i].descriptorSet, &outputImageInfo, 0);
		VkWriteDescriptorSet skyboxTexture = vkInit::WriteDescriptorSetImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_FrameData[i].descriptorSet, &skyboxImageInfo, 1);
		VkWriteDescriptorSet dimensionsSetWrite = vkInit::WriteDescriptorSetBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_FrameData[i].descriptorSet, &dimensionsBufferInfo, 2);
		VkWriteDescriptorSet sceneSetWrite = vkInit::WriteDescriptorSetBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_FrameData[i].descriptorSet, &sceneBufferInfo, 3);
		VkWriteDescriptorSet lightSetWrite = vkInit::WriteDescriptorSetBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_FrameData[i].descriptorSet, &lightBufferInfo, 4);
		VkWriteDescriptorSet writeSets[] = { imageOutputSetWrite, skyboxTexture, dimensionsSetWrite, sceneSetWrite, lightSetWrite };
		vkUpdateDescriptorSets(m_Device, 5, writeSets, 0, nullptr);
	}
}

void ComputeShader::UpdateShaderVariables(int currentFrame, VkEngine* engine)
{
	//Update dimensions buffer
	void* data = engine->GetBufferMemory(m_FrameData[currentFrame].dimensionsBuffer);
	memcpy(data, &m_DimensionsBufferData, sizeof(DimensionsBufferData));
	engine->ReleaseBufferMemory(m_FrameData[currentFrame].dimensionsBuffer);

	//Update scene buffer
	data = engine->GetBufferMemory(m_FrameData[currentFrame].sceneBuffer);
	memcpy(data, &m_SceneBufferData, sizeof(SceneBufferData));
	engine->ReleaseBufferMemory(m_FrameData[currentFrame].sceneBuffer);

	//update light buffer
	data = engine->GetBufferMemory(m_FrameData[currentFrame].lightBuffer);
	memcpy(data, &m_LightBufferData, sizeof(LightBufferData));
	engine->ReleaseBufferMemory(m_FrameData[currentFrame].lightBuffer);
}
