#include "pch.h"
#include "VkEngine.h"
#include "Shader.h"
#include <string>
#include <chrono>

void VkEngine::GLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_1 && action == GLFW_PRESS)
	{
		VkEngine* engine = reinterpret_cast<VkEngine*>(glfwGetWindowUserPointer(window));

		engine->CleanPipelines();
		engine->InitPipelines();

		std::cout << "Shaders reloaded!" << '\n';
	}
}

void VkEngine::GLFWMouseCallback(GLFWwindow* window, double xPos, double yPos)
{
	//get movement normalized
	glm::vec2 deltaMouse = _PrevMousePos - glm::vec2(xPos, yPos);

	//Update camera rotation
	_Camera.ProcessMouseMovement(deltaMouse.x, deltaMouse.y);

	//Update pos
	_PrevMousePos.x = xPos;
	_PrevMousePos.y = yPos;
}

void VkEngine::Init()
{
	//Init glfw window
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);	//Make sure no OpenGL context is created
	m_pWindow = glfwCreateWindow(m_WindowExtent.width, m_WindowExtent.height, "Vulkan tutorial", NULL, NULL);
	glfwSetWindowUserPointer(m_pWindow, this);
	glfwSetKeyCallback(m_pWindow, GLFWKeyCallback);
	glfwSetCursorPosCallback(m_pWindow, GLFWMouseCallback);
	glfwSetInputMode(m_pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//init vulkan
	InitVulkan();
	InitSwapchain();
	InitDefaultRenderPass();
	InitFramebuffers();
	InitCommands();
	InitSyncStructures();
	LoadTextures();
	InitDescriptors();
	InitPipelines();

	m_IsInitialized = true;
}

void VkEngine::Cleanup()
{
	if (m_IsInitialized)
	{
		m_DeletionQueue.Flush();

		CleanPipelines();

		vkDestroySurfaceKHR(m_Instance, m_WindowSurface, nullptr);
		vkb::destroy_debug_utils_messenger(m_Instance, m_DebugMessenger, nullptr);

		vkDestroyDevice(m_Device, nullptr);
		vkDestroyInstance(m_Instance, nullptr);

		//Cleanup window
		glfwDestroyWindow(m_pWindow);
	}
}

void VkEngine::Run()
{
	while (!glfwWindowShouldClose(m_pWindow))
	{
		glfwPollEvents();
		Update();
		DrawCompute();
		Draw();
	}

	vkDeviceWaitIdle(m_Device);
}

