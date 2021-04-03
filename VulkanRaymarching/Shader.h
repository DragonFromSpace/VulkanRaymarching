#pragma once
#include <map>

class VkEngine;

class Shader
{
public:
	Shader(const VkDevice& device, const std::string& vertShaderFile, const std::string& fragShaderFile);
	Shader(const VkDevice& device, const std::string& computeShaderFile);

	const VkShaderModule& GetVertShaderModule() { return m_VertShaderModule; }
	const VkShaderModule& GetFragShaderModule() { return m_FragShaderModule; }
	const VkShaderModule& GetComputeShaderModule() { return m_ComputeShaderModule; }
	const VkDescriptorSetLayout& GetDescriptorSetLayout() { return m_descriptorSetLayout; }

	virtual void InitDescriptors(int overlappingFrames, VkEngine* engine) = 0; //TODO: make abstract
	virtual void UpdateShaderVariables(int currentFrame, VkEngine* engine) = 0;

	void ReloadShader(std::string newVertLocation, std::string newFragLocation);
	void ReloadShader(std::string newComputeLocation);

	virtual void CleanModules();

protected:
	VkDevice m_Device = VK_NULL_HANDLE;

	VkDescriptorPool m_DescriptorPool;
	VkDescriptorSetLayout m_descriptorSetLayout;

private:
	void LoadVertAndFragShader();
	void LoadComputeShader();

	std::vector<char> ReadFile(const std::string& fileName);
	VkShaderModule CreateShaderModule(const VkDevice& device, const std::vector<char>& byteCode);

	VkShaderModule m_VertShaderModule = VK_NULL_HANDLE;
	VkShaderModule m_FragShaderModule = VK_NULL_HANDLE;
	VkShaderModule m_ComputeShaderModule = VK_NULL_HANDLE;

	std::string m_VertLocation;
	std::string m_FragLocation;
	std::string m_ComputeLocation;
};