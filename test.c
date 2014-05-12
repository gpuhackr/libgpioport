#include <stdio.h>
#include <inttypes.h>
#include <pciaccess.h>
#include <pci/header.h> /* FIXME for PCI dev classes */
#include <err.h>

struct pci_mem_region * find_regs_region(struct pci_device *dev)
{
    struct pci_mem_region *regs_region = NULL;

    for(int i = 0; i < sizeof(dev->regions) / sizeof(*dev->regions); 
            i++) {
        struct pci_mem_region *region = &dev->regions[i];

#if 0
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

void pci_test()
{
    int ret;

    ret = pci_system_init();
    if(ret != 0)
        err(1, "Unable to initialise libpciaccess");

    /* Match any AMD/ATI PCI device */
    struct pci_id_match match = {
        .vendor_id = 0x1002,
        .device_id = PCI_MATCH_ANY,
        .subvendor_id = PCI_MATCH_ANY,
        .subdevice_id = PCI_MATCH_ANY,
    };

    struct pci_device_iterator *it = pci_id_match_iterator_create(&match);
    struct pci_device *dev;

    while((dev = pci_device_next(it)) != NULL) {
        /* match video device (func=0), AMD audio devices are func 1*/
        if(((dev->device_class >> 8) == PCI_CLASS_DISPLAY_VGA) && 
            (dev->func == 0)) { 
            printf("%04x:%04x ", dev->vendor_id, dev->device_id);

            ret = pci_device_probe(dev);
            if(ret != 0)
                warnx(1, "unable to probe PCI device");

            const char *vendor_name, *device_name;
            vendor_name = pci_device_get_vendor_name(dev);
            if(vendor_name == NULL)
                vendor_name = "[Unknown Vendor]";

            device_name = pci_device_get_device_name(dev);
            if(device_name == NULL)
                device_name = "[Unknown Device]";

            printf("%s %s\n", vendor_name, device_name);
            printf("rev=%d IRQ=%d ROM size=%d\n", dev->revision,
                    dev->irq, dev->rom_size);

            struct pci_mem_region *regs_region;
            regs_region = find_regs_region(dev);

            if(regs_region == NULL) {
                warnx("unable to find register BAR");
                continue;
            }

            volatile uint32_t *regs;

            printf("BAR bus_addr=0x%llx cpu_addr=0x%llx size=0x%llx\n",
                    (unsigned long long) regs_region->bus_addr,
                    (unsigned long long) regs_region->base_addr,
                    (unsigned long long) regs_region->size);

            ret = pci_device_map_range(dev, regs_region->base_addr, 
                    regs_region->size, 0, &regs);

            if(ret != 0)
                err(1, "unable to map device memory range");

            uint32_t mmio_device_id = regs[0x4000 / 4];
            uint32_t mmio_device_id_expected = (dev->vendor_id & 0xffff) |
                ((dev->device_id & 0xffff) << 16);

            printf("PCI device ID via MMIO register read = 0x%" PRIx32 "\n",
                    mmio_device_id);

            if(mmio_device_id != mmio_device_id_expected) {
                printf("expected device ID 0x%" PRIx32 " but got 0x%" PRIx32 
                        " instead\n", mmio_device_id_expected, mmio_device_id);
            }

            ret = pci_device_unmap_range(dev, regs, regs_region->size);
            if(ret != 0)
                err(1, "unable to unmap device memory");
        }
    }

    pci_system_cleanup();
}

int main(int argc, char *argv[])
{
    pci_test();
    return 0;
}
