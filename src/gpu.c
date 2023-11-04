/**
 * @file pci.c
 * @brief GPU driver.
 * @version 0.1
 * @date 2023-10-25
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <debug.h>
#include <csr.h>
#include <gpu.h>
#include <kmalloc.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <vector.h>
#include <mmu.h>
#include "virtio.h"

static Vector *device_active_jobs;
static VirtioDevice *gpu_device;
static Console console; // NOTE: Figure how this is supposed to be interfaced, allocate appropriately

bool gpu_test() {
    return gpu_init(gpu_device);
}

void gpu_device_init() {
    device_active_jobs = vector_new();
    gpu_device = virtio_get_gpu_device();
    // debugf("GPU device init done for device at %p\n", gpu_device->pcidev->ecam_header);
    virtio_set_device_name(gpu_device, "GPU Device");
    gpu_device->ready = true;
    volatile VirtioGpuConfig *config = virtio_get_gpu_config(gpu_device);
    debugf("GPU device has %d events that needs to be read\n", config->events_read);
    debugf("GPU device has %d scanouts\n", config->num_scanouts);
}

// TODO: Implement checking for error responses
bool gpu_init(VirtioDevice *gpu_device) {
    VirtioGpuDispInfoResp disp_info;
    gpu_get_display_info(gpu_device, &disp_info);
    
    // Allocate memory for frame buffer
    console.width = disp_info.displays[0].rect.width;
    console.height = disp_info.displays[0].rect.height;
    console.frame_buf = kcalloc(console.width * console.height, sizeof(Pixel));
    debugf("gpu_init: Allocated frame buffer of (%d * %d) bytes at %p\n",
           sizeof(Pixel), console.width * console.height, console.frame_buf);

    VirtioGpuResCreate2d res2d;
    res2d.hdr.type = VIRTIO_GPU_CMD_RESOURCE_CREATE_2D;
    res2d.resource_id = 1; // Give an arbitrary unique number
    res2d.format = R8G8B8A8_UNORM;
    res2d.width = console.width;
    res2d.height = console.height;

    VirtioGpuCtrlHdr resp_hdr;
    resp_hdr.type = 0;

    gpu_send_command(gpu_device, 0, &res2d, sizeof(res2d), NULL, 0, &resp_hdr, sizeof(resp_hdr));
    if (resp_hdr.type == VIRTIO_GPU_RESP_OK_NODATA) {
        debugf("gpu_init: Create 2D resource OK\n");
    } else {
        // debugf("gpu_init: Create 2D resource failed with %s\n", gpu_get_resp_string(resp_hdr.type));
        // return false;
    }

    // Attach resource 2D
    VirtioGpuResourceAttachBacking attach_backing;
    attach_backing.hdr.type = VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING;
    attach_backing.resource_id = 1;
    attach_backing.nr_entries = 1;
    VirtioGpuMemEntry mem;
    // mem.addr = kernel_mmu_translate((uintptr_t)console.frame_buf
    mem.addr = kernel_mmu_translate((uintptr_t)console.frame_buf);
    mem.length = console.width * console.width * sizeof(Pixel);
    mem.padding = 0;
    resp_hdr.type = 0;
    
    gpu_send_command(gpu_device, 0, &attach_backing, sizeof(attach_backing), &mem, sizeof(mem), &resp_hdr, sizeof(resp_hdr));

    if (resp_hdr.type == VIRTIO_GPU_RESP_OK_NODATA) {
        debugf("gpu_init: Attach backing OK\n");
    } else {
        // debugf("gpu_init: Attach backing failed with %s\n", gpu_get_resp_string(resp_hdr.type));
        // return false;
    }

    VirtioGpuSetScanout scan;
    scan.hdr.type = VIRTIO_GPU_CMD_SET_SCANOUT;
    scan.rect.x = 0;
    scan.rect.y = 0;
    scan.rect.width = console.width;
    scan.rect.height = console.height;
    scan.resource_id = 1;
    scan.scanout_id = 0;
    resp_hdr.type = 0;

    gpu_send_command(gpu_device, 0, &scan, sizeof(scan), NULL, 0, &resp_hdr, sizeof(resp_hdr));

    Rectangle r1 = {0, 0, console.width, console.height};
    Rectangle r2 = {100, 100, console.width - 150, console.height - 150};
    Pixel p1 = {255, 100, 50, 255};
    Pixel p2 = {88, 89, 91, 255};

    if (resp_hdr.type == VIRTIO_GPU_RESP_OK_NODATA) {
        debugf("gpu_init: Set scanout OK\n");
    } else {
        // debugf("gpu_init: Set scanout failed with %s\n", gpu_get_resp_string(resp_hdr.type));
        // return false;
    }

    fill_rect(console.width, console.height, console.frame_buf, &r1, &p1);
    stroke_rect(console.width, console.height, console.frame_buf, &r2, &p2, 10);

    VirtioGpuTransferToHost2d tx;
    tx.hdr.type = VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D;
    tx.rect.x = 0;
    tx.rect.y = 0;
    tx.rect.width = console.width;
    tx.rect.height = console.height;
    tx.offset = 0;
    tx.resource_id = 1;
    tx.padding = 0;
    resp_hdr.type = 0;

    gpu_send_command(gpu_device, 0, &tx, sizeof(tx), NULL, 0, &resp_hdr, sizeof(resp_hdr));
    
    if (resp_hdr.type == VIRTIO_GPU_RESP_OK_NODATA) {
        debugf("gpu_init: Transfer OK\n");
    } else {
        // debugf("gpu_init: Transfer failed with %s\n", gpu_get_resp_string(resp_hdr.type));
        // return false;
    }

    VirtioGpuResourceFlush flush;
    flush.hdr.type = VIRTIO_GPU_CMD_RESOURCE_FLUSH;
    flush.rect.x = 0;
    flush.rect.y = 0;
    flush.rect.width = console.width;
    flush.rect.height = console.height;
    flush.resource_id = 1;
    flush.padding = 0;
    resp_hdr.type = 0;
    gpu_send_command(gpu_device, 0, &flush, sizeof(flush), NULL, 0, &resp_hdr, sizeof(resp_hdr));
    
    if (resp_hdr.type == VIRTIO_GPU_RESP_OK_NODATA) {
        debugf("gpu_init: Flush OK\n");
    } else {
        // debugf("gpu_init: Flush failed with %s\n", gpu_get_resp_string(resp_hdr.type));
        // return false;
    }

    return true;
}

// Send a command to GPU.
// To send a command/response or a command/data pair set resp0 to NULL and resp0_size to 0.
// To send a command/data/response chain set every argument in order.
void gpu_send_command(VirtioDevice *gpu_device,
                      uint16_t which_queue,
                      void *cmd,
                      size_t cmd_size,
                      void *resp0,
                      size_t resp0_size,
                      void *resp1,
                      size_t resp1_size) {
    VirtioDescriptor cmd_desc;
    cmd_desc.addr = kernel_mmu_translate((uintptr_t)cmd);
    cmd_desc.len = cmd_size;
    cmd_desc.flags = VIRTQ_DESC_F_NEXT;

    VirtioDescriptor resp0_desc;
    resp0_desc.addr = kernel_mmu_translate((uintptr_t)resp0);
    resp0_desc.len = resp0_size;
    resp0_desc.flags = VIRTQ_DESC_F_NEXT;
    
    VirtioDescriptor resp1_desc;
    resp1_desc.addr = kernel_mmu_translate((uintptr_t)resp1);
    resp1_desc.len = resp1_size;
    resp1_desc.flags = VIRTQ_DESC_F_WRITE;

    VirtioDescriptor chain0[2] = {cmd_desc, resp1_desc};
    VirtioDescriptor chain1[3] = {cmd_desc, resp0_desc, resp1_desc};
    VirtioDescriptor *chain = resp0 == NULL ? chain0 : chain1;
    unsigned num_descriptors = resp0 == NULL ? 2 : 3;
    virtio_send_descriptor_chain(gpu_device, which_queue, chain, num_descriptors, true);
    
    // Wait until device_idx catches up 
    while (gpu_device->device_idx != gpu_device->device->idx) {
        debugf("GPU WAITING\n");
    }
    debugf("gpu_send_command: device_idx caught up\n");
}

// Get display info and set frame buffer's info.
// Return true if display is enabled. Return false if not.
bool gpu_get_display_info(VirtioDevice *gpu_device,
                          VirtioGpuDispInfoResp *disp_resp) {
    VirtioGpuCtrlHdr hdr;
    hdr.type = VIRTIO_GPU_CMD_GET_DISPLAY_INFO;
    hdr.flags = 0;
    hdr.fence_id = 0;
    hdr.context_id = 0;
    hdr.padding = 0;

    VirtioDescriptor hdr_desc;
    hdr_desc.addr = kernel_mmu_translate((uintptr_t)&hdr);
    hdr_desc.len = sizeof(hdr);
    hdr_desc.flags = VIRTQ_DESC_F_NEXT;

    VirtioDescriptor disp_resp_desc;
    disp_resp_desc.addr = kernel_mmu_translate((uintptr_t)disp_resp);
    disp_resp_desc.len = sizeof(VirtioGpuDispInfoResp);
    disp_resp_desc.flags = VIRTQ_DESC_F_WRITE;

    VirtioDescriptor chain[2] = {hdr_desc, disp_resp_desc};

    virtio_send_descriptor_chain(gpu_device, 0, chain, 2, true);

    debugf("Internal desc_idx: %d\n", gpu_device->desc_idx);
    debugf("Internal driver_idx: %d\n", gpu_device->driver_idx);
    debugf("Internal device_idx: %d\n", gpu_device->device_idx);
    debugf("Driver ring index: %d\n", gpu_device->driver->idx);
    debugf("Device ring index: %d\n", gpu_device->device->idx);

    debugf("used element id: %d\n", gpu_device->device->ring[0].id);
    debugf("used element len: %d\n", gpu_device->device->ring[0].len);
    debugf("used element id: %d\n", gpu_device->device->ring[1].id);
    debugf("used element len: %d\n", gpu_device->device->ring[1].len);

    if (disp_resp->hdr.type == VIRTIO_GPU_RESP_OK_DISPLAY_INFO)
        debugf("gpu_get_display_info: Received display info\n");

    if (!disp_resp->displays[0].enabled)
        return false;
    debugf("gpu_get_display_info: Display 0 enabled\n");

    return true;
}

void fill_rect(uint32_t screen_width,
               uint32_t screen_height,
               Pixel *frame_buf,
               const Rectangle *rect,
               const Pixel *fill_color)
               {
    uint32_t top = rect->y;
    uint32_t bottom = rect->y + rect->height;
    uint32_t left = rect->x;
    uint32_t right = rect->x + rect->width;
    uint32_t row;
    uint32_t col;
    uint32_t offset;

    if (bottom > screen_height) {
        bottom = screen_height;
    }
    if (right > screen_width) {
        right = screen_width;
    }

    for (row = top; row < bottom; row++) {
        for (col = left;col < right;col++) {
            frame_buf[row * screen_width + col] = *fill_color;
        }
   }
}

static inline void RVALS(Rectangle *r,
                         uint32_t x,
                         uint32_t y,
                         uint32_t width,
                         uint32_t height)
{
    r->x = x;
    r->y = y;
    r->width = width;
    r->height = height;
}

void stroke_rect(uint32_t screen_width,
                 uint32_t screen_height,
                 Pixel *frame_buf,
                 const Rectangle *rect,
                 const Pixel *line_color,
                 uint32_t line_size)
{
    struct Rectangle r;
    // Top
    RVALS(&r, rect->x, rect->y,
              rect->width, line_size);
    fill_rect(screen_width, screen_height, frame_buf, &r, line_color);

    // Bottom
    RVALS(&r, rect->x, rect->height + rect->y,
              rect->width, line_size);
    fill_rect(screen_width, screen_height, frame_buf, &r, line_color);

    // Left
    RVALS(&r, rect->x, rect->y,
              line_size, rect->height);
    fill_rect(screen_width, screen_height, frame_buf, &r, line_color);

    // Right
    RVALS(&r, rect->x + rect->width, rect->y,
              line_size, rect->height + line_size);
    fill_rect(screen_width, screen_height, frame_buf, &r, line_color);
}

// Return the respective response message string
static char *gpu_get_resp_string(VirtioGpuCtrlType type) {
    switch (type) {
        // Success responses
        case VIRTIO_GPU_RESP_OK_NODATA: return "VIRTIO_GPU_RESP_OK_NODATA";
        case VIRTIO_GPU_RESP_OK_DISPLAY_INFO: return "VIRTIO_GPU_RESP_OK_DISPLAY_INFO";
        case VIRTIO_GPU_RESP_OK_CAPSET_INFO: return "VIRTIO_GPU_RESP_OK_CAPSET_INFO";
        case VIRTIO_GPU_RESP_OK_CAPSET: return "VIRTIO_GPU_RESP_OK_CAPSET";
        case VIRTIO_GPU_RESP_OK_EDID: return "VIRTIO_GPU_RESP_OK_EDID";
        // Error responses
        case VIRTIO_GPU_RESP_ERR_UNSPEC: return "VIRTIO_GPU_RESP_ERR_UNSPEC";
        case VIRTIO_GPU_RESP_ERR_OUT_OF_MEMORY: return "VIRTIO_GPU_RESP_ERR_OUT_OF_MEMORY";
        case VIRTIO_GPU_RESP_ERR_INVALID_SCANOUT_ID: return "VIRTIO_GPU_RESP_ERR_INVALID_SCANOUT_ID";
        case VIRTIO_GPU_RESP_ERR_INVALID_RESOURCE_ID: return "VIRTIO_GPU_RESP_ERR_INVALID_RESOURCE_ID";
        case VIRTIO_GPU_RESP_ERR_INVALID_CONTEXT_ID: return "VIRTIO_GPU_RESP_ERR_INVALID_CONTEXT_ID";
        case VIRTIO_GPU_RESP_ERR_INVALID_PARAMETER: return "VIRTIO_GPU_RESP_ERR_INVALID_PARAMETER";
        default: return "Invalid type argument to gpu_get_resp_string"; break;
    }
}
