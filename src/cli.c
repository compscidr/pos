#include "screen.h"
#include "kb.h"
#include "mm.h"
//#include "net.h"
//#include "net/dhcp.h"

void cli_main(void)
{
  print_string("--------------------------------------------------------------------------------");
  print_string("Welcome to POS console\n");
  print_string("commands: help clear dhcp freemem ip reboot\n");
	
  char buffer[1024];
    
  while(1)
  {
    print_string("$ ");
    kb_gets(buffer);
      
    //compare the input against known commands we can execute
    //eventually this should check some path in the filesystem
    //for the programs we know about (or the current console path)
    if(strcmp(buffer,"help")==0)
      print_string("commands: help clear dhcp freemem ip reboot\n");
    else if(strcmp(buffer,"reboot")==0)
      reboot();
    else if(strcmp(buffer,"clear")==0)
      screen_clear();
    else if(strcmp(buffer,"freemem")==0)
      display_free_bytes();
    else if(strcmp(buffer,"dhcp")==0) {
      //dhcp_discover();
    }      
    else if(strcmp(buffer,"ip")==0) {
      //ip();
    }
    else
      print_string("Unknown command");
    
    __asm__("hlt");
  }
}
