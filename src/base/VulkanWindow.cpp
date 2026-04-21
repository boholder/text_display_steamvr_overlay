#include "VulkanRenderer.h"
#include "VulkanUtils.h"

VulkanWindow::VulkanWindow(const uint8_t index)
{
    this->index=index;
    width=0;
    height=0;
    swapchain=VK_NULL_HANDLE;
    surface=VK_NULL_HANDLE;
    surface_format={};
    present_mode={};
    render_pass=VK_NULL_HANDLE;
    pipeline=VK_NULL_HANDLE;
    clear_enable=false;
    clear_value={};
    frame_index=0;
    image_count=0;
    semaphore_count=0;
    semaphore_index=0;
    is_minimized=false;
}

void VulkanWindow::set_background_color(const ImVec4& color)
{ vk_util::set_vk_clear_value_background_color(this->clear_value, color); }
