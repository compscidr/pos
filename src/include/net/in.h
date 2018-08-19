#ifndef IN_HEADER
#define IN_HEADER

void print_ip(unsigned char * ip);
void print_mac(unsigned char * mac);

unsigned short ntohs(unsigned short netshort);
unsigned long ntohl(unsigned long netlong);
unsigned short htons(unsigned short hostshort);
unsigned long htonl(unsigned long hostlong);

#endif
