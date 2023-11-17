#include <execinfo.h>
#include <iostream>
#include <iterator>
#include <stdio.h>
#include <strings.h>
#include <type_traits>
#include <unistd.h>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include "xassert.h"
#include <GLFW/glfw3.h>
#include <algorithm> // Necessary for std::clamp
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits> // Necessary for std::numeric_limits
#include <optional>
#include <set>
#include <string>
#include <vector>


// warning maybe unreadable
int stack_indent = 3;
void print_stack(void)
{
   void  *funcs[1024];
   int    count = backtrace(funcs, 1024);
   char **symbols = backtrace_symbols(funcs, count);
   printf(COL_GRN "stack trace:\n" COL_CLR);
   count = count - 3;
   int print_i = 0;
   for (int i = count - 1; i > 2; i--) {
      int indent = 0;
      for(int j = 0;symbols[i][j] != '\0';j++){
         if(symbols[i][j] == '('){
            if (symbols[i][j + 1] == '+'){
               // we don't have a name we can't do much
               goto next_symbol;
            }
            symbols[i][j] = '\n';
            symbols[i] = symbols[i] + j + 1;
            break;
         }
      }
      for(int j = 0;symbols[i][j] != '\0';j++){
         // replace the address with ()
         if(symbols[i][j] == '+'){
            symbols[i][j] = '(';
            symbols[i][j + 1] = ')';
            symbols[i][j + 2] = '\0';
            break;
         }
      }
      indent = print_i * stack_indent;
      for(int j = 0;j < indent;j++){
         printf(" ");
      }
      print_i++;
      printf(COL_YLW "%s{\n" COL_CLR, symbols[i]);
   next_symbol:
      continue;
   }
   int indent = print_i * stack_indent;
   for(int j = 0;j < indent;j++){
      printf(" ");
   }
   printf("...\n");
   print_i--;
   for(int i = 2;i < count;i++){
      int indent = (i - 2) * stack_indent;
      for(int j = 0;symbols[i][j] != '\0';j++){
         if(symbols[i][j] == '('){
            if (symbols[i][j + 1] == '+'){
               goto next_symbol_l2;
            }
            break;
         }
      }
      indent = print_i * stack_indent;
      for(int j = 0;j < indent;j++){
         printf(" ");
      }
      print_i--;
      printf(COL_YLW "}" COL_CLR "\n");
      next_symbol_l2:
      continue;
   }
   free(symbols);
}

const bool                      enableValidationLayers = true;
const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

const int MAX_FRAMES_IN_FLIGHT = 2;

struct QueueFamilyIndices {
   std::optional<uint32_t> graphicsFamily;
   std::optional<uint32_t> presentFamily;

   bool isComplete()
   {
      return graphicsFamily.has_value() && presentFamily.has_value();
   }
};

struct SwapChainSupportDetails {
   VkSurfaceCapabilitiesKHR        capabilities;
   std::vector<VkSurfaceFormatKHR> formats;
   std::vector<VkPresentModeKHR>   presentModes;
};

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT             messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData
)
{
   if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
      print_stack();
      printline("validition", COL_YLW, "%s", pCallbackData->pMessage);
   } else if (messageSeverity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
      print_stack();
      printline("validition", COL_RED, "%s", pCallbackData->pMessage);
   }
   return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT    *pDebugMessenger
)
{
   PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT
   )vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
   if (func != NULL) {
      return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
   } else {
      return VK_ERROR_EXTENSION_NOT_PRESENT;
   }
}

bool checkValidationLayerSupport()
{
   uint32_t layerCount;
   vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

   std::vector<VkLayerProperties> availableLayers(layerCount);
   vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
   for (const char *layerName : validationLayers) {
      for (const auto &layerProperties : availableLayers) {
         if (strcmp(layerName, layerProperties.layerName) == 0) {
            return true;
         }
      }
   }

   return false;
}

void DestroyDebugUtilsMessengerEXT(
    VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks *pAllocator
)
{
   PFN_vkDestroyDebugUtilsMessengerEXT func =
       (PFN_vkDestroyDebugUtilsMessengerEXT
       )vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
   if (func != NULL) {
      func(instance, debugMessenger, pAllocator);
   }
}

