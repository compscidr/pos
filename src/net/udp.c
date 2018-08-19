#include "common.h"
#include "screen.h"
#include "net/in.h"
#include "net/ip.h"
#include "mm.h"
#include "mutex.h"

#define MAX_PORTS 1024

#define PORT_FREE - 1
#define PORT_LISTEN 0

//if -1 port is free, otherwise the size of the last operation
int ports[MAX_PORTS] = {
  PORT_FREE
}; 

//temp buffer for copying
char buffer[1024] = {
  0
};

struct udp_packet_header {
  unsigned short source_port;
  unsigned short destination_port;
  unsigned short length;
  unsigned short checksum;
};

struct udp_pseudo_packet_header {
  unsigned long source_address;
  unsigned long destination_address;
  unsigned char zeros;
  unsigned char protocol;
  unsigned short udplength;
  unsigned short source_port;
  unsigned short destination_port;
  unsigned short length;
  unsigned short checksum;
};

// initializes all of the udp ports to not be listening
void udp_init() {
  int i;
  for (i = 0; i < MAX_PORTS; i++)
    ports[i] = PORT_FREE;
}

void udp_receive_packet(char * data, unsigned short length) {
  char temp[40];

  if (length < sizeof(struct udp_packet_header)) {
    //print_string("UDP Packet Header too small, dropping packet\n");
    return;
  }

  struct udp_packet_header packet;
  memcpy( & packet, data, sizeof(struct udp_packet_header));

  //print_string("UDP packet received ");
  //print_string(itoa(length,temp,10));
  //print_string(" bytes from port: ");
  //print_string(itoa(ntohs(packet.source_port), temp, 10));
  //print_string(" to port: ");
  //print_string(itoa(ntohs(packet.destination_port), temp, 10));
  //print_string("\n");

  //only continue if the port is correct
  int port = ntohs(packet.destination_port);
  if (port > 0 && port < MAX_PORTS) {
    //check if the port is even listening before proceeding
    if (ports[port] == PORT_LISTEN) {
      //copy the received packet to the buffer
      spinlock();
      memcpy(buffer, 0, 1024);
      print_string("Copying ");
      print_string(itoa(length - sizeof(struct udp_packet_header), temp, 10));
      print_string(" to buffer\n");
      memcpy(buffer, data + sizeof(struct udp_packet_header), length - sizeof(struct udp_packet_header));
      spinunlock();
      ports[ntohs(packet.destination_port)] = length - sizeof(struct udp_packet_header);
    }
  }
}

/*
 * For now, use IP 0.0.0.0, but in future, get the systems known ip
 */
void udp_broadcast(unsigned char * data, unsigned short length, unsigned short source_port, unsigned short destination_port) {
  struct udp_packet_header udp;
  udp.source_port = htons(source_port);
  udp.destination_port = htons(destination_port);
  udp.length = htons(length + 8); //add 8 for header
  udp.checksum = 0x0000; //initially zero until we compute the checksum

  char * buffer = calloc(length + sizeof(struct udp_packet_header));
  memcpy(buffer, & udp, sizeof(struct udp_packet_header));
  memcpy(buffer + sizeof(struct udp_packet_header), data, length);
  ipv4_broadcast(buffer, length + sizeof(struct udp_packet_header), 17);
}

/*
 * Blocking call which waits for data on the specific port and fills it
 * into the provided data buffer. Assumes the buffer is large enough
 * to handle the data
 */
int udp_listen(unsigned short port, char * data, int length) {

  //check if the port is already busy
  if (ports[port] == PORT_FREE)
    ports[port] = PORT_LISTEN;
  else
    return -1;

  //make sure we don't overflow buffer
  if (length > 1024)
    return -1;

  //sleep while waiting for data on the port
  while (ports[port] == PORT_LISTEN) {
    //__asm__("hlt"); 
  }

  spinlock();
  memcpy(data, buffer, length);
  int size = ports[port];
  ports[port] = PORT_FREE;
  spinunlock();

  return size;;
}