void VkEngine::DrawCompute()
{
	//Copy data in the dimensions
	uint32_t* dimData;
	vmaMapMemory(m_Allocator, GetCurrentFrame().dimensionsBuffer.allocation, (void**)&dimData);
	dimData[0] = m_WindowExtent.width;
	dimData[1] = m_WindowExtent.height;
	vmaUnmapMemory(m_Allocator, GetCurrentFrame().dimensionsBuffer.allocation);

	//Create camera data
	glm::mat4 view = _Camera.GetViewMatrix();
	glm::mat4 proj = glm::perspective(glm::radians(70.0f), (float)m_WindowExtent.width / (float)m_WindowExtent.height, 0.1f, 200.0f);

	GPUSceneData sceneData;
	sceneData.ViewMat = view;
	sceneData.ViewInverseMat = glm::inverse(view);
	sceneData.ProjInverseMat = glm::inverse(proj);
	sceneData.time = glfwGetTime();

	void* data;
	vmaMapMemory(m_Allocator, GetCurrentFrame().sceneBuffer.allocation, &data);
	memcpy(data, &sceneData, sizeof(GPUSceneData));
	vmaUnmapMemory(m_Allocator, GetCurrentFrame().sceneBuffer.allocation);

	//set light data
	GPULightData lightData;
	lightData.lightColor = glm::vec4(1.0f, 1.0f, 0.95f, 1.0f);
	lightData.lightDirection = glm::normalize(glm::vec4(0.5f, -0.9f, 0.3f, 1.0f));

	vmaMapMemory(m_Allocator, GetCurrentFrame().lightBuffer.allocation, &data);
	memcpy(data, &lightData, sizeof(GPULightData));
	vmaUnmapMemory(m_Allocator, GetCurrentFrame().lightBuffer.allocation);

	//Run the compute buffer
	VkCommandBufferBeginInfo beginInfo = vkInit::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK(vkBeginCommandBuffer(GetCurrentFrame().computeCommandBuffer, &beginInfo), "VkEngine::DrawCompute() >> Failed to begin command buffer!");

	//Bind compute pipeline
	vkCmdBindPipeline(GetCurrentFrame().computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline);

	//bind descriptor sets
	vkCmdBindDescriptorSets(GetCurrentFrame().computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipelineLayout, 0, 1, &GetCurrentFrame().computeDescriptorSet, 0, nullptr);

	//Dispatch compute
	uint32_t groupsX = (uint32_t)glm::ceil(m_WindowExtent.width / 32.0f);
	uint32_t groupsY = (uint32_t)glm::ceil(m_WindowExtent.height / 32.0f);
	vkCmdDispatch(GetCurrentFrame().computeCommandBuffer, groupsX, groupsY, 1);

	VK_CHECK(vkEndCommandBuffer(GetCurrentFrame().computeCommandBuffer), "VkEngine::DrawCompute() Failed to end command buffer!");

	//Submit the queue
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &GetCurrentFrame().computeCommandBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;
	vkQueueSubmit(m_ComputeQueue, 1, &submitInfo, GetCurrentFrame().computeFence);

	vkWaitForFences(m_Device, 1, &GetCurrentFrame().computeFence, VK_TRUE, UINT64_MAX);
	vkResetFences(m_Device, 1, &GetCurrentFrame().computeFence);

	vkResetCommandBuffer(GetCurrentFrame().computeCommandBuffer, 0);
}

void VkEngine::Draw()
{
	//get image in swapchain
	uint32_t imageIndex;
	VK_CHECK(vkAcquireNextImageKHR(m_Device, m_Swapchain, UINT64_MAX, GetCurrentFrame().presentSemaphore, nullptr, &imageIndex), "VkEngine::Draw() >> Failed to acquire next image in swapchain!");

	//Transition image from undefined to src_present
	VkCommandBufferBeginInfo beginInfo = vkInit::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	vkBeginCommandBuffer(GetCurrentFrame().graphicsCommandBuffer, &beginInfo);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = m_SwapchainImages[imageIndex];
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	barrier.srcAccessMask = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
	barrier.dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(GetCurrentFrame().graphicsCommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	vkEndCommandBuffer(GetCurrentFrame().graphicsCommandBuffer);

	VkPipelineStageFlags waitPipelineFlag = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	VkSubmitInfo submitInfo = vkInit::SubmitInfo(&GetCurrentFrame().graphicsCommandBuffer);
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &GetCurrentFrame().presentSemaphore;
	submitInfo.pWaitDstStageMask = &waitPipelineFlag;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &GetCurrentFrame().imageTransSemaphore;

	vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, GetCurrentFrame().imageTransFence);
	vkWaitForFences(m_Device, 1, &GetCurrentFrame().imageTransFence, VK_TRUE, UINT64_MAX);
	vkResetFences(m_Device, 1, &GetCurrentFrame().imageTransFence);
	vkResetCommandBuffer(GetCurrentFrame().graphicsCommandBuffer, 0);

	//PRESENT the image in the swapchain
	VkPresentInfoKHR presentInfo = vkInit::PresentInfoKHR();
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_Swapchain;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &GetCurrentFrame().imageTransSemaphore;
	presentInfo.pImageIndices = &imageIndex;
	VK_CHECK(vkQueuePresentKHR(m_GraphicsQueue, &presentInfo), "VkEngine::Draw() >> Failed to present the queue!");

	m_FrameNumber++;
}