static std::vector<char> readFile(const std::string &filename)
{
   std::ifstream file(filename, std::ios::ate | std::ios::binary);
   check_eq(!file.is_open(), "Failed to open file");
   size_t            fileSize = (size_t)file.tellg();
   std::vector<char> buffer(fileSize);
   file.seekg(0);
   file.read(buffer.data(), fileSize);
   file.close();
   return buffer;
}

class Program {
 public:
   void createWindow(int w, int h, const char *title);
   void setupDebugMessage();
   void createInstance();
   void pickPhysicalDevice();
   void createDevice();
   void createSurface();
   bool
   checkDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char *> &);
   void createSwapChain();
   void createImageViews();
   void createGraphicsPipeline();
   void createRenderPass();
   void createFrameBuffers();
   void createCommandPool();
   void createCommandBuffer();
   void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
   void createSyncObjects();
   void drawFrame();
   ~Program();
   // helper funcitons
   bool                    shouldclose();
   bool                    isDeviceSuitable(VkPhysicalDevice);
   VkShaderModule          createShaderModule(const std::vector<char> &code);
   SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
   QueueFamilyIndices      findQueueFamilies(VkPhysicalDevice);
   VkSurfaceFormatKHR      chooseSwapSurfaceFormat(
            const std::vector<VkSurfaceFormatKHR> &availableFormats
        );
   VkPresentModeKHR chooseSwapPresentMode(
       const std::vector<VkPresentModeKHR> &availablePresentModes
   );
   VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

 private:
   VkInstance                 instance;
   VkDebugUtilsMessengerEXT   debugMessenger;
   GLFWwindow                *window;
   VkSurfaceKHR               surface;
   VkPhysicalDevice           physicalDevice = VK_NULL_HANDLE;
   VkDevice                   Logicaldevice;
   VkQueue                    graphicsQueue;
   VkQueue                    presentQueue;
   VkSwapchainKHR             swapChain;
   std::vector<VkImage>       swapChainImages;
   VkFormat                   swapChainImageFormat;
   VkExtent2D                 swapChainExtent;
   std::vector<VkImageView>   swapChainImageViews;
   std::vector<VkFramebuffer> swapChainFramebuffers;
   VkPipelineLayout           pipelineLayout;
   VkRenderPass               renderPass;
   VkPipeline                 graphicsPipeline;
   VkCommandPool              commandPool;
   VkCommandBuffer            commandBuffer;
   VkSemaphore                imageAvailableSemaphore;
   VkSemaphore                renderFinishedSemaphore;
   VkFence                    inFlightFence;
};

