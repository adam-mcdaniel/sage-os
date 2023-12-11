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
#include <lock.h>
#include "virtio.h"


#define GPU_DEBUG
#ifdef GPU_DEBUG
#define debugf(...) debugf(__VA_ARGS__)
#else
#define debugf(...)
#endif

static Vector *device_active_jobs;
static VirtioDevice *gpu_device = NULL;
static Console console; // NOTE: Figure how this is supposed to be interfaced, allocate appropriately
static Rectangle screen_rect;

bool gpu_test() {
    return gpu_init(gpu_device);
}

uint64_t rect_area(const Rectangle *rect) {
    return rect->width * rect->height;
}

VirtioDevice *gpu_get_device() {
    return gpu_device;
}

Console *gpu_get_console() {
    return &console;
}

Pixel *gpu_get_frame_buf() {
    return console.frame_buf;
}

Rectangle *gpu_get_screen_rect() {
    return &screen_rect;
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

// TODO: Implement checking for error responses
bool gpu_init(VirtioDevice *gpu_device) {
    VirtioGpuDispInfoResp disp_info;
    gpu_get_display_info(gpu_device, &disp_info);
    
    // Allocate memory for frame buffer
    console.width = disp_info.displays[0].rect.width;
    console.height = disp_info.displays[0].rect.height;
    
    screen_rect.x = 0;
    screen_rect.y = 0;
    screen_rect.width = console.width;
    screen_rect.height = console.height;

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
        debugf("gpu_init: Attach backing OK (%s)\n", gpu_get_resp_string(resp_hdr.type));
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

    // VirtioGpuTransferToHost2d tx;
    // tx.hdr.type = VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D;
    // tx.rect.x = 0;
    // tx.rect.y = 0;
    // tx.rect.width = console.width;
    // tx.rect.height = console.height;
    // tx.offset = 0;
    // tx.resource_id = 1;
    // tx.padding = 0;
    // resp_hdr.type = 0;

    // gpu_send_command(gpu_device, 0, &tx, sizeof(tx), NULL, 0, &resp_hdr, sizeof(resp_hdr));
    
    // if (resp_hdr.type == VIRTIO_GPU_RESP_OK_NODATA) {
    //     debugf("gpu_init: Transfer OK\n");
    // } else {
    //     // debugf("gpu_init: Transfer failed with %s\n", gpu_get_resp_string(resp_hdr.type));
    //     // return false;
    // }
    gpu_transfer_to_host_2d(&r1, 1, 0);

    // VirtioGpuResourceFlush flush;
    // flush.hdr.type = VIRTIO_GPU_CMD_RESOURCE_FLUSH;
    // flush.rect.x = 0;
    // flush.rect.y = 0;
    // flush.rect.width = console.width;
    // flush.rect.height = console.height;
    // flush.resource_id = 1;
    // flush.padding = 0;
    // resp_hdr.type = 0;
    // gpu_send_command(gpu_device, 0, &flush, sizeof(flush), NULL, 0, &resp_hdr, sizeof(resp_hdr));
    
    // if (resp_hdr.type == VIRTIO_GPU_RESP_OK_NODATA) {
    //     debugf("gpu_init: Flush OK\n");
    // } else {
    //     // debugf("gpu_init: Flush failed with %s\n", gpu_get_resp_string(resp_hdr.type));
    //     // return false;
    // }
    gpu_flush(screen_rect);

    return true;
}

void gpu_handle_job(VirtioDevice *dev, Job *job) {
    if (job == NULL) {
        warnf("gpu_handle_job: job is NULL\n");
        return;
    }
    debugf("Handling GPU device job %u\n", job->job_id);
    job_debug(job);
    if (job->data == NULL) {
        warnf("gpu_handle_job: job->data is NULL\n");
        return;
    }
    // VirtioGpuCtrlType *result = (VirtioGpuCtrlType *)job->data;
    // debugf("Packet status in handle: %x\n", packet->status);

    // infof("GPU job result: %p -> %s\n", gpu_get_resp_string(*result));
    
    // kfree(job->data);
    // job->data = NULL;
}

void gpu_transfer_to_host_2d(const Rectangle *rect, uint32_t resource_id, uint64_t offset) {
    VirtioGpuTransferToHost2d *tx = (VirtioGpuTransferToHost2d*)kzalloc(sizeof(VirtioGpuTransferToHost2d));
    tx->hdr.type = VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D;
    tx->rect.x = rect->x;
    tx->rect.y = rect->y;
    tx->rect.width = rect->width;
    tx->rect.height = rect->height;
    tx->offset = offset;
    tx->resource_id = resource_id;
    tx->padding = 0;
    VirtioGpuCtrlHdr *resp_hdr = (VirtioGpuCtrlHdr*)kzalloc(sizeof(VirtioGpuCtrlHdr));
    resp_hdr->type = 0;

    gpu_send_command(gpu_device, 0, tx, sizeof(*tx), NULL, 0, resp_hdr, sizeof(*resp_hdr));
    // kfree(tx);
    // if (resp_hdr->type == VIRTIO_GPU_RESP_OK_NODATA) {
    //     debugf("gpu_transfer_to_host_2d: Transfer OK\n");
    //     kfree(resp_hdr);
    // } else {
    //     fatalf("gpu_transfer_to_host_2d: Transfer failed with %s\n", gpu_get_resp_string(resp_hdr->type));
    // }
}

void gpu_flush(Rectangle rect) {
    VirtioGpuResourceFlush *flush = (VirtioGpuResourceFlush*)kzalloc(sizeof(VirtioGpuResourceFlush));
    flush->hdr.type = VIRTIO_GPU_CMD_RESOURCE_FLUSH;
    // flush->rect.x = 0;
    // flush->rect.y = 0;
    // flush->rect.width = console.width;
    // flush->rect.height = console.height;
    // flush->rect.x = rect.x;
    // flush->rect.y = rect.y;
    // flush->rect.width = rect.width;
    // flush->rect.height = rect.height;
    flush->rect = rect;
    debugf("gpu_flush: Flushing rect (%d, %d, %d, %d)\n", flush->rect.x, flush->rect.y, flush->rect.width, flush->rect.height);
    flush->resource_id = 1;
    flush->padding = 0;
    VirtioGpuCtrlHdr *resp_hdr = (VirtioGpuCtrlHdr*)kzalloc(sizeof(VirtioGpuCtrlHdr));
    resp_hdr->type = 0;
    gpu_send_command(gpu_device, 0, flush, sizeof(*flush), NULL, 0, resp_hdr, sizeof(*resp_hdr));
    // kfree(flush);
    // if (resp_hdr->type == VIRTIO_GPU_RESP_OK_NODATA) {
    //     debugf("gpu_flush: Flush OK\n");
    // } else {
    //     // debugf("gpu_flush: Flush failed with %s\n", gpu_get_resp_string(resp_hdr.type));
    //     // return false;
    // }
}
static Mutex gpu_device_mutex = MUTEX_UNLOCKED;
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
    // mutex_spinlock(&gpu_device_mutex);
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
    virtio_create_job_with_data(gpu_device, 1, gpu_handle_job, resp1);
    virtio_send_descriptor_chain(gpu_device, which_queue, chain, num_descriptors, true);
    // Wait until device_idx catches up 
    // debugf("GPU WAITING\n");
    // virtio_wait_for_descriptor(gpu_device, which_queue);
    // WFI();
    // while (gpu_device->device_idx != gpu_device->device->idx) {}
    // debugf("gpu_send_command: device_idx caught up\n");
    // mutex_unlock(&gpu_device_mutex);
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
    // uint32_t offset;

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

void gpu_fill_rect(Rectangle rect, Pixel color) {
    fill_rect(console.width, console.height, console.frame_buf, &rect, &color);
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


void gpu_draw_pixel(uint32_t screen_width,
                    uint32_t screen_height,
                    Pixel *frame_buf,
                    uint32_t x,
                    uint32_t y,
                    const Pixel *color)
{
    if (x >= screen_width || y >= screen_height) {
        return;
    }
    frame_buf[y * screen_width + x] = *color;
}

void gpu_draw_circle(uint32_t screen_width,
                     uint32_t screen_height,
                     Pixel *frame_buf,
                     uint32_t x,
                     uint32_t y,
                     uint32_t radius,
                     const Pixel *color)
{
    int32_t x1 = 0;
    int32_t y1 = radius;
    int32_t d = 3 - 2 * radius;

    while (x1 <= y1) {
        gpu_draw_pixel(screen_width, screen_height, frame_buf, x + x1, y + y1, color);
        gpu_draw_pixel(screen_width, screen_height, frame_buf, x + x1, y - y1, color);
        gpu_draw_pixel(screen_width, screen_height, frame_buf, x - x1, y + y1, color);
        gpu_draw_pixel(screen_width, screen_height, frame_buf, x - x1, y - y1, color);
        gpu_draw_pixel(screen_width, screen_height, frame_buf, x + y1, y + x1, color);
        gpu_draw_pixel(screen_width, screen_height, frame_buf, x + y1, y - x1, color);
        gpu_draw_pixel(screen_width, screen_height, frame_buf, x - y1, y + x1, color);
        gpu_draw_pixel(screen_width, screen_height, frame_buf, x - y1, y - x1, color);

        if (d < 0) {
            d = d + 4 * x1 + 6;
        } else {
            d = d + 4 * (x1 - y1) + 10;
            y1--;
        }
        x1++;
    }
}

void gpu_draw_line(uint32_t screen_width,
                   uint32_t screen_height,
                   Pixel *frame_buf,
                   uint32_t x0,
                   uint32_t y0,
                   uint32_t x1,
                   uint32_t y1,
                   const Pixel *color)
{
    int32_t dx = x1 - x0;
    int32_t dy = y1 - y0;

    for (int32_t x = x0; x <= x1; x++) {
        int32_t y = y0 + dy * (x - x0) / dx;
        gpu_draw_pixel(screen_width, screen_height, frame_buf, x, y, color);
    }
}

void gpu_draw_triangle(uint32_t screen_width,
                       uint32_t screen_height,
                       Pixel *frame_buf,
                       uint32_t x0,
                       uint32_t y0,
                       uint32_t x1,
                       uint32_t y1,
                       uint32_t x2,
                       uint32_t y2,
                       const Pixel *color)
{
    gpu_draw_line(screen_width, screen_height, frame_buf, x0, y0, x1, y1, color);
    gpu_draw_line(screen_width, screen_height, frame_buf, x1, y1, x2, y2, color);
    gpu_draw_line(screen_width, screen_height, frame_buf, x2, y2, x0, y0, color);
}

void gpu_draw_rect(uint32_t screen_width,
                   uint32_t screen_height,
                   Pixel *frame_buf,
                   uint32_t x,
                   uint32_t y,
                   uint32_t width,
                   uint32_t height,
                   const Pixel *color)
{
    struct Rectangle r;
    RVALS(&r, x, y, width, height);
    fill_rect(screen_width, screen_height, frame_buf, &r, color);
}