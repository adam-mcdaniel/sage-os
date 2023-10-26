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
#include <virtio.h>
#include <gpu.h>
#include <kmalloc.h>
#include <vector.h>

static Vector *device_active_jobs;
static VirtioDevice *gpu_device;

void gpu_test() {
    gpu_init(gpu_device);
}

void gpu_device_init() {
    device_active_jobs = vector_new();
    gpu_device = virtio_get_gpu_device();
    // debugf("GPU device init done for device at %p\n", gpu_device->pcidev->ecam_header);
    gpu_device->ready = true;
    volatile VirtioGpuConfig *config = virtio_get_gpu_config(gpu_device);
    debugf("GPU device has %d events that needs to be read\n", config->events_read);
    debugf("GPU device has %d scanouts\n", config->num_scanouts);
}

bool gpu_init(VirtioDevice *gpu_device) {
    // DisplayInfoResponse disp;
    // VirtioGpuCtrlHdr ctrl;
    // ResourceCreate2dRequest res2d;
    // GpuResourceAttachBacking attach;
    // SetScanoutRequest scan;
    // GpuMemEntry mem;
    // GpuResourceFlush flush;
    // GpuTransferToHost2d tx;

    gpu_set_display_info(gpu_device);

    // // 2. Create a resource 2D.
    // memset(&res2d, 0, sizeof(res2d));
    // res2d.format = R8G8B8A8_UNORM;
    // res2d.height = gdev->height;
    // res2d.width = gdev->width;
    // res2d.resource_id = 1;
    // res2d.hdr.control_type = VIRTIO_GPU_CMD_RESOURCE_CREATE_2D;
    // gpu_send(gdev, &res2d, sizeof(res2d), &ctrl, sizeof(ctrl));
    // gpu_wait_for_response(gdev);
    // // 3. Attach resource 2D.
    // memset(&attach, 0, sizeof(attach));
    // attach.nr_entries = 1;
    // attach.resource_id = 1;
    // attach.hdr.control_type = VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING;
    // mem.addr = mmu_translate(kernel_mmu_table, (uint64_t)gdev->framebuffer);
    // mem.length = sizeof(PixelRGBA) * gdev->width * gdev->height;
    // mem.padding = 0;
    // memset(&ctrl, 0, sizeof(ctrl));
    // gpu_send_3(gdev, &attach, sizeof(attach), &mem, sizeof(mem), &ctrl, sizeof(ctrl));
    // gpu_wait_for_response(gdev);
    // // 4. Set scanout and connect it to the resource.
    // memset(&scan, 0, sizeof(scan));
    // scan.hdr.control_type = VIRTIO_GPU_CMD_SET_SCANOUT;
    // scan.rect.width = gdev->width;
    // scan.rect.height = gdev->height;
    // scan.resource_id = 1;
    // scan.scanout_id = 0;
    // memset(&ctrl, 0, sizeof(ctrl));
    // gpu_send(gdev, &scan, sizeof(scan), &ctrl, sizeof(ctrl));
    // gpu_wait_for_response(gdev);
    // // Add something to the framebuffer so it will show up on screen.
    // Rect r1 = {0, 0, gdev->width, gdev->height};
    // Rect r2 = {100, 100, gdev->width - 150, gdev->height - 150};
    // PixelRGBA z1 = { 255, 100, 50, 255 };
    // PixelRGBA z2 = { 50, 0, 255, 255 };
    // fb_fill_rect(gdev->width, gdev->height, gdev->framebuffer, &r1, &z1);
    // fb_stroke_rect(gdev->width, gdev->height, gdev->framebuffer, &r2, &z2, 10);
    // // 5. Transfer the framebuffer to the host 2d.
    // memset(&tx, 0, sizeof(tx));
    // tx.hdr.control_type = VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D;
    // tx.rect.width = gdev->width;
    // tx.rect.height = gdev->height;
    // tx.resource_id = 1;
    // memset(&ctrl, 0, sizeof(ctrl));
    // gpu_send(gdev, &tx, sizeof(tx), &ctrl, sizeof(ctrl));
    // gpu_wait_for_response(gdev);
    // // 6. Flush the resource to draw to the screen.
    // memset(&flush, 0, sizeof(flush));
    // flush.hdr.control_type = VIRTIO_GPU_CMD_RESOURCE_FLUSH;
    // flush.rect.width = gdev->width;
    // flush.rect.height = gdev->height;
    // flush.resource_id = 1;
    // memset(&ctrl, 0, sizeof(ctrl));
    // gpu_send(gdev, &flush, sizeof(flush), &ctrl, sizeof(ctrl));
    // gpu_wait_for_response(gdev);
    return true;
}

