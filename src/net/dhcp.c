#include "common.h"
#include "screen.h"
#include "net/udp.h"
#include "net/in.h"
#include "net/ip.h"
#include "dev/rtl8139.h"
#include "mm.h"

extern unsigned char * ipv4_address;

//doc: http://en.wikipedia.org/wiki/Dynamic_Host_Configuration_Protocol
//doc: http://www.pcvr.nl/tcpip/bootp.htm

//more of these here: http://www.iana.org/assignments/bootp-dhcp-parameters/bootp-dhcp-parameters.xhtml#options
//tested with a ddwrt v24 router
enum dhcp_option_type {
  DHCP_PAD = 0,
  DHCP_SUBNET = 1,
  DHCP_ROUTER = 3,
  DHCP_DNS = 6,
  DHCP_DOMAIN = 15,
  DHCP_BROADCAST = 28,
  DHCP_IP_REQUEST = 50,
  DHCP_LEASE_TIME = 51,
  DHCP_MSG = 53,
  DHCP_SERVER = 54,
  DHCP_RENEW_TIME = 58,
  DHCP_REBIND = 59,
};

enum dhcp_msg_value {
  DHCP_DISCOVER = 1,
  DHCP_OFFER = 2,
  DHCP_REQUEST = 3,
  DHCP_DECLINE = 4,
  DHCP_ACK = 5,
  DHCP_NACK = 6,
  DHCP_RELEASE = 7,
  DHCP_INFORM = 8,
};

struct dhcp_option {
  unsigned char type;
  unsigned char length;
  unsigned char * data;
};

struct dhcp_packet {
  unsigned char opcode;
  unsigned char htype;
  unsigned char hlen;
  unsigned char hops;
  unsigned long int xid;
  unsigned short int secs;
  unsigned short int zero;
  unsigned char ciaddr[4];
  unsigned char yiaddr[4];
  unsigned char siaddr[4];
  unsigned char giaddr[4];
  unsigned char chaddr[16];
  unsigned char sname[64];
  unsigned char file[128];
  unsigned long magic;        //for dhcp should always be 0x63825363
  unsigned char options[60];  //should be 64 but magic takes up first 4 bytes
};

/*
 * Returns a dhcp option structure from a buffer which points
 * to the start of the option
 */
struct dhcp_option dhcp_get_option(unsigned char * option) {
  struct dhcp_option o;
  o.type = * option;
  option++;
  o.length = * option;
  option++;
  o.data = (unsigned char * ) malloc(sizeof(char) * o.length);
  memcpy(o.data, option, o.length);
  return o;
}

/*
 * Initiates a dhcp discovery request to receive an IP address
 */
