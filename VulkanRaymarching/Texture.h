#pragma once
class VkEngine;

namespace VkUtils
{
	bool LoadFromFile(VkEngine& engine, const char* fileLocation, AllocatedImage& outImage);
}