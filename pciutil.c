/* 
 * PCI bus access convenience functions - work in progress
 *
 * (C) 2014 gpuhackr@gmail.com
 *
 */

#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>
#include <pciaccess.h>
#include <pci/header.h>
#include <err.h> // FIXME 

#include "pciutil.h"

bool pciutil_inited = false;

static void pciutil_init_check()
{
    int ret;

    if(!pciutil_inited) {
        int ret = pci_system_init();
        if(ret != 0) {
            // FIXME
            errx(1, "Unable to inialise libpciaccess (ret=%d)", ret);
        }
        pciutil_inited = true;
    }
}

static void pciutil_fini()
{
    if(pciutil_inited)
        pci_system_cleanup();
}

struct pci_mem_region * pciutil_find_regs(struct pci_device *dev)
{
    struct pci_mem_region *regs_region = NULL;

    for(int i = 0; i < sizeof(dev->regions) / sizeof(*dev->regions); 
            i++) {
        struct pci_mem_region *region = &dev->regions[i];

#if 1
        printf("BAR %d addr=0x%" PRIx64 "\n", i, (uint64_t) region->base_addr);
#endif

        if(region->is_IO || region->is_prefetchable || region->size == 0)
            continue;

        if(region->size > 256 * 1024)
            continue;

        regs_region = region;
        break;
    }

    return regs_region;
}

// TODO split into pciutil_dev_find_XXX() and pci_dev_map() where XXX may be:
//
//          list: find by list
//  vendor_class: find by vendor and device class
//
int pciutil_dev_map(struct pciutil_device *devices, int num_devs, int index, 
        void **mem)
{
    if(mem == NULL) {
        // FIXME
        fprintf(stderr, "mem is null!");
        return -1;
    }

    int ret;

    pciutil_init_check();

    struct pci_device_iterator *it = pci_id_match_iterator_create(NULL);
    struct pci_device *dev, *dev_found = NULL;

    int dev_index = 0;

    while((dev = pci_device_next(it)) != NULL) {
        bool found = false;
        for(int i = 0; i < num_devs; i++) {
            if((dev->vendor_id == devices[i].vendor_id) && 
               (dev->device_id == devices[i].device_id)) {
                found = true;
                break;
            }
        }

        if(found) {
            if(index == dev_index) {
                dev_found = dev;
                break;
            }

            dev_index++;
        }
   }

    if(!dev_found) {
        // FIXME
        fprintf(stderr, "device not found in system\n");
        return -1;
    }

    ret = pci_device_probe(dev_found);
    if(ret != 0) {
        // FIXME
        fprintf(stderr, "failed to probe device\n");
        return -1;
    }

    struct pci_mem_region *regs_region;
    regs_region = pciutil_find_regs(dev);

    if(regs_region == NULL) {
        // FIXME
        fprintf(stderr, "unable to find register BAR\n");
        return -1;
    }

    void **dev_mem;

    ret = pci_device_map_range(dev, regs_region->base_addr, regs_region->size, 
            PCI_DEV_MAP_FLAG_WRITABLE /* | PCI_DEV_MAP_FLAG_WRITE_COMBINE */,
            &dev_mem);

    if(ret != 0) {
        // FIXME
        fprintf(stderr, "unable to map device memory, are you root?\n");
        return -1;
    }

    // update 'mem' with the mmap()'d PCI register aperture
    *mem = dev_mem;

    return 0;
}

int pciutil_dev_unmap(void *mem)
{
    return 0;
}
