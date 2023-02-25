#include "net/ip.h"
#include "net/in.h"

extern unsigned char * ipv4_address;

void icmp_ping(char * destination) {
    print_string("sending ping from: ");
    print_ip(ipv4_address);
}