## WolfSSL example projects:

1. `wolf_demo_aes` based on the `periph_aes` example. It has console options to run the Wolf tests and benchmarks ('t' for the WolfSSL Tests and 'b' for the WolfSSL Benchmarks).

2. `wolf_demo_lwip` which includes the LWIP network stack and Ethernet support (originally based on the `lwip_freertos_tcpecho` example). 

## Static libraries projects:

1. `lib_wolfssl` for WolfSSL. The WolfSSL port for the LPC18XX platform is located in `workspace/lib_wolfssl/lpc_18xx_port.c`. This has port functions for `current_time` and `rand_gen`. The `WOLF_USER_SETTINGS` define is set which allows all WolfSSL settings to exist in the `user_settings.h` file (see this for all customizations set).

2. `lib_freertos` for FreeRTOS. Due to the fragmented RAM areas on the LPC18S37 the FreeRTOS memeory management scheme heap5 was used, which allows multiple RAM areas to be defined for the heap. The RedLib heap is still used for printf support, so the first 0x200 is reserved for RedLib heap. See `#define REDLIB_HEAP` in heap5.c. The heap is using the remainder of RAM1 (~30KB) and all of RAM2 (40KB).

3. `lib_lwip` for LWIP. This contains the Ethernet driver and LWIP network stack.