void Program::drawFrame()
{
   vkWaitForFences(Logicaldevice, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
   vkResetFences(Logicaldevice, 1, &inFlightFence);

   vkResetCommandBuffer(commandBuffer, 0);

   uint32_t imageIndex;
   vkAcquireNextImageKHR(
       Logicaldevice,
       swapChain,
       UINT64_MAX,
       imageAvailableSemaphore,
       VK_NULL_HANDLE,
       &imageIndex
   );

   recordCommandBuffer(commandBuffer, imageIndex);

   VkSubmitInfo submitInfo{};
   submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

   VkSemaphore          waitSemaphores[] = {imageAvailableSemaphore};
   VkPipelineStageFlags waitStages[] = {
       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
   submitInfo.waitSemaphoreCount = 1;
   submitInfo.pWaitSemaphores = waitSemaphores;
   submitInfo.pWaitDstStageMask = waitStages;
   submitInfo.commandBufferCount = 1;
   submitInfo.pCommandBuffers = &commandBuffer;

   VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
   submitInfo.signalSemaphoreCount = 1;
   submitInfo.pSignalSemaphores = signalSemaphores;
   check_eq(
       vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) !=
           VK_SUCCESS,
       "failed to submit draw command buffer"
   );
   VkPresentInfoKHR presentInfo{};
   presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

   presentInfo.waitSemaphoreCount = 1;
   presentInfo.pWaitSemaphores = signalSemaphores;
   VkSwapchainKHR swapChains[] = {swapChain};
   presentInfo.swapchainCount = 1;
   presentInfo.pSwapchains = swapChains;
   presentInfo.pImageIndices = &imageIndex;
   vkQueuePresentKHR(presentQueue, &presentInfo);
}

void Program::createSyncObjects()
{
   VkSemaphoreCreateInfo semaphoreInfo{};
   semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
   VkFenceCreateInfo fenceInfo{};
   fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
   fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
   check_eq(
       vkCreateSemaphore(
           Logicaldevice, &semaphoreInfo, nullptr, &imageAvailableSemaphore
       ) != VK_SUCCESS,
       "Failed to create semaphore"
   );
   check_eq(
       vkCreateSemaphore(
           Logicaldevice, &semaphoreInfo, nullptr, &renderFinishedSemaphore
       ) != VK_SUCCESS,
       "Failed to create semaphore"
   );
   check_eq(
       vkCreateFence(Logicaldevice, &fenceInfo, nullptr, &inFlightFence) !=
           VK_SUCCESS,
       "Failed to create fences you are now unsafe"
   )
}

void Program::createCommandBuffer()
{
   VkCommandBufferAllocateInfo allocInfo{};
   allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   allocInfo.commandPool = commandPool;
   allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   allocInfo.commandBufferCount = 1;
   check_eq(
       vkAllocateCommandBuffers(Logicaldevice, &allocInfo, &commandBuffer) !=
           VK_SUCCESS,
       "Failed to create Command buffer"
   );
}

void Program::recordCommandBuffer(
    VkCommandBuffer commandBuffer, uint32_t imageIndex
)
{
   VkCommandBufferBeginInfo beginInfo{};
   beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

   check_eq(
       vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS,
       "Failed to record Command Buffer"
   );
   VkRenderPassBeginInfo renderPassInfo{};
   renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
   renderPassInfo.renderPass = renderPass;
   renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
   renderPassInfo.renderArea.offset = {0, 0};
   renderPassInfo.renderArea.extent = swapChainExtent;

   VkClearValue clearColor = {{{.1f, 0.1f, 0.1f, 1.0f}}};
   renderPassInfo.clearValueCount = 1;
   renderPassInfo.pClearValues = &clearColor;
   vkCmdBeginRenderPass(
       commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE
   );
   vkCmdBindPipeline(
       commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline
   );

   VkViewport viewport{};
   viewport.x = 0.0f;
   viewport.y = 0.0f;
   viewport.width = static_cast<float>(swapChainExtent.width);
   viewport.height = static_cast<float>(swapChainExtent.height);
   viewport.minDepth = 0.0f;
   viewport.maxDepth = 1.0f;
   vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

   VkRect2D scissor{};
   scissor.offset = {0, 0};
   scissor.extent = swapChainExtent;
   vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

   vkCmdDraw(commandBuffer, 3, 1, 0, 0);
   vkCmdEndRenderPass(commandBuffer);
   check_eq(
       vkEndCommandBuffer(commandBuffer) != VK_SUCCESS,
       "Failed to record Command Buffer"
   );
}

void Program::createCommandPool()
{
   QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

   VkCommandPoolCreateInfo poolInfo{};
   poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
   poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
   poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
   check_eq(
       vkCreateCommandPool(Logicaldevice, &poolInfo, nullptr, &commandPool) !=
           VK_SUCCESS,
       "Failed To Create command Pool"
   );
}

void Program::createFrameBuffers()
{
   swapChainFramebuffers.resize(swapChainImageViews.size());
   for (size_t i = 0; i < swapChainImageViews.size(); i++) {
      VkImageView attachments[] = {swapChainImageViews[i]};

      VkFramebufferCreateInfo framebufferInfo{};
      framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebufferInfo.renderPass = renderPass;
      framebufferInfo.attachmentCount = 1;
      framebufferInfo.pAttachments = attachments;
      framebufferInfo.width = swapChainExtent.width;
      framebufferInfo.height = swapChainExtent.height;
      framebufferInfo.layers = 1;
      check_eq(
          vkCreateFramebuffer(
              Logicaldevice,
              &framebufferInfo,
              nullptr,
              &swapChainFramebuffers[i]
          ) != VK_SUCCESS,
          "failed to create framebuffer!"
      );
   }
}

void Program::createRenderPass()
{
   VkAttachmentDescription colorAttachment{};
   colorAttachment.format = swapChainImageFormat;
   colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
   colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
   colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
   colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
   colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

   VkAttachmentReference colorAttachmentRef{};
   colorAttachmentRef.attachment = 0;
   colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

   VkSubpassDescription subpass{};
   subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
   subpass.colorAttachmentCount = 1;
   subpass.pColorAttachments = &colorAttachmentRef;

   VkSubpassDependency dependency{};
   dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
   dependency.dstSubpass = 0;
   dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
   dependency.srcAccessMask = 0;
   dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
   dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

   VkRenderPassCreateInfo renderPassInfo{};
   renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
   renderPassInfo.attachmentCount = 1;
   renderPassInfo.pAttachments = &colorAttachment;
   renderPassInfo.subpassCount = 1;
   renderPassInfo.pSubpasses = &subpass;
   renderPassInfo.dependencyCount = 1;
   renderPassInfo.pDependencies = &dependency;

   check_eq(
       vkCreateRenderPass(
           Logicaldevice, &renderPassInfo, nullptr, &renderPass
       ) != VK_SUCCESS,
       "failed to create render pass!"
   );
}

VkShaderModule Program::createShaderModule(const std::vector<char> &code)
{
   VkShaderModuleCreateInfo createInfo{};
   createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
   createInfo.codeSize = code.size();
   createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());
   VkShaderModule shader;
   check_eq(
       vkCreateShaderModule(Logicaldevice, &createInfo, nullptr, &shader) !=
           VK_SUCCESS,
       "Failed to Compile Shader"
   );
   return shader;
}

