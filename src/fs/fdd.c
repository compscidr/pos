#include "fs/fdd.h"
#include "screen.h"
#include "common.h"
#include "idt.h"
#include "timer.h"

// https://forum.osdev.org/viewtopic.php?t=13538
static const char * drive_types[8] = {
    "none",
    "360kB 5.25\"",
    "1.2MB 5.25\"",
    "720kB 3.5\"",

    "1.44MB 3.5\"",
    "2.88MB 3.5\"",
    "unknown type",
    "unknown type"
};

// standard base address of the primary floppy controller
static const int floppy_base = 0x03f0;
// standard IRQ number for floppy controllers
static const int floppy_irq = 6;

// The registers of interest. There are more, but we only use these here.
enum floppy_registers {
  FLOPPY_DOR  = 2,  // digital output register
  FLOPPY_MSR  = 4,  // master status register, read only
  FLOPPY_FIFO = 5,  // data FIFO, in DMA operation for commands
  FLOPPY_CCR  = 7   // configuration control register, write only
};

// The commands of interest. There are more, but we only use these here.
enum floppy_commands {
  CMD_SPECIFY = 3,            // SPECIFY
  CMD_WRITE_DATA = 5,         // WRITE DATA
  CMD_READ_DATA = 6,          // READ DATA
  CMD_RECALIBRATE = 7,        // RECALIBRATE
  CMD_SENSE_INTERRUPT = 8,    // SENSE INTERRUPT
  CMD_SEEK = 15,              // SEEK
};

//
// The MSR byte: [read-only]
// -------------
//
//  7   6   5    4    3    2    1    0
// MRQ DIO NDMA BUSY ACTD ACTC ACTB ACTA
//
// MRQ is 1 when FIFO is ready (test before read/write)
// DIO tells if controller expects write (1) or read (0)
//
// NDMA tells if controller is in DMA mode (1 = no-DMA, 0 = DMA)
// BUSY tells if controller is executing a command (1=busy)
//
// ACTA, ACTB, ACTC, ACTD tell which drives position/calibrate (1=yes)
//
//

void floppy_write_cmd(int base, char cmd) {
  int i; // do timeout, 60 seconds
  for(i = 0; i < 600; i++) {
    sleep(10); // sleep 10 ms
    if(0x80 & inportb_p(base+FLOPPY_MSR)) {
      return (void) outportb_p(base+FLOPPY_FIFO, cmd);
    }
  }
  print_string("floppy_write_cmd: timeout");
}

unsigned char floppy_read_data(int base) {

  int i; // do timeout, 60 seconds
  for(i = 0; i < 600; i++) {
    sleep(10); // sleep 10 ms
    if(0x80 & inportb_p(base+FLOPPY_MSR)) {
      return inportb_p(base+FLOPPY_FIFO);
    }
  }
  print_string("floppy_read_data: timeout");
  return 0; // not reached
}

void floppy_check_interrupt(int base, int *st0, int *cyl) {

  floppy_write_cmd(base, CMD_SENSE_INTERRUPT);

  *st0 = floppy_read_data(base);
  *cyl = floppy_read_data(base);
}

//
// The DOR byte: [write-only]
// -------------
//
//  7    6    5    4    3   2    1   0
// MOTD MOTC MOTB MOTA DMA NRST DR1 DR0
//
// DR1 and DR0 together select "current drive" = a/00, b/01, c/10, d/11
// MOTA, MOTB, MOTC, MOTD control motors for the four drives (1=on)
//
// DMA line enables (1 = enable) interrupts and DMA
// NRST is "not reset" so controller is enabled when it's 1
//
enum { floppy_motor_off = 0, floppy_motor_on, floppy_motor_wait };
static volatile int floppy_motor_ticks = 0;
static volatile int floppy_motor_state = 0;

void floppy_motor(int base, int onoff) {

  if(onoff) {
    if(!floppy_motor_state) {
      // need to turn on
      outportb_p(base + FLOPPY_DOR, 0x1c);
      sleep(500); // wait 500 ms = hopefully enough for modern drives
    }
    floppy_motor_state = floppy_motor_on;
  } else {
    if(floppy_motor_state == floppy_motor_wait) {
      print_string("floppy_motor: strange, fd motor-state already waiting..\n");
    }
    floppy_motor_ticks = 3000; // 3 seconds, see floppy_timer() below
    floppy_motor_state = floppy_motor_wait;
  }
}

