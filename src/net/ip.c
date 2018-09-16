#include "common.h"
#include "screen.h"
#include "net/eth.h"
#include "net/ip.h"
#include "net/udp.h"
#include "net/in.h"
#include "mm.h"

struct ipv4_packet_header
{
  unsigned char version_ihl;    //ip ver & internet header len (4bit ea)
  unsigned char tos;            //type of service (QoS params)
  unsigned short int length;    //total length of IP packet
  unsigned short int id;        //packet id
  unsigned short int foffset;   //3 lsbs flags, rest is fragement offset
  unsigned char ttl;            //time to live (hops)
  unsigned char protocol;       //ex tcp, udp
  unsigned short int checksum;  //used for error checking
  unsigned char source_address[4];
  unsigned char destination_address[4];
};

void ipv4_init(void)
{
  memset(ipv4_address, 0, 4);
}

void ipv4_receive_packet(char * data, unsigned short length)
{
  /*
  char temp[40];
  print_string("IPv4 RECV - ");
  print_string(itoa(length, temp, 10));
  print_string(" bytes\n");
  */
    
  if(length < sizeof(struct ipv4_packet_header))
  {
    print_string("IPv4 header too small, dropping packet\n");
    return;
  }
    
  struct ipv4_packet_header packet;
  memcpy(&packet, data, sizeof(struct ipv4_packet_header));
  
  switch (packet.protocol)
  {
    case 1:		//ICMP
      print_string("ICMP Packet\n");
    break;
    case 2:		//IGMP
      print_string("IGMP Packet\n");
    break;
    case 6:		//TCP
      print_string("TCP Packet\n");
      //tcp_receive_packet(data[sizeof(struct ipv4_packet_header)], length - sizeof(struct ipv4_packet_header));
    break;
      
    case 17:		//UDP
      print_string("UDP Packet\n");
      udp_receive_packet(&data[sizeof(struct ipv4_packet_header)], length - sizeof(struct ipv4_packet_header));
    break;

    default:
      print_string("Unknown type of IP packet: ");
      char temp[33];
      print_string(itoa(packet.protocol, temp, 10));
      print_string(", cannot handle at the moment\n");
    break;
  }
}

/*
 * compute IPv4 checksum of an ip header given the address of a packet
 * structure - already returned in network byte ordering 
 * (no need for htons)
 */
unsigned long ipv4_checksum(unsigned short * packet)
{
  unsigned long sum = 0;
  unsigned short count = sizeof(struct ipv4_packet_header);

  while(count > 1)
  {
    sum += *packet++;
    count -= 2;
  }

  //add left-over byte, if any
  if(count > 0)
    sum += * (unsigned char *) packet;

  //fold 32-bit sum to 16 bits
  while(sum >> 16)
    sum = (sum & 0xffff) + (sum >> 16);

  return ~sum;
}

/*
 * Broadcast an ipv4 packet
 * ie) destination = 255.255.255.255
 * limitation: only source can be 0.0.0.0 for now
 */
void ipv4_broadcast(char * data, unsigned short length, unsigned char protocol)
{	
  struct ipv4_packet_header packet;
  packet.version_ihl = 0x45;						//version = ipv4
  packet.tos = 0x10;
  packet.length = htons(sizeof(struct ipv4_packet_header) + length);
  packet.id = htons(0x00);						//may need to fix this to give proper id
  packet.foffset = htons(0x00);					//do not support offsets at this point
  packet.ttl = 128;								//set this with a define somewhere maybe?
  packet.protocol = protocol;
  packet.checksum = 0x0000;						//set to zero before compute
  memset(packet.source_address, 0x00, 4);			//0.0.0.0 (temporarily)
  memset(packet.destination_address, 0xff, 4);	//255.255.255.255
  packet.checksum = ipv4_checksum((unsigned short *)&packet);
    
  unsigned char * buffer = calloc(length + sizeof(struct ipv4_packet_header));
  memcpy(buffer, &packet, sizeof(struct ipv4_packet_header));
  memcpy(buffer + sizeof(struct ipv4_packet_header), data, length);
    
  eth_broadcast(buffer, length + sizeof(struct ipv4_packet_header), 0x0800);
}