void VkEngine::InitVulkan()
{
	//Create vulkan instance
	vkb::InstanceBuilder instanceBuilder;
	instanceBuilder.set_app_name("Vulkan Tutorial");
	instanceBuilder.enable_validation_layers(m_EnableValidationLayers);
	instanceBuilder.require_api_version(1, 1, 0);
	instanceBuilder.use_default_debug_messenger();
	vkb::detail::Result<vkb::Instance> vkbInstanceResult = instanceBuilder.build();
	vkb::Instance vkbInstance = vkbInstanceResult.value();

	m_Instance = vkbInstance.instance;
	m_DebugMessenger = vkbInstance.debug_messenger;

	//Create window surface
	VK_CHECK(glfwCreateWindowSurface(m_Instance, m_pWindow, nullptr, &m_WindowSurface), "VkEngine::InitVulkan() >> Failed to create window surface!");

	//Select physical device
	vkb::PhysicalDeviceSelector physDeviceSelector{ vkbInstance };
	physDeviceSelector.set_minimum_version(1, 1);
	physDeviceSelector.set_surface(m_WindowSurface);
	vkb::PhysicalDevice physDevice = physDeviceSelector.select().value();
	m_PhysicalDevice = physDevice.physical_device;

	//Get gpu properties
	vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_GPUProperties);
	std::cout << "The gpu min alignment for uniform buffers is: " << m_GPUProperties.limits.minUniformBufferOffsetAlignment << '\n';

	//Create logical device
	vkb::DeviceBuilder deviceBuilder{ physDevice };
	vkb::Device device = deviceBuilder.build().value();
	m_Device = device.device;

	//Get the graphics queue
	m_GraphicsQueue = device.get_queue(vkb::QueueType::graphics).value();
	m_GraphicsQueueFamily = device.get_queue_index(vkb::QueueType::graphics).value();

	//Get compute queue
	m_ComputeQueue = device.get_queue(vkb::QueueType::compute).value();
	m_ComputeQueueFamily = device.get_queue_index(vkb::QueueType::compute).value();

	//Initialize memory allocator
	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.physicalDevice = m_PhysicalDevice;
	allocatorInfo.device = m_Device;
	allocatorInfo.instance = m_Instance;
	VK_CHECK(vmaCreateAllocator(&allocatorInfo, &m_Allocator), "VkEngine::InitVulkan() >> Failed to create vma allocator!");

	m_DeletionQueue.PushFunction([=]() {vmaDestroyAllocator(m_Allocator); });
}

void VkEngine::InitSwapchain()
{
	vkb::SwapchainBuilder swapchainBuilder{ m_PhysicalDevice, m_Device, m_WindowSurface };
	swapchainBuilder.use_default_format_selection();
	swapchainBuilder.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR);
	swapchainBuilder.set_desired_extent(m_WindowExtent.width, m_WindowExtent.height);
	swapchainBuilder.add_image_usage_flags(VK_IMAGE_USAGE_STORAGE_BIT);

	vkb::Swapchain vkbSwapchain = swapchainBuilder.build().value();
	m_Swapchain = vkbSwapchain.swapchain;
	m_SwapchainImages = vkbSwapchain.get_images().value();
	m_SwapchainImageViews = vkbSwapchain.get_image_views().value();
	m_SwapchainImageFormat = vkbSwapchain.image_format;

	m_Frames.resize(m_SwapchainImages.size());
	m_FramesOverlapping = m_Frames.size();

	//Add to deletion queue
	m_DeletionQueue.PushFunction([=]()
		{
			for (VkImageView image : m_SwapchainImageViews)
				vkDestroyImageView(m_Device, image, nullptr);
			vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
		});
}

