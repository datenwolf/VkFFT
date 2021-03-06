﻿#include <iostream>
#include <vkFFT.h>
#include <vulkan/vulkan.h>
#include <string.h>
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

VkInstance instance = {};
VkDebugReportCallbackEXT debugReportCallback = {};
VkPhysicalDevice physicalDevice = {};
VkPhysicalDeviceProperties physicalDeviceProperties = {};
VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties = {};
VkDevice device = {};
VkDebugUtilsMessengerEXT debugMessenger = {};
uint32_t queueFamilyIndex = {};
std::vector<const char*> enabledLayers;
VkQueue queue = {};
VkCommandPool commandPool = {};
VkFence fence = {};

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};
static VKAPI_ATTR VkBool32 VKAPI_CALL debugReportCallbackFn(
	VkDebugReportFlagsEXT                       flags,
	VkDebugReportObjectTypeEXT                  objectType,
	uint64_t                                    object,
	size_t                                      location,
	int32_t                                     messageCode,
	const char* pLayerPrefix,
	const char* pMessage,
	void* pUserData) {

	printf("Debug Report: %s: %s\n", pLayerPrefix, pMessage);

	return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}


void setupDebugMessenger() {
	if (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;

	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger");
	}
}

std::vector<const char*> getRequiredExtensions() {
	std::vector<const char*> extensions;

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

bool checkValidationLayerSupport() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

void createInstance() {
	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("validation layers creation failed");
	}

	VkApplicationInfo applicationInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	applicationInfo.pApplicationName = "VkFFT";
	applicationInfo.applicationVersion = 1.0;
	applicationInfo.pEngineName = "VkFFT";
	applicationInfo.engineVersion = 1.0;
	applicationInfo.apiVersion = VK_API_VERSION_1_1;;

	VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	createInfo.flags = 0;
	createInfo.pApplicationInfo = &applicationInfo;

	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugCreateInfo.pfnUserCallback = debugCallback;
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;

		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("instance creation failed");
	}


}

void findPhysicalDevice() {

	uint32_t deviceCount;
	vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
	if (deviceCount == 0) {
		throw std::runtime_error("device with vulkan support not found");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());


	for (VkPhysicalDevice device : devices) {
		if (true) {
			physicalDevice = device;
			break;
		}
	}
}

uint32_t getComputeQueueFamilyIndex() {
	uint32_t queueFamilyCount;

	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	uint32_t i = 0;
	for (; i < queueFamilies.size(); ++i) {
		VkQueueFamilyProperties props = queueFamilies[i];

		if (props.queueCount > 0 && (props.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
			break;
		}
	}

	if (i == queueFamilies.size()) {
		throw std::runtime_error("queue family creation failed");
	}

	return i;
}

void createDevice() {

	VkDeviceQueueCreateInfo queueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	queueFamilyIndex = getComputeQueueFamilyIndex();
	queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
	queueCreateInfo.queueCount = 1;
	float queuePriorities = 1.0;
	queueCreateInfo.pQueuePriorities = &queuePriorities;
	VkDeviceCreateInfo deviceCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.shaderFloat64 = true;
	deviceCreateInfo.enabledLayerCount = enabledLayers.size();
	deviceCreateInfo.ppEnabledLayerNames = enabledLayers.data();
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
	vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, &device);
	vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);

}


