#include "pch.h"
#include "Shader.h"
#include <fstream>

Shader::Shader(const VkDevice& device, const std::string& vertShaderFile, const std::string& fragShaderFile)
{
	m_Device = device;

	auto vertShaderCode = ReadFile(vertShaderFile);
	auto fragShaderCode = ReadFile(fragShaderFile);

	m_VertShaderModule = CreateShaderModule(device, vertShaderCode);
	m_FragShaderModule = CreateShaderModule(device, fragShaderCode);
}

Shader::Shader(const VkDevice& device, const std::string& computeShaderFile)
{
	m_Device = device;

	auto computeShaderCode = ReadFile(computeShaderFile);
	m_ComputeShaderModule = CreateShaderModule(device, computeShaderCode);
}

Shader::~Shader()
{
	vkDestroyShaderModule(m_Device, m_VertShaderModule, nullptr);
	vkDestroyShaderModule(m_Device, m_FragShaderModule, nullptr);
	vkDestroyShaderModule(m_Device, m_ComputeShaderModule, nullptr);
}

std::vector<char> Shader::ReadFile(const std::string& fileName)
{
	//use ate to start at the end of the file to easily get the size of the file
	std::ifstream file(fileName, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("Shader::ReadFile() >> Failed to open file!");
	}

	//Get the size of the file and init buffer for the chars
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	//Go to beginning of file and start reading data
	file.seekg(0);
	file.read(buffer.data(), fileSize);

	//cleanup
	file.close();

	if (buffer.size() != fileSize)
	{
		throw std::runtime_error("Shader::ReadFile() >> Failed to read shader code!");
	}

	return buffer;
}

VkShaderModule Shader::CreateShaderModule(const VkDevice& device, const std::vector<char>& byteCode)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = byteCode.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(byteCode.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("Shader::CreateShaderModule() >> Failed to create shaderModule!");
	}

	return shaderModule;
}