void VkEngine::InitCommands()
{
	//Create graphics and compute command pool
	VkCommandPoolCreateInfo commandPoolInfo = vkInit::CommandPoolCreateInfo(m_GraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	VkCommandPoolCreateInfo computeCommandPool = vkInit::CommandPoolCreateInfo(m_ComputeQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	for (int i = 0; i < m_FramesOverlapping; ++i)
	{
		VK_CHECK(vkCreateCommandPool(m_Device, &commandPoolInfo, nullptr, &m_Frames[i].graphicsCommandPool), "VkEngine::InitCommands() >> failed to create command pool!");
		VK_CHECK(vkCreateCommandPool(m_Device, &computeCommandPool, nullptr, &m_Frames[i].computeCommandPool), "VkEngine::InitCommands() >> failed to create compute command pool!");

		//Create command buffer
		VkCommandBufferAllocateInfo allocInfo = vkInit::CommandBufferAllocateInfo(m_Frames[i].graphicsCommandPool, 1);
		VK_CHECK(vkAllocateCommandBuffers(m_Device, &allocInfo, &m_Frames[i].graphicsCommandBuffer), "VkEngine::InitCommands() >> Failed to allocate command buffers");

		VkCommandBufferAllocateInfo computeAllocInfo = vkInit::CommandBufferAllocateInfo(m_Frames[i].computeCommandPool, 1);
		VK_CHECK(vkAllocateCommandBuffers(m_Device, &computeAllocInfo, &m_Frames[i].computeCommandBuffer), "VkEngine::InitCommands() >> Failed to allocate compute command buffers");

		m_DeletionQueue.PushFunction([=]()
			{
				vkDestroyCommandPool(m_Device, m_Frames[i].graphicsCommandPool, nullptr);
				vkDestroyCommandPool(m_Device, m_Frames[i].computeCommandPool, nullptr);
			});
	}

	//create command pool for the immediate submit
	VkCommandPoolCreateInfo uploadCommandPool = vkInit::CommandPoolCreateInfo(m_GraphicsQueueFamily);
	VK_CHECK(vkCreateCommandPool(m_Device, &uploadCommandPool, nullptr, &m_UploadContext.commandPool), "VkEngine::InitCommands() >> Failed to create upload command pool!");
	m_DeletionQueue.PushFunction([=]()
		{
			vkDestroyCommandPool(m_Device, m_UploadContext.commandPool, nullptr);
		});
}

void VkEngine::InitDefaultRenderPass()
{
	//COLOR
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_SwapchainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	//Subpass and renderpass
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = nullptr;

	VkRenderPassCreateInfo renderPass{};
	renderPass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPass.attachmentCount = 1;
	renderPass.pAttachments = &colorAttachment;
	renderPass.subpassCount = 1;
	renderPass.pSubpasses = &subpass;

	VK_CHECK(vkCreateRenderPass(m_Device, &renderPass, nullptr, &m_RenderPass), "VkEngine::InitDefaultRenderPass() >> Failed to create render pass!");
	m_DeletionQueue.PushFunction([=]() {vkDestroyRenderPass(m_Device, m_RenderPass, nullptr); });
}

void VkEngine::InitFramebuffers()
{
	VkFramebufferCreateInfo framebufferInfo = vkInit::FramebufferCreateInfo(m_RenderPass, m_WindowExtent);

	uint32_t imageCount = m_SwapchainImages.size();
	m_Framebuffers = std::vector<VkFramebuffer>(imageCount);

	for (uint32_t i = 0; i < imageCount; ++i)
	{
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = &m_SwapchainImageViews[i];
		VK_CHECK(vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_Framebuffers[i]), "VkEngine::InitFramebuffers() >> Failed to create framebuffer!");
		m_DeletionQueue.PushFunction([=]() {vkDestroyFramebuffer(m_Device, m_Framebuffers[i], nullptr); });
	}
}

void VkEngine::InitSyncStructures()
{
	VkFenceCreateInfo fenceInfoSignaled = vkInit::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	VkFenceCreateInfo fenceInfo = vkInit::FenceCreateInfo();
	VkSemaphoreCreateInfo semaphoreInfo = vkInit::SemaphoreCreateInfo();

	for (int i = 0; i < m_FramesOverlapping; ++i)
	{
		VK_CHECK(vkCreateFence(m_Device, &fenceInfo, nullptr, &m_Frames[i].computeFence), "VkEngine::InitSyncStructures() >> Failed to create compute fence!");
		VK_CHECK(vkCreateFence(m_Device, &fenceInfo, nullptr, &m_Frames[i].imageTransFence), "VkEngine::InitSyncStructures() >> Failed to create compute fence!");

		VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_Frames[i].presentSemaphore), "VkEngine::InitSyncStructures() >> Failed to create present semaphore!");
		VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_Frames[i].imageTransSemaphore), "VkEngine::InitSyncStructures() >> Failed to create present semaphore!");

		m_DeletionQueue.PushFunction([=]()
			{
				vkDestroyFence(m_Device, m_Frames[i].computeFence, nullptr);
				vkDestroyFence(m_Device, m_Frames[i].imageTransFence, nullptr);
				vkDestroySemaphore(m_Device, m_Frames[i].presentSemaphore, nullptr);
				vkDestroySemaphore(m_Device, m_Frames[i].imageTransSemaphore, nullptr);
			});
	}

	//create fence for the immediate submit
	VkFenceCreateInfo uploadFenceInfo = vkInit::FenceCreateInfo();
	VK_CHECK(vkCreateFence(m_Device, &uploadFenceInfo, nullptr, &m_UploadContext.uploadFence), "VkEngine::InitSyncStructures() >> Failed to create upload fence!");
	m_DeletionQueue.PushFunction([=]()
		{
			vkDestroyFence(m_Device, m_UploadContext.uploadFence, nullptr);
		});
}

