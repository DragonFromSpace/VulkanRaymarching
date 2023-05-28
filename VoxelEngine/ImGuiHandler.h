// #pragma once
// #include "imgui/imfilebrowser.h"

// class VkEngine;

// class ImGuiHandler
// {
// public:
// 	ImGuiHandler(VkEngine* engine);

// 	void Init();
// 	void Draw();
// 	void Render(VkCommandBuffer cmd);

// private:
// 	VkResult InitDescriptors();
// 	void InitImgui();

// 	void DrawShaderWindow();

// 	VkEngine* m_pEngine;

// 	ImGui::FileBrowser m_FileBrowser;

// 	VkDescriptorPool m_ImguiDescriptorPool;
// };