void Program::createGraphicsPipeline()
{
   auto           vertShaderCode = readFile("shaders/vert.spv");
   auto           fragShaderCode = readFile("shaders/frag.spv");
   VkShaderModule vertShader = createShaderModule(vertShaderCode);
   VkShaderModule fragShader = createShaderModule(fragShaderCode);

   VkPipelineShaderStageCreateInfo vertShaderStageInfo{
       .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_VERTEX_BIT,
       .module = vertShader,
       .pName = "main",
   };

   VkPipelineShaderStageCreateInfo fragShaderStageInfo{
       .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
       .module = fragShader,
       .pName = "main",
   };

   VkPipelineShaderStageCreateInfo shaderStages[] = {
       vertShaderStageInfo, fragShaderStageInfo};

   VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
   vertexInputInfo.sType =
       VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
   vertexInputInfo.vertexBindingDescriptionCount = 0;
   vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
   vertexInputInfo.vertexAttributeDescriptionCount = 0;
   vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

   VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
   inputAssembly.sType =
       VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
   inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
   inputAssembly.primitiveRestartEnable = VK_FALSE;

   VkViewport viewport{};
   viewport.x = 0.0f;
   viewport.y = 0.0f;
   viewport.width = (float)swapChainExtent.width;
   viewport.height = (float)swapChainExtent.height;
   viewport.minDepth = 0.0f;
   viewport.maxDepth = 1.0f;

   VkRect2D scissor{};
   scissor.offset = {0, 0};
   scissor.extent = swapChainExtent;

   std::vector<VkDynamicState> dynamicStates = {
       VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
   VkPipelineDynamicStateCreateInfo dynamicState{};
   dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
   dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
   dynamicState.pDynamicStates = dynamicStates.data();

   VkPipelineViewportStateCreateInfo viewportState{};
   viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
   viewportState.viewportCount = 1;
   viewportState.pViewports = &viewport;
   viewportState.scissorCount = 1;
   viewportState.pScissors = &scissor;

   VkPipelineRasterizationStateCreateInfo rasterizer{};
   rasterizer.sType =
       VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
   rasterizer.depthClampEnable = VK_FALSE;
   rasterizer.rasterizerDiscardEnable = VK_FALSE;
   rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // wireframe mode switch
   rasterizer.lineWidth = 1.0f;
   rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
   rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

   rasterizer.depthBiasEnable = VK_FALSE;
   rasterizer.depthBiasConstantFactor = 0.0f; // Optional
   rasterizer.depthBiasClamp = 0.0f;          // Optional
   rasterizer.depthBiasSlopeFactor = 0.0f;    // Optional

   VkPipelineMultisampleStateCreateInfo multisampling{};
   multisampling.sType =
       VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
   multisampling.sampleShadingEnable = VK_FALSE;
   multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
   multisampling.minSampleShading = 1.0f;          // Optional
   multisampling.pSampleMask = nullptr;            // Optional
   multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
   multisampling.alphaToOneEnable = VK_FALSE;      // Optional

   VkPipelineColorBlendAttachmentState colorBlendAttachment{};
   colorBlendAttachment.colorWriteMask =
       VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
       VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
   colorBlendAttachment.blendEnable = VK_FALSE;
   colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
   colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
   colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;             // Optional
   colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
   colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
   colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;             // Optional

   VkPipelineColorBlendStateCreateInfo colorBlending{};
   colorBlending.sType =
       VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
   colorBlending.logicOpEnable = VK_FALSE;
   colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
   colorBlending.attachmentCount = 1;
   colorBlending.pAttachments = &colorBlendAttachment;
   colorBlending.blendConstants[0] = 0.0f; // Optional
   colorBlending.blendConstants[1] = 0.0f; // Optional
   colorBlending.blendConstants[2] = 0.0f; // Optional
   colorBlending.blendConstants[3] = 0.0f; // Optional

   VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
   pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
   pipelineLayoutInfo.setLayoutCount = 0;            // Optional
   pipelineLayoutInfo.pSetLayouts = nullptr;         // Optional
   pipelineLayoutInfo.pushConstantRangeCount = 0;    // Optional
   pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

   check_eq(
       vkCreatePipelineLayout(
           Logicaldevice, &pipelineLayoutInfo, nullptr, &pipelineLayout
       ) != VK_SUCCESS,
       "failed to create pipeline layout!"
   );

   VkGraphicsPipelineCreateInfo pipelineInfo{};
   pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
   pipelineInfo.stageCount = 2;
   pipelineInfo.pStages = shaderStages;
   pipelineInfo.pVertexInputState = &vertexInputInfo;
   pipelineInfo.pInputAssemblyState = &inputAssembly;
   pipelineInfo.pViewportState = &viewportState;
   pipelineInfo.pRasterizationState = &rasterizer;
   pipelineInfo.pMultisampleState = &multisampling;
   pipelineInfo.pDepthStencilState = nullptr; // Optional
   pipelineInfo.pColorBlendState = &colorBlending;
   pipelineInfo.pDynamicState = &dynamicState;
   pipelineInfo.layout = pipelineLayout;
   pipelineInfo.renderPass = renderPass;
   pipelineInfo.subpass = 0;
   //switcing pipelines
   pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
   pipelineInfo.basePipelineIndex = -1;              // Optional

   check_eq(
       vkCreateGraphicsPipelines(
           Logicaldevice,
           VK_NULL_HANDLE,
           1,
           &pipelineInfo,
           nullptr,
           &graphicsPipeline
       ) != VK_SUCCESS,
       "Failed to Create PipeLine"
   );

   vkDestroyShaderModule(Logicaldevice, fragShader, nullptr);
   vkDestroyShaderModule(Logicaldevice, vertShader, nullptr);
}

void Program::createImageViews()
{
   swapChainImageViews.resize(swapChainImages.size());
   for (size_t i = 0; i < swapChainImages.size(); i++) {
      VkImageViewCreateInfo createInfo{};
      createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      createInfo.image = swapChainImages[i];
      createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
      createInfo.format = swapChainImageFormat;
      // siwizzlinggggg
      createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      createInfo.subresourceRange.baseMipLevel = 0;
      createInfo.subresourceRange.levelCount = 1;
      createInfo.subresourceRange.baseArrayLayer = 0;
      createInfo.subresourceRange.layerCount = 1;
      check_eq(
          vkCreateImageView(
              Logicaldevice, &createInfo, nullptr, &swapChainImageViews[i]
          ) != VK_SUCCESS,
          "Failed to create image views"
      )
   }
}

void Program::createSwapChain()
{
   SwapChainSupportDetails swapChainSupport =
       querySwapChainSupport(physicalDevice);

   VkSurfaceFormatKHR surfaceFormat =
       chooseSwapSurfaceFormat(swapChainSupport.formats);
   VkPresentModeKHR presentMode =
       chooseSwapPresentMode(swapChainSupport.presentModes);
   VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

   swapChainImageFormat = surfaceFormat.format;
   swapChainExtent = extent;

   uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
   if (swapChainSupport.capabilities.maxImageCount > 0 &&
       imageCount > swapChainSupport.capabilities.maxImageCount) {
      imageCount = swapChainSupport.capabilities.maxImageCount;
   }

   VkSwapchainCreateInfoKHR createInfo{
       .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
       .surface = surface,
       .minImageCount = imageCount,
       .imageFormat = surfaceFormat.format,
       .imageColorSpace = surfaceFormat.colorSpace,
       .imageExtent = extent,
       .imageArrayLayers = 1,
       .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, // important for
                                                          // image processing
       .preTransform = swapChainSupport.capabilities.currentTransform,
       .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
       .presentMode = presentMode,
       .clipped = VK_TRUE,
       .oldSwapchain = VK_NULL_HANDLE,
   };

   QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
   uint32_t           queueFamilyIndices[] = {
       indices.graphicsFamily.value(), indices.presentFamily.value()};

   if (indices.graphicsFamily != indices.presentFamily) {
      createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      createInfo.queueFamilyIndexCount = 2;
      createInfo.pQueueFamilyIndices = queueFamilyIndices;
   } else {
      createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      createInfo.queueFamilyIndexCount = 0;
      createInfo.pQueueFamilyIndices = nullptr;
   }
   check_eq(
       vkCreateSwapchainKHR(Logicaldevice, &createInfo, nullptr, &swapChain) !=
           VK_SUCCESS,
       "Failed to creeate swapchain"
   );
   vkGetSwapchainImagesKHR(Logicaldevice, swapChain, &imageCount, nullptr);
   swapChainImages.resize(imageCount);
   vkGetSwapchainImagesKHR(
       Logicaldevice, swapChain, &imageCount, swapChainImages.data()
   );
}

VkSurfaceFormatKHR Program::chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats
)
{
   for (const auto &availableFormat : availableFormats) {
      if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
          availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
         return availableFormat;
      }
   }

   return availableFormats[0];
}