void VkEngine::InitDescriptors()
{
	//Create pool that can hold 10 storage buffers
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

	VK_CHECK(vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool), "VkEngine::InitDescriptors() >> Failed to create descriptor pool!");
	m_DeletionQueue.PushFunction([=]() {vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr); });

	//Create descriptor set layout
	VkDescriptorSetLayoutBinding outputImageBinding = vkInit::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0);
	VkDescriptorSetLayoutBinding skyboxImageBinding = vkInit::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT, 1);
	VkDescriptorSetLayoutBinding dimensionsBinding = vkInit::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 2);
	VkDescriptorSetLayoutBinding sceneDataBinding = vkInit::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 3);
	VkDescriptorSetLayoutBinding lightDataBinding = vkInit::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 4);
	VkDescriptorSetLayoutBinding layoutBindings[] = { outputImageBinding, skyboxImageBinding, dimensionsBinding, sceneDataBinding, lightDataBinding };

	//Create descriptor set layout for the storage buffer
	VkDescriptorSetLayoutCreateInfo setInfo{};
	setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	setInfo.flags = 0;
	setInfo.bindingCount = 5;
	setInfo.pBindings = layoutBindings;
	VK_CHECK(vkCreateDescriptorSetLayout(m_Device, &setInfo, nullptr, &m_ComputeDescriptorSetLayout), "VkEngine::InitDescriptors() >> Failed to create descriptor set layout!");

	m_DeletionQueue.PushFunction([=]()
		{
			vkDestroyDescriptorSetLayout(m_Device, m_ComputeDescriptorSetLayout, nullptr);
		});

	//Create sampler for the textures
	VkSamplerCreateInfo samplerInfo = vkInit::SamplerCreateInfo(VK_FILTER_LINEAR);
	VkSampler blockySampler;
	vkCreateSampler(m_Device, &samplerInfo, nullptr, &blockySampler);
	m_DeletionQueue.PushFunction([=]()
		{
			vkDestroySampler(m_Device, blockySampler, nullptr);
		});

	//Create buffer for each frame
	for (int i = 0; i < m_FramesOverlapping; ++i)
	{
		//Create buffers for the dimensions and the output
		m_Frames[i].dimensionsBuffer = CreateBuffer(sizeof(uint32_t) * 2, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		m_Frames[i].sceneBuffer = CreateBuffer(sizeof(glm::mat4) * 3 + sizeof(float), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		//const size_t lightBufferPaddedSize = m_FramesOverlapping * PadUniformBufferSize(sizeof(GPULightData));
		m_Frames[i].lightBuffer = CreateBuffer(sizeof(glm::vec4) * 2, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		m_DeletionQueue.PushFunction([=]()
			{
				//vmaDestroyBuffer(m_Allocator, m_Frames[i].outputBuffer.buffer, m_Frames[i].outputBuffer.allocation);
				vmaDestroyBuffer(m_Allocator, m_Frames[i].dimensionsBuffer.buffer, m_Frames[i].dimensionsBuffer.allocation);
				vmaDestroyBuffer(m_Allocator, m_Frames[i].sceneBuffer.buffer, m_Frames[i].sceneBuffer.allocation);
				vmaDestroyBuffer(m_Allocator, m_Frames[i].lightBuffer.buffer, m_Frames[i].lightBuffer.allocation);
			});

		//Allocate descriptor set per frame
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorSetCount = 1;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.pSetLayouts = &m_ComputeDescriptorSetLayout;
		VK_CHECK(vkAllocateDescriptorSets(m_Device, &allocInfo, &m_Frames[i].computeDescriptorSet), "VkEngine::InitDescriptors() >> Failed to allocate descriptor set for the objects!");

		//Write buffer to descriptor set
		VkDescriptorImageInfo outputImageInfo{};
		outputImageInfo.imageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		outputImageInfo.imageView = m_SwapchainImageViews[i];
		outputImageInfo.sampler = VK_NULL_HANDLE;

		VkDescriptorImageInfo skyboxImageInfo{};
		skyboxImageInfo.sampler = blockySampler;
		skyboxImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		skyboxImageInfo.imageView = m_SkyBoxTexture.imageView;

		VkDescriptorBufferInfo dimensionsBufferInfo{};
		dimensionsBufferInfo.buffer = m_Frames[i].dimensionsBuffer.buffer;
		dimensionsBufferInfo.offset = 0;
		dimensionsBufferInfo.range = sizeof(GPUDimensionsData);

		VkDescriptorBufferInfo sceneBufferInfo{};
		sceneBufferInfo.buffer = m_Frames[i].sceneBuffer.buffer;
		sceneBufferInfo.offset = 0;
		sceneBufferInfo.range = sizeof(GPUSceneData);

		VkDescriptorBufferInfo lightBufferInfo{};
		lightBufferInfo.buffer = m_Frames[i].lightBuffer.buffer;
		lightBufferInfo.offset = 0; //Use offset of the padded size
		lightBufferInfo.range = sizeof(GPULightData);

		//Write texture to the descriptor set
		VkWriteDescriptorSet imageOutputSetWrite = vkInit::WriteDescriptorSetImage(VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_Frames[i].computeDescriptorSet, &outputImageInfo, 0);
		VkWriteDescriptorSet skyboxTexture = vkInit::WriteDescriptorSetImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_Frames[i].computeDescriptorSet, &skyboxImageInfo, 1);
		VkWriteDescriptorSet dimensionsSetWrite = vkInit::WriteDescriptorSetBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_Frames[i].computeDescriptorSet, &dimensionsBufferInfo, 2);
		VkWriteDescriptorSet sceneSetWrite = vkInit::WriteDescriptorSetBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_Frames[i].computeDescriptorSet, &sceneBufferInfo, 3);
		VkWriteDescriptorSet lightSetWrite = vkInit::WriteDescriptorSetBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_Frames[i].computeDescriptorSet, &lightBufferInfo, 4);
		VkWriteDescriptorSet writeSets[] = { imageOutputSetWrite, skyboxTexture, dimensionsSetWrite, sceneSetWrite, lightSetWrite };
		vkUpdateDescriptorSets(m_Device, 5, writeSets, 0, nullptr);
	}
}