void dhcp_discover(void) {
  char buffer[1024];
  struct dhcp_packet dhcp;
  memset( &dhcp, 0, sizeof(dhcp)); //init to all zero

  dhcp.opcode = 1;              //BOOTREQUEST
  dhcp.htype = 1;               //ethernet
  dhcp.hlen = 6;                //ethernet mac address length
  dhcp.hops = 0;                //always set to zero by clients (RFC951)
  dhcp.xid = htonl(0xb00b1e5);  //transmission id (should be random)
  dhcp.secs = htons(0x00);      //seconds since start of bootp
  dhcp.zero = htons(0x00);      //always zero
  memset( &dhcp.ciaddr, 0, 4); //client IP (zero if unknown)
  memset( &dhcp.yiaddr, 0, 4); //client IP returned from server
  memset( &dhcp.siaddr, 0, 4); //server IP returned from server
  memset( &dhcp.giaddr, 0, 4); //gateway IP, not used (used for cross-gw boot)

  //todo make this section not dependent on rtl8139!
  //client hardware addr from RTL8139
  rtl8139_get_mac48_address( &dhcp.chaddr);

  memset( &dhcp.sname, 0, 64); //optional server hostname
  memset( &dhcp.file, 0, 128); //boot file or null in bootrequest

  dhcp.magic = htonl(0x63825363);

  //options section
  dhcp.options[0] = 0x35; //option 53 (dhcp message type)
  dhcp.options[1] = 0x01; //length = 1
  dhcp.options[2] = 0x01; //type = dhcp discover
  dhcp.options[3] = 0x32; //option 50 (request ip address)
  dhcp.options[4] = 0x04; //length = 4
  dhcp.options[5] = 0xc0; //192
  dhcp.options[6] = 0xa8; //168
  dhcp.options[7] = 0x0A; //10
  dhcp.options[8] = 0x78; //120
  dhcp.options[9] = 0xff; //end option

  print_string("DHCP DISCOVER (Will wait forever if net is down, or driver broken)\n");

  ///////part 1: request, should offer afterwards
  udp_bind(68);
  udp_broadcast((unsigned char * )&dhcp, sizeof(dhcp), 68, 67);
  int size = udp_listen(68, buffer, 1024);

  //copy that data into the dhcp packet structure
  if (size > (int) sizeof(struct dhcp_packet)) {
    print_string("MALFORMED DHCP RESPONSE\n");
    print_string("EXPECTING: ");
    char temp[33] = {
        0
    };
    print_string(itoa(sizeof(struct dhcp_packet), temp, 10));
    print_string(" GOT: ");
    print_string(itoa(size, temp, 10));
    return;
  }

  struct dhcp_packet d;
  memcpy( &d, buffer, size);

  unsigned char * pointer = &d.options[0];
  struct dhcp_option option = dhcp_get_option(pointer);
  if (option.type == DHCP_MSG) {
    print_string("  DHCP MSG: ");
    if (option.length == 1) {
      if (option.data[0] == (unsigned char) DHCP_OFFER) {
        print_string("OFFER: ");
        print_ip(d.yiaddr);
        print_string("FROM: ");
        print_ip(d.siaddr);

        //modify options section to make our response request
        dhcp.options[0] = DHCP_MSG; //option 53 (dhcp message type)
        dhcp.options[1] = 0x01; //length = 1
        dhcp.options[2] = DHCP_REQUEST; //type = dhcp request (3)

        dhcp.options[3] = DHCP_IP_REQUEST;
        dhcp.options[4] = 0x04; //length = 4
        memcpy( &dhcp.options[5], d.yiaddr, 4);

        dhcp.options[9] = 0xFF; //end
      } else if (option.data[0] == (unsigned char) DHCP_ACK)
        print_string("ACK\n");
      else
        print_string("UNKNOWN DHCP MESSAGE TYPE\n");
    } else
      print_string("CORRUPT DHCP MSG - NOT RIGHT LENGTH\n");
  }

  ////part 2: request #2, should ack after
  udp_broadcast((unsigned char * ) &dhcp, sizeof(dhcp), 68, 67);
  size = udp_listen(68, buffer, 1024);
  
  //copy that data into the dhcp packet structure
  if (size > (int) sizeof(struct dhcp_packet)) {
    print_string("MALFORMED DHCP RESPONSE\n");
    return;
  }
  memcpy( &d, buffer, size);

  pointer = &d.options[0];
  option = dhcp_get_option(pointer);
  if (option.type == DHCP_MSG) {
    print_string("  DHCP MSG: ");
    if (option.length == 1) {
      if (option.data[0] == (unsigned char) DHCP_ACK) {
        memcpy(ipv4_address, d.yiaddr, 4);
        print_string("ACK the IP IS OURS!!!! ");
        print_ip(ipv4_address);
      } else {
        print_string("UNKNOWN DHCP MESSAGE TYPE: ");
        char c = option.data[0];
        int res = 0;
        res += c;
        char temp[33] = {
          0
        };
        print_string(itoa(res, temp, 10));
        print_string("\n");
      }

    } else
      print_string("CORRUPT DHCP MSG - NOT RIGHT LENGTH\n");
  }

  /*
  //continue processing options until end
  while(*pointer != 0xFF)
  {
  	if(*pointer == DHCP_PAD)
  	{
  		pointer++;
  		continue;
  	}
  	
  	struct dhcp_option option = dhcp_get_option(pointer);
  	
  	if(option.type == DHCP_MSG)
  	{
  		print_string("  DHCP MSG: ");
  		if(option.length == 1)
  		{
  			if(option.data[0] == (unsigned char)DHCP_OFFER)
  			{
  				print_string("OFFER\n");
  				//options section
  				dhcp.options[0] = 0x35;			//option 53 (dhcp message type)
  				dhcp.options[1] = 0x01;			//length = 1
  				dhcp.options[2] = 0x03;			//type = dhcp request (3)
  				dhcp.options[3] = 0xff;			//end option
  			}
  			else if(option.data[0] == (unsigned char)DHCP_ACK)
  				print_string("ACK\n");
  			else
  				print_string("UNKNOWN DHCP MESSAGE TYPE\n");
  		}
  		else
  			print_string("CORRUPT DHCP MSG - NOT RIGHT LENGTH\n");
  	}
  	
  	else if(option.type == DHCP_SUBNET)
  	{
  		print_string("  DHCP SUBNET: ");
  		print_ip(option.data);
  	}
  	else if(option.type == DHCP_ROUTER)
  	{
  		print_string("  DHCP ROUTER: ");
  		print_ip(option.data);
  	}
  	else if(option.type == DHCP_DNS)
  	{
  		print_string("  DHCP DNS: ");
  		print_ip(option.data);
  	}
  	else if(option.type == DHCP_BROADCAST)
  	{
  		print_string("  DHCP BROADCAST: ");
  		print_ip(option.data);
  	}
  	else if(option.type == DHCP_DOMAIN)
  	{
  		print_string("  DHCP DOMAIN: ");
  		print_ip(option.data);
  	}
  	else if(option.type == DHCP_SERVER)
  	{
  		print_string("  DHCP SERVER: ");
  		print_ip(option.data);
  	}
  	else if(option.type == DHCP_LEASE_TIME)
  		print_string("  DHCP LEASE TIME\n");
  	else if(option.type == DHCP_RENEW_TIME)
  		print_string("  DHCP RENEW TIME\n");
  	else if(option.type == DHCP_REBIND)
  		print_string("  DHCP REBINDING TIME\n");
  	else
  	{
  		print_string("UNKNOWN DHCP OPTION: ");
  		print_string(itoa(option.type, temp, 10));
  		print_string("\n");
  		break;
  	}
  	pointer = pointer + 2 + option.length;
  }
  */
}
