/* 
 * Bt8xx GPIO driver testing - work in progress
 *
 * (C) 2014 gpuhackr@gmail.com
 *
 */

#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>
#include <err.h>

#include "pciutil.h"

// Supported Bt8xx devices
struct pciutil_device bt8xx_pci_devices[] = {
    { 0x109e, 0x0350}, // Bt848
    { 0x109e, 0x0351}, // Bt849A
    { 0x109e, 0x036e}, // Bt878 
    { 0x109e, 0x036f}  // Bt879
};

// Bt8xx GPIO registers
#define BT8XX_SRESET          0x07c
#define BT8XX_GPIO_DMA_CTL    0x10c
#define BT8XX_GPIO_OUT_EN     0x118
#define BT8XX_GPIO_REG_INP    0x11c
#define BT8XX_GPIO_DATA       0x200 /* 0x200 - 0x2ff (64 DWORD) */

volatile uint32_t *regs_mem;

void pci_test2()
{
    int ret;

    ret = pciutil_dev_map(bt8xx_pci_devices, sizeof(bt8xx_pci_devices) /
            sizeof(*bt8xx_pci_devices), 0, (void **) &regs_mem);
    if(ret != 0) {
        errx(1, "pciutil_dev_map() failed (ret=%d)", ret);
    }

    regs_mem[BT8XX_SRESET / 4] = 0;
    regs_mem[BT8XX_GPIO_DMA_CTL / 4] = 0; // Normal GPIO port mode
    regs_mem[BT8XX_GPIO_OUT_EN / 4] = 0xff;

    /* The maximum toggle frequency is limited by the underying PCI bus.
     * Since caches are disabled, every access to the registers even in 
     * sequential order results in a new PCI cycle being generated for
     * each access.
     *
     * Hack - if the Bt8xx registers MTRR is set as write combining the 
     * maximum toggle speed increases to 16.666MHz versus 3.333MHz when 
     * uncached.
     */
#if 1
    uint32_t value = 0;

    for(;;) {
        for(int i = 0; i < 64; i++) {
            regs_mem[BT8XX_GPIO_DATA / 4 + i] = value;
            value ^= 1;
        }
    }

#else
    for(;;) {
        regs_mem[GPIO_DATA / 4] = 0x01;
        regs_mem[GPIO_DATA / 4] = 0x00;
    }
#endif
}

int main(int argc, char *argv[])
{
    pci_test2();
    return 0;
}

