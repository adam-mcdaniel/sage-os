/**
 * @file pci.h
 * @brief GPU driver header.
 * @version 0.1
 * @date 2023-10-25
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <stdint.h>
#include <stdbool.h>

#define VIRTIO_GPU_EVENT_DISPLAY (1 << 0)
typedef struct virtio_gpu_config {
    uint32_t events_read;
    uint32_t events_clear;
    uint32_t num_scanouts;
    uint32_t reserved;
};

typedef struct virtio_gpu_ctrl_hdr {
    uint32_t type;
    uint32_t flags;
    uint64_t fence_id;
    uint32_t ctx_id;
    uint32_t padding;
};

typedef struct virtio_gpu_mem_entry {
    uint64_t addr;
    uint32_t length;
    uint32_t padding;
};

enum virtio_gpu_ctrl_type {
    /* 2d commands */
    VIRTIO_GPU_CMD_GET_DISPLAY_INFO = 0x0100,
    VIRTIO_GPU_CMD_RESOURCE_CREATE_2D,
    VIRTIO_GPU_CMD_RESOURCE_UNREF,
    VIRTIO_GPU_CMD_SET_SCANOUT,
    VIRTIO_GPU_CMD_RESOURCE_FLUSH,
    VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D,
    VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING,
    VIRTIO_GPU_CMD_RESOURCE_DETACH_BACKING,
    VIRTIO_GPU_CMD_GET_CAPSET_INFO,
    VIRTIO_GPU_CMD_GET_CAPSET,
    VIRTIO_GPU_CMD_GET_EDID,
    /* cursor commands */
    VIRTIO_GPU_CMD_UPDATE_CURSOR = 0x0300,
    VIRTIO_GPU_CMD_MOVE_CURSOR,
    /* success responses */
    VIRTIO_GPU_RESP_OK_NODATA = 0x1100,
    VIRTIO_GPU_RESP_OK_DISPLAY_INFO,
    VIRTIO_GPU_RESP_OK_CAPSET_INFO,
    VIRTIO_GPU_RESP_OK_CAPSET,
    VIRTIO_GPU_RESP_OK_EDID,
    /* error responses */
    VIRTIO_GPU_RESP_ERR_UNSPEC = 0x1200,
    VIRTIO_GPU_RESP_ERR_OUT_OF_MEMORY,
    VIRTIO_GPU_RESP_ERR_INVALID_SCANOUT_ID,
    VIRTIO_GPU_RESP_ERR_INVALID_RESOURCE_ID,
    VIRTIO_GPU_RESP_ERR_INVALID_CONTEXT_ID,
    VIRTIO_GPU_RESP_ERR_INVALID_PARAMETER,
};

#define VIRTIO_GPU_MAX_SCANOUTS 16
typedef struct DisplayInfoResponse {
   virtio_gpu_ctrl_hdr hdr;  /* VIRTIO_GPU_RESP_OK_DISPLAY_INFO */
   struct GpuDisplay {
       struct Rectangle rect;
       uint32_t enabled;
       uint32_t flags;
   } displays[VIRTIO_GPU_MAX_SCANOUTS];
};

enum GpuFormats {
   B8G8R8A8_UNORM = 1,
   B8G8R8X8_UNORM = 2,
   A8R8G8B8_UNORM = 3,
   X8R8G8B8_UNORM = 4,
   R8G8B8A8_UNORM = 67,
   X8B8G8R8_UNORM = 68,
   A8B8G8R8_UNORM = 121,
   R8G8B8X8_UNORM = 134,
};
struct virtio_gpu_resource_create_2d {
    struct virtio_gpu_ctrl_hdr hdr;
    uint32_t resource_id;
    uint32_t format;
    uint32_t width;
    uint32_t height;
};

struct virtio_gpu_set_scanout {
    struct virtio_gpu_ctrl_hdr hdr;
    struct Rectangle rect;
    uint32_t scanout_id;
    uint32_t resource_id;
};

struct virtio_gpu_resource_attach_backing {
    struct virtio_gpu_ctrl_hdr hdr;
    uint32_t resource_id;
    uint32_t nr_entries;
};

struct virtio_gpu_mem_entry {
    uint64_t addr;
    uint32_t length;
    uint32_t padding;
};

/* drawings */
typedef struct Rectangle {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
};

typedef struct Pixel {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

/* methods */
void fill_rect(uint32_t screen_width,
               uint32_t screen_height,
               struct pixel *buffer,
               const struct rectangle *rect,
               const struct pixel *fill_color);

void stroke_rect(uint32_t screen_width, uint32_t screen_height,
                 struct Pixel *buffer, struct Rectangle *rect,
                 struct Pixel *line_color, uint32_t line_size);