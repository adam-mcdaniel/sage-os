struct virtio_input_event;

unsigned int get_events(struct virtio_input_event *event_buffer, unsigned int max_events) {
    unsigned int ret;
    __asm__ volatile("mv a7, %1\nmv a0, %2\nmv a1, %3\necall\nmv %0, a0" : "=r"(ret) : "r"(5), "r"(event_buffer), "r"(max_events) : "a0", "a1", "a7");
    return ret;
}
