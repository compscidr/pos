#include "common.h"
#include "pci.h"
#include "screen.h"
#include "idt.h"
#include "dev/rtl8139.h"
#include "mm.h"
#include "net/eth.h"
#include "net/dhcp.h"
#include "task.h"

/*
 * Double-check this init procedure, this code seems to freeze after a few mins of running
 */

unsigned long int ioaddr;
unsigned short int tx_current_buffer;
unsigned short int rx_index;
char rx_buffer[RX_BUFFER_SIZE];  //for some reason if the rx buffer is dyanmically allocated we go full reboot :/ prob an alignment problem
char * tx_buffers;

/*
 * Registers the IRQ handler for the device, inits the device
 * 
 * Going for a Port-Mapped I/O driver here - since I failed at the
 * MMIO previously (although I think it was only because I didn't put
 * the device into bus master mode for DMA to work. Todo: build a MMIO
 * driver again and give the user the option at which one to load at
 * runtime?
 */
void install_rtl8139(struct pci_device * device) {
  char temp[35] = { 0 };
  unsigned char c;

  print_string("RTL8139 Network Device Found\n");
  
  unsigned short pci_command_register = pci_config_read(device->bus, device->slot, device->function, PCI_COMMAND);
  pci_config_write_word(device->bus, device->slot, device->function, PCI_COMMAND, pci_command_register | PCI_CMD_BUSMASTER);
  
  for (c = 0; c < 6; c++) {
    if(device->bar[c].memory_type == PCI_MMIO) {
      print_string("Memory Mapped IO (MMIO) BAR: ");
      print_address(device->bar[c].base_address);
      print_string("\n");
      //note we don't set the ioaddr here because we aren't going
      //to use mmio for this device.
    } else if(device->bar[c].memory_type == PCI_IO) {
      print_string("Port Mapped IO (PMIO) BAR: ");
      ioaddr = device->bar[c].base_address;
      print_address(device->bar[c].base_address);
      print_string("\n");
      break;
    } else {
      print_string("Invalid BAR\n");
    }
  }

  print_string("  At memory address: ");
  print_address(ioaddr);
  print_string("\n");

  irq_install_handler(device->interrupt_line, rtl8139_handler);

  print_string("  Installing on IRQ: ");
  print_string(itoa(device->interrupt_line, temp, 10));
  print_string("\n");
  
  print_string("  MAC Address of RTL8139:  ");
  unsigned char val;
  for (int i = 0; i < 6; i++) {
    val = inportb(ioaddr + i);
    print_string(itoa(val, temp, 16));
    if (i != 5)
      print_string(":");
  }
  print_string("\n");

  //wake-up / power on
  outportb(ioaddr + ChipConfig1, 0x00);
  
  //reset the card
  print_string("  Resetting RTL8139...");
  outportb(ioaddr + ChipCmd, CmdReset);
  
  //allocate the transmit buffers
  tx_buffers = calloc(sizeof(char) * TX_BUF_SIZE * NUM_TX_DESC);
  tx_current_buffer = 0;
  
  //allocate the receive buffer
  //rx_buffer = calloc(sizeof(char) * RX_BUFFER_SIZE);
  rx_index = 0;
  outportl(ioaddr + ChipRxBuffer, (unsigned long)&rx_buffer);
  
  //enable transmit and receive
  outportb(ioaddr + ChipCmd, CmdRxEnb | CmdTxEnb);
  
  //configure how packets are received (accept all)
  //see configuring the receive buffer: https://wiki.osdev.org/RTL8139#Init_Receive_buffer
  //we are leaving the ring bit unset which means we are using a ring buffer which wraps around
  outportl(ioaddr + ChipRxConfig, 0xF);
  
  //enable all interrupts
  outportw(ioaddr + ChipIMR, 0xFFFF);
  
  print_status(1);
}

void rtl8139_send_handler() {}

void rtl8139_recv_handler() {
  unsigned long length = (rx_buffer[3 + rx_index] << 8) + rx_buffer[2+rx_index];
  
  //todo: need to free these packets at some point or we'll run out of
  //memory just from packets being received.
  char * packet = calloc(sizeof(unsigned char) * length);
  unsigned long ring_offset = rx_index % RX_BUFFER_SIZE;
  
  if(ring_offset + length > RX_BUFFER_SIZE) {
    //case where we've reached the end of the ring and have to perform
    //two memcopies (one at the end of the ring, one at the start)
    unsigned long semi_count = RX_BUFFER_SIZE - ring_offset - 4;
    memcpy(packet, &rx_buffer[ring_offset + 4], semi_count);
    memcpy(packet + semi_count, rx_buffer, length - semi_count);
  } else {
    //normal case where we can make a single copy from the ring buffer 
    //to our new packet
    memcpy(packet, &rx_buffer[ring_offset + 4], length);
  }
  
  //pass the received packet up to the next layer
  eth_receive_frame(packet, length);
  
  //compute the new index in the ring buffer
  rx_index = (rx_index + length + 4 + 3) & ~3;
  
  //let the network card know how far we got at unloading the buffer
  outportw(ioaddr + ChipRxBufTail, rx_index - 16);
}

void rtl8139_handler(struct regs * r) {
  unsigned short val = inportw(ioaddr + ChipISR);

  if (val & RxOK) {
    print_string("RTL8139 Receive OK\n");
    rtl8139_recv_handler();
  } else if (val & TxOK) {
    print_string("RTL8139 Transmit OK\n");
  } else if (val & RxErr) {
    print_string("RTL8139 Receive Error\n");
  } else if (val & TxErr) {
    print_string("RTL8139 Transmit Error\n");
  } else if (val & RxOverflow) {
    print_string("RTL8139 Receive Buffer Overflow\n");
  } else if (val & RxUnderrun) {
    print_string("RTL8139 Receive Underrun\n");
  } else if (val & RxFIFOOver) {
    print_string("RTL8139 Receive FIFO Overflow\n");
  } else {
    char temp[33] = {0};
    print_string(" Some other event at RTL8139: ");
    print_string(itoa(val, temp, 16));
    print_string("\n");
  }

  outportw(ioaddr + ChipISR, val);
  r = r;
}

void rtl8139_send_packet(void * data, unsigned long int length) {
  //copy the packet into the buffer
  memcpy(&tx_buffers[tx_current_buffer * TX_BUF_SIZE], data, length); 
  
  //zero pad
  while (length < ETH_ZLEN) {
    tx_buffers[tx_current_buffer + length++] = '\0';
  }

  //notify the card of the starting address of the buffer and the length
  outportl(ioaddr + ChipTxBuffer + (tx_current_buffer*4), (unsigned long)&tx_buffers[tx_current_buffer * TX_BUF_SIZE]);
  outportl(ioaddr + ChipTxStatus + (tx_current_buffer*4), length);

  //advance to next transmit buffer
  tx_current_buffer++;
  tx_current_buffer %= 4;
}

/*
 * Returns the mac48 address of the network card
 * requires an arry of unsigned char of size 6 to work
 * as expected
 */
void rtl8139_get_mac48_address(void * addr) {
  unsigned char macAddress[6] = {0};
  unsigned char i;
  for(i = 0; i < 6; i++) {
    macAddress[i] = inportb(ioaddr + i);
  }
  memcpy(addr, macAddress, 6);
}