VkPresentModeKHR Program::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes
)
{
   for (const auto &availablePresentMode : availablePresentModes) {
      if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
         return availablePresentMode;
      }
   }

   return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D
Program::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
{
   if (capabilities.currentExtent.width !=
       std::numeric_limits<uint32_t>::max()) {
      return capabilities.currentExtent;
   } else {
      int width, height;
      glfwGetFramebufferSize(window, &width, &height);

      VkExtent2D actualExtent = {
          static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

      actualExtent.width = std::clamp(
          actualExtent.width,
          capabilities.minImageExtent.width,
          capabilities.maxImageExtent.width
      );
      actualExtent.height = std::clamp(
          actualExtent.height,
          capabilities.minImageExtent.height,
          capabilities.maxImageExtent.height
      );
      return actualExtent;
   }
}

SwapChainSupportDetails Program::querySwapChainSupport(VkPhysicalDevice device)
{
   SwapChainSupportDetails details;
   vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
       device, surface, &details.capabilities
   );

   uint32_t formatCount;
   vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
   if (formatCount != 0) {
      details.formats.resize(formatCount);
      vkGetPhysicalDeviceSurfaceFormatsKHR(
          device, surface, &formatCount, details.formats.data()
      );
   }

   uint32_t presentModeCount;
   vkGetPhysicalDeviceSurfacePresentModesKHR(
       device, surface, &presentModeCount, nullptr
   );
   if (presentModeCount != 0) {
      details.presentModes.resize(presentModeCount);
      vkGetPhysicalDeviceSurfacePresentModesKHR(
          device, surface, &presentModeCount, details.presentModes.data()
      );
   }

   return details;
}

