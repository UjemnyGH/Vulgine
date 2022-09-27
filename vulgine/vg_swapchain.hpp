#pragma once
#define VG_SWAPCHAIN 1

#ifndef VG_DEVICE
#include "vg_devices.hpp"
#endif

#include <limits>
#include <algorithm>

namespace vg {
    class Vg_Swapchain {
    private:
        VkSwapchainKHR swapchain;
        VkFormat s_format;
        VkExtent2D s_extent;
        VkRenderPass rederPass;

        VkImage depthImage;
        VkImageView depthView;
        VkDeviceMemory depthMemory;
        std::vector<VkFramebuffer> swapchainFramebuffers;

        std::vector<VkImage> swapchainImages;
        std::vector<VkImageView> swapchainImageViews;
        
        Device* pDevice;

        VkSurfaceFormatKHR chooseFormat(std::vector<VkSurfaceFormatKHR>& formats) {
            for(const auto& format : formats) {
                if(format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    return format;
                }
            }

            return formats[0];
        }

        VkPresentModeKHR choosePresentMode(std::vector<VkPresentModeKHR>& presentModes) {
            for(const auto& presentMode : presentModes) {
                if(presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                    return presentMode;
                }
            }

            return VK_PRESENT_MODE_FIFO_KHR;
        }

        VkExtent2D chooseExtent(VkSurfaceCapabilitiesKHR& capabilities, int width, int height) {
            if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
                return capabilities.currentExtent;
            }
            else {
                VkExtent2D extent = {
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height)
                };

                extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
                extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
            
