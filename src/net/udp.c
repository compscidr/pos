#include "common.h"
#include "screen.h"
#include "net/in.h"
#include "net/ip.h"
#include "mm.h"
#include "mutex.h"

#define MAX_PORTS   1024
#define UDP_BUFFER  1024

enum {
  PORT_FREE,
  PORT_BIND,
  PORT_LISTEN,
  DATA_READY
};


//if -1 port is free, otherwise the size of the last operation
int ports[MAX_PORTS] = {
  PORT_FREE
}; 

//temp buffer for copying
char buffer[UDP_BUFFER] = {
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

/*
 * TODO: buffer for each process, make each buffer a cyclical buffer
 * and only overwrite data when it has been pushed to the process
 */
void udp_receive_packet(char * data, unsigned short length) {
  char temp[40];

  if (length < sizeof(struct udp_packet_header)) {
    //print_string("UDP Packet Header too small, dropping packet\n");
    return;
  }

  struct udp_packet_header packet;
  memcpy( &packet, data, sizeof(struct udp_packet_header));

  /*
  print_string("UDP packet received ");
  print_string(itoa(length,temp,10));
  print_string(" bytes from port: ");
  print_string(itoa(ntohs(packet.source_port), temp, 10));
  print_string(" to port: ");
  print_string(itoa(ntohs(packet.destination_port), temp, 10));
  print_string("\n");
  */

  //only continue if the port is correct
  int port = ntohs(packet.destination_port);
  if (port > 0 && port < MAX_PORTS) {
    //check if the port is even listening before proceeding
    if (ports[port] != PORT_FREE) {
      if(ports[port] == DATA_READY) {
        print_string("PORT HAS DATA READY - PROBABLY GOING TO OVERWRITE STUFF WAITING IN QUEUE FOR PROCESS");
      }
      //copy the received packet to the buffer
      spinlock();
      memcpy(buffer, 0, 1024);
      //print_string("Copying ");
      //print_string(itoa(length - sizeof(struct udp_packet_header), temp, 10));
      //print_string(" to buffer\n");
      memcpy(buffer, data + sizeof(struct udp_packet_header), length - sizeof(struct udp_packet_header));
      spinunlock();
      ports[ntohs(packet.destination_port)] = length - sizeof(struct udp_packet_header);
    } else {
      print_string("Not listening on UDP port: ");
      print_string(itoa(ntohs(packet.destination_port), temp, 10));
      print_string("\n");
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
  memcpy(buffer, &udp, sizeof(struct udp_packet_header));
  memcpy(buffer + sizeof(struct udp_packet_header), data, length);
  
  //print_string("DUMPING UDP PACKET: \n");
  //hd((unsigned long int)buffer, (unsigned long int)buffer + length + sizeof(struct udp_packet_header));
  
  ipv4_broadcast(buffer, length + sizeof(struct udp_packet_header), 17);
  //print_string("UDP Broadcast done\n");
}

int udp_bind(unsigned short port) {
  char temp[33] = {0};
  print_string("UDP Binding to port: ");
  print_string(itoa(port, temp, 10));
  print_string("\n");
  
  //check if the port is already busy
  if (ports[port] == PORT_FREE) {
    ports[port] = PORT_BIND;
    return 1;
  }
  else {
    print_string("PORT TAKEN ALREADY");
    return -1;
  }  
}

int udp_close(unsigned short port) {
  if(port > MAX_PORTS) {
    return -1;
  }
  ports[port] = PORT_FREE;
  return 1;
}

/*
 * Blocking call which waits for data on the specific port and fills it
 * into the provided data buffer. Assumes the buffer is large enough
 * to handle the data
 */
int udp_listen(unsigned short port, char * data, int length) {
  if(ports[port] == PORT_BIND) {
    ports[port] = PORT_LISTEN;
  }
  
  //make sure we don't overflow buffer
  if (length > UDP_BUFFER) {
    print_string("UDP Buffer Overflow\n");
    return -1;
  }

  //sleep while waiting for data on the port
  while (ports[port] == PORT_LISTEN) {
    __asm__("hlt"); 
  }

  spinlock();
  memcpy(data, buffer, length);
  int size = ports[port];
  ports[port] = PORT_LISTEN;
  spinunlock();

  return size;
}