void floppy_motor_kill(int base) {
  outportb_p(base + FLOPPY_DOR, 0x0c);
  floppy_motor_state = floppy_motor_off;
}

//
// THIS SHOULD BE STARTED IN A SEPARATE THREAD.
//
//
void floppy_timer() {
  while(1) {
    // sleep for 500ms at a time, which gives around half
    // a second jitter, but that's hardly relevant here.
    sleep(500);
    if(floppy_motor_state == floppy_motor_wait) {
      floppy_motor_ticks -= 500;
      if(floppy_motor_ticks <= 0) {
        floppy_motor_kill(floppy_base);
      }
    }
  }
}

// Move to cylinder 0, which calibrates the drive..
int floppy_calibrate(int base) {

  int i, st0, cyl = -1; // set to bogus cylinder

  floppy_motor(base, floppy_motor_on);

  for(i = 0; i < 10; i++) {
    // Attempt to positions head to cylinder 0
    floppy_write_cmd(base, CMD_RECALIBRATE);
    floppy_write_cmd(base, 0); // argument is drive, we only support 0

    irq_wait(floppy_irq);
    floppy_check_interrupt(base, &st0, &cyl);

    if(st0 & 0xC0) {
      static const char * status[] =
          { 0, "error", "invalid", "drive" };
      print_string("floppy_calibrate: status = ");
      print_string(status[st0 >> 6]);
      print_string("\n");
      continue;
    }

    if(!cyl) { // found cylinder 0 ?
      floppy_motor(base, floppy_motor_off);
      return 0;
    }
  }

  print_string("floppy_calibrate: 10 retries exhausted\n");
  floppy_motor(base, floppy_motor_off);
  return -1;
}


int floppy_reset(int base) {

  outportb_p(base + FLOPPY_DOR, 0x00); // disable controller
  outportb_p(base + FLOPPY_DOR, 0x0C); // enable controller

  irq_wait(floppy_irq); // sleep until interrupt occurs

  {
    int st0, cyl; // ignore these here..
    floppy_check_interrupt(base, &st0, &cyl);
  }

  // set transfer speed 500kb/s
  outportb_p(base + FLOPPY_CCR, 0x00);

  //  - 1st byte is: bits[7:4] = steprate, bits[3:0] = head unload time
  //  - 2nd byte is: bits[7:1] = head load time, bit[0] = no-DMA
  //
  //  steprate    = (8.0ms - entry*0.5ms)*(1MB/s / xfer_rate)
  //  head_unload = 8ms * entry * (1MB/s / xfer_rate), where entry 0 -> 16
  //  head_load   = 1ms * entry * (1MB/s / xfer_rate), where entry 0 -> 128
  //
  floppy_write_cmd(base, CMD_SPECIFY);
  floppy_write_cmd(base, 0xdf); /* steprate = 3ms, unload time = 240ms */
  floppy_write_cmd(base, 0x02); /* load time = 16ms, no-DMA = 0 */

  // it could fail...
  if(floppy_calibrate(base)) return -1;

}

// Seek for a given cylinder, with a given head
int floppy_seek(int base, unsigned cyli, int head) {

  unsigned i, st0, cyl = -1; // set to bogus cylinder

  floppy_motor(base, floppy_motor_on);

  for(i = 0; i < 10; i++) {
    // Attempt to position to given cylinder
    // 1st byte bit[1:0] = drive, bit[2] = head
    // 2nd byte is cylinder number
    floppy_write_cmd(base, CMD_SEEK);
    floppy_write_cmd(base, head<<2);
    floppy_write_cmd(base, cyli);

    irq_wait(floppy_irq);
    floppy_check_interrupt(base, &st0, &cyl);

    if(st0 & 0xC0) {
      static const char * status[] =
          { "normal", "error", "invalid", "drive" };
      print_string("floppy_seek: status = ");
      print_string(status[st0 >> 6]);
      print_string("\n");
      continue;
    }

    if(cyl == cyli) {
      floppy_motor(base, floppy_motor_off);
      return 0;
    }

  }

  print_string("floppy_seek: 10 retries exhausted\n");
  floppy_motor(base, floppy_motor_off);
  return -1;
}

