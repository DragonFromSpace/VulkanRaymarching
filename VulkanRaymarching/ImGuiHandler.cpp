// #include "pch.h"
// #include "ImGuiHandler.h"

// #include "VkEngine.h"

// #include "imgui/imgui.h"
// #include "imgui/imgui_impl_glfw.h"
// #include "imgui/imgui_impl_vulkan.h"

// ImGuiHandler::ImGuiHandler(VkEngine* engine)
// 	:m_pEngine{engine}
// {
// 	m_FileBrowser.SetTitle("Shader selector");
// 	m_FileBrowser.SetTypeFilters({".spv"}); //only look for compiled shader code //TODO: search for the actual file and compile during runtime
// 	m_FileBrowser.SetPwd("../Resources/Shaders"); //start in the shader folder
// }

// void ImGuiHandler::Init()
// {
// 	InitDescriptors();
// 	InitImgui();
// }

// void ImGuiHandler::Draw()
// {
// 	ImGui_ImplVulkan_NewFrame();
// 	ImGui_ImplGlfw_NewFrame();

// 	ImGui::NewFrame();

// 	DrawShaderWindow();

// 	ImGui::Render();
// }

// void ImGuiHandler::Render(VkCommandBuffer cmd)
// {
// 	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
// }

// VkResult ImGuiHandler::InitDescriptors()
// {
// 	VkDescriptorPoolSize poolSizes[] =
// 	{
// 		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
// 		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
// 		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
// 		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
// 		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
// 		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
// 		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
// 		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
// 		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
// 		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
// 		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
// 	};

// 	VkDescriptorPoolCreateInfo poolInfo{};
// 	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
// 	poolInfo.flags = 0;
// 	poolInfo.maxSets = 1000;
// 	poolInfo.poolSizeCount = std::size(poolSizes);
// 	poolInfo.pPoolSizes = poolSizes;

// 	return vkCreateDescriptorPool(m_pEngine->m_Device, &poolInfo, nullptr, &m_ImguiDescriptorPool);
// }

// void ImGuiHandler::InitImgui()
// {
// 	ImGui::CreateContext();

// 	//Init GLFW
// 	ImGui_ImplGlfw_InitForVulkan(m_pEngine->m_pWindow, true);

// 	//Init vulkan
// 	ImGui_ImplVulkan_InitInfo initInfo{};
// 	initInfo.Instance = m_pEngine->m_Instance;
// 	initInfo.PhysicalDevice = m_pEngine->m_PhysicalDevice;
// 	initInfo.Device = m_pEngine->m_Device;
// 	initInfo.Queue = m_pEngine->m_GraphicsQueue;
// 	initInfo.DescriptorPool = m_ImguiDescriptorPool;
// 	initInfo.MinImageCount = 2;
// 	initInfo.ImageCount = m_pEngine->m_Frames.size();
	
// 	ImGui_ImplVulkan_Init(&initInfo, m_pEngine->m_RenderPass);

// 	//Upload font textures to gpu
// 	m_pEngine->ImmediateSubmit([&](VkCommandBuffer buffer)
// 		{
// 			ImGui_ImplVulkan_CreateFontsTexture(buffer);
// 		});

// 	//Clear font textures
// 	ImGui_ImplVulkan_DestroyFontUploadObjects();

// 	//Mark for deletion
// 	m_pEngine->m_DeletionQueue.PushFunction([=]()
// 		{
// 			vkDestroyDescriptorPool(m_pEngine->m_Device, m_ImguiDescriptorPool, nullptr);
// 			ImGui_ImplVulkan_Shutdown();
// 		});
// }

// void ImGuiHandler::DrawShaderWindow()
// {
// 	ImGui::Begin("Shader selector");

// 	//Open file browser
// 	ImGui::Text((std::string("Current shader: ") + m_pEngine->m_CurrentShader).c_str());
// 	if (ImGui::Button("Browse...", { 60, 30 }))
// 	{
// 		m_FileBrowser.Open();
// 	}

// 	if (ImGui::Button("Reload shader", { 100, 40 }))
// 	{
// 		m_pEngine->ReloadShaders();
// 	}

// 	//Display the browser
// 	m_FileBrowser.Display();

// 	//When file is selected
// 	if (m_FileBrowser.HasSelected())
// 	{
// 		m_pEngine->m_CurrentShader = m_FileBrowser.GetSelected().string();
// 		m_FileBrowser.ClearSelected();
// 		m_pEngine->ReloadShaders();
// 	}

// 	ImGui::End();
// }