bool Program::shouldclose()
{
   return glfwWindowShouldClose(window);
}

Program::~Program()
{
   vkDeviceWaitIdle(Logicaldevice);
   if (enableValidationLayers) {
      DestroyDebugUtilsMessengerEXT(instance, debugMessenger, NULL);
   }
   vkDestroySemaphore(Logicaldevice, imageAvailableSemaphore, nullptr);
   vkDestroySemaphore(Logicaldevice, renderFinishedSemaphore, nullptr);
   vkDestroyFence(Logicaldevice, inFlightFence, nullptr);

   vkDestroyCommandPool(Logicaldevice, commandPool, nullptr);
   for (auto imageView : swapChainImageViews) {
      vkDestroyImageView(Logicaldevice, imageView, nullptr);
   }
   for (auto framebuffer : swapChainFramebuffers) {
      vkDestroyFramebuffer(Logicaldevice, framebuffer, nullptr);
   }

   vkDestroyPipeline(Logicaldevice, graphicsPipeline, nullptr);
   vkDestroyPipelineLayout(Logicaldevice, pipelineLayout, nullptr);
   vkDestroyRenderPass(Logicaldevice, renderPass, nullptr);

   vkDestroySwapchainKHR(Logicaldevice, swapChain, nullptr);
   vkDestroyDevice(Logicaldevice, nullptr);
   vkDestroySurfaceKHR(instance, surface, nullptr);
   vkDestroyInstance(instance, nullptr);
   glfwDestroyWindow(window);
   glfwTerminate();
}

