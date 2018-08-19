#ifndef IP_HEADER
#define IP_HEADER

unsigned char ipv4_address[4];

void ipv4_init(void);
void ipv4_receive_packet(char * data, unsigned short length);
void ipv4_broadcast(char * data, unsigned short length, unsigned char protocol);

#endif