uint32_t findMemoryType(uint32_t memoryTypeBits, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memoryProperties = {};

	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
		if ((memoryTypeBits & (1 << i)) &&
			((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties))
			return i;
	}
	return -1;
}
void allocateFFTBuffer(VkBuffer* buffer, VkDeviceMemory* deviceMemory, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkDeviceSize size) {
	uint32_t queueFamilyIndices;
	VkBufferCreateInfo bufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.queueFamilyIndexCount = 1;
	bufferCreateInfo.pQueueFamilyIndices = &queueFamilyIndices;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = usageFlags;
	vkCreateBuffer(device, &bufferCreateInfo, NULL, buffer);
	VkMemoryRequirements memoryRequirements = {};
	vkGetBufferMemoryRequirements(device, buffer[0], &memoryRequirements);
	VkMemoryAllocateInfo memoryAllocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, propertyFlags);
	vkAllocateMemory(device, &memoryAllocateInfo, NULL, deviceMemory);
	vkBindBufferMemory(device, buffer[0], deviceMemory[0], 0);

}
void transferDataFromCPU(float* arr, VkFFT::VkFFTConfiguration configuration) {
	VkDeviceSize stagingBufferSize = configuration.bufferSize[0];
	VkBuffer stagingBuffer = {};
	VkDeviceMemory stagingBufferMemory = {};
	allocateFFTBuffer(&stagingBuffer, &stagingBufferMemory, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBufferSize);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, stagingBufferSize, 0, &data);
	memcpy(data, arr, stagingBufferSize);
	vkUnmapMemory(device, stagingBufferMemory);
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = 1;
	VkCommandBuffer commandBuffer = {};
	vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer);
	VkCommandBufferBeginInfo commandBufferBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = stagingBufferSize;
	vkCmdCopyBuffer(commandBuffer, stagingBuffer, configuration.buffer[0], 1, &copyRegion);
	vkEndCommandBuffer(commandBuffer);
	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	vkQueueSubmit(queue, 1, &submitInfo, fence);
	vkWaitForFences(device, 1, &fence, VK_TRUE, 100000000000);
	vkResetFences(device, 1, &fence);
	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	vkDestroyBuffer(device, stagingBuffer, NULL);
	vkFreeMemory(device, stagingBufferMemory, NULL);
}
void transferDataToCPU(float* arr, VkFFT::VkFFTConfiguration configuration) {
	VkDeviceSize stagingBufferSize = configuration.bufferSize[0];
	VkBuffer stagingBuffer = {};
	VkDeviceMemory stagingBufferMemory = {};
	allocateFFTBuffer(&stagingBuffer, &stagingBufferMemory, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBufferSize);


	VkCommandBufferAllocateInfo commandBufferAllocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = 1;
	VkCommandBuffer commandBuffer = {};
	vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer);
	VkCommandBufferBeginInfo commandBufferBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = stagingBufferSize;
	vkCmdCopyBuffer(commandBuffer, configuration.buffer[0], stagingBuffer, 1, &copyRegion);
	vkEndCommandBuffer(commandBuffer);
	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	vkQueueSubmit(queue, 1, &submitInfo, fence);
	vkWaitForFences(device, 1, &fence, VK_TRUE, 100000000000);
	vkResetFences(device, 1, &fence);
	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, stagingBufferSize, 0, &data);
	memcpy(arr, data, stagingBufferSize);
	vkUnmapMemory(device, stagingBufferMemory);
	vkDestroyBuffer(device, stagingBuffer, NULL);
	vkFreeMemory(device, stagingBufferMemory, NULL);
}

void performVulkanFFT(VkFFT::VkFFTApplication* app, uint32_t batch) {
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = 1;
	VkCommandBuffer commandBuffer = {};
	vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer);
	VkCommandBufferBeginInfo commandBufferBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
	//Record commands batch times. Allows to perform multiple convolutions/transforms in one submit.
	for (uint32_t i = 0; i < batch; i++) {
		app->VkFFTAppend(commandBuffer);
	}
	vkEndCommandBuffer(commandBuffer);
	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	auto timeSubmit = std::chrono::steady_clock::now();
	vkQueueSubmit(queue, 1, &submitInfo, fence);
	vkWaitForFences(device, 1, &fence, VK_TRUE, 100000000000);
	auto timeEnd = std::chrono::steady_clock::now();
	double totTime = std::chrono::duration_cast<std::chrono::microseconds>(timeEnd - timeSubmit).count() * 0.001;
	printf("Pure submit execution time per batch: %.3f ms\n", totTime/batch );
	vkResetFences(device, 1, &fence);
	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}
void performVulkanFFTiFFT(VkFFT::VkFFTApplication* app_forward, VkFFT::VkFFTApplication* app_inverse, uint32_t batch) {
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = 1;
	VkCommandBuffer commandBuffer = {};
	vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer);
	VkCommandBufferBeginInfo commandBufferBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
	for (uint32_t i = 0; i < batch; i++) {
		app_forward->VkFFTAppend(commandBuffer);
		app_inverse->VkFFTAppend(commandBuffer);
	}
	vkEndCommandBuffer(commandBuffer);
	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	auto timeC = std::chrono::steady_clock::now();
	vkQueueSubmit(queue, 1, &submitInfo, fence);
	vkWaitForFences(device, 1, &fence, VK_TRUE, 100000000000);
	auto timeC2 = std::chrono::steady_clock::now();
	double totTime = std::chrono::duration_cast<std::chrono::microseconds>(timeC2 - timeC).count() * 0.001;
	printf("Pure submit execution time per batch: %.3f ms\n", totTime / batch);
	vkResetFences(device, 1, &fence);
	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

