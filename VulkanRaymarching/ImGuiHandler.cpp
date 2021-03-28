#include "pch.h"
#include "ImGuiHandler.h"

#include "VkEngine.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

ImGuiHandler::ImGuiHandler(VkEngine* engine)
	:m_pEngine{engine}
{}

void ImGuiHandler::Init()
{
	InitDescriptors();
	InitImgui();
}

void ImGuiHandler::Draw()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	ImGui::NewFrame();
	ImGui::ShowDemoWindow();
	ImGui::Render();
}

void ImGuiHandler::Render(VkCommandBuffer cmd)
{
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

VkResult ImGuiHandler::InitDescriptors()
{
	VkDescriptorPoolSize poolSizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = 0;
	poolInfo.maxSets = 1000;
	poolInfo.poolSizeCount = std::size(poolSizes);
	poolInfo.pPoolSizes = poolSizes;

	return vkCreateDescriptorPool(m_pEngine->m_Device, &poolInfo, nullptr, &m_ImguiDescriptorPool);
}

void ImGuiHandler::InitImgui()
{
	ImGui::CreateContext();

	//Init GLFW
	ImGui_ImplGlfw_InitForVulkan(m_pEngine->m_pWindow, true);

	//Init vulkan
	ImGui_ImplVulkan_InitInfo initInfo{};
	initInfo.Instance = m_pEngine->m_Instance;
	initInfo.PhysicalDevice = m_pEngine->m_PhysicalDevice;
	initInfo.Device = m_pEngine->m_Device;
	initInfo.Queue = m_pEngine->m_GraphicsQueue;
	initInfo.DescriptorPool = m_ImguiDescriptorPool;
	initInfo.MinImageCount = 2;
	initInfo.ImageCount = m_pEngine->m_Frames.size();
	
	ImGui_ImplVulkan_Init(&initInfo, m_pEngine->m_RenderPass);

	//Upload font textures to gpu
	m_pEngine->ImmediateSubmit([&](VkCommandBuffer buffer)
		{
			ImGui_ImplVulkan_CreateFontsTexture(buffer);
		});

	//Clear font textures
	ImGui_ImplVulkan_DestroyFontUploadObjects();

	//Mark for deletion
	m_pEngine->m_DeletionQueue.PushFunction([=]()
		{
			vkDestroyDescriptorPool(m_pEngine->m_Device, m_ImguiDescriptorPool, nullptr);
			ImGui_ImplVulkan_Shutdown();
		});
}