std::vector<const char *> getRequiredExtentions()
{
   uint32_t     glfwExtensionCount = 0;
   const char **glfwExtensions;
   glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

   std::vector<const char *> extensions;

   for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
      extensions.emplace_back(glfwExtensions[i]);
   }
   extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

   if (enableValidationLayers) {
      extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
   }
   return extensions;
}

void Program::createInstance()
{
   if (enableValidationLayers) {
      check_eq(!checkValidationLayerSupport(), "No Validation Layer Found");
   }

   VkApplicationInfo appInfo = {};
   appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
   appInfo.pApplicationName = "Hello Triangle";
   appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
   appInfo.pEngineName = "No Engine";
   appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
   appInfo.apiVersion = VK_API_VERSION_1_0;

   VkInstanceCreateInfo createInfo = {};
   createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
   createInfo.pApplicationInfo = &appInfo;

   std::vector<const char *> extentions = getRequiredExtentions();
   createInfo.enabledExtensionCount = extentions.size();
   createInfo.ppEnabledExtensionNames = extentions.data();

   createInfo.enabledLayerCount = validationLayers.size();
   createInfo.ppEnabledLayerNames = validationLayers.data();
   check_eq(
       vkCreateInstance(&createInfo, NULL, &instance) != VK_SUCCESS,
       "Failed to create VK Instance"
   );
}

void Program::createWindow(int w, int h, const char *title)
{
   glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
   glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
   window = glfwCreateWindow(w, h, title, NULL, NULL);
}

void Program::setupDebugMessage()
{
   if (!enableValidationLayers) {
      return;
   }
   VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
   createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
   createInfo.messageSeverity =
       VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
       VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
       VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
   createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
   createInfo.pfnUserCallback = debugCallback;
   createInfo.pUserData = NULL; // Optional
   check_eq(
       CreateDebugUtilsMessengerEXT(
           instance, &createInfo, NULL, &debugMessenger
       ) != VK_SUCCESS,
       "Failed to add debug callback"
   );
}

QueueFamilyIndices Program::findQueueFamilies(VkPhysicalDevice device)
{
   QueueFamilyIndices indices;
   uint32_t           queueFamilyCount = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

   std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
   vkGetPhysicalDeviceQueueFamilyProperties(
       device, &queueFamilyCount, queueFamilies.data()
   );

   int i = 0;
   for (const auto &queueFamily : queueFamilies) {
      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
      if (presentSupport) {
         indices.presentFamily = i;
      }
      if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
         indices.graphicsFamily = i;
      }
      i++;
   }

   return indices;
}