void VkEngine::InitPipelines()
{
	//Init shaders
	Shader ComputeShader{ m_Device, "../Resources/Shaders/TestComputeShader_comp.spv" };
	VkShaderModule computeShaderModule = ComputeShader.GetComputeShaderModule();

	//Create compute pipeline layout
	VkPipelineLayoutCreateInfo computePipelineLayoutCreateInfo = vkInit::PipelineLayoutCreateInfo();
	computePipelineLayoutCreateInfo.setLayoutCount = 1;
	computePipelineLayoutCreateInfo.pSetLayouts = &m_ComputeDescriptorSetLayout;

	VK_CHECK(vkCreatePipelineLayout(m_Device, &computePipelineLayoutCreateInfo, nullptr, &m_ComputePipelineLayout), "VkEngine::InitPipelines() >> Failed to create compute pipeline layout!");

	//Create base pipeline
	ComputePipelineBuilder builder{};
	builder.m_PipelineLayout = m_ComputePipelineLayout;
	builder.m_ShaderStageCreateInfo = vkInit::PipelineShaderStageInfo(VK_SHADER_STAGE_COMPUTE_BIT, computeShaderModule);
	m_ComputePipeline = builder.BuildPipeline(m_Device);

	/*m_DeletionQueue.PushFunction([=]()
		{
			vkDestroyPipeline(m_Device, m_ComputePipeline, nullptr);
			vkDestroyPipelineLayout(m_Device, m_ComputePipelineLayout, nullptr);
		});*/
}

