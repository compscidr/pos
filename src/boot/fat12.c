#include "common.h"
#include "screen.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "task.h"

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

// todo: use more than 1 sector at a time in the buffer, make sure its aligned correctly and doesn't cross a
//  64k boundary. It can be up to 64k-1 in size for ISA DMA. In the example here, they use 0x4800 bytes, which is a full
//  track of 18 sectors. The track size is determined by the geometry of the disk, and the sectors per track, ie)
//  2 * 18 * 512 = 0x4800 = 18432. https://forum.osdev.org/viewtopic.php?t=13538
// todo: we may be able to relocate this to a fixed position so that it doesn't make the binary larger than it needs
//  to be: https://stackoverflow.com/questions/4067811/how-to-place-a-variable-at-a-given-absolute-address-in-memory-with-gcc
#define floppy_dmalen 0x200
static const char floppy_dmabuf[floppy_dmalen];

// can't actually implement the init function here because main needs to be the first function
void fdd_initialize();

/**
 * This is the stage2 fat12 loader which will locate the kernel.bin file and load it
 * to 0x00100000 and then jump to it.
 */
int main(void) {
    /* Global Descriptor Table - Sets up Rings, Flat memory segments
     * (ie, 4GB available for each ring) */
    gdt_init();

    screen_init();
    screen_clear();

    print_string("Setting Up Interrupts...");
    interrupt_init();
    print_status(1);

    /* Sets up the thread structures so we can do context switches */
    print_string("Initializing Threads & System Timer...");
    thread_init();
    print_status(1);

    /* Enable interrupts */
    asm volatile ("sti");
    fdd_initialize();

    while (1) {
        __asm__("hlt");
    }
}

// Used by floppy_dma_init and floppy_do_track to specify direction
typedef enum {
    floppy_dir_read = 1,
    floppy_dir_write = 2
} floppy_dir;

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
    char temp[33] = {0};
    itoa(base + FLOPPY_DOR, temp, 16);
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
    return 0;
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

/**
 * Read or write a sector from the floppy.
 * @param base the base address of the floppy drive
 * @param cyl the cylinder number
 * @param head the head number
 * @param sect the sector number
 * @param dir read or write
 * @return 0 on success, -1 on error
 */
