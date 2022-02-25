#include "common.h"
#include "pci.h"
#include "fs/ide.h"
#include "screen.h"
#include "dev/rtl8139.h"
//#include "dev/pcnet.h"
//#include "dev/i825xx.h"

unsigned long int pci_config_read(unsigned short int bus, unsigned short int device, unsigned short int func, unsigned int content);

//todo: make a list or structure of pci devices for each one we find

void pci_init(void) {
  unsigned short int bus, slot, function;
  unsigned int vendor_id;
  int count = 0;
    
  for(bus = 0; bus < PCI_MAX_BUS; bus++) {
    for(slot = 0; slot < PCI_MAX_DEVICES; slot++) {
      unsigned char header_type = pci_config_read(bus, slot, 0, PCI_HEADER_TYPE);
      unsigned char function_count = PCI_MAX_FUNCTIONS;
      // Bit 7 in header type (Bit 23-16) --> multifunctional
      if(!(header_type & 0x80)) {
        function_count = 1; // --> not multifunctional, only function 0 used
      }
      
      for(function = 0; function < function_count ; function++) {
        vendor_id = pci_config_read(bus, slot, function, PCI_VENDOR_ID);
        
        //check to make sure vendor_id != all ones, this means no device present
        if(vendor_id && ((vendor_id & 0xFFFF) != 0xFFFF)) {
          
          struct pci_device device;
          device.bus = bus;
          device.slot = slot;
          device.function = function;
          device.vendor_id = vendor_id;
          device.device_id = pci_config_read(bus, slot, function, PCI_DEVICE_ID);
          device.class_id = pci_config_read(bus, slot, function, PCI_CLASS_ID);
          device.subclass_id = pci_config_read(bus, slot, function, PCI_SUBCLASS_ID);
          device.prog_if = pci_config_read(bus, slot, function, PCI_PROG_IF);
          device.revision_id = pci_config_read(bus, slot, function, PCI_REVISION_ID);
          device.interrupt_line = pci_config_read(bus, slot, function, PCI_INTERRUPT_LINE);
          
          //read the BARs
          for(unsigned char i = 0; i < 6; i++) {
            device.bar[i].base_address = pci_config_read(bus, slot, function, PCI_BAR0 + i*4);
            if(device.bar[i].base_address) {
              device.bar[i].memory_type = device.bar[i].base_address & 0x01;
              if(device.bar[i].memory_type == PCI_MMIO) {
                //Memory Mapped I/O: https://en.wikipedia.org/wiki/Memory-mapped_I/O
                device.bar[i].base_address &= 0xFFFFFFF0;
              } else {
                //PCI Port Mapped I/O: https://en.wikipedia.org/wiki/Memory-mapped_I/O
                device.bar[i].base_address &= 0xFFFC;
              }
            } else {
              device.bar[i].memory_type = PCI_INVALIDBAR;
            }
          }
          
          char temp[33] = {0};
          print_string("Found device - ");
          print_string(itoa(vendor_id & 0xFFFF, temp, 16));
          print_string(":");
          print_string(itoa(device.device_id, temp, 16));
          print_string("\n");
          count++;

          if(device.class_id == 0x01) {
            if (device.subclass_id = 0x01) {
              // https://wiki.osdev.org/PCI_IDE_Controller
              print_string("  FOUND IDE STORAGE CONTROLLER\n");

              // apparently this will only support parallel IDE
              // todo: look into these values more
              ide_initialize(0x1F0, 0x3F6, 0x170, 0x376, 0x000);
            }
          }

          if(device.device_id == 0x8139) {
            install_rtl8139(&device);
          }
          else if(device.device_id == 0x2000) {
            //install_pcnet(&device);
          }
          else if(device.device_id == 0x100e) {
            //install_i825xx(&device);
          } else {
            if(device.device_id != 0xFFFF) {
              char temp[33] = {0};
              print_string("UNKNOWN DEV - ");
              print_string(itoa(vendor_id & 0xFFFF, temp, 16));
              print_string(":");
              print_string(itoa(device.device_id, temp, 16));
              print_string("\n");
            }
          }
        }
        else
          break;
          
        //check for multifunction, if not, don't bother checking rest of functions
        if(!(pci_config_read(bus, slot, 0, PCI_HEADER_TYPE) & 0x80))
        break;
      }
    } /* end scanning slots */
  } /* end scanning busses */
}

/*
 * Source: Pretty OS pci.c (note: may not be free/open src)
 * Source: http://wiki.osdev.org/PCI
 */
unsigned long int pci_config_read(unsigned short int bus, unsigned short int device, unsigned short int func, unsigned int content)
{
  // example: PCI_VENDOR_ID 0x0200 ==> length: 0x02 reg: 0x00 offset: 0x00
  unsigned short int length  = content >> 8;
  unsigned short int reg_off = content & 0x00FF;
  unsigned short int reg     = reg_off & 0xFC;     // bit mask: 11111100b
  unsigned short int offset  = reg_off % 0x04;     // remainder of modulo operation provides offset

  outportl(PCI_CONFIG_ADDRESS, 0x80000000 | (bus << 16) | (device << 11) | (func <<  8) | (reg));

  // use offset to find searched content
  unsigned long int readVal = inportl(PCI_CONFIG_DATA) >> (8 * offset);

  switch (length)
  {
    case 1:
      readVal &= 0x000000FF;
      break;
    case 2:
      readVal &= 0x0000FFFF;
      break;
    case 4:
      readVal &= 0xFFFFFFFF;
      break;
  }
  return readVal;
}

void pci_config_write_word(unsigned short int bus, unsigned short int device, unsigned short int func, unsigned short reg, unsigned int val)
{
  outportl(PCI_CONFIG_ADDRESS, 0x80000000 | (bus << 16) | (device << 11) | (func << 8) | (reg & 0xFC));
  outportw(PCI_CONFIG_DATA, val);
}
