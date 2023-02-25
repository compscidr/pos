#ifndef IP_HEADER
#define IP_HEADER


void ipv4_init(void);
void ipv4_receive_packet(char * data, unsigned short length);
void ipv4_broadcast(char * data, unsigned short length, unsigned char protocol);
void ipv4_unicast(char * data, unsigned short length, unsigned char protocol, unsigned char source_address[4], unsigned char destination_address[4]);

#endif
