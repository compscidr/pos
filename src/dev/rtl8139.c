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

unsigned short int rx_index;
unsigned long int ioaddr;
unsigned short int rx_index;
char * rx_buffer;
char * tx_buffer;
char * network_buffer;
char ** tx_buffers[NUM_TX_DESC];
int current_buffer;

/*
 * Registers the IRQ handler for the device, inits the device
 */
void install_rtl8139(struct pci_device * device) {
  char temp[35] = {
    0
  };
  unsigned short int c, memory_type;

  print_string("RTL8139 Network Device Found\n");

  //check the network card BARs
  for (c = 0; c < 6; ++c) {
    if (device->bar[c]) {
      memory_type = device->bar[c] & 0x01;
      if (memory_type == 0) {
        ioaddr = device->bar[c] &= 0xFFFFFFF0;
        print_string("Memory space BAR: ");
        print_address(ioaddr);
        print_string("\n");
        hd(ioaddr, ioaddr + 16);
      } else if (memory_type == 1) {
        ioaddr = device->bar[c] &= 0xFFFFFFFC;
        print_string("IO Space BAR: ");
        print_address(ioaddr);
        print_string("\n");
        hd(ioaddr, ioaddr + 16);
      } else
        print_string("  Error determining IO address of the device\n");
    }
  }

  print_string("  At memory address: ");
  print_address(ioaddr);
  print_string("\n");

  irq_install_handler(device->interrupt_line, rtl8139_handler);

  print_string("  Installing on IRQ: ");
  print_string(itoa(device->interrupt_line, temp, 10));
  print_string("\n");

  //wake-up / power on
  *((unsigned char * )(ioaddr + ChipConfig1)) = 0x00;

  //reset the card
  print_string("  Resetting RTL8139...");
  *((unsigned char * )(ioaddr + ChipCmd)) = CmdReset;
  print_status(1);

  //enable transmit and receive
  *((unsigned char * )(ioaddr + ChipCmd)) = CmdRxEnb | CmdTxEnb;

  //Configure how packets are received (Accept All)
  *((unsigned long int * )(ioaddr + ChipRxConfig)) = 0x000001F;

  //tell the card where the rx buffer is
  rx_index = 0;
  rx_buffer = calloc(sizeof(char) * (8192 + 16 + 1500));
  network_buffer = calloc(sizeof(char) * (8192 + 16 + 1500));
  *((unsigned long int * )(ioaddr + ChipRxBuffer)) = (unsigned long) rx_buffer;

  //*((unsigned long int *)(ioaddr + ChipTxConfig)) = (TX_DMA_BURST << 8);

  //allocate the tx buffers
  int i;
  for (i = 0; i < NUM_TX_DESC; i++)
    tx_buffers[i] = calloc(sizeof(char) * (TX_BUF_SIZE));
  current_buffer = 0;

  print_string("  MAC Address of RTL8139:  ");
  unsigned char val;
  for (i = 0; i < 6; i++) {
    memcpy((void * ) & val, (void * ) ioaddr + i, 1);
    print_string(itoa(val, temp, 16));
    if (i != 5)
      print_string(":");
  }
  print_string("\n");

  //set imr (int mask reg) + isr (int service reg) (accept all interrupts)
  //*((unsigned short int *)(ioaddr + ChipIMR)) = 0xffff;
  *((unsigned short int * )(ioaddr + ChipIMR)) = (TxOK | RxOK); //rx ok and tx ok only
}

void rtl8139_send_handler() {}

void rtl8139_recv_handler() {
  unsigned long length = (rx_buffer[rx_index + 3] << 8) + rx_buffer[rx_index + 2];
  memcpy(network_buffer, & rx_buffer[rx_index], length);

  //packets aligned to 32 bits
  rx_index += length + 4;
  rx_index = (rx_index + 3) & ~0x3;

  //handle wrap
  rx_index %= 8192;

  //notify card where we have handled to
  *((unsigned short * )(ioaddr + ChipRxBufTail)) = rx_index - 0x10;

  eth_receive_frame( & network_buffer[4], length);
}

void rtl8139_handler(struct regs * r) {
  unsigned char val = * ((unsigned char * )(ioaddr + ChipISR));

  if (val & (RxOK)) {
    //print_string(" Receive OK: ");
    rtl8139_recv_handler();
  } else if (val & (TxOK)) {
    inportl(ioaddr + ChipTxStatus);

    print_string(" Send OK: \n");

    outportw(ioaddr + ChipISR, TxOK);

    *((unsigned short * )(ioaddr + ChipISR)) = 0xffff;
    //((unsigned short int *)(ioaddr + ChipTxStatus)) = (val & (TxOK));
    //inportl(ioaddr + ChipTxStatus);
    //long int t = *(unsigned long *)(ioaddr + ChipTxStatus);
    //t = t;
  }

  *((unsigned char * )(ioaddr + ChipISR)) = val;
  r = r;
}

void rtl8139_send_packet(void * data, unsigned long int length) {
  memcpy(tx_buffers[current_buffer], data, length); //copy the packet into the buffer

  //zero pad
  while (length < ETH_ZLEN)
    tx_buffers[current_buffer][length] = '\0';

  //print_string("Sending Data:\n");
  //hd((unsigned long)tx_buffer, (unsigned long)tx_buffer + length);

  //while(1);

  //set addr and size of tx buffer
  *((unsigned long int * )(ioaddr + ChipTxBuffer + 4 * current_buffer)) = (unsigned long) tx_buffers[current_buffer];
  *((unsigned long int * )(ioaddr + ChipTxStatus + 4 * current_buffer)) = length;

  current_buffer++;
  current_buffer %= 4;
}

/*
 * Returns the mac48 address of the network card
 * requires an arry of unsigned char of size 6 to work
 * as expected
 */
void rtl8139_get_mac48_address(void * addr) {
  memcpy(addr, (void * ) ioaddr, 6);
}