int floppy_do_sector(int base, unsigned cyl, unsigned head, unsigned sect, floppy_dir dir) {
    // transfer command, set below
    unsigned char cmd;
    int drive = 0; // we only support the first drive here

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
            return -1;
    }

    // seek to the correct cylinder and head
    if(floppy_seek(base, cyl, head)) {
        return -1;
    }

    int i;
    for(i = 0; i < 20; i++) {
        floppy_motor(base, floppy_motor_on);

        // init dma..
        floppy_dma_init(dir);

        sleep(100); // give some time (100ms) to settle after the seeks

        floppy_write_cmd(base, cmd);  // set above for current direction
        floppy_write_cmd(base, (head << 2) | drive);    // 0:0:0:0:0:HD:US1:US0 = head and drive
        floppy_write_cmd(base, cyl);  // cylinder
        floppy_write_cmd(base, head);    // first head (should match with above)
        floppy_write_cmd(base, sect);    // first sector, strangely counts from 1
        floppy_write_cmd(base, 2);    // bytes/sector, 128*2^x (x=2 -> 512)
        floppy_write_cmd(base, 1);   // number of tracks to operate on
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

struct boot_sector {
    char ignore[3]; // ignore until we need it
    char oem[8];
    unsigned short sector_size;
    unsigned char sectors_per_cluster;
    unsigned short reserved_sectors;
    unsigned char number_of_fats;
    unsigned short root_dir_entries;
    unsigned short total_sectors_short; // if zero, later field is used
    unsigned char media_descriptor;
    unsigned short sectors_per_fat;
    unsigned short sectors_per_track;
    unsigned short number_of_heads;
    unsigned int hidden_sectors;
    unsigned int total_sectors_long;

    // extended BIOS parameter block
    unsigned char drive_number;
    unsigned char _flags;
    unsigned char signature;
    unsigned int volume_id;
    char volume_label[11];
    char fs_type[8];
    char boot_code[448];
    unsigned short boot_sector_signature;
} __attribute__((packed));

void debug_mbr(struct boot_sector *boot_sector) {
    print_string("OEM: ");
    print_string(boot_sector->oem);
    print_string("\n");
    print_string("Sector size: ");
    char temp[33] = {0};
    itoa(boot_sector->sector_size, temp, 10);
    print_string(temp);
    print_string("\n");
    print_string("Sectors per cluster: ");
    itoa(boot_sector->sectors_per_cluster, temp, 10);
    print_string(temp);
    print_string("\n");
    print_string("Reserved sectors: ");
    itoa(boot_sector->reserved_sectors, temp, 10);
    print_string(temp);
    print_string("\n");
    print_string("Number of FATs: ");
    itoa(boot_sector->number_of_fats, temp, 10);
    print_string(temp);
    print_string("\n");
    print_string("Root directory entries: ");
    itoa(boot_sector->root_dir_entries, temp, 10);
    print_string(temp);
    print_string("\n");
    print_string("Total sectors short: ");
    itoa(boot_sector->total_sectors_short, temp, 10);
    print_string(temp);
    print_string("\n");
    print_string("Media descriptor: ");
    itoa(boot_sector->media_descriptor, temp, 10);
    print_string(temp);
    print_string("\n");
    print_string("Sectors per FAT: ");
    itoa(boot_sector->sectors_per_fat, temp, 10);
    print_string(temp);
    print_string("\n");
    print_string("Sectors per track: ");
    itoa(boot_sector->sectors_per_track, temp, 10);
    print_string(temp);
    print_string("\n");
    print_string("Number of heads: ");
    itoa(boot_sector->number_of_heads, temp, 10);
    print_string(temp);
    print_string("\n");
    print_string("Hidden sectors: ");
    itoa(boot_sector->hidden_sectors, temp, 10);
    print_string(temp);
    print_string("\n");
    print_string("Total sectors long: ");
    itoa(boot_sector->total_sectors_long, temp, 10);
    print_string(temp);
    print_string("\n");
    print_string("Drive number: ");
    itoa(boot_sector->drive_number, temp, 10);
    print_string(temp);
    print_string("\n");
    print_string("Flags: ");
    itoa(boot_sector->_flags, temp, 10);
    print_string(temp);
    print_string("\n");
    print_string("Signature: ");
    itoa(boot_sector->signature, temp, 10);
    print_string(temp);
    print_string("\n");
    print_string("Volume ID: ");
    itoa(boot_sector->volume_id, temp, 10);
    print_string(temp);
    print_string("\n");
    print_string("Volume label: ");
    print_n_string(boot_sector->volume_label, 11);
    print_string("\n");
    print_string("FS type: ");
    print_n_string(boot_sector->fs_type, 8);
    print_string("\n");
    print_string("Boot sector signature: ");
    itoa(boot_sector->boot_sector_signature, temp, 10);
    print_string(temp);
    print_string("\n");
}

struct chs {
    unsigned char head;
    unsigned char sector;
    unsigned char cylinder;
} __attribute__((packed));

struct chs convert_lba_to_chs(unsigned int lba) {
    struct chs chs;
    chs.head = (lba % (18 * 2)) / 18;
    chs.sector = (lba % 18) + 1;
    chs.cylinder = lba / (18 * 2);
    return chs;
}

struct fat_directory_entry {
    char filename[8];
    char extension[3];
    unsigned char attributes;
    unsigned char reserved;
    unsigned char creation_time_tenths;
    unsigned short creation_time;
    unsigned short creation_date;
    unsigned short last_access_date;
    unsigned short first_cluster_high;
    unsigned short last_modification_time;
    unsigned short last_modification_date;
    unsigned short first_cluster_low;
    unsigned int file_size;
} __attribute__((packed));

struct fat_directory_sector {
    struct fat_directory_entry entries[16];
} __attribute__((packed));

int load_sector_and_copy_to_memory(int base, unsigned int lba, void *memory) {
    struct chs chs = convert_lba_to_chs(lba);
    int result = floppy_do_sector(base, chs.cylinder, chs.head, chs.sector, floppy_dir_read);
    if (result != 0) {
        return result;
    }
    memcpy(memory, (void*)floppy_dmabuf, floppy_dmalen);
    return 0;
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

    print_string("Resetting floppy controller");
    int reset_result = floppy_reset(floppy_base);
    print_status(reset_result == 0);

//    print_string("Reading the MBR from the floppy");
//    int read_result = floppy_do_sector(floppy_base, 0, 0, 1, floppy_dir_read);
//    print_status(read_result == 0);
//
//    struct boot_sector *boot_sector = (struct boot_sector*)floppy_dmabuf;
//    debug_mbr(boot_sector);

    // read the root directory
    struct chs chs = convert_lba_to_chs(19);
    print_string("Reading the Root Directory from the floppy");
    int root_dir_result = floppy_do_sector(floppy_base, chs.cylinder, chs.head, chs.sector, floppy_dir_read);
    print_status(root_dir_result == 0);

    struct fat_directory_sector *root_dir = (struct fat_directory_sector*)floppy_dmabuf;
    int kernel_index = -1;
    for (int i = 0; i < 16; i++) {
        if (root_dir->entries[i].filename[0] == 0x00) {
            break;
        }
        if (root_dir->entries[i].filename[0] == 0xE5) {
            continue;
        }
        print_string("  ");
        print_n_string(root_dir->entries[i].filename, 8);
        print_string(".");
        print_n_string(root_dir->entries[i].extension, 3);
        print_string("\n");

        if ((strncmp("KERNEL  ", root_dir->entries[i].filename, 8) == 0) && (strncmp("BIN", root_dir->entries[i].extension, 3) == 0)) {
            kernel_index = i;
        }
    }

    if (kernel_index != -1) {
        print_string("Found the kernel file\n");
        char temp[33] = {0};
        itoa(root_dir->entries[kernel_index].file_size, temp, 10);
        print_string("  File size: ");
        print_string(temp);
        print_string(" bytes\n");
        itoa(root_dir->entries[kernel_index].first_cluster_low, temp, 10);
        print_string("  First cluster: ");
        print_string(temp);
        print_string("\n");

        //int destination = 0x00100000;
        //load_sector_and_copy_to_memory(floppy_base, root_dir->entries[kernel_index].first_cluster_low, (void*)destination);

        print_string("Reading the first cluster of the kernel file...");
        struct chs first_chs = convert_lba_to_chs(root_dir->entries[kernel_index].first_cluster_low);
        int first_cluster_result = floppy_do_sector(floppy_base, first_chs.cylinder, first_chs.head, first_chs.sector, floppy_dir_read);
        print_status(first_cluster_result == 0);
    } else {
        print_string("Kernel file not found\n");
    }



    // todo, parse the FAT, find the kernel.bin file, load it to 0x00100000, and jump to it
}