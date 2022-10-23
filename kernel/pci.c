//
// Created by bear on 10/22/2022.
//

#include "pci.h"
#include "pci_regs.h"

#include "debug.h"
#include "slab.h"

#define SHOW_PCI_DEVS
#define SHOW_PCI_ADDRS

// PCI "configuration mechanism one"
static uint32 pci_conf1_addr_ioport = 0x0cf8;
static uint32 pci_conf1_data_ioport = 0x0cfc;

// Forward declarations
static int pci_bridge_attach(struct pci_func *pcif);

// PCI driver table
struct pci_driver
{
	uint32 key1, key2;
	int (*attachfn)(struct pci_func *pcif);
};

// pci_attach_class matches the class and subclass of a PCI device
struct pci_driver pci_attach_class[] = {
	{PCI_CLASS_BRIDGE, PCI_SUBCLASS_BRIDGE_PCI, &pci_bridge_attach},
	{0, 0, 0},
};

// pci_attach_vendor matches the vendor ID and device ID of a PCI device. key1
// and key2 should be the vendor ID and device ID respectively
struct pci_driver pci_attach_vendor[] = {
//	{0x8086, 0x100e, &e1000_attach},
	{0, 0, 0},
};

static void
pci_conf1_set_addr(uint32 bus,
				   uint32 dev,
				   uint32 func,
				   uint32 offset)
{
	KDEBUG_ASSERT(bus < 256);
	KDEBUG_ASSERT(dev < 32);
	KDEBUG_ASSERT(func < 8);
	KDEBUG_ASSERT(offset < 256);
	KDEBUG_ASSERT((offset & 0x3) == 0);

	uint32 v = (1 << 31) |        // config-space
		(bus << 16) | (dev << 11) | (func << 8) | (offset);
	outl(pci_conf1_addr_ioport, v);
}

static uint32
pci_conf_read(struct pci_func *f, uint32 off)
{
	pci_conf1_set_addr(f->bus->busno, f->dev, f->func, off);
	return inl(pci_conf1_data_ioport);
}

static void
pci_conf_write(struct pci_func *f, uint32 off, uint32 v)
{
	pci_conf1_set_addr(f->bus->busno, f->dev, f->func, off);
	outl(pci_conf1_data_ioport, v);
}

static int __attribute__((warn_unused_result))
pci_attach_match(uint32 key1, uint32 key2,
				 struct pci_driver *list, struct pci_func *pcif)
{
	uint32 i;

	for (i = 0; list[i].attachfn; i++)
	{
		if (list[i].key1 == key1 && list[i].key2 == key2)
		{
			int r = list[i].attachfn(pcif);
			if (r > 0)
			{
				return r;
			}
			if (r < 0)
			{
				cprintf("pci_attach_match: attaching "
						"%x.%x (%p): e\n",
						key1, key2, list[i].attachfn, r);
			}
		}
	}
	return 0;
}

static int
pci_attach(struct pci_func *f)
{
	return
		pci_attach_match(PCI_CLASS(f->dev_class),
						 PCI_SUBCLASS(f->dev_class),
						 &pci_attach_class[0], f) ||
			pci_attach_match(PCI_VENDOR(f->dev_id),
							 PCI_PRODUCT(f->dev_id),
							 &pci_attach_vendor[0], f);
}

static const char *pci_classes[] =
	{
		"Unclassified",
		"Mass storage controller",
		"Network controller",
		"Display controller",
		"Multimedia controller",
		"Memory controller",
		"Bridge device",
		"Simple communication controller",
		"Base system peripheral",
		"Input device controller",
		"Docking station",
		"Processor",
		"Serial bus controller",
		"Wireless controller",
		"Intelligent controller",
		"Satellite communication controller",
		"Encryption controller",
		"Signal processing controller",
	};

static void
pci_print_func(struct pci_func *f)
{
	const char *class = pci_classes[0];
	if (PCI_CLASS(f->dev_class) < NELEM(pci_classes))
	{
		class = pci_classes[PCI_CLASS(f->dev_class)];
	}

	cprintf("PCI: %x:%x.%d: %x:%x: class: %x.%x (%s) irq: %d\n",
			f->bus->busno, f->dev, f->func,
			PCI_VENDOR(f->dev_id), PCI_PRODUCT(f->dev_id),
			PCI_CLASS(f->dev_class), PCI_SUBCLASS(f->dev_class), class,
			f->irq_line);
}

