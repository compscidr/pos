#ifndef ETH_HEADER
#define ETH_HEADER

void eth_receive_frame(char * data, unsigned short length);
void eth_send_frame(char * data, unsigned short length, unsigned char * destination_mac48_address, unsigned short protocol);
void eth_broadcast(unsigned char * data, unsigned short length, unsigned short protocol);

#endif
