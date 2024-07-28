#ifndef SCREEN_H
#define SCREEN_H

void screen_init(void);
void screen_clear(void);
void print_address(unsigned long addr);
void print_char(unsigned char c);
void print_string(char * string);
void print_n_string(char * string, int n);
void print_string_at(char * text, int x, int y);
void print_string_atx(char * text, int x);
void print_status(unsigned char status);
void settextcolor(unsigned char forecolor, unsigned char backcolor);
void hd(unsigned long int start_location, unsigned long int end_location);

#define VGABLACK		0x0
#define VGABLUE			0x1
#define VGAGREEN		0x2
#define VGACYAN			0x3
#define VGARED			0x4
#define VGAPURPLE		0x5
#define VGAORANGE		0x6
#define VGAWHITE		0x7
#define VGAGREY			0x8
#define VGALIGHTBLUE	0x9
#define VGALIGHTGREEN	0xA
#define VGALIGHTCYAN	0xB
#define VGALIGHTRED		0xC
#define VGALIGHTPURPLE	0xD
#define VGAYELLOW		0xE
#define VGABRIGHTWHITE	0xF

#endif
