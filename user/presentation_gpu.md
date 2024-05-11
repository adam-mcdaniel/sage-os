# GPU Subsystem
The GPU subsystem consists of:
- Virtio GPU driver
    - GPU device initialzation
    - Set up descriptors for sending GPU commands
    - Send and receive descriptors throught Virtio susbsystem
- System call interface
    - `int screen_get_dims(Rectangle *rect)`
        - Return the dimensions of the current screen through rect
    - `int screen_draw_rect(Pixel *buf, Rectangle *rect, uint64_t x_scale, uint64_t y_scale)`
        - Transfer the user program's pixel buffer (`Pixel *buf`) to the driver physical memory
        - Transfer size determined by `rect->width` and `rect->height`
    - `void screen_flush(Rectangle *rect)`
        - Flush the driver physical memory with size determiine by `rect->width` and `rect->height