void gpu_set_display_info(VirtioDevice *gpu_device) {
    VirtioGpuCtrlHdr ctrl_header;
    ctrl_header.control_type = VIRTIO_GPU_CMD_GET_DISPLAY_INFO;
    ctrl_header.flags = 0;
    ctrl_header.fence_id = 0;
    ctrl_header.context_id = 0;
    ctrl_header.padding = 0;

    VirtioDescriptor ctrl_header_desc;
    ctrl_header_desc.addr = kernel_mmu_translate(&ctrl_header);
    ctrl_header_desc.len = sizeof(ctrl_header);
    ctrl_header_desc.flags = VIRTQ_DESC_F_NEXT;

    VirtioGpuDispInfoResp disp_resp;
    VirtioDescriptor disp_resp_desc;
    disp_resp_desc.addr = kernel_mmu_translate(&disp_resp);
    disp_resp_desc.len = sizeof(disp_resp);
    disp_resp_desc.flags = VIRTQ_DESC_F_WRITE;

    VirtioDescriptor chain[2] = {ctrl_header_desc, disp_resp_desc};

    virtio_send_descriptor_chain(gpu_device, 0, chain, 2, true);

    // TODO
    while (gpu_device->device_idx != gpu_device->device->idx) {
        ++gpu_device->device_idx;
    }

    debugf("Internal desc_idx: %d\n", gpu_device->desc_idx);
    debugf("Internal driver_idx: %d\n", gpu_device->driver_idx);
    debugf("Internal device_idx: %d\n", gpu_device->device_idx);
    debugf("Driver ring index: %d\n", gpu_device->driver->idx);
    debugf("Device ring index: %d\n", gpu_device->device->idx);

    debugf("used element id: %d\n", gpu_device->device->ring[0].id);
    debugf("used element id: %d\n", gpu_device->device->ring[0].len);
    debugf("used element id: %d\n", gpu_device->device->ring[1].id);
    debugf("used element id: %d\n", gpu_device->device->ring[1].len);

    if (disp_resp.ctrl_header.control_type == VIRTIO_GPU_RESP_OK_DISPLAY_INFO)
        debugf("gpu_set_display_info: Received display info\n");

    if (!disp_resp.displays[0].enabled)
        return false;
    debugf("gpu_set_display_info: Display 0 enabled\n");
}

// void fill_rect(uint32_t screen_width,
//                uint32_t screen_height,
//                struct pixel *buffer,
//                const struct rectangle *rect,
//                const struct pixel *fill_color)
//                {
//     uint32_t top = rect->y;
//     uint32_t bottom = rect->y + rect->height;
//     uint32_t left = rect->x;
//     uint32_t right = rect->x + rect->width;
//     uint32_t row;
//     uint32_t col;
//     uint32_t offset;

//     if (bottom > screen_height) {
//         bottom = screen_height;
//     }
//     if (right > screen_width) {
//         right = screen_width;
//     }

//     for (row = top;row < bottom;row++) {
//         for (col = left;col < right;col++) {
//             buffer[row * screen_width + col] = *fill_color;
//         }
//    }
// }

// static inline void RVALS(Rectangle *r,
//                          uint32_t x,
//                          uint32_t y,
//                          uint32_t width,
//                          uint32_t height)
// {
//     r->x = x;
//     r->y = y;
//     r->width = width;
//     r->height = height;
// }

// void stroke_rect(uint32_t screen_width,
//                  uint32_t screen_height,
//                  Pixel *buffer,
//                  const Rectangle *rect,
//                  const Pixel *line_color,
//                  uint32_t line_size)
// {
//     struct Rectangle r;
//     // Top
//     RVALS(&r, rect->x, rect->y,
//               rect->width, line_size);
//     fill_rect(screen_width, screen_height, buffer, &r, line_color);

//     // Bottom
//     RVALS(&r, rect->x, rect->height + rect->y,
//               rect->width, line_size);
//     fill_rect(screen_width, screen_height, buffer, &r, line_color);

//     // Left
//     RVALS(&r, rect->x, rect->y,
//               line_size, rect->height);
//     fill_rect(screen_width, screen_height, buffer, &r, line_color);

//     // Right
//     RVALS(&r, rect->x + rect->width, rect->y,
//               line_size, rect->height + line_size);
//     fill_rect(screen_width, screen_height, buffer, &r, line_color);
// }