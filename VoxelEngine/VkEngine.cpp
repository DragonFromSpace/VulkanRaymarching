#include "pch.h"
#include "VkEngine.h"
#include "ComputeShader.h"
#include <string>
#include <chrono>

static bool isMouseHidden = true;

void VkEngine::GLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		if (isMouseHidden)
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			isMouseHidden = false;
		}
		else
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			isMouseHidden = true;
		}
	}
}

void VkEngine::GLFWMouseCallback(GLFWwindow* window, double xPos, double yPos)
{
	//get movement normalized
	glm::vec2 deltaMouse = _PrevMousePos - glm::vec2(xPos, yPos);

	//Update camera rotation
	if(isMouseHidden)
		_Camera.ProcessMouseMovement(deltaMouse.x, deltaMouse.y);

	//Update pos
	_PrevMousePos.x = (float)xPos;
	_PrevMousePos.y = (float)yPos;
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
	InitUIRenderPass();
	InitFramebuffers();
	InitCommands();
	InitSyncStructures();
	LoadTextures();
	InitShaders();
	InitDescriptors();
	InitPipelines();

	//m_ImGui.Init();

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

		delete m_ComputeShader;
	}
}

void VkEngine::ReloadShaders()
{
	CleanPipelines();

	m_ComputeShader->ReloadShader(m_CurrentShader);
	InitPipelines();
}

void VkEngine::Run()
{
	while (!glfwWindowShouldClose(m_pWindow))
	{
		glfwPollEvents();
		Update();

		//Imgui
		//m_ImGui.Draw();

		Draw();
	}

	vkDeviceWaitIdle(m_Device);
}

void VkEngine::Draw()
{
	uint32_t currentFrame;
	VK_CHECK(vkAcquireNextImageKHR(m_Device, m_Swapchain, UINT64_MAX, GetCurrentFrame().presentSemaphore, nullptr, &currentFrame), "VkEngine::Draw() >> Failed to acquire next image in swapchain!");

	DrawCompute(currentFrame);
	DrawGraphics(currentFrame);
}

FrameData& VkEngine::GetCurrentFrame()
{
	return m_Frames[m_FrameNumber % m_OverlappingFrameCount];
}

void VkEngine::DrawCompute(uint32_t frameNumber)
{
	//Reset command buffer
	vkResetCommandBuffer(m_Frames[frameNumber].computeCommandBuffer, 0);

	//Run the compute buffer
	VkCommandBufferBeginInfo beginInfo = vkInit::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK(vkBeginCommandBuffer(m_Frames[frameNumber].computeCommandBuffer, &beginInfo), "VkEngine::DrawCompute() >> Failed to begin command buffer!");

	//Bind compute pipeline
	vkCmdBindPipeline(m_Frames[frameNumber].computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline);

	//bind descriptor sets
	vkCmdBindDescriptorSets(m_Frames[frameNumber].computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipelineLayout, 0, 1, &m_ComputeShader->GetDescriptorSet(frameNumber), 0, nullptr);

	//Dispatch compute
	uint32_t groupsX = (uint32_t)glm::ceil(m_WindowExtent.width / 32.0f);
	uint32_t groupsY = (uint32_t)glm::ceil(m_WindowExtent.height / 32.0f);
	vkCmdDispatch(m_Frames[frameNumber].computeCommandBuffer, groupsX, groupsY, 1);

	VK_CHECK(vkEndCommandBuffer(m_Frames[frameNumber].computeCommandBuffer), "VkEngine::DrawCompute() Failed to end command buffer!");

	//Submit the queue
	VkPipelineStageFlags waitPipelineFlag = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_Frames[frameNumber].computeCommandBuffer;
	submitInfo.waitSemaphoreCount = 1; //wait untill the image is good to go
	submitInfo.pWaitSemaphores = &m_Frames[frameNumber].presentSemaphore;
	submitInfo.pWaitDstStageMask = &waitPipelineFlag;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_Frames[frameNumber].computeSempahore;
	vkQueueSubmit(m_ComputeQueue, 1, &submitInfo, VK_NULL_HANDLE);
}

