/*
 * @brief LWIP FreeRTOS TCP Echo example
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2013
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include <string.h>

#if LWIP
/* LWIP Includes */
#include "lwip/init.h"
#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/memp.h"
#include "lwip/tcpip.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/timers.h"
#include "netif/etharp.h"

#if LWIP_DHCP
#include "lwip/dhcp.h"
#endif
#endif

#include "board.h"
#if LWIP
#include "arch/lpc18xx_43xx_emac.h"
#include "arch/lpc_arch.h"
#include "arch/sys_arch.h"
#include "lpc_phy.h" /* For the PHY monitor support */
#endif

#include "otp_18xx_43xx.h" /* For RNG */

#include "FreeRTOSCommonHooks.h"

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
#include <wolfcrypt/test/test.h>

//#include <wolfcrypt/benchmark/benchmark.h>
extern int benchmark_test(void *args);


/* When building the example to run in FLASH, the number of available
   pbufs and memory size (in lwipopts.h) and the number of descriptors
   (in lpc_18xx43xx_emac_config.h) can be increased due to more available
   IRAM. */

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

#if LWIP
/* NETIF data */
static struct netif lpc_netif;
#endif

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* Sets up system hardware */
static void prvSetupHardware(void)
{
	/* LED0 is used for the link status, on = PHY cable detected */
	SystemCoreClockUpdate();
	Board_Init();

	/* Initial LED state is off to show an unconnected cable state */
	Board_LED_Set(0, false);
}

#if LWIP
/* Callback for TCPIP thread to indicate TCPIP init is done */
static void tcpip_init_done_signal(void *arg)
{
	/* Tell main thread TCP/IP init is done */
	*(s32_t *) arg = 1;
}

/* LWIP kickoff and PHY link monitor thread */
static void vSetupIFTask(void *pvParameters) {
	ip_addr_t ipaddr, netmask, gw;
	volatile s32_t tcpipdone = 0;
	uint32_t physts;
	static int prt_ip = 0;

	/* Wait until the TCP/IP thread is finished before
	   continuing or wierd things may happen */
	LWIP_DEBUGF(LWIP_DBG_ON, ("Waiting for TCPIP thread to initialize...\n"));
	tcpip_init(tcpip_init_done_signal, (void *) &tcpipdone);
	while (!tcpipdone) {
		msDelay(1);
	}

	LWIP_DEBUGF(LWIP_DBG_ON, ("Starting LWIP TCP echo server...\n"));

	/* Static IP assignment */
#if LWIP_DHCP
	IP4_ADDR(&gw, 0, 0, 0, 0);
	IP4_ADDR(&ipaddr, 0, 0, 0, 0);
	IP4_ADDR(&netmask, 0, 0, 0, 0);
#else
	IP4_ADDR(&gw, 10, 1, 10, 1);
	IP4_ADDR(&ipaddr, 10, 1, 10, 234);
	IP4_ADDR(&netmask, 255, 255, 255, 0);
#endif

	/* Add netif interface for lpc17xx_8x */
	if (!netif_add(&lpc_netif, &ipaddr, &netmask, &gw, NULL, lpc_enetif_init,
				   tcpip_input)) {
		LWIP_ASSERT("Net interface failed to initialize\r\n", 0);
	}
	netif_set_default(&lpc_netif);
	netif_set_up(&lpc_netif);

	/* Enable MAC interrupts only after LWIP is ready */
	NVIC_SetPriority(ETHERNET_IRQn, config_ETHERNET_INTERRUPT_PRIORITY);
	NVIC_EnableIRQ(ETHERNET_IRQn);

#if LWIP_DHCP
	dhcp_start(&lpc_netif);
#endif

	/* Initialize and start application */
	tcpecho_init();

	/* This loop monitors the PHY link and will handle cable events
	   via the PHY driver. */
	while (1) {
		/* Call the PHY status update state machine once in a while
		   to keep the link status up-to-date */
		physts = lpcPHYStsPoll();

		/* Only check for connection state when the PHY status has changed */
		if (physts & PHY_LINK_CHANGED) {
			if (physts & PHY_LINK_CONNECTED) {
				Board_LED_Set(0, true);
				prt_ip = 0;

				/* Set interface speed and duplex */
				if (physts & PHY_LINK_SPEED100) {
					Chip_ENET_SetSpeed(LPC_ETHERNET, 1);
					NETIF_INIT_SNMP(&lpc_netif, snmp_ifType_ethernet_csmacd, 100000000);
				}
				else {
					Chip_ENET_SetSpeed(LPC_ETHERNET, 0);
					NETIF_INIT_SNMP(&lpc_netif, snmp_ifType_ethernet_csmacd, 10000000);
				}
				if (physts & PHY_LINK_FULLDUPLX) {
					Chip_ENET_SetDuplex(LPC_ETHERNET, true);
				}
				else {
					Chip_ENET_SetDuplex(LPC_ETHERNET, false);
				}

				tcpip_callback_with_block((tcpip_callback_fn) netif_set_link_up,
										  (void *) &lpc_netif, 1);
			}
			else {
				Board_LED_Set(0, false);
				tcpip_callback_with_block((tcpip_callback_fn) netif_set_link_down,
										  (void *) &lpc_netif, 1);
			}

			DEBUGOUT("Link connect status: %d\r\n", ((physts & PHY_LINK_CONNECTED) != 0));
		}

		/* Print IP address info */
		if (!prt_ip) {
			if (lpc_netif.ip_addr.addr) {
				char tmp_buff[16];
				DEBUGOUT("IP_ADDR    : %s\r\n", ipaddr_ntoa_r((const ip_addr_t *) &lpc_netif.ip_addr, tmp_buff, 16));
				DEBUGOUT("NET_MASK   : %s\r\n", ipaddr_ntoa_r((const ip_addr_t *) &lpc_netif.netmask, tmp_buff, 16));
				DEBUGOUT("GATEWAY_IP : %s\r\n", ipaddr_ntoa_r((const ip_addr_t *) &lpc_netif.gw, tmp_buff, 16));
				prt_ip = 1;
			}
		}

		/* Delay for link detection (250mS) */
		vTaskDelay(pdMS_TO_TICKS(250));
	}
}
#endif


