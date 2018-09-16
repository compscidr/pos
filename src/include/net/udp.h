#ifndef UDP_HEADER
#define UDP_HEADER

void udp_init(void);
void udp_receive_packet(char * data, unsigned short length);
void udp_broadcast(unsigned char * data, unsigned short length, unsigned short source_port, unsigned short destination_port);
int udp_bind(unsigned short port);
int udp_listen(unsigned short port, char * data, int length);
int udp_close(unsigned short port);

#endif