static int
pci_scan_bus(struct pci_bus *bus)
{
	int totaldev = 0;
	struct pci_func df;
	memset(&df, 0, sizeof(df));
	df.bus = bus;

	for (df.dev = 0; df.dev < 32; df.dev++)
	{
		uint32 bhlc = pci_conf_read(&df, PCI_BHLC_REG);
		if (PCI_HDRTYPE_TYPE(bhlc) > 1)
		{        // Unsupported or no device
			continue;
		}

		totaldev++;

		struct pci_func f = df;
		for (f.func = 0; f.func < (PCI_HDRTYPE_MULTIFN(bhlc) ? 8 : 1);
			 f.func++)
		{
			struct pci_func af = f;

			af.dev_id = pci_conf_read(&f, PCI_ID_REG);
			if (PCI_VENDOR(af.dev_id) == 0xffff)
			{
				continue;
			}

			uint32 intr = pci_conf_read(&af, PCI_INTERRUPT_REG);
			af.irq_line = PCI_INTERRUPT_LINE(intr);

			af.dev_class = pci_conf_read(&af, PCI_CLASS_REG);

#ifdef SHOW_PCI_DEVS
			pci_print_func(&af);
#endif
			pci_attach(&af);
		}
	}

	return totaldev;
}

static int
pci_bridge_attach(struct pci_func *pcif)
{
	uint32 ioreg = pci_conf_read(pcif, PCI_BRIDGE_STATIO_REG);
	uint32 busreg = pci_conf_read(pcif, PCI_BRIDGE_BUS_REG);

	if (PCI_BRIDGE_IO_32BITS(ioreg))
	{
		cprintf("PCI: %x:%x.%d: 32-bit bridge IO not supported.\n",
				pcif->bus->busno, pcif->dev, pcif->func);
		return 0;
	}

	struct pci_bus nbus;
	memset(&nbus, 0, sizeof(nbus));
	nbus.parent_bridge = pcif;
	nbus.busno = (busreg >> PCI_BRIDGE_BUS_SECONDARY_SHIFT) & 0xff;

#ifdef SHOW_PCI_DEVS
	cprintf("PCI: %x:%x.%d: bridge to PCI bus %d--%d\n",
			pcif->bus->busno, pcif->dev, pcif->func,
			nbus.busno,
			(busreg >> PCI_BRIDGE_BUS_SUBORDINATE_SHIFT) & 0xff);
#endif

	pci_scan_bus(&nbus);
	return 1;
}

// External PCI subsystem interface

void
pci_func_enable(struct pci_func *f)
{
	pci_conf_write(f, PCI_COMMAND_STATUS_REG,
				   PCI_COMMAND_IO_ENABLE |
					   PCI_COMMAND_MEM_ENABLE |
					   PCI_COMMAND_MASTER_ENABLE);

	uint32 bar_width = 4;
	uint32 bar = PCI_MAPREG_START;
	for (bar = PCI_MAPREG_START; bar < PCI_MAPREG_END;
		 bar += bar_width)
	{
		uint32 oldv = pci_conf_read(f, bar);

		bar_width = 4;
		pci_conf_write(f, bar, 0xffffffff);
		uint32 rv = pci_conf_read(f, bar);

		if (rv == 0)
		{
			continue;
		}

		int regnum = PCI_MAPREG_NUM(bar);
		uint32 base, size;
		if (PCI_MAPREG_TYPE(rv) == PCI_MAPREG_TYPE_MEM)
		{
			if (PCI_MAPREG_MEM_TYPE(rv) == PCI_MAPREG_MEM_TYPE_64BIT)
			{
				bar_width = 8;
			}

			size = PCI_MAPREG_MEM_SIZE(rv);
			base = PCI_MAPREG_MEM_ADDR(oldv);
#ifdef SHOW_PCI_ADDRS
			cprintf("  mem region %d: %d bytes at 0x%x\n",
					regnum, size, base);
#endif
		}
		else
		{
			size = PCI_MAPREG_IO_SIZE(rv);
			base = PCI_MAPREG_IO_ADDR(oldv);
#ifdef SHOW_PCI_ADDRS
			cprintf("  io region %d: %d bytes at 0x%x\n",
					regnum, size, base);
#endif
		}

		pci_conf_write(f, bar, oldv);
		f->reg_base[regnum] = base;
		f->reg_size[regnum] = size;

		if (size && !base)
		{
			cprintf("PCI device %x:%x.%d (%x:%x) "
					"may be misconfigured: "
					"region %d: base 0x%x, size %d\n",
					f->bus->busno, f->dev, f->func,
					PCI_VENDOR(f->dev_id), PCI_PRODUCT(f->dev_id),
					regnum, base, size);
		}
	}

	cprintf("PCI function %x:%x.%d (%x:%x) enabled\n",
			f->bus->busno, f->dev, f->func,
			PCI_VENDOR(f->dev_id), PCI_PRODUCT(f->dev_id));
}

void pci_init(void)
{
	static struct pci_bus root_bus;
	memset(&root_bus, 0, sizeof(root_bus));

	int devs = pci_scan_bus(&root_bus);
	cprintf("PCI: Found %d devices\n", devs);
}