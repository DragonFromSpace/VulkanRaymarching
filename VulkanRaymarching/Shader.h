#pragma once
class Shader
{
public:
	Shader(const VkDevice& device, const std::string& vertShaderFile, const std::string& fragShaderFile);
	Shader(const VkDevice& device, const std::string& computeShaderFile);
	~Shader();

	const VkShaderModule& GetVertShaderModule() { return m_VertShaderModule; }
	const VkShaderModule& GetFragShaderModule() { return m_FragShaderModule; }
	const VkShaderModule& GetComputeShaderModule() { return m_ComputeShaderModule; }

private:
	std::vector<char> ReadFile(const std::string& fileName);
	VkShaderModule CreateShaderModule(const VkDevice& device, const std::vector<char>& byteCode);

	VkDevice m_Device = VK_NULL_HANDLE;

	VkShaderModule m_VertShaderModule = VK_NULL_HANDLE;
	VkShaderModule m_FragShaderModule = VK_NULL_HANDLE;
	VkShaderModule m_ComputeShaderModule = VK_NULL_HANDLE;
};