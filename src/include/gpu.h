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
#include <virtio.h>

/* drawings */
typedef struct Rectangle {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
} Rectangle;

typedef struct Pixel {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} Pixel;

typedef struct Console {
    uint32_t width;
    uint32_t height;
    Pixel *frame_buf; // Pointer to heap allocated frame buffer
} Console;

#define VIRTIO_GPU_EVENT_DISPLAY (1 << 0) // Reread display info

typedef struct VirtioGpuConfig {
    uint32_t events_read; // Read-only, number of events driver needs to read
    uint32_t events_clear; // Write-to-clear, mirrors events_read
    uint32_t num_scanouts; // Number of display outputs device supports
    uint32_t reserved;
} VirtioGpuConfig;

typedef struct VirtioGpuCtrlHdr {
    uint32_t type;
    uint32_t flags;
    uint64_t fence_id;
    uint32_t context_id;
    uint32_t padding;
} VirtioGpuCtrlHdr;

typedef enum VirtioGpuCtrlType {
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
} VirtioGpuCtrlType;

#define VIRTIO_GPU_MAX_SCANOUTS 16
typedef struct VirtioGpuDispInfoResp {
   VirtioGpuCtrlHdr hdr;  /* VIRTIO_GPU_RESP_OK_DISPLAY_INFO */
   struct GpuDisplay {
       Rectangle rect;
       uint32_t enabled;
       uint32_t flags;
   } displays[VIRTIO_GPU_MAX_SCANOUTS];
} VirtioGpuDispInfoResp;

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

typedef struct VirtioGpuResCreate2d {
    VirtioGpuCtrlHdr hdr;
    uint32_t resource_id;
    uint32_t format;
    uint32_t width;
    uint32_t height;
} VirtioGpuResCreate2d;

typedef struct VirtioGpuSetScanout {
    VirtioGpuCtrlHdr hdr;
    Rectangle rect;
    uint32_t scanout_id;
    uint32_t resource_id;
} VirtioGpuSetScanout;

// Response type is VIRTIO_GPU_RESP_OK_NODATA
typedef struct VirtioGpuTransferToHost2d {
    VirtioGpuCtrlHdr hdr;
    Rectangle rect;
    uint64_t offset; // The start point of transfer
    uint32_t resource_id;
    uint32_t padding;
} VirtioGpuTransferToHost2d;

typedef struct VirtioGpuResourceFlush {
    VirtioGpuCtrlHdr hdr;
    Rectangle rect;
    uint32_t resource_id;
    uint32_t padding;
} VirtioGpuResourceFlush;

typedef struct VirtioGpuResourceAttachBacking {
    struct VirtioGpuCtrlHdr hdr;
    uint32_t resource_id;
    uint32_t nr_entries;
} VirtioGpuResourceAttachBacking;

typedef struct VirtioGpuMemEntry {
    uint64_t addr;
    uint32_t length;
    uint32_t padding;
} VirtioGpuMemEntry;

/* methods */
bool gpu_test();
void gpu_device_init();
bool gpu_init(VirtioDevice *gpu_device);
void gpu_send_command(VirtioDevice *gpu_device,
                      uint16_t which_queue,
                      void *cmd,
                      size_t cmd_size,
                      void *resp0,
                      size_t resp0_size,
                      void *resp1,
                      size_t resp1_size);
bool gpu_get_display_info(VirtioDevice *gpu_device, VirtioGpuDispInfoResp *disp_resp);
void fill_rect(uint32_t screen_width,
               uint32_t screen_height,
               Pixel *frame_buf,
               const Rectangle *rect,
               const Pixel *fill_color);
static inline void RVALS(Rectangle *r,
                         uint32_t x,
                         uint32_t y,
                         uint32_t width,
                         uint32_t height);
void stroke_rect(uint32_t screen_width,
                 uint32_t screen_height,
                 Pixel *frame_buf,
                 const Rectangle *rect,
                 const Pixel *line_color,
                 uint32_t line_size);
static char *gpu_get_resp_string(VirtioGpuCtrlType type);