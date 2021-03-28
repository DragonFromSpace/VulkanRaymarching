#pragma once
class VkEngine;

class ImGuiHandler
{
public:
	ImGuiHandler(VkEngine* engine);

	void Init();
	void Draw();
	void Render(VkCommandBuffer cmd);

private:
	VkResult InitDescriptors();
	void InitImgui();

	VkEngine* m_pEngine;

	VkDescriptorPool m_ImguiDescriptorPool;
};