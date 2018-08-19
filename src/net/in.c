#include "screen.h" //for printing an ip and mac address
#include "common.h"
#include "mm.h"

/*
 * Converts a string representation of an IPv4 address to binary
 * memory version that can be used in protocol
 * 
 * (input must be null-terminated)
 */
unsigned char * string_to_ip(char * string_ip) {
  unsigned char * ip = (unsigned char * ) malloc(4 * sizeof(char));
  while ( * string_ip) {
    //todo - need an itoa function for each octet
  }
  return ip;
}

/*
 * Prints an IP address given the starting byte
 */
void print_ip(unsigned char * ip) {
  char temp[33] = {
    0
  };
  int i;
  for (i = 0; i < 4; i++) {
    int res = 0;
    unsigned char octet = ip[i];
    res += octet;
    print_string(itoa(res, temp, 10));
    if (i < 3)
      print_string(".");
  }
  print_string("\n");
}

/*
 * Prints a MAC address given the starting byte
 */
void print_mac(unsigned char * mac) {
  char temp[33] = {
    0
  };
  int i;
  for (i = 0; i < 6; i++) {
    int res = 0;
    unsigned char octet = mac[i];
    res += octet;
    print_string(itoa(res, temp, 16));
    if (i < 5)
      print_string(":");
  }
  print_string("\n");
}

/*
 * Converts a word (16 bit) from network byte ordering (big end) to
 * host byte ordering (little endian), assuming x86 arch
 */
unsigned short ntohs(unsigned short netshort) {
  unsigned char * temp = (unsigned char * ) & netshort;
  return ((unsigned short) temp[0] << 8) + ((unsigned char) temp[1]);
}

/*
 * Converts a long (32 bit) from network byte ordering (big end) to
 * host byte ordering (little endian), assuming x86 arch
 */
unsigned long ntohl(unsigned long netlong) {
  unsigned char * temp = (unsigned char * ) & netlong;
  return ((unsigned long) temp[0] << 24) + ((unsigned long) temp[1] << 16) + ((unsigned long) temp[2] << 8) + ((unsigned char) temp[3]);
}

/*
 * Converts a word (16 bit) from host byte ordering (little endian) to
 * network byte ordering (big end), assuming x86 arch
 */
unsigned short htons(unsigned short hostshort) {
  unsigned char * temp = (unsigned char * ) & hostshort;
  return ((unsigned short) temp[0] << 8) + ((unsigned char) temp[1]);
}

/*
 * Converts a long (32 bit) from host byte ordering (little endian) to 
 * network byte ordering (big end), assuming x86 arch
 */
unsigned long htonl(unsigned long hostlong) {
  unsigned char * temp = (unsigned char * ) & hostlong;
  return ((unsigned long) temp[0] << 24) + ((unsigned long) temp[1] << 16) + ((unsigned long) temp[2] << 8) + ((unsigned char) temp[3]);
}
