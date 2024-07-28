#ifndef PCI_HEADER
#define PCI_HEADER

void pci_init(void);
unsigned long int pci_config_read(unsigned short int bus, unsigned short int device, unsigned short int func, unsigned int content);
void pci_config_write_word(unsigned short int bus, unsigned short int device, unsigned short int func, unsigned short reg, unsigned int val);

void lspci(void);

//determines how many pci devices to check for
#define PCI_MAX_BUS           255
#define PCI_MAX_DEVICES       32
#define PCI_MAX_FUNCTIONS     8

//base addresses used to read and write to pci bus
#define PCI_CONFIG_DATA       0xCFC
#define PCI_CONFIG_ADDRESS    0xCF8

//important offsets for reading and writing pci configurations
#define PCI_VENDOR_ID           0x00
#define PCI_DEVICE_ID           0x02
#define PCI_COMMAND             0x04
#define PCI_STATUS              0x06
#define PCI_REVISION_ID         0x08
#define PCI_PROG_IF             0x09
#define PCI_SUBCLASS_ID         0xA
#define PCI_CLASS_ID            0xB
#define PCI_CACHE_LINE_SIZE     0xC
#define PCI_LATENCY_TIMER       0xD
#define PCI_HEADER_TYPE         0xE
#define PCI_BIST                0xF
#define PCI_BAR0                0x10
#define PCI_BAR1                0x14
#define PCI_BAR2                0x18
#define PCI_BAR3                0x1C
#define PCI_BAR4                0x20
#define PCI_BAR5                0x24
#define PCI_CARDBUS             0x28
#define PCI_SUBSYSTEM_VENDOR_ID 0x2C
#define PCI_SUBSYSTEM_ID        0x2E
#define PCI_EPROM_BASE          0x30
#define PCI_CAPABILITIES        0x34
#define PCI_INTERRUPT_LINE      0x3C
#define PCI_INTERRUPT_PIN       0x3D
#define PCI_MIN_GRANT           0x3E
#define PCI_MAX_LATENCY         0x3F

#define PCI_CMD_BUSMASTER       0x4

enum
{
    PCI_MMIO, PCI_IO, PCI_INVALIDBAR
};

struct pci_bar {
  unsigned long base_address;
  unsigned int  memory_size;
  unsigned char memory_type;
};

struct pci_device
{
  unsigned short bus, slot, function;
  unsigned int device_id, vendor_id, status, command;
  unsigned char class_id, subclass_id, prog_if, revision_id, bist, header_type, latency_timer, cache_line_size;
  unsigned long cardbus;
  unsigned int subsystem_id, subsystem_vendor_id;
  unsigned long exp_rom_bar;
  unsigned char capabilities_pointer;
  unsigned char max_latency, min_grant, interrupt_line;
  struct pci_bar bar[6];
};

#endif