// Used by floppy_dma_init and floppy_do_track to specify direction
typedef enum {
  floppy_dir_read = 1,
  floppy_dir_write = 2
} floppy_dir;


// we statically reserve a totally uncomprehensive amount of memory
// must be large enough for whatever DMA transfer we might desire
// and must not cross 64k borders so easiest thing is to align it
// to 2^N boundary at least as big as the block

/*
#define floppy_dmalen 0x4800
static const char floppy_dmabuf[floppy_dmalen]
    __attribute__((aligned(0x8000)));
*/
#define floppy_dmalen 0x4800
static const char * floppy_dmabuf
    __attribute__((aligned(0x8000)));

static void floppy_dma_init(floppy_dir dir) {

  union {
    unsigned char b[4]; // 4 bytes
    unsigned long l;    // 1 long = 32-bit
  } a, c; // address and count

  a.l = (unsigned) &floppy_dmabuf;
  c.l = (unsigned) floppy_dmalen - 1; // -1 because of DMA counting

  // check that address is at most 24-bits (under 16MB)
  // check that count is at most 16-bits (DMA limit)
  // check that if we add count and address we don't get a carry
  // (DMA can't deal with such a carry, this is the 64k boundary limit)
  if((a.l >> 24) || (c.l >> 16) || (((a.l&0xffff)+c.l)>>16)) {
    print_string("floppy_dma_init: static buffer problem\n");
  }

  unsigned char mode;
  switch(dir) {
  // 01:0:0:01:10 = single/inc/no-auto/to-mem/chan2
  case floppy_dir_read:  mode = 0x46; break;
  // 01:0:0:10:10 = single/inc/no-auto/from-mem/chan2
  case floppy_dir_write: mode = 0x4a; break;
  default: print_string("floppy_dma_init: invalid direction");
    return; // not reached, please "mode user uninitialized"
  }

  outportb_p(0x0a, 0x06);   // mask chan 2

  outportb_p(0x0c, 0xff);   // reset flip-flop
  outportb_p(0x04, a.b[0]); //  - address low byte
  outportb_p(0x04, a.b[1]); //  - address high byte

  outportb_p(0x81, a.b[2]); // external page register

  outportb_p(0x0c, 0xff);   // reset flip-flop
  outportb_p(0x05, c.b[0]); //  - count low byte
  outportb_p(0x05, c.b[1]); //  - count high byte

  outportb_p(0x0b, mode);   // set mode (see above)

  outportb_p(0x0a, 0x02);   // unmask chan 2
}

