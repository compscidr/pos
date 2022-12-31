#include "screen.h"
#include "net/ip.h"
#include "net/in.h"

extern unsigned char * ipv4_address;

void ip(void)
{
	print_string("IP Address: ");
	print_ip(ipv4_address);
}
