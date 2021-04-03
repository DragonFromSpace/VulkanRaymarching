#include "pch.h"
#include "Texture.h"

#include "stb_image.h"
#include "VkEngine.h"

bool VkUtils::LoadFromFile(VkEngine& engine, const char* fileLocation, AllocatedImage& outImage)
{
	int texWidth, texHeight, texChannels;

	//load pixels from the image
	stbi_uc* pixels = stbi_load(fileLocation, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	if (!pixels)
	{
		std::cout << "vkUtil::LoadImageFromFile() >> Failed to load pixels from image!";
		return false;
	}

	//store pixels in staging buffer
	void* pixelPtr = pixels;
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	AllocatedBuffer stagingBuffer = engine.CreateBuffer(imageSize , VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, false);

	void* data;
	VK_CHECK(vmaMapMemory(engine.m_Allocator, stagingBuffer.allocation, &data), "Texture::LoadFromFile() >> Failed to map image memory!");
	memcpy(data, pixelPtr, static_cast<size_t>(imageSize));
	vmaUnmapMemory(engine.m_Allocator, stagingBuffer.allocation);

	stbi_image_free(pixels);

	//Create the image by copying the staging buffer into the image
	VkExtent3D imageExtent{};
	imageExtent.width = texWidth;
	imageExtent.height = texHeight;
	imageExtent.depth = 1;

	VkImageCreateInfo imageCreateInfo = vkInit::ImageCreateInfo(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);
	AllocatedImage image;
	VmaAllocationCreateInfo imageAllocInfo{};
	imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	VK_CHECK(vmaCreateImage(engine.m_Allocator, &imageCreateInfo, &imageAllocInfo, &image.image, &image.allocation, nullptr), "Texture::LoadFromFile() >> Failed to create image!");

	//transition the image and copy the buffer into it
	engine.ImmediateSubmit([&](VkCommandBuffer cmdBuffer)
		{
			//Tells what part of the image will be transformed (only 1 layer and no mip levels so just the first one)
			VkImageSubresourceRange range;
			range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			range.baseMipLevel = 0;
			range.levelCount = 1;
			range.baseArrayLayer = 0;
			range.layerCount = 1;

			//Create a barrier that transitions the image layout
			VkImageMemoryBarrier imageBarrier_ToTransfer{};
			imageBarrier_ToTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageBarrier_ToTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; //image started from undefined
			imageBarrier_ToTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; //transfer_dst marks the image to be the destination of a transfer
			imageBarrier_ToTransfer.image = image.image;
			imageBarrier_ToTransfer.subresourceRange = range;

			imageBarrier_ToTransfer.srcAccessMask = 0;
			imageBarrier_ToTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			//Bind the barrier to the pipeline where the GPU execution will pause going from top stage to the transition stage while the image is not yet changed
			vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_ToTransfer);

			//Copy the buffer into the image
			VkBufferImageCopy copyRegion{};
			copyRegion.bufferOffset = 0;
			copyRegion.bufferRowLength = 0;
			copyRegion.bufferImageHeight = 0;

			copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.imageSubresource.mipLevel = 0;
			copyRegion.imageSubresource.baseArrayLayer = 0;
			copyRegion.imageSubresource.layerCount = 1;
			copyRegion.imageExtent = imageExtent;
			vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

			//Change the layout of the image again so the shader can read it
			VkImageMemoryBarrier imageBarrier_ToReadable = imageBarrier_ToTransfer;
			imageBarrier_ToReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageBarrier_ToReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			
			imageBarrier_ToReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageBarrier_ToReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_ToReadable);
		});

	//Cleanup
	engine.m_DeletionQueue.PushFunction([=]()
		{
			vmaDestroyImage(engine.m_Allocator, image.image, image.allocation);
		});

	vmaDestroyBuffer(engine.m_Allocator, stagingBuffer.buffer, stagingBuffer.allocation);

	std::cout << "Texture " << fileLocation << " loaded successfully" << '\n';

	outImage = image;
	return true;
}