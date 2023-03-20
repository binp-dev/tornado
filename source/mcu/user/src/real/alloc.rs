use freertos::FreeRtosAllocator;

#[global_allocator]
static GLOBAL: FreeRtosAllocator = FreeRtosAllocator;