// This monster does full cylinder (both tracks) transfer to
// the specified direction (since the difference is small).
//
// It retries (a lot of times) on all errors except write-protection
// which is normally caused by mechanical switch on the disk.
//
int floppy_do_track(int base, unsigned cyl, floppy_dir dir) {

  // transfer command, set below
  unsigned char cmd;

  // Read is MT:MF:SK:0:0:1:1:0, write MT:MF:0:0:1:0:1
  // where MT = multitrack, MF = MFM mode, SK = skip deleted
  //
  // Specify multitrack and MFM mode
  static const int flags = 0xC0;
  switch(dir) {
  case floppy_dir_read:
    cmd = CMD_READ_DATA | flags;
    break;
  case floppy_dir_write:
    cmd = CMD_WRITE_DATA | flags;
    break;
  default:

    print_string("floppy_do_track: invalid direction");
    return 0; // not reached, but pleases "cmd used uninitialized"
  }

  // seek both heads
  if(floppy_seek(base, cyl, 0)) {
    return -1;
  }

  if(floppy_seek(base, cyl, 1)) {
    return -1;
  }

  int i;
  for(i = 0; i < 20; i++) {
    floppy_motor(base, floppy_motor_on);

    // init dma..
    floppy_dma_init(dir);

    sleep(100); // give some time (100ms) to settle after the seeks

    floppy_write_cmd(base, cmd);  // set above for current direction
    floppy_write_cmd(base, 0);    // 0:0:0:0:0:HD:US1:US0 = head and drive
    floppy_write_cmd(base, cyl);  // cylinder
    floppy_write_cmd(base, 0);    // first head (should match with above)
    floppy_write_cmd(base, 1);    // first sector, strangely counts from 1
    floppy_write_cmd(base, 2);    // bytes/sector, 128*2^x (x=2 -> 512)
    floppy_write_cmd(base, 18);   // number of tracks to operate on
    floppy_write_cmd(base, 0x1b); // GAP3 length, 27 is default for 3.5"
    floppy_write_cmd(base, 0xff); // data length (0xff if B/S != 0)

    irq_wait(floppy_irq); // don't SENSE_INTERRUPT here!

    // first read status information
    unsigned char st0, st1, st2, rcy, rhe, rse, bps;
    st0 = floppy_read_data(base);
    st1 = floppy_read_data(base);
    st2 = floppy_read_data(base);

    // These are cylinder/head/sector values, updated with some
    // rather bizarre logic, that I would like to understand.
    rcy = floppy_read_data(base);
    rhe = floppy_read_data(base);
    rse = floppy_read_data(base);
    // bytes per sector, should be what we programmed in
    bps = floppy_read_data(base);

    int error = 0;

    if(st0 & 0xC0) {
      static const char * status[] =
          { 0, "error", "invalid command", "drive not ready" };
      print_string("floppy_do_sector: status =");
      print_string(status[st0 >> 6]);
      print_string("\n");
      error = 1;
    }
    if(st1 & 0x80) {
      print_string("floppy_do_sector: end of cylinder\n");
      error = 1;
    }
    if(st0 & 0x08) {
      print_string("floppy_do_sector: drive not ready\n");
      error = 1;
    }
    if(st1 & 0x20) {
      print_string("floppy_do_sector: CRC error\n");
      error = 1;
    }
    if(st1 & 0x10) {
      print_string("floppy_do_sector: controller timeout\n");
      error = 1;
    }
    if(st1 & 0x04) {
      print_string("floppy_do_sector: no data found\n");
      error = 1;
    }
    if((st1|st2) & 0x01) {
      print_string("floppy_do_sector: no address mark found\n");
      error = 1;
    }
    if(st2 & 0x40) {
      print_string("floppy_do_sector: deleted address mark\n");
      error = 1;
    }
    if(st2 & 0x20) {
      print_string("floppy_do_sector: CRC error in data\n");
      error = 1;
    }
    if(st2 & 0x10) {
      print_string("floppy_do_sector: wrong cylinder\n");
      error = 1;
    }
    if(st2 & 0x04) {
      print_string("floppy_do_sector: uPD765 sector not found\n");
      error = 1;
    }
    if(st2 & 0x02) {
      print_string("floppy_do_sector: bad cylinder\n");
      error = 1;
    }
    if(bps != 0x2) {
      print_string("floppy_do_sector: wanted 512B/sector, got ");
      char temp[33] = {0};
      itoa((1<<(bps+7)), temp, 10);
      print_string(temp);
      print_string("\n");
      error = 1;
    }
    if(st1 & 0x02) {
      print_string("floppy_do_sector: not writable\n");
      error = 2;
    }

    if(!error) {
      floppy_motor(base, floppy_motor_off);
      return 0;
    }
    if(error > 1) {
      print_string("floppy_do_sector: not retrying..\n");
      floppy_motor(base, floppy_motor_off);
      return -2;
    }
  }

  print_string("floppy_do_sector: 20 retries exhausted\n");
  floppy_motor(base, floppy_motor_off);
  return -1;

}

void fdd_initialize() {
  print_string("Looking for floppy drives\n");
  outportb_p(0x70, 0x10);
  unsigned int drives = inportb_p(0x71);
  print_string("  Floppy drive 0: ");
  print_string((char*)drive_types[drives >> 4]);
  print_string("\n  Floppy drive 1: ");
  print_string((char*)drive_types[drives & 0xf]);
  print_string("\n");
}