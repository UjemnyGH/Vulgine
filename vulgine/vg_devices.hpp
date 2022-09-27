#pragma once
#define VG_DEVICES 1

#include <vulkan/vulkan.hpp>
#include <optional>
#include <iostream>
#include <set>

#ifndef VG_INSTANCE 
#include "vg_instance.hpp"
#endif

namespace vg {
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapchainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    class Vg_Device {
    private:
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice logicalDevice;

        VkQueue presentQueue;
        VkQueue graphicsQueue;

        Instance* _pInstance;

        bool checkDeviceExtensionSupport() {
            uint32_t count = 0;
            vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, nullptr);

            std::vector<VkExtensionProperties> extProp(count);
            vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, extProp.data());

            std::set<std::string> reqs(deviceExtensions.begin(), deviceExtensions.end());

            for(const auto& e : extProp) {
                reqs.erase(e.extensionName);
            }

            return reqs.empty();
        }

        bool isDeviceSuitable(VkPhysicalDevice _pd_) {
            QueueFamilyIndices indices;

            uint32_t count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(_pd_, &count, nullptr);

            std::vector<VkQueueFamilyProperties> familyProp(count);
            vkGetPhysicalDeviceQueueFamilyProperties(_pd_, &count, familyProp.data());

            int i = 0;
            for(const auto& fam : familyProp) {
                VkBool32 presentSupported = false;

                vkGetPhysicalDeviceSurfaceSupportKHR(_pd_, i, _pInstance->presentSurface, &presentSupported);

                if(fam.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    indices.graphicsFamily = i;
                }

                if(presentSupported) {
                    indices.presentFamily = i;
                }

                if(indices.isComplete()) {
                    break;
                }

                i++;
            }

            uint32_t count = 0;
            vkEnumerateDeviceExtensionProperties(_pd_, nullptr, &count, nullptr);

            std::vector<VkExtensionProperties> extProp(count);
            vkEnumerateDeviceExtensionProperties(_pd_, nullptr, &count, extProp.data());

            std::set<std::string> reqs(deviceExtensions.begin(), deviceExtensions.end());

            for(const auto& e : extProp) {
                reqs.erase(e.extensionName);
            }

            bool extensionSupported = reqs.empty();

            bool swapchainAdequate = false;

            if(extensionSupported) {
                SwapchainSupportDetails details;

                vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_pd_, _pInstance->presentSurface, &details.capabilities);

                uint32_t count = 0;
                vkGetPhysicalDeviceSurfaceFormatsKHR(_pd_, _pInstance->presentSurface, &count, nullptr);

                if(count != 0) {
                    details.formats.resize(count);

                    vkGetPhysicalDeviceSurfaceFormatsKHR(_pd_, _pInstance->presentSurface, &count, details.formats.data());
                }

                vkGetPhysicalDeviceSurfacePresentModesKHR(_pd_, _pInstance->presentSurface, &count, nullptr);

                if(count != 0) {
                    details.presentModes.resize(count);

                    vkGetPhysicalDeviceSurfacePresentModesKHR(_pd_, _pInstance->presentSurface, &count, details.presentModes.data());
                }

                swapchainAdequate = !details.formats.empty() && !details.presentModes.empty();
            }

            VkPhysicalDeviceFeatures features;
            vkGetPhysicalDeviceFeatures(_pd_, &features);

            return indices.isComplete() && extensionSupported && swapchainAdequate && features.samplerAnisotropy;
        }

    public:
        /**
         * @brief Create a Devices (physical and logical)
         * 
         * @param pInstance give pointer to Instance class
         * @return int should return 0
         */
        int CreateDevices(Instance* pInstance) {
            _pInstance = pInstance;

            uint32_t count = 0;
            vkEnumeratePhysicalDevices(*_pInstance->getInstancePtr(), &count, nullptr);

            std::vector<VkPhysicalDevice> physicalDevices(count);
            vkEnumeratePhysicalDevices(*_pInstance->getInstancePtr(), &count, physicalDevices.data());

            for(const auto& _pd__ : physicalDevices) {
                if(isDeviceSuitable(_pd__)) {
                    physicalDevice = _pd__;

                    break;
                }
            }

            if(physicalDevice == VK_NULL_HANDLE) {
                std::cerr << "Cannot find proper physical device!\n";

                exit(5);
            }

            QueueFamilyIndices indices = findQueueFamily();

            std::vector<VkDeviceQueueCreateInfo> queueInfos;
            std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

            float queuePriority = 1.0f;

            for(uint32_t queueFamily : uniqueQueueFamilies) {
                VkDeviceQueueCreateInfo info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
                info.queueFamilyIndex = queueFamily;
                info.queueCount = 1;
                info.pQueuePriorities = &queuePriority;
                
                queueInfos.push_back(info);
            }

            VkPhysicalDeviceFeatures deviceFeatures{};
            deviceFeatures.samplerAnisotropy = VK_TRUE;

            VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
            deviceInfo.queueCreateInfoCount = queueInfos.size();
            deviceInfo.pQueueCreateInfos = queueInfos.data();
            deviceInfo.pEnabledFeatures = &deviceFeatures;
            deviceInfo.enabledExtensionCount = deviceExtensions.size();
            deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();

            if(enableValidationLayers) {
                deviceInfo.enabledLayerCount = validationLayers.size();
                deviceInfo.ppEnabledLayerNames = validationLayers.data();
            }
            else {
                deviceInfo.enabledLayerCount = 0;
            }

            if(vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &logicalDevice) != VK_SUCCESS) {
                std::cerr << "Cannot create logical device!\n";

                exit(6);
            }

            vkGetDeviceQueue(logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
            vkGetDeviceQueue(logicalDevice, indices.presentFamily.value(), 0, &presentQueue);
        }

        /**
         * @brief Find queue families support eg. graphics nd present support
         * 
         * @return QueueFamilyIndices 
         */
        QueueFamilyIndices findQueueFamily() {
            QueueFamilyIndices indices;

            uint32_t count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);

            std::vector<VkQueueFamilyProperties> familyProp(count);
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, familyProp.data());

            int i = 0;
            for(const auto& fam : familyProp) {
                VkBool32 presentSupported = false;

                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, _pInstance->presentSurface, &presentSupported);

                if(fam.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    indices.graphicsFamily = i;
                }

                if(presentSupported) {
                    indices.presentFamily = i;
                }

                if(indices.isComplete()) {
                    break;
                }

                i++;
            }

            return indices;
        }

        /**
         * @brief Get support of presentation surface
         * 
         * @return SwapchainSupportDetails 
         */
        SwapchainSupportDetails querySwapchainSupport() {
            SwapchainSupportDetails details;

            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, _pInstance->presentSurface, &details.capabilities);

            uint32_t count = 0;
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, _pInstance->presentSurface, &count, nullptr);

            if(count != 0) {
                details.formats.resize(count);

                vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, _pInstance->presentSurface, &count, details.formats.data());
            }

            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, _pInstance->presentSurface, &count, nullptr);

            if(count != 0) {
                details.presentModes.resize(count);

                vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, _pInstance->presentSurface, &count, details.presentModes.data());
            }

            return details;
        }

        /**
         * @brief Get the Physical Device Ptr
         * 
         * @return VkPhysicalDevice* 
         */
        VkPhysicalDevice* getPhysicalDevicePtr() { return &physicalDevice; }

        /**
         * @brief Get the Logical Device Ptr
         * 
         * @return VkDevice* 
         */
        VkDevice* getLogicalDevicePtr() { return &logicalDevice; }

        /**
         * @brief Get the Graphics Queue Ptr
         * 
         * @return VkQueue* 
         */
        VkQueue* getGraphicsQueuePtr() { return &graphicsQueue; }

        /**
         * @brief Get the Present Queue Ptr
         * 
         * @return VkQueue* 
         */
        VkQueue* getPresentQueuePtr() { return &presentQueue; }

        /**
         * @brief Get the Instance Ptr 
         * 
         * @return VkInstance* 
         */
        Instance* getInstancePtr() { return _pInstance; }

        /**
         * @brief Destroy the Vg_Device object call it manually for proper work
         * 
         */
        ~Vg_Device() {
            vkDestroyDevice(logicalDevice, nullptr);

            _pInstance->~Vg_Instance();
        }
    };

    typedef Vg_Device Device;
}