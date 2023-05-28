#include "pch.h"
#include <iostream>
#include "VkEngine.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int main(int argc, char* argv[])
{
	VkEngine engine;

	engine.Init();

	try
	{
		engine.Run();
	}
	catch (std::runtime_error& e)
	{
		std::cout << "Exception thrown: " << e.what() << '\n';
	}
	engine.Cleanup();

	system("pause");
	return 0;
}