#pragma once
#include <functional>
#include <deque>
#include <unordered_map>

#include "Camera.h"
#include "Texture.h"

#define VK_CHECK(x, msg) if(x != VK_SUCCESS) throw std::runtime_error(msg);

class GraphicsPipelineBuilder
{
public:
	VkPipeline BuildPipeline(const VkDevice& device, const VkRenderPass& renderPass);

	std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages;
	VkPipelineVertexInputStateCreateInfo m_VertexInputInfo;
	VkPipelineInputAssemblyStateCreateInfo m_InputAssemblyInfo;
	VkViewport m_Viewport;
	VkRect2D m_Scissor;
	VkPipelineRasterizationStateCreateInfo m_RasterizerInfo;
	VkPipelineColorBlendAttachmentState m_ColorBlendAttachment;
	VkPipelineMultisampleStateCreateInfo m_MultisampleInfo;
	VkPipelineLayout m_PipelineLayout;
	VkPipelineDepthStencilStateCreateInfo m_DepthStencilInfo;
};

class ComputePipelineBuilder
{
public:
	VkPipeline BuildPipeline(const VkDevice& device);

	VkPipelineShaderStageCreateInfo m_ShaderStageCreateInfo;
	VkPipelineLayout m_PipelineLayout;
};

struct DeletionQueue
{
public:
	void PushFunction(std::function<void()>&& function)
	{
		m_Deletors.push_back(function);
	}

	void Flush()
	{
		for (auto it = m_Deletors.rbegin(); it != m_Deletors.rend(); ++it)
		{
			(*it)();
		}
		m_Deletors.clear();
	}

private:
	std::deque<std::function<void()>> m_Deletors;
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities{};
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct GPUDimensionsData
{
	uint32_t dimX;
	uint32_t dimY;
};

struct GPUSceneData
{
	glm::mat4 ViewMat;
	glm::mat4 ViewInverseMat;
	glm::mat4 ProjInverseMat;
};

struct GPULightData
{
	glm::vec4 lightDirection; //w = intensity
	glm::vec4 lightColor;
};

struct Texture
{
	AllocatedImage image;
	VkImageView imageView;
};

struct FrameData
{
	//semaphores and fences for each frame
	VkSemaphore presentSemaphore, imageTransSemaphore;
	VkFence computeFence, imageTransFence;

	//Command pool and buffer for each frame
	VkCommandPool graphicsCommandPool;
	VkCommandBuffer graphicsCommandBuffer;
	VkCommandPool computeCommandPool;
	VkCommandBuffer computeCommandBuffer;

	//Buffer with camera info and descriptor set for each frame
	AllocatedBuffer dimensionsBuffer;
	AllocatedBuffer sceneBuffer;
	AllocatedBuffer lightBuffer;
	VkDescriptorSet computeDescriptorSet;
};

//Data for the immediate submit
struct UploadContext
{
	VkFence uploadFence;
	VkCommandPool commandPool;
};

static Camera _Camera{glm::vec3(0,0,10)};
static glm::vec2 _PrevMousePos;

class VkEngine
{
public:
	void Init();
	void Cleanup();
	void Run();
	void DrawCompute();
	void Draw();

	//Will push commands immediatly to the graphics queue (mainly used to store textures on the gpu once in the initialization)
	void ImmediateSubmit(std::function<void(VkCommandBuffer)>&& function);

	AllocatedBuffer CreateBuffer(size_t allocationSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

	VmaAllocator m_Allocator;
	DeletionQueue m_DeletionQueue;

private:
	static void GLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void GLFWMouseCallback(GLFWwindow* window, double xPos, double yPos);

	void InitVulkan();
	void InitSwapchain();
	void InitDefaultRenderPass();
	void InitFramebuffers();
	void InitCommands();
	void InitSyncStructures();
	void InitDescriptors();
	void InitPipelines();
	void LoadTextures();

	void Update();

	FrameData& GetCurrentFrame();
	size_t PadUniformBufferSize(size_t originalSize);
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);

	bool m_IsInitialized = false;
	int m_FrameNumber = 0;
	unsigned int m_FramesOverlapping = 2;

#ifdef NDEBUG
	bool m_EnableValidationLayers = false;
#else
	bool m_EnableValidationLayers = true;
#endif

	VkExtent2D m_WindowExtent{ 1280, 720 };
	GLFWwindow* m_pWindow = nullptr;

	VkInstance m_Instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
	VkSurfaceKHR m_WindowSurface = VK_NULL_HANDLE;
	VkDevice m_Device = VK_NULL_HANDLE;
	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties m_GPUProperties;

	VkPipeline m_ComputePipeline;
	VkPipelineLayout m_ComputePipelineLayout;

	VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
	VkFormat m_SwapchainImageFormat = VK_FORMAT_UNDEFINED;
	std::vector<VkImage> m_SwapchainImages;
	std::vector<VkImageView> m_SwapchainImageViews;

	VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
	uint32_t m_GraphicsQueueFamily;

	VkQueue m_ComputeQueue = VK_NULL_HANDLE;
	uint32_t m_ComputeQueueFamily;

	VkRenderPass m_RenderPass = VK_NULL_HANDLE;
	std::vector<VkFramebuffer> m_Framebuffers;

	VkDescriptorSetLayout m_ComputeDescriptorSetLayout;
	VkDescriptorPool m_DescriptorPool;

	std::vector<FrameData> m_Frames;

	UploadContext m_UploadContext;

	Texture m_SkyBoxTexture;

	int m_FPSCounter;
	float m_FPSTimePassed;
};