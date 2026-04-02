#include "VulkanRenderer.h"
#include "VulkanUtils.h"

void Vulkan_Window::set_background_color(const ImVec4& color)
{ vk_util::set_vk_clear_value_background_color(this->clear_value, color); }