typedef struct func_args {
    int    argc;
    char** argv;
    int    return_code;
} func_args;

static void vWolfTestTask(void *pvParameters)
{
	func_args args;

	memset(&args, 0, sizeof(args));
	printf("\nCrypt Test\n");
	wolfcrypt_test(&args);
	printf("Crypt Test: Return code %d\n", args.return_code);

	vPrintRtosStats();

	memset(&args, 0, sizeof(args));
	printf("\nBenchmark Test\n");
	benchmark_test(&args);
	printf("Benchmark Test: Return code %d\n", args.return_code);

	vPrintRtosStats();

#if LWIP
	/* Add another thread for initializing physical interface. This
	   is delayed from the main LWIP initialization. */
	xTaskCreate(vSetupIFTask, "SetupIFx",
				configMINIMAL_STACK_SIZE * 2, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);
#endif

	vTaskDelete( NULL );
}


/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	MilliSecond delay function based on FreeRTOS
 * @param	ms	: Number of milliSeconds to delay
 * @return	Nothing
 * Needed for some functions, do not use prior to FreeRTOS running
 */
void msDelay(uint32_t ms)
{
	vTaskDelay(pdMS_TO_TICKS(ms));
}


#define REDLIB_HEAP	0x200
static void prvInitHeapMemory(void)
{
   extern char _ebss[];
   extern char __top_RAM[];
   extern char __base_RAM2[];
   extern char __top_RAM2[];

   /* Allocate two blocks of RAM for use by the heap. */
   const char* base_RAM = _ebss + REDLIB_HEAP;
   const HeapRegion_t xHeapRegions[] =
   {
      { ( uint8_t * ) base_RAM, __top_RAM - base_RAM },        // Remainder of first 32KB
      { ( uint8_t * ) __base_RAM2, __top_RAM2 - __base_RAM2},  // 40KB
      { NULL, 0 } /* Terminates the array. */
   };
   vPortDefineHeapRegions(xHeapRegions);
}

/* Memory location of the generated random numbers (for total of 128 bits) */
static volatile uint32_t* mRandData = (uint32_t*)0x40045050;
static uint32_t mRandIndex = 0;
static uint32_t mRandCount = 0;
uint32_t rand_gen(void)
{
	uint32_t rand = 0;
	uint32_t status = LPC_OK;
	if(mRandIndex == 0) {
		status = Chip_OTP_GenRand();
	}
	if(status == LPC_OK) {
		rand = mRandData[mRandIndex];
	}
	else {
		DEBUGOUT("GenRand Failed 0x%x\n", status);
	}
	if(++mRandIndex > 4) {
		mRandIndex = 0;
	}
	mRandCount++;
	return rand;
}

/**
 * @brief	main routine for example_lwip_tcpecho_freertos_18xx43xx
 * @return	Function should not exit
 */
int main(void)
{
	volatile uint32_t status;

	prvSetupHardware();

	prvInitHeapMemory();

	/* Initialize the OTP Controller */
	status = Chip_OTP_Init();
	if(status != LPC_OK) {
		DEBUGSTR("OTP Init Failed!\n");
	}

	/* Do the Wolf Test */
	xTaskCreate(vWolfTestTask, "WolfTest",
					configMINIMAL_STACK_SIZE * 42, NULL, (tskIDLE_PRIORITY + 1UL),
					(xTaskHandle *) NULL);

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Should never arrive here */
	return 1;
}