                return extent;
            }
        }

        VkFormat findSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags features) {
            for(VkFormat format : formats) {
                VkFormatProperties props;
                vkGetPhysicalDeviceFormatProperties(*pDevice->getPhysicalDevicePtr(), format, &props);

                if(tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                    return format;
                }
                else if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                    return format;
                }
            }

            std::cerr << "Wrong formats and features!\n";

            exit(10);
        }

        VkFormat findDepthFormat() {
            return findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
        }

    public:
        int CreateSwapchain(Device* _pDevice, int width, int height) {
            pDevice = _pDevice;

            SwapchainSupportDetails details = pDevice->querySwapchainSupport();

            VkSurfaceFormatKHR swapchainFormat = chooseFormat(details.formats);
            VkPresentModeKHR swapchainPresentMode = choosePresentMode(details.presentModes);
            VkExtent2D swapchainExtent = chooseExtent(details.capabilities, width, height);

            uint32_t imagesCount = details.capabilities.minImageCount + 1;

            if(details.capabilities.maxImageCount > 0 && details.capabilities.maxImageCount < imagesCount) {
                imagesCount = details.capabilities.maxImageCount;
            }

            VkSwapchainCreateInfoKHR swapchainInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
            swapchainInfo.surface = pDevice->getInstancePtr()->presentSurface;
            swapchainInfo.minImageCount = imagesCount;
            swapchainInfo.imageFormat = swapchainFormat.format;
            swapchainInfo.imageColorSpace = swapchainFormat.colorSpace;
            swapchainInfo.imageExtent = swapchainExtent;
            swapchainInfo.imageArrayLayers = 1;
            swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            QueueFamilyIndices indices = pDevice->findQueueFamily();

            uint32_t queueFamIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

            if(indices.graphicsFamily != indices.presentFamily) {
                swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                swapchainInfo.queueFamilyIndexCount = 1;
                swapchainInfo.pQueueFamilyIndices = queueFamIndices;
            }
            else {
                swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                swapchainInfo.queueFamilyIndexCount = 0;
            }

            swapchainInfo.preTransform = details.capabilities.currentTransform;

            swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

            swapchainInfo.presentMode = swapchainPresentMode;
            swapchainInfo.clipped = VK_TRUE;

            swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

            if(vkCreateSwapchainKHR(*pDevice->getLogicalDevicePtr(), &swapchainInfo, nullptr, &swapchain) != VK_SUCCESS) {
                std::cerr << "Cannot create swapchain!\n";

                exit(7);
            }

            vkGetSwapchainImagesKHR(*pDevice->getLogicalDevicePtr(), swapchain, &imagesCount, nullptr);

            swapchainImages.resize(imagesCount);
            vkGetSwapchainImagesKHR(*pDevice->getLogicalDevicePtr(), swapchain, &imagesCount, swapchainImages.data());

            s_format = swapchainFormat.format;
            s_extent = swapchainExtent;
        }

        void RecreateSwapchain(int width, int height) {
            vkDeviceWaitIdle(*pDevice->getLogicalDevicePtr());

            CleanSwapchain();
        }

        VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags imageAspectFlags) {
            VkImageViewCreateInfo imgViewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            imgViewInfo.image = image;
            imgViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imgViewInfo.format = format;
            imgViewInfo.subresourceRange.aspectMask = imageAspectFlags;
            imgViewInfo.subresourceRange.baseMipLevel = 0;
            imgViewInfo.subresourceRange.levelCount = 1;
            imgViewInfo.subresourceRange.baseArrayLayer = 0;
            imgViewInfo.subresourceRange.layerCount = 1;

            VkImageView imageView;

            if(vkCreateImageView(*pDevice->getLogicalDevicePtr(), &imgViewInfo, nullptr, &imageView) != VK_SUCCESS) {
                std::cerr << "Cannot create image view!\n";

                exit(8);
            }

            return imageView;
        }

        void CleanSwapchain() {
            vkDestroyImageView(*pDevice->getLogicalDevicePtr(), depthView, nullptr);
            vkDestroyImage(*pDevice->getLogicalDevicePtr(), depthImage, nullptr);
            vkFreeMemory(*pDevice->getLogicalDevicePtr(), depthMemory, nullptr);

            for(auto fb : swapchainFramebuffers) {
                vkDestroyFramebuffer(*pDevice->getLogicalDevicePtr(), fb, nullptr);
            }

            for(auto iv : swapchainImageViews) {
                vkDestroyImageView(*pDevice->getLogicalDevicePtr(), iv, nullptr);
            }

            vkDestroySwapchainKHR(*pDevice->getLogicalDevicePtr(), swapchain, nullptr);
        }

        void CreateImageViews() {
            swapchainImageViews.resize(swapchainImages.size());

            for (size_t i = 0; i < swapchainImages.size(); i++) {
                swapchainImageViews[i] = CreateImageView(swapchainImages[i], s_format, VK_IMAGE_ASPECT_COLOR_BIT);
            }
        }

        void CreateRenderPass() {
            VkAttachmentDescription colorAttachemntDescriptor{};
            colorAttachemntDescriptor.format = s_format;
            colorAttachemntDescriptor.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachemntDescriptor.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachemntDescriptor.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachemntDescriptor.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachemntDescriptor.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachemntDescriptor.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachemntDescriptor.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            VkAttachmentDescription depthAttachmentDescriptor{};
            depthAttachmentDescriptor.format = findDepthFormat();
            depthAttachemntDescriptor.samples = VK_SAMPLE_COUNT_1_BIT;
            depthAttachemntDescriptor.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachemntDescriptor.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachemntDescriptor.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachemntDescriptor.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachemntDescriptor.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthAttachemntDescriptor.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL

            VkAttachmentReference colorAttachmentReference{};
            colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorAttachmentReference.attachment = 0;

            VkAttachmentReference depthAttachmentReference{};
            depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depthAttachmentReference.attachment = 1;

            VkSubpassDescription subpassDescriptor{};
            subpassDescriptor.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassDescriptor.colorAttachmentCount = 1;
            subpassDescriptor.pColorAttachments = &colorAttachmentReference;
            subpassDescriptor.pDepthStencilAttachment = &depthAttachmentReference;

            VkSubpassDependency subpassDependency{};
            subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            subpassDependency.dstSubpass = 0;
            subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            subpassDependency.srcAccessMask = 0;
            subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            std::array<VkAttachmentDescription, 2> attachmentDescriptors = {colorAttachemntDescriptor, depthAttachmentDescriptor};

            VkRenderPassCreateInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
            renderPassInfo.attachmentCount = attachmentDescriptors.size();
            renderPassInfo.pAttachments = attachmentDescriptors.data();
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subpass;
            renderPassInfo.dependencyCount = 1;
            renderPassInfo.pDependencies = &subpassDependency;

            if(vkCreateRenderPass(*pDevice->getLogicalDevicePtr(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
                std::cerr << "Cannot create render pass!\n";

                exit(11);
            }
        }

        ~Vg_Swapchain() {
            CleanSwapchain();
        }
    };
}