void VkEngine::DrawGraphics(uint32_t frameNumber)
{
	//START RECORDING GRAPHICS COMMAND BUFFER
	VkCommandBufferBeginInfo beginInfo = vkInit::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	vkBeginCommandBuffer(m_Frames[frameNumber].graphicsCommandBuffer, &beginInfo);

	//Transition image from undefined to src_present
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = m_SwapchainImages[frameNumber];
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	barrier.dstAccessMask = 0;

	vkCmdPipelineBarrier(m_Frames[frameNumber].graphicsCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	
	//Begin a render pass
	VkClearValue clearValue{};
	clearValue.color = { 0.7f, 0.1f, 0.1f, 1.0f };

	VkRenderPassBeginInfo uiRpBeginInfo{};
	uiRpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	uiRpBeginInfo.renderPass = m_UIRenderPass;
	uiRpBeginInfo.framebuffer = m_Framebuffers[frameNumber];
	uiRpBeginInfo.renderArea.extent.width = m_WindowExtent.width;
	uiRpBeginInfo.renderArea.extent.height = m_WindowExtent.height;
	uiRpBeginInfo.clearValueCount = 1;
	uiRpBeginInfo.pClearValues = &clearValue;

	//Secons pass for the UI
	vkCmdBeginRenderPass(m_Frames[frameNumber].graphicsCommandBuffer, &uiRpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	//m_ImGui.Render(m_Frames[frameNumber].graphicsCommandBuffer);
	vkCmdEndRenderPass(m_Frames[frameNumber].graphicsCommandBuffer);

	vkEndCommandBuffer(m_Frames[frameNumber].graphicsCommandBuffer);

	//Submit the queue
	VkPipelineStageFlags waitPipelineFlag = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
	VkSubmitInfo submitInfo = vkInit::SubmitInfo(&m_Frames[frameNumber].graphicsCommandBuffer);
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &m_Frames[frameNumber].computeSempahore;
	submitInfo.pWaitDstStageMask = &waitPipelineFlag;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_Frames[frameNumber].imageTransSemaphore;

	vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_Frames[frameNumber].graphicsFence);

	//Make sure the graphics queue is done before presenting the final image
	vkWaitForFences(m_Device, 1, &m_Frames[frameNumber].graphicsFence, VK_TRUE, UINT64_MAX);
	vkResetFences(m_Device, 1, &m_Frames[frameNumber].graphicsFence);
	vkResetCommandBuffer(m_Frames[frameNumber].graphicsCommandBuffer, 0);

	//PRESENT the image in the swapchain
	VkPresentInfoKHR presentInfo = vkInit::PresentInfoKHR();
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_Swapchain;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_Frames[frameNumber].imageTransSemaphore;
	presentInfo.pImageIndices = &frameNumber;
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
	if (!vkbInstanceResult)
	{
    	std::cerr << "Failed to create Vulkan instance. Error: " << vkbInstanceResult.error().message() << "\n";
	}

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

	//Create logical device
	vkb::DeviceBuilder deviceBuilder{ physDevice };
	vkb::Device device = deviceBuilder.build().value();
	m_Device = device.device;

	//Get the graphics queue
	auto graphicsQueue = device.get_queue(vkb::QueueType::graphics);
	if(!graphicsQueue)
		throw std::runtime_error("VkEngine::InitVulksan() >> No graphics queue found on this device!");
	
	m_GraphicsQueue = device.get_queue(vkb::QueueType::graphics).value();
	m_GraphicsQueueFamily = device.get_queue_index(vkb::QueueType::graphics).value();

	//Get compute queue
	auto computeQueue = device.get_queue(vkb::QueueType::compute);
	if (!computeQueue)
	{
		//does graphics queue support compute?
		if ((device.queue_families[m_GraphicsQueueFamily].queueFlags & 2) == 2)
		{
			m_ComputeQueue = m_GraphicsQueue;
			m_ComputeQueueFamily = m_GraphicsQueueFamily;
		}
		else
		{
			throw std::runtime_error("VkEngine::InitVulkan() >> No compute queue found on this device!");
		}
	}
	else
	{
		m_ComputeQueue = computeQueue.value();
		m_ComputeQueueFamily = device.get_queue_index(vkb::QueueType::compute).value();
	}

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
	m_OverlappingFrameCount = (unsigned int)m_Frames.size();

	//Add to deletion queue
	m_DeletionQueue.PushFunction([=]()
		{
			for (VkImageView image : m_SwapchainImageViews)
				vkDestroyImageView(m_Device, image, nullptr);
			vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
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
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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

void VkEngine::InitUIRenderPass()
{
	//COLOR
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_SwapchainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
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

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPass{};
	renderPass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPass.attachmentCount = 1;
	renderPass.pAttachments = &colorAttachment;
	renderPass.subpassCount = 1;
	renderPass.pSubpasses = &subpass;
	renderPass.dependencyCount = 1;
	renderPass.pDependencies = &dependency;

	VK_CHECK(vkCreateRenderPass(m_Device, &renderPass, nullptr, &m_UIRenderPass), "VkEngine::InitUIRenderPass() >> Failed to create render pass!");
	m_DeletionQueue.PushFunction([=]() {vkDestroyRenderPass(m_Device, m_UIRenderPass, nullptr); });
}

void VkEngine::InitCommands()
{
	//Create graphics and compute command pool
	VkCommandPoolCreateInfo commandPoolInfo = vkInit::CommandPoolCreateInfo(m_GraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	VkCommandPoolCreateInfo computeCommandPool = vkInit::CommandPoolCreateInfo(m_ComputeQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	for (unsigned int i = 0; i < m_OverlappingFrameCount; ++i)
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

void VkEngine::InitFramebuffers()
{
	VkFramebufferCreateInfo framebufferInfo = vkInit::FramebufferCreateInfo(m_RenderPass, m_WindowExtent);

	uint32_t imageCount = (uint32_t)m_SwapchainImages.size();
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

	for (unsigned int i = 0; i < m_OverlappingFrameCount; ++i)
	{
		VK_CHECK(vkCreateFence(m_Device, &fenceInfo, nullptr, &m_Frames[i].graphicsFence), "VkEngine::InitSyncStructures() >> Failed to create compute fence!");

		VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_Frames[i].presentSemaphore), "VkEngine::InitSyncStructures() >> Failed to create present semaphore!");
		VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_Frames[i].imageTransSemaphore), "VkEngine::InitSyncStructures() >> Failed to create present semaphore!");
		VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_Frames[i].computeSempahore), "VkEngine::InitSyncStructures() >> Failed to create compute semaphore!");

		m_DeletionQueue.PushFunction([=]()
			{
				vkDestroyFence(m_Device, m_Frames[i].graphicsFence, nullptr);
				vkDestroySemaphore(m_Device, m_Frames[i].presentSemaphore, nullptr);
				vkDestroySemaphore(m_Device, m_Frames[i].imageTransSemaphore, nullptr);
				vkDestroySemaphore(m_Device, m_Frames[i].computeSempahore, nullptr);
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

void VkEngine::InitShaders()
{
	m_ComputeShader = new ComputeShader(m_Device, m_CurrentShader);
}

void VkEngine::InitDescriptors()
{
	m_ComputeShader->SetSkyboxTexture(&m_SkyBoxTexture.imageView);
	m_ComputeShader->SetSwapchainImage(m_SwapchainImageViews.data());
	m_ComputeShader->InitDescriptors(m_OverlappingFrameCount, this);
}

void VkEngine::InitPipelines()
{
	//Init shaders
	VkShaderModule computeShaderModule = m_ComputeShader->GetComputeShaderModule();

	//Create compute pipeline layout
	VkPipelineLayoutCreateInfo computePipelineLayoutCreateInfo = vkInit::PipelineLayoutCreateInfo();
	computePipelineLayoutCreateInfo.setLayoutCount = 1;
	computePipelineLayoutCreateInfo.pSetLayouts = &m_ComputeShader->GetDescriptorSetLayout();

	VK_CHECK(vkCreatePipelineLayout(m_Device, &computePipelineLayoutCreateInfo, nullptr, &m_ComputePipelineLayout), "VkEngine::InitPipelines() >> Failed to create compute pipeline layout!");

	VkPipelineLayoutCreateInfo graphicsLayoutCreateInfo = vkInit::PipelineLayoutCreateInfo();
	graphicsLayoutCreateInfo.setLayoutCount = 0;

	//Create base pipeline
	ComputePipelineBuilder builder{};
	builder.m_PipelineLayout = m_ComputePipelineLayout;
	builder.m_ShaderStageCreateInfo = vkInit::PipelineShaderStageInfo(VK_SHADER_STAGE_COMPUTE_BIT, computeShaderModule);
	m_ComputePipeline = builder.BuildPipeline(m_Device);

	m_ComputeShader->CleanModules();
}

void VkEngine::LoadTextures()
{
	Texture skybox;

	bool b = VkUtils::LoadFromFile(*this, "../Resources/Textures/quarry_02_2k.jpg", skybox.image);
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

	//Update shader variables
	ComputeShader::DimensionsBufferData dimBufferData;
	dimBufferData.dimX = m_WindowExtent.width;
	dimBufferData.dimY = m_WindowExtent.height;
	m_ComputeShader->SetDimensionsBufferData(dimBufferData);

	ComputeShader::SceneBufferData sceneData;
	glm::mat4 view = _Camera.GetViewMatrix();
	glm::mat4 proj = glm::perspective(glm::radians(70.0f), (float)m_WindowExtent.width / (float)m_WindowExtent.height, 0.1f, 200.0f);
	sceneData.viewMat = view;
	sceneData.viewInverseMat = glm::inverse(view);
	sceneData.projInverseMat = glm::inverse(proj);
	sceneData.time = (float)glfwGetTime();
	m_ComputeShader->SetSceneBufferData(sceneData);

	ComputeShader::LightBufferData lightData;
	lightData.lightColor = glm::vec4(1.0f, 1.0f, 0.95f, 1.0f);
	lightData.lightDirection = glm::normalize(glm::vec4(0.5f, -0.9f, 0.3f, 1.0f));
	m_ComputeShader->SetLightBufferData(lightData);

	m_ComputeShader->UpdateShaderVariables(m_FrameNumber % m_OverlappingFrameCount, this);
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

AllocatedBuffer VkEngine::CreateBuffer(size_t allocationSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, bool markDeletion)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

	bufferInfo.size = allocationSize;
	bufferInfo.usage = usage;

	VmaAllocationCreateInfo vmaAllocInfo{};
	vmaAllocInfo.usage = memoryUsage;

	AllocatedBuffer newBuffer{};
	VK_CHECK(vmaCreateBuffer(m_Allocator, &bufferInfo, &vmaAllocInfo, &newBuffer.buffer, &newBuffer.allocation, nullptr), "VkEngine::CreateBuffer() >> Failed to create buffer!");

	if (markDeletion)
	{
		m_DeletionQueue.PushFunction([=]() {vmaDestroyBuffer(m_Allocator, newBuffer.buffer, newBuffer.allocation); });
	}

	return newBuffer;
}

void* VkEngine::GetBufferMemory(const AllocatedBuffer& buffer)
{
	void* data;
	vmaMapMemory(m_Allocator, buffer.allocation, &data);
	return data;
}

void VkEngine::ReleaseBufferMemory(const AllocatedBuffer& buffer)
{
	vmaUnmapMemory(m_Allocator, buffer.allocation);
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
	pipelineInfo.stageCount = (uint32_t)m_ShaderStages.size();
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