bool Program::checkDeviceExtensionSupport(
    VkPhysicalDevice device, const std::vector<const char *> &requiredExtentions
)
{
   uint32_t extensionCount;
   vkEnumerateDeviceExtensionProperties(
       device, nullptr, &extensionCount, nullptr
   );

   std::vector<VkExtensionProperties> availableExtensions(extensionCount);
   vkEnumerateDeviceExtensionProperties(
       device, nullptr, &extensionCount, availableExtensions.data()
   );

   for (const auto &requiredExtension : requiredExtentions) {
      bool found = false;
      for (const auto &availableExtension : availableExtensions) {
         if (strcmp(requiredExtension, availableExtension.extensionName) == 0) {
            found = true;
            break;
         }
      }
      if (found == false) {
         return false;
      }
   }
   return true;
}

bool Program::isDeviceSuitable(VkPhysicalDevice device)
{
   QueueFamilyIndices indices = findQueueFamilies(device);

   bool extensionsSupported =
       checkDeviceExtensionSupport(device, deviceExtensions);
   bool swapChainAdequate = false;
   if (extensionsSupported) {
      SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
      swapChainAdequate = !swapChainSupport.formats.empty() &&
                          !swapChainSupport.presentModes.empty();
   }

   return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

void Program::pickPhysicalDevice()
{
   uint32_t deviceCount = 0;
   vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
   if (deviceCount == 0) {
      printf("Failed to find a gpu that supports vulkan");
      exit(1);
   }
   std::vector<VkPhysicalDevice> devices(deviceCount);
   vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
   for (const auto &device : devices) {
      if (isDeviceSuitable(device)) {
         physicalDevice = device;
         break;
      }
   }
   check_eq(physicalDevice == VK_NULL_HANDLE, "Failed to find a sutable gpu");
}

void Program::createDevice()
{
   QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
   std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
   // note this can be easily optimized
   std::set<uint32_t>                   uniqueQueueFamilies = {
       indices.graphicsFamily.value(), indices.presentFamily.value()};
   float queuePriority = 1.0f;
   for (uint32_t queueFamily : uniqueQueueFamilies) {
      VkDeviceQueueCreateInfo queueCreateInfo{};
      queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueCreateInfo.queueFamilyIndex = queueFamily;
      queueCreateInfo.queueCount = 1;
      queueCreateInfo.pQueuePriorities = &queuePriority;
      queueCreateInfos.push_back(queueCreateInfo);
   }


   VkDeviceCreateInfo createInfo{};
   createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
   createInfo.pQueueCreateInfos = queueCreateInfos.data();
   createInfo.queueCreateInfoCount = queueCreateInfos.size();
   VkPhysicalDeviceFeatures deviceFeatures{};

   createInfo.pEnabledFeatures = &deviceFeatures;
   createInfo.enabledExtensionCount =
       static_cast<uint32_t>(deviceExtensions.size());
   createInfo.ppEnabledExtensionNames = deviceExtensions.data();

   check_eq(
       vkCreateDevice(physicalDevice, &createInfo, nullptr, &Logicaldevice) !=
           VK_SUCCESS,
       "Failed to create Logical Device"
   );
   vkGetDeviceQueue(
       Logicaldevice, indices.graphicsFamily.value(), 0, &graphicsQueue
   );
   vkGetDeviceQueue(
       Logicaldevice, indices.presentFamily.value(), 0, &presentQueue
   );
}

void Program::createSurface()
{
   check_eq(
       glfwCreateWindowSurface(instance, window, nullptr, &surface) !=
           VK_SUCCESS,
       "Failed to create window surface"
   );
}

int main(void)
{
   Program     program;
   GLFWvidmode screen;
   glfwInit();
   GLFWmonitor *monitor = glfwGetPrimaryMonitor();
   screen = *glfwGetVideoMode(monitor);
   // 175 just works well with my setup
   program.createWindow(screen.width / 2 + 175, screen.height, "Vulkan Window");
   program.createInstance();
   program.setupDebugMessage();
   program.createSurface();
   program.pickPhysicalDevice();
   program.createDevice();
   program.createSwapChain();
   program.createImageViews();
   program.createRenderPass();
   program.createGraphicsPipeline();
   program.createCommandPool();
   program.createCommandBuffer();
   program.createSyncObjects();
   program.createFrameBuffers();
   //what next ?
   while (!program.shouldclose()) {
      program.drawFrame();
      glfwPollEvents();
   }
}
