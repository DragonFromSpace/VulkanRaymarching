#pragma once
#include "Shader.h"

class VkEngine;

class ComputeShader : public Shader
{
public:
	ComputeShader(const VkDevice& device, const std::string& computeShaderFile);

	void SetSkyboxTexture(VkImageView* skyboxTexture);
	void SetSwapchainImage(VkImageView* swapchainImage);

	virtual void InitDescriptors(int overlappingFrames, VkEngine* engine);

	const AllocatedBuffer& GetDimensionsBuffer(int currentFrame) { return m_FrameData[currentFrame].dimensionsBuffer; }
	const AllocatedBuffer& GetLightBuffer(int currentFrame) { return m_FrameData[currentFrame].lightBuffer; }
	const AllocatedBuffer& GetSceneBuffer(int currentFrame) { return m_FrameData[currentFrame].sceneBuffer; }
	const VkDescriptorSet& GetDescriptorSet(int currentFrame) { return m_FrameData[currentFrame].descriptorSet; }

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
};