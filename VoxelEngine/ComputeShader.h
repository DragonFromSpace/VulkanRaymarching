#pragma once
#include "Shader.h"

class VkEngine;

class ComputeShader : public Shader
{
public:
	struct DimensionsBufferData
	{
		uint32_t dimX;
		uint32_t dimY;
	};

	struct SceneBufferData
	{
		glm::mat4 viewMat;
		glm::mat4 viewInverseMat;
		glm::mat4 projInverseMat;
		float time;
	};

	struct LightBufferData
	{
		glm::vec4 lightDirection;
		glm::vec4 lightColor;
	};

	ComputeShader(const VkDevice& device, const std::string& computeShaderFile);

	void SetSkyboxTexture(VkImageView* skyboxTexture);
	void SetSwapchainImage(VkImageView* swapchainImage);

	virtual void InitDescriptors(int overlappingFrames, VkEngine* engine);

	virtual void UpdateShaderVariables(int currentFrame, VkEngine* engine);

	const AllocatedBuffer& GetDimensionsBuffer(int currentFrame) { return m_FrameData[currentFrame].dimensionsBuffer; }
	const AllocatedBuffer& GetLightBuffer(int currentFrame) { return m_FrameData[currentFrame].lightBuffer; }
	const AllocatedBuffer& GetSceneBuffer(int currentFrame) { return m_FrameData[currentFrame].sceneBuffer; }
	const VkDescriptorSet& GetDescriptorSet(int currentFrame) { return m_FrameData[currentFrame].descriptorSet; }

	void SetDimensionsBufferData(DimensionsBufferData& bufferData) { m_DimensionsBufferData = bufferData; }
	void SetSceneBufferData(SceneBufferData& bufferData) { m_SceneBufferData = bufferData; }
	void SetLightBufferData(LightBufferData& bufferData) { m_LightBufferData = bufferData; }

private:
	struct FrameData
	{
		AllocatedBuffer sceneBuffer;
		AllocatedBuffer lightBuffer;
		AllocatedBuffer dimensionsBuffer;

		VkDescriptorSet descriptorSet;
	};
	std::vector<FrameData> m_FrameData;

	VkImageView* m_SkyboxTexture;
	VkImageView* m_SwapchainImage;

	//Shader variables
	DimensionsBufferData m_DimensionsBufferData;
	SceneBufferData m_SceneBufferData;
	LightBufferData m_LightBufferData;
};