int main()
{

	//Sample Vulkan project GPU initialization.
	createInstance();
	setupDebugMessenger();
	findPhysicalDevice();
	createDevice();
	VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fenceCreateInfo.flags = 0;
	vkCreateFence(device, &fenceCreateInfo, NULL, &fence);
	VkCommandPoolCreateInfo commandPoolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
	vkCreateCommandPool(device, &commandPoolCreateInfo, NULL, &commandPool);
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);

	uint32_t sample_id = 0;//setting parameter for VkFFT samples. 0 - FFT + iFFT. 1 - convolution.
	switch (sample_id) {
	case 0:
	{
		//Configuration + FFT application .
		VkFFT::VkFFTConfiguration forward_configuration;
		VkFFT::VkFFTConfiguration inverse_configuration;
		VkFFT::VkFFTApplication app_forward;
		VkFFT::VkFFTApplication app_inverse;
		//FFT + iFFT sample code.
		//Setting up FFT configuration for forward and inverse FFT.
		forward_configuration.FFTdim = 3; //FFT dimension, 1D, 2D or 3D (default 1).
		forward_configuration.size[0] = 256; //Multidimensional FFT dimensions sizes (default 1). For best performance (and stability), order dimensions in descendant size order as: x>y>z.   
		forward_configuration.size[1] = 256;
		forward_configuration.size[2] = 256;
		forward_configuration.performZeropadding = false; //(CURRENTLY DISABLED) Perform padding with zeros on GPU. Still need to properly align input data (no need to fill padding area with meaningful data) but this will increase performance due to the lower amount of the memory reads/writes.
		forward_configuration.performConvolution = false; //Perform convolution with precomputed kernel. 
		forward_configuration.performR2C = true; //Perform R2C/C2R transform. Can be combined with all other options. Reduces memory requirements by a factor of 2. Requires special input data alignment: for x*y*z system pad x*y plane to (x+2)*y with last 2*y elements reserved, total array dimensions are (x*y+2y)*z. Memory layout after R2C and before C2R can be found on github.
		forward_configuration.vectorDimension = 1; //Specify dimensionality of the input data vector (default 1). Each component is stored not as a vector, but as a separate system and padded on it's own according to other options (i.e. for x*y system of 3-vector, first x*y elements correspond to the first dimension, then goes x*y for the second, etc). 
		forward_configuration.inverse = false; //Direction of FFT. false - forward, true - inverse.
		//After this, configuration file contains pointers to Vulkan objects needed to work with the GPU: VkDevice* device - created device, [VkDeviceSize *bufferSize, VkBuffer *buffer, VkDeviceMemory* bufferDeviceMemory] - allocated GPU memory FFT is performed on. [VkDeviceSize *kernelSize, VkBuffer *kernel, VkDeviceMemory* kernelDeviceMemory] - allocated GPU memory, where kernel for convolution is stored.
		forward_configuration.device = &device;
		//Custom path to the floder with shaders, default is "shaders/");
		sprintf(forward_configuration.shaderPath, SHADER_DIR);

		//Allocate buffer for the input data.
		VkDeviceSize bufferSize = forward_configuration.vectorDimension * sizeof(float) * 2 * (forward_configuration.size[0] / 2 + 1) * forward_configuration.size[1] * forward_configuration.size[2];;
		VkBuffer buffer = {};
		VkDeviceMemory bufferDeviceMemory = {};

		allocateFFTBuffer(&buffer, &bufferDeviceMemory, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT, bufferSize);
		forward_configuration.buffer = &buffer;
		forward_configuration.bufferSize = &bufferSize;
		forward_configuration.bufferDeviceMemory = &bufferDeviceMemory;

		printf("Total memory needed for kernel: %d MB\n", bufferSize / 1024 / 1024);

		//Now we will create a similar configuration for inverse FFT and change inverse parameter to true.
		inverse_configuration = forward_configuration;
		inverse_configuration.inverse = true;

		//Fill data on CPU. It is best to perform all operations on GPU after initial upload.
		float* buffer_input = (float*)malloc(bufferSize);

		for (uint32_t v = 0; v < forward_configuration.vectorDimension; v++) {
			for (uint32_t k = 0; k < forward_configuration.size[2]; k++) {
				for (uint32_t j = 0; j < forward_configuration.size[1]; j++) {
					for (uint32_t i = 0; i < forward_configuration.size[0]; i++) {
						buffer_input[i + j * forward_configuration.size[0] + k * (forward_configuration.size[0] + 2) * forward_configuration.size[1] + v * (forward_configuration.size[0] + 2) * forward_configuration.size[1] * forward_configuration.size[2]] = i;//[-1,1]
					}
				}
			}
		}
		//Sample buffer transfer tool. Uses staging buffer of the same size as destination buffer, which can be reduced if transfer is done sequentially in small buffers.
		transferDataFromCPU(buffer_input, forward_configuration);
		//Initialize applications. This function loads shaders, creates pipeline and configures FFT based on configuration file. No buffer allocations inside VkFFT library.  
		app_forward.initializeVulkanFFT(forward_configuration);
		app_inverse.initializeVulkanFFT(inverse_configuration);
		//Submit FFT+iFFT.
		performVulkanFFTiFFT(&app_forward, &app_inverse, 500);
		float* buffer_output = (float*)malloc(bufferSize);
		//Transfer data from GPU using staging buffer.
		transferDataToCPU(buffer_output, inverse_configuration);

		//Print data, if needed.
		/*for (uint32_t v = 0; v < inverse_configuration.vectorDimension; v++) {
			printf("\naxis: %d\n\n", v);
			for (uint32_t k = 0; k < inverse_configuration.size[2]; k++) {
				for (uint32_t j = 0; j < inverse_configuration.size[1]; j++) {
					for (uint32_t i = 0; i < inverse_configuration.size[0]; i++) {
						printf("%.6f ", buffer_output[i + j * inverse_configuration.size[0] + k * (inverse_configuration.size[0] + 2) * inverse_configuration.size[1] + v * (inverse_configuration.size[0] + 2) * inverse_configuration.size[1] * inverse_configuration.size[2]]);
					}
					std::cout << "\n";
				}
			}
		}*/
		vkDestroyBuffer(device, buffer, NULL);
		vkFreeMemory(device, bufferDeviceMemory, NULL);
		break;
	}
	case 1:
	{
		//Configuration + FFT application.
		VkFFT::VkFFTConfiguration forward_configuration;
		VkFFT::VkFFTConfiguration inverse_configuration;
		VkFFT::VkFFTConfiguration convolution_configuration;
		VkFFT::VkFFTApplication app_convolution;
		VkFFT::VkFFTApplication app_kernel;
		//Convolution sample code
		//Setting up FFT configuration. FFT is performed in-place with no performance loss. 
		forward_configuration.FFTdim = 2; //FFT dimension, 1D, 2D or 3D (default 1).
		forward_configuration.size[0] = 4096; //Multidimensional FFT dimensions sizes (default 1). For best performance (and stability), order dimensions in descendant size order as: x>y>z. 
		forward_configuration.size[1] = 4096;
		forward_configuration.size[2] = 1;
		forward_configuration.performZeropadding = false; //(CURRENTLY DISABLED) Perform padding with zeros on GPU. Still need to properly align input data (no need to fill padding area with meaningful data) but this will increase performance due to the lower amount of the memory reads/writes.
		forward_configuration.performConvolution = false; //Perform convolution with precomputed kernel. As we perform forward FFT to get the kernel, it is set to false.
		forward_configuration.performR2C = true; //Perform R2C/C2R transform. Can be combined with all other options. Reduces memory requirements by a factor of 2. Requires special input data alignment: for x*y*z system pad x*y plane to (x+2)*y with last 2*y elements reserved, total array dimensions are (x*y+2y)*z. Memory layout after R2C and before C2R can be found on github.
		forward_configuration.vectorDimension = 9; //Specify dimensionality of the input data vector (default 1). Each component is stored not as a vector, but as a separate system and padded on it's own according to other options (i.e. for x*y system of 3-vector, first x*y elements correspond to the first dimension, then goes x*y for the second, etc).
		//vectorDimension number is an important constant for convolution. If we perform 1x1 convolution, it is equal to 1. If we perform 2x2 convolution, it is equal to 3 for symmetric kernel (stored as xx, xy, yy) and 4 for nonsymmetric (stored as xx, xy, yx, yy). Similarly, 6 (stored as xx, xy, xz, yy, yz, zz) and 9 (stored as xx, xy, xz, yx, yy, yz, zx, zy, zz) for 3x3 convolutions.
		forward_configuration.inverse = false; //Direction of FFT. false - forward, true - inverse.
		//After this, configuration file contains pointers to Vulkan objects needed to work with the GPU: VkDevice* device - created device, [VkDeviceSize *bufferSize, VkBuffer *buffer, VkDeviceMemory* bufferDeviceMemory] - allocated GPU memory FFT is performed on. [VkDeviceSize *kernelSize, VkBuffer *kernel, VkDeviceMemory* kernelDeviceMemory] - allocated GPU memory, where kernel for convolution is stored.
		forward_configuration.device = &device;
		sprintf(forward_configuration.shaderPath, SHADER_DIR);
		//In this example, we perform a convolution for a real vectorfield (3vector) with a symmetric kernel (6 values). We use forward_configuration to initialize convolution kernel first from real data, then we create convolution_configuration for convolution. The buffer object from forward_configuration is passed to convolution_configuration as kernel object.
		//1. Kernel forward FFT.
		VkDeviceSize kernelSize = forward_configuration.vectorDimension * sizeof(float) * 2 * (forward_configuration.size[0] / 2 + 1) * forward_configuration.size[1] * forward_configuration.size[2];;
		VkBuffer kernel = {};
		VkDeviceMemory kernelDeviceMemory = {};

		//Sample allocation tool.
		allocateFFTBuffer(&kernel, &kernelDeviceMemory, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT, kernelSize);
		forward_configuration.buffer = &kernel;
		forward_configuration.bufferSize = &kernelSize;
		forward_configuration.bufferDeviceMemory = &kernelDeviceMemory;

		printf("Total memory needed for kernel: %d MB\n", kernelSize / 1024 / 1024);

		//Fill kernel on CPU.
		float* kernel_input = (float*)malloc(kernelSize);
		for (uint32_t v = 0; v < forward_configuration.vectorDimension; v++) {
			for (uint32_t k = 0; k < forward_configuration.size[2]; k++) {
				for (uint32_t j = 0; j < forward_configuration.size[1]; j++) {
					
					//for (uint32_t i = 0; i < forward_configuration.size[0]; i++) {
					//	kernel_input[i + j * forward_configuration.size[0] + k * (forward_configuration.size[0] + 2) * forward_configuration.size[1] + v * (forward_configuration.size[0] + 2) * forward_configuration.size[1] * forward_configuration.size[2]] = 1;
					
					//Below is the test identity kernel for 3x3 nonsymmetric FFT
					for (uint32_t i = 0; i < forward_configuration.size[0] / 2 + 1; i++) {
						if ((v == 0) || (v == 4) || (v == 8)) 

							kernel_input[2*i + j * forward_configuration.size[0] + k * (forward_configuration.size[0] + 2) * forward_configuration.size[1] + v * (forward_configuration.size[0] + 2) * forward_configuration.size[1] * forward_configuration.size[2]] = 1;
							
						else
							kernel_input[2*i + j * forward_configuration.size[0] + k * (forward_configuration.size[0] + 2) * forward_configuration.size[1] + v * (forward_configuration.size[0] + 2) * forward_configuration.size[1] * forward_configuration.size[2]] = 0;
						kernel_input[2 * i+1 + j * forward_configuration.size[0] + k * (forward_configuration.size[0] + 2) * forward_configuration.size[1] + v * (forward_configuration.size[0] + 2) * forward_configuration.size[1] * forward_configuration.size[2]] = 0;
					
					}
				}
			}
		}
		//Sample buffer transfer tool. Uses staging buffer of the same size as destination buffer, which can be reduced if transfer is done sequentially in small buffers.
		transferDataFromCPU(kernel_input, forward_configuration);
		//Initialize application responsible for the kernel. This function loads shaders, creates pipeline and configures FFT based on configuration file. No buffer allocations inside VkFFT library.  
		app_kernel.initializeVulkanFFT(forward_configuration);
		//Sample forward FFT command buffer allocation + execution performed on kernel. Second number determines how many times perform application in one submit. FFT can also be appended to user defined command buffers.
		
		//Uncomment the line below if you want to perform kernel FFT. In this sample we use predefined identitiy kernel.
		//performVulkanFFT(&app_kernel, 1);

		//The kernel has been trasnformed.


		//2. Buffer convolution with transformed kernel.
		//Copy configuration, as it mostly remains unchanged. Change specific parts.
		convolution_configuration = forward_configuration;
		convolution_configuration.performConvolution = true;
		convolution_configuration.symmetricKernel = false;//Specify if convolution kernel is symmetric. In this case we only pass upper triangle part of it in the form of: (xx, xy, yy) for 2d and (xx, xy, xz, yy, yz, zz) for 3d.
		convolution_configuration.vectorDimension = 3;
		convolution_configuration.kernel = &kernel;
		convolution_configuration.kernelSize = &kernelSize;
		convolution_configuration.kernelDeviceMemory = &kernelDeviceMemory;

		//Allocate separate buffer for the input data.
		VkDeviceSize bufferSize = convolution_configuration.vectorDimension * sizeof(float) * 2 * (convolution_configuration.size[0] / 2 + 1) * convolution_configuration.size[1] * convolution_configuration.size[2];;
		VkBuffer buffer = {};
		VkDeviceMemory bufferDeviceMemory = {};

		allocateFFTBuffer(&buffer, &bufferDeviceMemory, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT, bufferSize);
		convolution_configuration.buffer = &buffer;
		convolution_configuration.bufferSize = &bufferSize;
		convolution_configuration.bufferDeviceMemory = &bufferDeviceMemory;

		printf("Total memory needed for buffer: %d MB\n", bufferSize / 1024 / 1024);
		//Fill data on CPU. It is best to perform all operations on GPU after initial upload.
		float* buffer_input = (float*)malloc(bufferSize);

		for (uint32_t v = 0; v < convolution_configuration.vectorDimension; v++) {
			for (uint32_t k = 0; k < convolution_configuration.size[2]; k++) {
				for (uint32_t j = 0; j < convolution_configuration.size[1]; j++) {
					for (uint32_t i = 0; i < convolution_configuration.size[0]; i++) {
						buffer_input[i + j * convolution_configuration.size[0] + k * (convolution_configuration.size[0] + 2) * convolution_configuration.size[1] + v * (convolution_configuration.size[0] + 2) * convolution_configuration.size[1] * convolution_configuration.size[2]] = i;
					}
				}
			}
		}
		//Transfer data to GPU using staging buffer.
		transferDataFromCPU(buffer_input, convolution_configuration);

		//Initialize application responsible for the convolution.
		app_convolution.initializeVulkanFFT(convolution_configuration);
		//Sample forward FFT command buffer allocation + execution performed on kernel. FFT can also be appended to user defined command buffers.
		performVulkanFFT(&app_convolution, 100);
		//The kernel has been trasnformed.

		float* buffer_output = (float*)malloc(bufferSize);
		//Transfer data from GPU using staging buffer.
		transferDataToCPU(buffer_output, convolution_configuration);

		//Print data, if needed.
		/*for (uint32_t v = 0; v < convolution_configuration.vectorDimension; v++) {
			printf("\naxis: %d\n\n", v);
			for (uint32_t k = 0; k < convolution_configuration.size[2]; k++) {
				for (uint32_t j = 0; j < convolution_configuration.size[1]; j++) {
					for (uint32_t i = 0; i < convolution_configuration.size[0]; i++) {
						printf("%.6f ", buffer_output[i + j * convolution_configuration.size[0] + k * (convolution_configuration.size[0] + 2) * convolution_configuration.size[1] + v * (convolution_configuration.size[0] + 2) * convolution_configuration.size[1] * convolution_configuration.size[2]]);
					}
					std::cout << "\n";
				}
			}
		}*/
		vkDestroyBuffer(device, buffer, NULL);
		vkFreeMemory(device, bufferDeviceMemory, NULL);
		vkDestroyBuffer(device, kernel, NULL);
		vkFreeMemory(device, kernelDeviceMemory, NULL);
		break;
	}
	}
}
