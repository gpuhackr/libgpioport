/* 
 * PCI access convenience functions - header file
 *
 * (C) 2014
 *
 */

#ifndef PCIUTIL_H
#define PCIUTIL_H

#include <stdint.h>

/* Device list entry */
struct pciutil_device {
    uint16_t vendor_id;
    uint16_t device_id;
};

/* Given a list of PCI devices, mmap() the MMIO register aperture and write
 * it to 'mem'.
 */
int pciutil_dev_map(struct pciutil_device *devices, int num_devs, int index,
        void **mem);

/* Unmap the device */
int pciutil_dev_unmap(void *mem);

#endif /* PCIUTIL_H */
