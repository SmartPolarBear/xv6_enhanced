#pragma once

#include "types.h"
#include "x86.h"

#define PCI_CONFIG_ADDR             0xCF8
#define PCI_CONFIG_DATA             0xCFC

// Forward declaration
struct pci_device;

/*
 * The PCI specification specifies 8 bits for the bus identifier, 5 bits for
 * the device and 3 bits for selecting a particular function. This is the BDF
 * (Bus Device Function) address of a PCI device
 */
#define PCI_MAX_DEVICES 32

typedef struct pci_common_header
{
	uint16 vendor_id;
	uint16 device_id;
	uint16 command;
	uint16 status;
	uint8 revision_id;
	uint8 prog_if;
	uint8 subclass;
	uint8 class_code;
	uint8 cache_line_size;
	uint8 latency_timer;
	uint8 header_type;
	uint8 bist;
} pci_common_header_t;

typedef struct pci_bus
{
	struct pci_device *bridge;
	uint32 bus_number;
} pci_bus_t;

typedef struct pci_device
{
	pci_common_header_t header;

	pci_bus_t *bus;
	uint32 device_number;
	uint32 function_number;

	uint32 reg_base[6];
	uint32 reg_size[6];

	uint8 cap[6]; // Maps cap type to offset within the pci config space.
	uint8 cap_bar[6]; // Maps cap type to their BAR number
	uint32 cap_off[6]; // Map cap type to offset within bar

	uintptr_t membase;
	uintptr_t iobase;
} pci_device_t;

/*
 * Macro to create the 32 bit register values for a PCI transaction. `off` here
 * is the register value in the PCI config space of the device specified by
 * `dev` on the bus specificed by `bus` and the function specified by `fn`.
 *
 * The MSB of the 32 bit value needs to be 1 in order to mark it as
 * a configuration transaction
 */
#define PCI_FORMAT(bus, dev, fn, off) ({0x80000000 | bus << 16 | dev << 11 | fn << 8 | off;})

#define PCI_CONF_READ8(bus, dev, func, reg) \
    (outl(PCI_CONFIG_ADDR, PCI_FORMAT(bus, dev, func, reg)), \
    inb(PCI_CONFIG_DATA+((reg)&3)))
#define PCI_CONF_READ16(bus, dev, func, reg) \
    (outl(PCI_CONFIG_ADDR, PCI_FORMAT(bus, dev, func, reg)), \
    inw(PCI_CONFIG_DATA+((reg)&2)))
#define PCI_CONF_READ32(bus, dev, func, reg) \
    (outl(PCI_CONFIG_ADDR, PCI_FORMAT(bus, dev, func, reg)), \
    inl(PCI_CONFIG_DATA))

#define PCI_CONF_WRITE8(bus, dev, func, reg, val) \
    (outl(PCI_CONFIG_ADDR, PCI_FORMAT(bus, dev, func, reg)), \
    outb(PCI_CONFIG_DATA+((reg)&3), (val)))
#define PCI_CONF_WRITE16(bus, dev, func, reg, val) \
    (outl(PCI_CONFIG_ADDR, PCI_FORMAT(bus, dev, func, reg)), \
    outw(PCI_CONFIG_DATA+((reg)&2), (val)))
#define PCI_CONF_WRITE32(bus, dev, func, reg, val) \
    (outl(PCI_CONFIG_ADDR, PCI_FORMAT(bus, dev, func, reg)), \
    outl(PCI_CONFIG_DATA, (val)))

/*
 * Class codes of PCI devices at their offsets
 */
extern char *PCI_CLASSES[18];

/*
 * Reads the register at offset `off` in the PCI config space of the device
 * `dev`
 */
static inline uint32 pci_conf_read32(struct pci_device *dev, uint32 off)
{
	return PCI_CONF_READ32(dev->bus->bus_number, dev->device_number, dev->function_number, off);
}

static inline uint32 pci_conf_read16(struct pci_device *dev, uint32 off)
{
	return PCI_CONF_READ16(dev->bus->bus_number, dev->device_number, dev->function_number, off);
}

static inline uint32 pci_conf_read8(struct pci_device *dev, uint32 off)
{
	return PCI_CONF_READ8(dev->bus->bus_number, dev->device_number, dev->function_number, off);
}

/*
 * Writes the provided value `value` at the register `off` for the device `dev`
 */
static inline void pci_conf_write32(struct pci_device *dev, uint32 off, uint32 value)
{
	return PCI_CONF_WRITE32(dev->bus->bus_number, dev->device_number, dev->function_number, off, value);
}

/*
 * Writes the provided value `value` at the register `off` for the device `dev`
 */
static inline void pci_conf_write16(struct pci_device *dev, uint32 off, uint16 value)
{
	return PCI_CONF_WRITE16(dev->bus->bus_number, dev->device_number, dev->function_number, off, value);
}

/*
 * Writes the provided value `value` at the register `off` for the device `dev`
 */
static inline void pci_conf_write8(struct pci_device *dev, uint32 off, uint8 value)
{
	return PCI_CONF_WRITE8(dev->bus->bus_number, dev->device_number, dev->function_number, off, value);
}
