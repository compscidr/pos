#include "screen.h"
#include "common.h"
#include "net/in.h"
#include "net/ip.h"
#include "mm.h"
#include "dev/rtl8139.h"

struct ethernet_frame {
  unsigned char destination_mac48_address[6];
  unsigned char source_mac48_address[6];
  unsigned short ethertype;
};

/*
 * Receives an ethernet frame from the network device driver and passes
 * it along to the appropriate upper layer protocols
 * 
 * if the protocol is not supported, ignore the packet
 */
void eth_receive_frame(char * data, unsigned short length) {
  char temp[40];
  /*
  print_string("ETH RECV - ");
  print_string(itoa(length, temp, 10));
  print_string(" bytes:\n");
  hd((unsigned long)data, (unsigned long)data+16);
  */

  if (length >= sizeof(struct ethernet_frame)) {

    struct ethernet_frame frame;
    memcpy(&frame, data, sizeof(struct ethernet_frame));
    frame.ethertype = ntohs(frame.ethertype);

    switch (frame.ethertype) {
    case 0x0800:
      print_string("IPv4 Packet\n");
      ipv4_receive_packet( &data[sizeof(struct ethernet_frame)], length - sizeof(struct ethernet_frame));
      break;

    case 0x86dd:
      print_string("IPv6 Packet\n");
      break;

    case 0x0806:
      print_string("ARP Packet\n");
      //arp_receive_packet(&data[sizeof(struct ethernet_frame)], length - sizeof(struct ethernet_frame));
      break;

    default:
      print_string("Unknown type or incorrect ethernet frame, dropping packet.\n");
      print_string("Type: ");
      print_string(itoa(frame.ethertype, temp, 10));
      print_string("\n");
      //hd((unsigned long int)&frame, (unsigned long int)&frame + sizeof(struct ethernet_frame));
      break;
    }
  } else
    print_string("Ethernet frame header too small, dropping packet\n");
}

void eth_send_frame(char * data, unsigned short length, unsigned char * destination_mac48_address, unsigned short protocol) {
  data = data;
  length = length;
  destination_mac48_address = destination_mac48_address;
  protocol = protocol;
}

void eth_broadcast(unsigned char * data, unsigned short length, unsigned short protocol) {
  struct ethernet_frame frame;
  memset( &frame.destination_mac48_address, 0xff, 6);
  rtl8139_get_mac48_address(frame.source_mac48_address);
  frame.ethertype = htons(protocol);
  unsigned char * buffer = malloc(length + sizeof(struct ethernet_frame));
  memcpy(buffer, & frame, sizeof(struct ethernet_frame));
  memcpy(buffer + sizeof(struct ethernet_frame), data, length);
  
  rtl8139_send_packet(buffer, length + sizeof(struct ethernet_frame));
}
