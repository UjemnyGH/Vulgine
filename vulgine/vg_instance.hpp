#pragma once
#define VG_INSTANCE 1

#include <vulkan/vulkan.hpp>
#include <iostream>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

namespace vg {
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    #ifdef _glfw3_h_
    /**
     * @brief Get the Extensions object
     * 
     * @return std::vector<const char*> 
     */
    std::vector<const char*> getExtensions() {
        uint32_t count = 0;
        const char** extensions = glfwGetRequiredInstanceExtensions(&count);

        std::vector<const char*> requiredExtensions(extension, extension + count);

        if(enableValidationLayers) {
            requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return requiredExtensions;
    }
    #endif

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
        std::cerr << pCallbackData->pMessage << "\n\n";

        return VK_FALSE;
    }

    class Vg_Instance {
    private:
        VkInstance _instance;
        VkDebugUtilsMessengerEXT debugMessenger;

        bool checkValidationLayerSupport() {
            uint32_t count;
            vkEnumerateInstanceLayerProperties(&count, nullptr);

            std::vector<VkLayerProperties> layerProp(count);
            vkEnumerateInstanceLayerProperties(&count, layerProp.data());

            for(const char* layerName : validationLayers) {
                bool layerFound = false;

                for(const auto& layer : layerProp) {
                    if(strcmp(layerName, layer.layerName)) {
                        layerFound = true;

                        break;
                    }
                }

                if(!layerFound) {
                    return false;
                }
            }

            return true;
        }

        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& info) {
            info = {};
            info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
            info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            info.pfnUserCallback = debugCallback;
        }

        VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
            auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

            if(func != nullptr) {
                func(instance, pCreateInfo, pAllocator, pDebugMessenger);
            }
        }

        void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

            if(func != nullptr) {
                func(instance, debugMessenger, pAllocator);
            }
        }

    public:
    VkSurfaceKHR presentSurface;

    #ifdef _glfw3_h_
        /**
         * @brief Create a Vulkan Instance (if you using GLFW include GLFW before vulgine/vulgine.hpp!!!!)
         * 
         * @param windowExtensions 
         * @param appName 
         * @param apiVersion 
         * @return int 
         */
        int CreateInstance(GLFWwindow* window, std::vector<const char*> windowExtensions = getExtensions(), const char* appName = "Application", uint32_t apiVersion = VK_API_VERSION_1_2) {
            if(enableValidationLayers && !checkValidationLayerSupport()) {
                std::cerr << "Validation layers are unavailable!\n";
                
                return 1;
            }

            VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
            appInfo.pApplicationName = appName;
            appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.pEngineName = "Vulgine";
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.apiVersion = apiVersion;

            VkInstanceCreateInfo instInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
            instInfo.pApplicationInfo = &appInfo;

            instInfo.enabledExtensionCount = windowExtensions.size();
            instInfo.ppEnabledExtensionNames = windowExtensions.data();

            VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
            if(enableValidationLayers) {
                instInfo.enabledLayerCount = validationLayers.size();
                instInfo.ppEnabledLayerNames = validationLayers.data();

                populateDebugMessengerCreateInfo(debugInfo);

                instInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugInfo;
            }
            else {
                instInfo.enabledLayerCount = 0;
            }

            if(vkCreateInstance(&instInfo, nullptr, &_instance) != VK_SUCCESS) {
                std::cerr << "Cannot create vulkan instance!\n";

                return 2;
            }

            uint32_t extensionCount = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

            std::vector<VkExtensionProperties> properties(extensionCount);
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, properties.data());

            if(enableValidationLayers) {
                VkDebugUtilsMessengerCreateInfoEXT debug{};
                populateDebugMessengerCreateInfo(debug);

                if(CreateDebugUtilsMessengerEXT(_instance, &debug, nullptr, &debugMessenger) != VK_SUCCESS) {
                    std::cerr << "Cannot create debug messenger!\n";

                    return 3;
                }
            }

            CreateGLFWPresentSurface(window);
        }

        /**
         * @brief Create GLFW present surface for vulkan (please include glfw header before vulgine header)
         * 
         * @param window 
         */
        void CreateGLFWPresentSurface(GLFWwindow* window, ) {
            if(glfwCreateWindowSurface(_instance, window, nullptr, &presentSurface) != VK_SUCCESS) {
                std::cerr << "Cannot create GLFW present surface!\n";

                exit(4);
            }
        }
    #else
        /**
         * @brief Create a Vulkan Instance (if you using GLFW include GLFW before vulgine/vulgine.hpp!!!!)
         * 
         * @param windowExtensions 
         * @param appName 
         * @param apiVersion 
         * @return int 
         */
        int CreateInstance(std::vector<const char*> windowExtensions, const char* appName = "Application", uint32_t apiVersion = VK_API_VERSION_1_2) {
            if(enableValidationLayers && !checkValidationLayerSupport()) {
                std::cerr << "Validation layers are unavailable!\n";
                
                return 1;
            }

            VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
            appInfo.pApplicationName = appName;
            appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.pEngineName = "Vulgine";
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.apiVersion = apiVersion;

            VkInstanceCreateInfo instInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
            instInfo.pApplicationInfo = &appInfo;

            instInfo.enabledExtensionCount = windowExtensions.size();
            instInfo.ppEnabledExtensionNames = windowExtensions.data();

            VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
            if(enableValidationLayers) {
                instInfo.enabledLayerCount = validationLayers.size();
                instInfo.ppEnabledLayerNames = validationLayers.data();

                populateDebugMessengerCreateInfo(debugInfo);

                instInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugInfo;
            }
            else {
                instInfo.enabledLayerCount = 0;
            }

            if(vkCreateInstance(&instInfo, nullptr, &_instance) != VK_SUCCESS) {
                std::cerr << "Cannot create vulkan instance!\n";

                return 2;
            }

            uint32_t extensionCount = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

            std::vector<VkExtensionProperties> properties(extensionCount);
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, properties.data());

            if(enableValidationLayers) {
                VkDebugUtilsMessengerCreateInfoEXT debug{};
                populateDebugMessengerCreateInfo(debug);

                if(CreateDebugUtilsMessengerEXT(_instance, &debug, nullptr, &debugMessenger) != VK_SUCCESS) {
                    std::cerr << "Cannot create debug messenger!\n";

                    return 3;
                }
            }
        }
    #endif

        /**
         * @brief Get the Instance Ptr object
         * 
         * @return VkInstance* 
         */
        VkInstance* getInstancePtr() { return &_instance; }

        /**
         * @brief Get the Debug Messenger Ptr object
         * 
         * @return VkDebugUtilsMessengerEXT* 
         */
        VkDebugUtilsMessengerEXT* getDebugMessengerPtr() { return &debugMessenger; }

        /**
         * @brief Destroy the Vg_Instance, please call manually to get desired effect
         * 
         */
        ~Vg_Instance() {
            if(enableValidationLayers)
                DestroyDebugUtilsMessengerEXT(_instance, debugMessenger, nullptr);

            vkDestroyInstance(_instance, nullptr);
        }
    };

    typedef Vg_Instance Instance;
}