void VkEngine::LoadTextures()
{
	Texture skybox;

	//bool b = VkUtils::LoadFromFile(*this, "../Resources/Textures/TexturesCom_HDRPanorama040_harbor_street_1K_hdri_sphere_tone.jpg", skybox.image);
	//bool b = VkUtils::LoadFromFile(*this, "../Resources/Textures/SunsetHDR_Converted.jpg", skybox.image);
	bool b = VkUtils::LoadFromFile(*this, "../Resources/Textures/quarry_02_2k.jpg", skybox.image);
	//bool b = VkUtils::LoadFromFile(*this, "../Resources/Textures/quarry_02_2k.png", skybox.image);
	//bool b = VkUtils::LoadFromFile(*this, "../Resources/Textures/SnowyHDR_Converted.jpg", skybox.image);
	if (!b)
	{
		throw std::runtime_error("VkEngine::LoadTextures() >> Failed to load skybox texture!");
	}

	VkImageViewCreateInfo imageInfo = vkInit::ImageViewCreateInfo(VK_FORMAT_R8G8B8A8_UNORM, skybox.image.image, VK_IMAGE_ASPECT_COLOR_BIT);
	VK_CHECK(vkCreateImageView(m_Device, &imageInfo, nullptr, &skybox.imageView), "VkEngine::LoadTextures() >> Failed to create Image view!");

	m_DeletionQueue.PushFunction([=]()
		{
			vkDestroyImageView(m_Device, skybox.imageView, nullptr);
		});

	m_SkyBoxTexture = skybox;
}

void VkEngine::Update()
{
	//Camera movement
	if (glfwGetKey(m_pWindow, GLFW_KEY_W))
	{
		_Camera.ProcessKeyboard(Camera_Movement::FORWARD);
	}
	else if (glfwGetKey(m_pWindow, GLFW_KEY_S))
	{
		_Camera.ProcessKeyboard(Camera_Movement::BACKWARD);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_A))
	{
		_Camera.ProcessKeyboard(Camera_Movement::LEFT);
	}
	else if (glfwGetKey(m_pWindow, GLFW_KEY_D))
	{
		_Camera.ProcessKeyboard(Camera_Movement::RIGHT);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_SPACE))
	{
		_Camera.ProcessKeyboard(Camera_Movement::UP);
	}
	else if (glfwGetKey(m_pWindow, GLFW_KEY_LEFT_CONTROL))
	{
		_Camera.ProcessKeyboard(Camera_Movement::DOWN);
	}
}

void VkEngine::CleanPipelines()
{
	vkDestroyPipeline(m_Device, m_ComputePipeline, nullptr);
	vkDestroyPipelineLayout(m_Device, m_ComputePipelineLayout, nullptr);
}

