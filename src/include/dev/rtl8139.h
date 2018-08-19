#ifndef RTL8139_HEADER
#define RTL8139_HEADER

#include "pci.h"

#define NUM_TX_DESC 4
#define TX_BUF_SIZE 1536
#define TX_DMA_BURST 4
#define ETH_ZLEN 60

enum RTL8139_registers {
  ChipTxStatus = 0x10,
  ChipTxBuffer = 0x20,
  ChipRxBuffer = 0x30,
  ChipCmd = 0x37,
  ChipRxBufTail = 0x38,
  ChipRxBufHead = 0x3A,
  ChipIMR = 0x3C,
  ChipISR = 0x3E,
  ChipTxConfig = 0x40,
  ChipRxConfig = 0x44,
  ChipConfig0 = 0x51,
  ChipConfig1 = 0x52,
  ChipStatConf = 0x74,
};

enum RTL8139_cmd_bits {
  CmdReset = 0x10,
  CmdRxEnb = 0x08,
  CmdTxEnb = 0x04,
  RxBufEmpty = 0x01,
};

enum RTL8139_intr_status_bits {
  RxOK = 0x01,
  RxErr = 0x02,
  TxOK = 0x04,
  TxErr = 0x08,
  RxOverflow = 0x10,
  RxUnderrun = 0x20,
  RxFIFOOver = 0x40,
  PCIErr = 0x8000
};

enum RTL8139_rx_mode_bits {
  AcceptBroadcast = 0x08,
  AcceptMyPhy = 0x02,
  AcceptAllPhy = 0x01,
};

void install_rtl8139(struct pci_device * device);
void rtl8139_handler(struct regs * r);
void rtl8139_send_packet(void * data, unsigned long int length);
void rtl8139_get_mac48_address(void * addr);

#endif