void VkEngine::ImmediateSubmit(std::function<void(VkCommandBuffer)>&& function)
{
	//allocate command buffer
	VkCommandBufferAllocateInfo cmdBufferAllocInfo = vkInit::CommandBufferAllocateInfo(m_UploadContext.commandPool, 1);

	VkCommandBuffer cmdBuffer;
	VK_CHECK(vkAllocateCommandBuffers(m_Device, &cmdBufferAllocInfo, &cmdBuffer), "VkEngine::ImmediateSubmit() >> Failed to allocate command buffer!");

	VkCommandBufferBeginInfo cmdBufferBeginInfo = vkInit::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo), "VkEngine::ImmediateSubmit() >> Failed to begin command buffer!");

	//execute function
	function(cmdBuffer);

	VK_CHECK(vkEndCommandBuffer(cmdBuffer), "VkEngine::ImmediateSubmit() >> Failed to end command buffer!");

	//Submit queue and wait for the fence to be done
	VkSubmitInfo submitInfo = vkInit::SubmitInfo(&cmdBuffer);
	VK_CHECK(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_UploadContext.uploadFence), "VkEngine::ImmediateSubmit() >> Failed to submit queue!");

	vkWaitForFences(m_Device, 1, &m_UploadContext.uploadFence, VK_TRUE, UINT64_MAX);
	vkResetFences(m_Device, 1, &m_UploadContext.uploadFence);

	//Clear command pool and command buffers with it
	vkResetCommandPool(m_Device, m_UploadContext.commandPool, 0);
}

AllocatedBuffer VkEngine::CreateBuffer(size_t allocationSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

	bufferInfo.size = allocationSize;
	bufferInfo.usage = usage;

	VmaAllocationCreateInfo vmaAllocInfo{};
	vmaAllocInfo.usage = memoryUsage;

	AllocatedBuffer newBuffer{};
	VK_CHECK(vmaCreateBuffer(m_Allocator, &bufferInfo, &vmaAllocInfo, &newBuffer.buffer, &newBuffer.allocation, nullptr), "VkEngine::CreateBuffer() >> Failed to create buffer!");

	return newBuffer;
}

FrameData& VkEngine::GetCurrentFrame()
{
	return m_Frames[m_FrameNumber % m_FramesOverlapping];
}

size_t VkEngine::PadUniformBufferSize(size_t originalSize)
{
	size_t minAllignment = m_GPUProperties.limits.minUniformBufferOffsetAlignment;
	size_t alignedSize = originalSize;
	if (minAllignment > 0)
	{
		alignedSize = (alignedSize + minAllignment - 1) & ~(minAllignment - 1);
	}

	return alignedSize;
}

SwapChainSupportDetails VkEngine::QuerySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;

	//capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_WindowSurface, &details.capabilities);

	//formats
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_WindowSurface, &formatCount, nullptr);
	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_WindowSurface, &formatCount, details.formats.data());
	}

	//presentation modes
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_WindowSurface, &presentModeCount, nullptr);
	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_WindowSurface, &presentModeCount, details.presentModes.data());
	}

	return details;
}



//////////////////////////////////////////////////////////////////
/****************************************************************/
//						PIPELINEBUILDER							//
/****************************************************************/
//////////////////////////////////////////////////////////////////
VkPipeline GraphicsPipelineBuilder::BuildPipeline(const VkDevice& device, const VkRenderPass& renderPass)
{
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &m_Viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &m_Scissor;

	VkPipelineColorBlendStateCreateInfo  colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &m_ColorBlendAttachment;

	//Build the pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = m_ShaderStages.size();
	pipelineInfo.pStages = m_ShaderStages.data();
	pipelineInfo.pVertexInputState = &m_VertexInputInfo;
	pipelineInfo.pInputAssemblyState = &m_InputAssemblyInfo;
	pipelineInfo.pRasterizationState = &m_RasterizerInfo;
	pipelineInfo.pMultisampleState = &m_MultisampleInfo;
	pipelineInfo.pDepthStencilState = &m_DepthStencilInfo;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.layout = m_PipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VkPipeline newPipeline;
	VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline), "PipelineBuilder::BuildPipeline() >> Failed to create graphics pipeline");

	return newPipeline;
}

VkPipeline ComputePipelineBuilder::BuildPipeline(const VkDevice& device)
{
	//Build the pipeline
	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = m_PipelineLayout;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.stage = m_ShaderStageCreateInfo;

	VkPipeline newPipeline;
	VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline), "PipelineBuilder::BuildPipeline() >> Failed to create graphics pipeline");

	return newPipeline;
}