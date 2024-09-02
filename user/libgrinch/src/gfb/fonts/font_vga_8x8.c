/* Font definitions (GPLv2) are copied and converted from the Linux kernel */

#include <grinch/gfb/font.h>

const struct gfont font_vga_8x8 = {
	.width = 8,
	.height = 8,
	.charcount = 256,
	.name = "font_vga_8x8",
	.data = {
		/* 0 0x00 ' ' */
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		/* 1 0x01 ' ' */
		0x007e,
		0x0081,
		0x00a5,
		0x0081,
		0x00bd,
		0x0099,
		0x0081,
		0x007e,
		/* 2 0x02 ' ' */
		0x007e,
		0x00ff,
		0x00db,
		0x00ff,
		0x00c3,
		0x00e7,
		0x00ff,
		0x007e,
		/* 3 0x03 ' ' */
		0x0036,
		0x007f,
		0x007f,
		0x007f,
		0x003e,
		0x001c,
		0x0008,
		0x0000,
		/* 4 0x04 ' ' */
		0x0008,
		0x001c,
		0x003e,
		0x007f,
		0x003e,
		0x001c,
		0x0008,
		0x0000,
		/* 5 0x05 ' ' */
		0x001c,
		0x003e,
		0x001c,
		0x007f,
		0x007f,
		0x006b,
		0x0008,
		0x001c,
		/* 6 0x06 ' ' */
		0x0008,
		0x001c,
		0x003e,
		0x007f,
		0x007f,
		0x003e,
		0x0008,
		0x001c,
		/* 7 0x07 ' ' */
		0x0000,
		0x0000,
		0x0018,
		0x003c,
		0x003c,
		0x0018,
		0x0000,
		0x0000,
		/* 8 0x08 ' ' */
		0x00ff,
		0x00ff,
		0x00e7,
		0x00c3,
		0x00c3,
		0x00e7,
		0x00ff,
		0x00ff,
		/* 9 0x09 ' ' */
		0x0000,
		0x003c,
		0x0066,
		0x0042,
		0x0042,
		0x0066,
		0x003c,
		0x0000,
		/* 10 0x0a ' ' */
		0x00ff,
		0x00c3,
		0x0099,
		0x00bd,
		0x00bd,
		0x0099,
		0x00c3,
		0x00ff,
		/* 11 0x0b ' ' */
		0x00f0,
		0x00e0,
		0x00f0,
		0x00be,
		0x0033,
		0x0033,
		0x0033,
		0x001e,
		/* 12 0x0c ' ' */
		0x003c,
		0x0066,
		0x0066,
		0x0066,
		0x003c,
		0x0018,
		0x007e,
		0x0018,
		/* 13 0x0d ' ' */
		0x00fc,
		0x00cc,
		0x00fc,
		0x000c,
		0x000c,
		0x000e,
		0x000f,
		0x0007,
		/* 14 0x0e ' ' */
		0x00fe,
		0x00c6,
		0x00fe,
		0x00c6,
		0x00c6,
		0x00e6,
		0x0067,
		0x0003,
		/* 15 0x0f ' ' */
		0x0018,
		0x00db,
		0x003c,
		0x00e7,
		0x00e7,
		0x003c,
		0x00db,
		0x0018,
		/* 16 0x10 ' ' */
		0x0001,
		0x0007,
		0x001f,
		0x007f,
		0x001f,
		0x0007,
		0x0001,
		0x0000,
		/* 17 0x11 ' ' */
		0x0040,
		0x0070,
		0x007c,
		0x007f,
		0x007c,
		0x0070,
		0x0040,
		0x0000,
		/* 18 0x12 ' ' */
		0x0018,
		0x003c,
		0x007e,
		0x0018,
		0x0018,
		0x007e,
		0x003c,
		0x0018,
		/* 19 0x13 ' ' */
		0x0066,
		0x0066,
		0x0066,
		0x0066,
		0x0066,
		0x0000,
		0x0066,
		0x0000,
		/* 20 0x14 ' ' */
		0x00fe,
		0x00db,
		0x00db,
		0x00de,
		0x00d8,
		0x00d8,
		0x00d8,
		0x0000,
		/* 21 0x15 ' ' */
		0x007c,
		0x0086,
		0x003c,
		0x0066,
		0x0066,
		0x003c,
		0x0061,
		0x003e,
		/* 22 0x16 ' ' */
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x007e,
		0x007e,
		0x007e,
		0x0000,
		/* 23 0x17 ' ' */
		0x0018,
		0x003c,
		0x007e,
		0x0018,
		0x007e,
		0x003c,
		0x0018,
		0x00ff,
		/* 24 0x18 ' ' */
		0x0018,
		0x003c,
		0x007e,
		0x0018,
		0x0018,
		0x0018,
		0x0018,
		0x0000,
		/* 25 0x19 ' ' */
		0x0018,
		0x0018,
		0x0018,
		0x0018,
		0x007e,
		0x003c,
		0x0018,
		0x0000,
		/* 26 0x1a ' ' */
		0x0000,
		0x0018,
		0x0030,
		0x007f,
		0x0030,
		0x0018,
		0x0000,
		0x0000,
		/* 27 0x1b ' ' */
		0x0000,
		0x000c,
		0x0006,
		0x007f,
		0x0006,
		0x000c,
		0x0000,
		0x0000,
		/* 28 0x1c ' ' */
		0x0000,
		0x0000,
		0x0003,
		0x0003,
		0x0003,
		0x007f,
		0x0000,
		0x0000,
		/* 29 0x1d ' ' */
		0x0000,
		0x0024,
		0x0066,
		0x00ff,
		0x0066,
		0x0024,
		0x0000,
		0x0000,
		/* 30 0x1e ' ' */
		0x0000,
		0x0018,
		0x003c,
		0x007e,
		0x00ff,
		0x00ff,
		0x0000,
		0x0000,
		/* 31 0x1f ' ' */
		0x0000,
		0x00ff,
		0x00ff,
		0x007e,
		0x003c,
		0x0018,
		0x0000,
		0x0000,
		/* 32 0x20 ' ' */
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		/* 33 0x21 '!' */
		0x0018,
		0x003c,
		0x003c,
		0x0018,
		0x0018,
		0x0000,
		0x0018,
		0x0000,
		/* 34 0x22 '"' */
		0x0066,
		0x0066,
		0x0024,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		/* 35 0x23 '#' */
		0x0036,
		0x0036,
		0x007f,
		0x0036,
		0x007f,
		0x0036,
		0x0036,
		0x0000,
		/* 36 0x24 '$' */
		0x0018,
		0x007c,
		0x0006,
		0x003c,
		0x0060,
		0x003e,
		0x0018,
		0x0000,
		/* 37 0x25 '%' */
		0x0000,
		0x0063,
		0x0033,
		0x0018,
		0x000c,
		0x0066,
		0x0063,
		0x0000,
		/* 38 0x26 '&' */
		0x001c,
		0x0036,
		0x001c,
		0x006e,
		0x003b,
		0x0033,
		0x006e,
		0x0000,
		/* 39 0x27 ''' */
		0x0018,
		0x0018,
		0x000c,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		/* 40 0x28 '(' */
		0x0030,
		0x0018,
		0x000c,
		0x000c,
		0x000c,
		0x0018,
		0x0030,
		0x0000,
		/* 41 0x29 ')' */
		0x000c,
		0x0018,
		0x0030,
		0x0030,
		0x0030,
		0x0018,
		0x000c,
		0x0000,
		/* 42 0x2a '*' */
		0x0000,
		0x0066,
		0x003c,
		0x00ff,
		0x003c,
		0x0066,
		0x0000,
		0x0000,
		/* 43 0x2b '+' */
		0x0000,
		0x0018,
		0x0018,
		0x007e,
		0x0018,
		0x0018,
		0x0000,
		0x0000,
		/* 44 0x2c ',' */
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0018,
		0x0018,
		0x000c,
		/* 45 0x2d '-' */
		0x0000,
		0x0000,
		0x0000,
		0x007e,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		/* 46 0x2e '.' */
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0018,
		0x0018,
		0x0000,
		/* 47 0x2f '/' */
		0x0060,
		0x0030,
		0x0018,
		0x000c,
		0x0006,
		0x0003,
		0x0001,
		0x0000,
		/* 48 0x30 '0' */
		0x001c,
		0x0036,
		0x0063,
		0x006b,
		0x0063,
		0x0036,
		0x001c,
		0x0000,
		/* 49 0x31 '1' */
		0x0018,
		0x001c,
		0x0018,
		0x0018,
		0x0018,
		0x0018,
		0x007e,
		0x0000,
		/* 50 0x32 '2' */
		0x003e,
		0x0063,
		0x0060,
		0x0038,
		0x000c,
		0x0066,
		0x007f,
		0x0000,
		/* 51 0x33 '3' */
		0x003e,
		0x0063,
		0x0060,
		0x003c,
		0x0060,
		0x0063,
		0x003e,
		0x0000,
		/* 52 0x34 '4' */
		0x0038,
		0x003c,
		0x0036,
		0x0033,
		0x007f,
		0x0030,
		0x0078,
		0x0000,
		/* 53 0x35 '5' */
		0x007f,
		0x0003,
		0x0003,
		0x003f,
		0x0060,
		0x0063,
		0x003e,
		0x0000,
		/* 54 0x36 '6' */
		0x001c,
		0x0006,
		0x0003,
		0x003f,
		0x0063,
		0x0063,
		0x003e,
		0x0000,
		/* 55 0x37 '7' */
		0x007f,
		0x0063,
		0x0030,
		0x0018,
		0x000c,
		0x000c,
		0x000c,
		0x0000,
		/* 56 0x38 '8' */
		0x003e,
		0x0063,
		0x0063,
		0x003e,
		0x0063,
		0x0063,
		0x003e,
		0x0000,
		/* 57 0x39 '9' */
		0x003e,
		0x0063,
		0x0063,
		0x007e,
		0x0060,
		0x0030,
		0x001e,
		0x0000,
		/* 58 0x3a ':' */
		0x0000,
		0x0018,
		0x0018,
		0x0000,
		0x0000,
		0x0018,
		0x0018,
		0x0000,
		/* 59 0x3b ';' */
		0x0000,
		0x0018,
		0x0018,
		0x0000,
		0x0000,
		0x0018,
		0x0018,
		0x000c,
		/* 60 0x3c '<' */
		0x0060,
		0x0030,
		0x0018,
		0x000c,
		0x0018,
		0x0030,
		0x0060,
		0x0000,
		/* 61 0x3d '=' */
		0x0000,
		0x0000,
		0x007e,
		0x0000,
		0x0000,
		0x007e,
		0x0000,
		0x0000,
		/* 62 0x3e '>' */
		0x0006,
		0x000c,
		0x0018,
		0x0030,
		0x0018,
		0x000c,
		0x0006,
		0x0000,
		/* 63 0x3f '?' */
		0x003e,
		0x0063,
		0x0030,
		0x0018,
		0x0018,
		0x0000,
		0x0018,
		0x0000,
		/* 64 0x40 '@' */
		0x003e,
		0x0063,
		0x007b,
		0x007b,
		0x007b,
		0x0003,
		0x001e,
		0x0000,
		/* 65 0x41 'A' */
		0x001c,
		0x0036,
		0x0063,
		0x007f,
		0x0063,
		0x0063,
		0x0063,
		0x0000,
		/* 66 0x42 'B' */
		0x003f,
		0x0066,
		0x0066,
		0x003e,
		0x0066,
		0x0066,
		0x003f,
		0x0000,
		/* 67 0x43 'C' */
		0x003c,
		0x0066,
		0x0003,
		0x0003,
		0x0003,
		0x0066,
		0x003c,
		0x0000,
		/* 68 0x44 'D' */
		0x001f,
		0x0036,
		0x0066,
		0x0066,
		0x0066,
		0x0036,
		0x001f,
		0x0000,
		/* 69 0x45 'E' */
		0x007f,
		0x0046,
		0x0016,
		0x001e,
		0x0016,
		0x0046,
		0x007f,
		0x0000,
		/* 70 0x46 'F' */
		0x007f,
		0x0046,
		0x0016,
		0x001e,
		0x0016,
		0x0006,
		0x000f,
		0x0000,
		/* 71 0x47 'G' */
		0x003c,
		0x0066,
		0x0003,
		0x0003,
		0x0073,
		0x0066,
		0x005c,
		0x0000,
		/* 72 0x48 'H' */
		0x0063,
		0x0063,
		0x0063,
		0x007f,
		0x0063,
		0x0063,
		0x0063,
		0x0000,
		/* 73 0x49 'I' */
		0x003c,
		0x0018,
		0x0018,
		0x0018,
		0x0018,
		0x0018,
		0x003c,
		0x0000,
		/* 74 0x4a 'J' */
		0x0078,
		0x0030,
		0x0030,
		0x0030,
		0x0033,
		0x0033,
		0x001e,
		0x0000,
		/* 75 0x4b 'K' */
		0x0067,
		0x0066,
		0x0036,
		0x001e,
		0x0036,
		0x0066,
		0x0067,
		0x0000,
		/* 76 0x4c 'L' */
		0x000f,
		0x0006,
		0x0006,
		0x0006,
		0x0046,
		0x0066,
		0x007f,
		0x0000,
		/* 77 0x4d 'M' */
		0x0063,
		0x0077,
		0x007f,
		0x007f,
		0x006b,
		0x0063,
		0x0063,
		0x0000,
		/* 78 0x4e 'N' */
		0x0063,
		0x0067,
		0x006f,
		0x007b,
		0x0073,
		0x0063,
		0x0063,
		0x0000,
		/* 79 0x4f 'O' */
		0x003e,
		0x0063,
		0x0063,
		0x0063,
		0x0063,
		0x0063,
		0x003e,
		0x0000,
		/* 80 0x50 'P' */
		0x003f,
		0x0066,
		0x0066,
		0x003e,
		0x0006,
		0x0006,
		0x000f,
		0x0000,
		/* 81 0x51 'Q' */
		0x003e,
		0x0063,
		0x0063,
		0x0063,
		0x0063,
		0x0073,
		0x003e,
		0x0070,
		/* 82 0x52 'R' */
		0x003f,
		0x0066,
		0x0066,
		0x003e,
		0x0036,
		0x0066,
		0x0067,
		0x0000,
		/* 83 0x53 'S' */
		0x003c,
		0x0066,
		0x000c,
		0x0018,
		0x0030,
		0x0066,
		0x003c,
		0x0000,
		/* 84 0x54 'T' */
		0x007e,
		0x007e,
		0x005a,
		0x0018,
		0x0018,
		0x0018,
		0x003c,
		0x0000,
		/* 85 0x55 'U' */
		0x0063,
		0x0063,
		0x0063,
		0x0063,
		0x0063,
		0x0063,
		0x003e,
		0x0000,
		/* 86 0x56 'V' */
		0x0063,
		0x0063,
		0x0063,
		0x0063,
		0x0063,
		0x0036,
		0x001c,
		0x0000,
		/* 87 0x57 'W' */
		0x0063,
		0x0063,
		0x0063,
		0x006b,
		0x006b,
		0x007f,
		0x0036,
		0x0000,
		/* 88 0x58 'X' */
		0x0063,
		0x0063,
		0x0036,
		0x001c,
		0x0036,
		0x0063,
		0x0063,
		0x0000,
		/* 89 0x59 'Y' */
		0x0066,
		0x0066,
		0x0066,
		0x003c,
		0x0018,
		0x0018,
		0x003c,
		0x0000,
		/* 90 0x5a 'Z' */
		0x007f,
		0x0063,
		0x0031,
		0x0018,
		0x004c,
		0x0066,
		0x007f,
		0x0000,
		/* 91 0x5b '[' */
		0x003c,
		0x000c,
		0x000c,
		0x000c,
		0x000c,
		0x000c,
		0x003c,
		0x0000,
		/* 92 0x5c '\' */
		0x0003,
		0x0006,
		0x000c,
		0x0018,
		0x0030,
		0x0060,
		0x0040,
		0x0000,
		/* 93 0x5d ']' */
		0x003c,
		0x0030,
		0x0030,
		0x0030,
		0x0030,
		0x0030,
		0x003c,
		0x0000,
		/* 94 0x5e '^' */
		0x0008,
		0x001c,
		0x0036,
		0x0063,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		/* 95 0x5f '_' */
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x00ff,
		/* 96 0x60 '`' */
		0x000c,
		0x0018,
		0x0030,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		/* 97 0x61 'a' */
		0x0000,
		0x0000,
		0x001e,
		0x0030,
		0x003e,
		0x0033,
		0x006e,
		0x0000,
		/* 98 0x62 'b' */
		0x0007,
		0x0006,
		0x003e,
		0x0066,
		0x0066,
		0x0066,
		0x003b,
		0x0000,
		/* 99 0x63 'c' */
		0x0000,
		0x0000,
		0x003e,
		0x0063,
		0x0003,
		0x0063,
		0x003e,
		0x0000,
		/* 100 0x64 'd' */
		0x0038,
		0x0030,
		0x003e,
		0x0033,
		0x0033,
		0x0033,
		0x006e,
		0x0000,
		/* 101 0x65 'e' */
		0x0000,
		0x0000,
		0x003e,
		0x0063,
		0x007f,
		0x0003,
		0x003e,
		0x0000,
		/* 102 0x66 'f' */
		0x003c,
		0x0066,
		0x0006,
		0x001f,
		0x0006,
		0x0006,
		0x000f,
		0x0000,
		/* 103 0x67 'g' */
		0x0000,
		0x0000,
		0x006e,
		0x0033,
		0x0033,
		0x003e,
		0x0030,
		0x001f,
		/* 104 0x68 'h' */
		0x0007,
		0x0006,
		0x0036,
		0x006e,
		0x0066,
		0x0066,
		0x0067,
		0x0000,
		/* 105 0x69 'i' */
		0x0018,
		0x0000,
		0x001c,
		0x0018,
		0x0018,
		0x0018,
		0x003c,
		0x0000,
		/* 106 0x6a 'j' */
		0x0060,
		0x0000,
		0x0060,
		0x0060,
		0x0060,
		0x0066,
		0x0066,
		0x003c,
		/* 107 0x6b 'k' */
		0x0007,
		0x0006,
		0x0066,
		0x0036,
		0x001e,
		0x0036,
		0x0067,
		0x0000,
		/* 108 0x6c 'l' */
		0x001c,
		0x0018,
		0x0018,
		0x0018,
		0x0018,
		0x0018,
		0x003c,
		0x0000,
		/* 109 0x6d 'm' */
		0x0000,
		0x0000,
		0x0037,
		0x007f,
		0x006b,
		0x006b,
		0x006b,
		0x0000,
		/* 110 0x6e 'n' */
		0x0000,
		0x0000,
		0x003b,
		0x0066,
		0x0066,
		0x0066,
		0x0066,
		0x0000,
		/* 111 0x6f 'o' */
		0x0000,
		0x0000,
		0x003e,
		0x0063,
		0x0063,
		0x0063,
		0x003e,
		0x0000,
		/* 112 0x70 'p' */
		0x0000,
		0x0000,
		0x003b,
		0x0066,
		0x0066,
		0x003e,
		0x0006,
		0x000f,
		/* 113 0x71 'q' */
		0x0000,
		0x0000,
		0x006e,
		0x0033,
		0x0033,
		0x003e,
		0x0030,
		0x0078,
		/* 114 0x72 'r' */
		0x0000,
		0x0000,
		0x003b,
		0x006e,
		0x0006,
		0x0006,
		0x000f,
		0x0000,
		/* 115 0x73 's' */
		0x0000,
		0x0000,
		0x007e,
		0x0003,
		0x003e,
		0x0060,
		0x003f,
		0x0000,
		/* 116 0x74 't' */
		0x000c,
		0x000c,
		0x003f,
		0x000c,
		0x000c,
		0x006c,
		0x0038,
		0x0000,
		/* 117 0x75 'u' */
		0x0000,
		0x0000,
		0x0033,
		0x0033,
		0x0033,
		0x0033,
		0x006e,
		0x0000,
		/* 118 0x76 'v' */
		0x0000,
		0x0000,
		0x0063,
		0x0063,
		0x0063,
		0x0036,
		0x001c,
		0x0000,
		/* 119 0x77 'w' */
		0x0000,
		0x0000,
		0x0063,
		0x006b,
		0x006b,
		0x007f,
		0x0036,
		0x0000,
		/* 120 0x78 'x' */
		0x0000,
		0x0000,
		0x0063,
		0x0036,
		0x001c,
		0x0036,
		0x0063,
		0x0000,
		/* 121 0x79 'y' */
		0x0000,
		0x0000,
		0x0063,
		0x0063,
		0x0063,
		0x007e,
		0x0060,
		0x003f,
		/* 122 0x7a 'z' */
		0x0000,
		0x0000,
		0x007e,
		0x0032,
		0x0018,
		0x004c,
		0x007e,
		0x0000,
		/* 123 0x7b '{' */
		0x0070,
		0x0018,
		0x0018,
		0x000e,
		0x0018,
		0x0018,
		0x0070,
		0x0000,
		/* 124 0x7c '|' */
		0x0018,
		0x0018,
		0x0018,
		0x0018,
		0x0018,
		0x0018,
		0x0018,
		0x0000,
		/* 125 0x7d '}' */
		0x000e,
		0x0018,
		0x0018,
		0x0070,
		0x0018,
		0x0018,
		0x000e,
		0x0000,
		/* 126 0x7e '~' */
		0x006e,
		0x003b,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		/* 127 0x7f ' ' */
		0x0000,
		0x0008,
		0x001c,
		0x0036,
		0x0063,
		0x0063,
		0x007f,
		0x0000,
		/* 128 0x80 ' ' */
		0x003e,
		0x0063,
		0x0003,
		0x0003,
		0x0063,
		0x003e,
		0x0030,
		0x001e,
		/* 129 0x81 ' ' */
		0x0033,
		0x0000,
		0x0033,
		0x0033,
		0x0033,
		0x0033,
		0x006e,
		0x0000,
		/* 130 0x82 ' ' */
		0x0030,
		0x0018,
		0x003e,
		0x0063,
		0x007f,
		0x0003,
		0x003e,
		0x0000,
		/* 131 0x83 ' ' */
		0x003e,
		0x0041,
		0x001e,
		0x0030,
		0x003e,
		0x0033,
		0x006e,
		0x0000,
		/* 132 0x84 ' ' */
		0x0063,
		0x0000,
		0x001e,
		0x0030,
		0x003e,
		0x0033,
		0x006e,
		0x0000,
		/* 133 0x85 ' ' */
		0x000c,
		0x0018,
		0x001e,
		0x0030,
		0x003e,
		0x0033,
		0x006e,
		0x0000,
		/* 134 0x86 ' ' */
		0x000c,
		0x000c,
		0x001e,
		0x0030,
		0x003e,
		0x0033,
		0x006e,
		0x0000,
		/* 135 0x87 ' ' */
		0x0000,
		0x0000,
		0x007e,
		0x0003,
		0x0003,
		0x007e,
		0x0030,
		0x001c,
		/* 136 0x88 ' ' */
		0x003e,
		0x0041,
		0x003e,
		0x0063,
		0x007f,
		0x0003,
		0x003e,
		0x0000,
		/* 137 0x89 ' ' */
		0x0063,
		0x0000,
		0x003e,
		0x0063,
		0x007f,
		0x0003,
		0x003e,
		0x0000,
		/* 138 0x8a ' ' */
		0x000c,
		0x0018,
		0x003e,
		0x0063,
		0x007f,
		0x0003,
		0x003e,
		0x0000,
		/* 139 0x8b ' ' */
		0x0066,
		0x0000,
		0x001c,
		0x0018,
		0x0018,
		0x0018,
		0x003c,
		0x0000,
		/* 140 0x8c ' ' */
		0x003e,
		0x0041,
		0x001c,
		0x0018,
		0x0018,
		0x0018,
		0x003c,
		0x0000,
		/* 141 0x8d ' ' */
		0x000c,
		0x0018,
		0x0000,
		0x001c,
		0x0018,
		0x0018,
		0x003c,
		0x0000,
		/* 142 0x8e ' ' */
		0x0063,
		0x001c,
		0x0036,
		0x0063,
		0x007f,
		0x0063,
		0x0063,
		0x0000,
		/* 143 0x8f ' ' */
		0x001c,
		0x0036,
		0x003e,
		0x0063,
		0x007f,
		0x0063,
		0x0063,
		0x0000,
		/* 144 0x90 ' ' */
		0x0018,
		0x000c,
		0x007f,
		0x0003,
		0x001f,
		0x0003,
		0x007f,
		0x0000,
		/* 145 0x91 ' ' */
		0x0000,
		0x0000,
		0x007e,
		0x0018,
		0x007e,
		0x001b,
		0x007e,
		0x0000,
		/* 146 0x92 ' ' */
		0x007c,
		0x0036,
		0x0033,
		0x007f,
		0x0033,
		0x0033,
		0x0073,
		0x0000,
		/* 147 0x93 ' ' */
		0x003e,
		0x0041,
		0x003e,
		0x0063,
		0x0063,
		0x0063,
		0x003e,
		0x0000,
		/* 148 0x94 ' ' */
		0x0063,
		0x0000,
		0x003e,
		0x0063,
		0x0063,
		0x0063,
		0x003e,
		0x0000,
		/* 149 0x95 ' ' */
		0x000c,
		0x0018,
		0x003e,
		0x0063,
		0x0063,
		0x0063,
		0x003e,
		0x0000,
		/* 150 0x96 ' ' */
		0x001e,
		0x0021,
		0x0000,
		0x0033,
		0x0033,
		0x0033,
		0x006e,
		0x0000,
		/* 151 0x97 ' ' */
		0x0006,
		0x000c,
		0x0033,
		0x0033,
		0x0033,
		0x0033,
		0x006e,
		0x0000,
		/* 152 0x98 ' ' */
		0x0063,
		0x0000,
		0x0063,
		0x0063,
		0x0063,
		0x007e,
		0x0060,
		0x003f,
		/* 153 0x99 ' ' */
		0x0063,
		0x001c,
		0x0036,
		0x0063,
		0x0063,
		0x0036,
		0x001c,
		0x0000,
		/* 154 0x9a ' ' */
		0x0063,
		0x0000,
		0x0063,
		0x0063,
		0x0063,
		0x0063,
		0x003e,
		0x0000,
		/* 155 0x9b ' ' */
		0x0018,
		0x0018,
		0x007e,
		0x0003,
		0x0003,
		0x007e,
		0x0018,
		0x0018,
		/* 156 0x9c ' ' */
		0x001c,
		0x0036,
		0x0026,
		0x000f,
		0x0006,
		0x0066,
		0x003f,
		0x0000,
		/* 157 0x9d ' ' */
		0x0066,
		0x0066,
		0x003c,
		0x007e,
		0x0018,
		0x007e,
		0x0018,
		0x0018,
		/* 158 0x9e ' ' */
		0x001f,
		0x0033,
		0x0033,
		0x005f,
		0x0063,
		0x00f3,
		0x0063,
		0x00e3,
		/* 159 0x9f ' ' */
		0x0070,
		0x00d8,
		0x0018,
		0x003c,
		0x0018,
		0x001b,
		0x000e,
		0x0000,
		/* 160 0xa0 ' ' */
		0x0018,
		0x000c,
		0x001e,
		0x0030,
		0x003e,
		0x0033,
		0x006e,
		0x0000,
		/* 161 0xa1 ' ' */
		0x0030,
		0x0018,
		0x0000,
		0x001c,
		0x0018,
		0x0018,
		0x003c,
		0x0000,
		/* 162 0xa2 ' ' */
		0x0030,
		0x0018,
		0x003e,
		0x0063,
		0x0063,
		0x0063,
		0x003e,
		0x0000,
		/* 163 0xa3 ' ' */
		0x0018,
		0x000c,
		0x0033,
		0x0033,
		0x0033,
		0x0033,
		0x006e,
		0x0000,
		/* 164 0xa4 ' ' */
		0x006e,
		0x003b,
		0x0000,
		0x003b,
		0x0066,
		0x0066,
		0x0066,
		0x0000,
		/* 165 0xa5 ' ' */
		0x006e,
		0x003b,
		0x0000,
		0x0067,
		0x006f,
		0x007b,
		0x0073,
		0x0000,
		/* 166 0xa6 ' ' */
		0x003c,
		0x0036,
		0x0036,
		0x007c,
		0x0000,
		0x007e,
		0x0000,
		0x0000,
		/* 167 0xa7 ' ' */
		0x001c,
		0x0036,
		0x0036,
		0x001c,
		0x0000,
		0x003e,
		0x0000,
		0x0000,
		/* 168 0xa8 ' ' */
		0x0018,
		0x0000,
		0x0018,
		0x0018,
		0x000c,
		0x00c6,
		0x007c,
		0x0000,
		/* 169 0xa9 ' ' */
		0x0000,
		0x0000,
		0x0000,
		0x007f,
		0x0003,
		0x0003,
		0x0000,
		0x0000,
		/* 170 0xaa ' ' */
		0x0000,
		0x0000,
		0x0000,
		0x007f,
		0x0060,
		0x0060,
		0x0000,
		0x0000,
		/* 171 0xab ' ' */
		0x00c6,
		0x0067,
		0x0036,
		0x007e,
		0x00cc,
		0x0066,
		0x0033,
		0x00f0,
		/* 172 0xac ' ' */
		0x00c6,
		0x0067,
		0x0036,
		0x005e,
		0x006c,
		0x0056,
		0x00fb,
		0x0060,
		/* 173 0xad ' ' */
		0x0018,
		0x0000,
		0x0018,
		0x0018,
		0x003c,
		0x003c,
		0x0018,
		0x0000,
		/* 174 0xae ' ' */
		0x0000,
		0x00cc,
		0x0066,
		0x0033,
		0x0066,
		0x00cc,
		0x0000,
		0x0000,
		/* 175 0xaf ' ' */
		0x0000,
		0x0033,
		0x0066,
		0x00cc,
		0x0066,
		0x0033,
		0x0000,
		0x0000,
		/* 176 0xb0 ' ' */
		0x0044,
		0x0011,
		0x0044,
		0x0011,
		0x0044,
		0x0011,
		0x0044,
		0x0011,
		/* 177 0xb1 ' ' */
		0x00aa,
		0x0055,
		0x00aa,
		0x0055,
		0x00aa,
		0x0055,
		0x00aa,
		0x0055,
		/* 178 0xb2 ' ' */
		0x00ee,
		0x00bb,
		0x00ee,
		0x00bb,
		0x00ee,
		0x00bb,
		0x00ee,
		0x00bb,
		/* 179 0xb3 ' ' */
		0x0018,
		0x0018,
		0x0018,
		0x0018,
		0x0018,
		0x0018,
		0x0018,
		0x0018,
		/* 180 0xb4 ' ' */
		0x0018,
		0x0018,
		0x0018,
		0x0018,
		0x001f,
		0x0018,
		0x0018,
		0x0018,
		/* 181 0xb5 ' ' */
		0x0018,
		0x0018,
		0x001f,
		0x0018,
		0x001f,
		0x0018,
		0x0018,
		0x0018,
		/* 182 0xb6 ' ' */
		0x006c,
		0x006c,
		0x006c,
		0x006c,
		0x006f,
		0x006c,
		0x006c,
		0x006c,
		/* 183 0xb7 ' ' */
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x007f,
		0x006c,
		0x006c,
		0x006c,
		/* 184 0xb8 ' ' */
		0x0000,
		0x0000,
		0x001f,
		0x0018,
		0x001f,
		0x0018,
		0x0018,
		0x0018,
		/* 185 0xb9 ' ' */
		0x006c,
		0x006c,
		0x006f,
		0x0060,
		0x006f,
		0x006c,
		0x006c,
		0x006c,
		/* 186 0xba ' ' */
		0x006c,
		0x006c,
		0x006c,
		0x006c,
		0x006c,
		0x006c,
		0x006c,
		0x006c,
		/* 187 0xbb ' ' */
		0x0000,
		0x0000,
		0x007f,
		0x0060,
		0x006f,
		0x006c,
		0x006c,
		0x006c,
		/* 188 0xbc ' ' */
		0x006c,
		0x006c,
		0x006f,
		0x0060,
		0x007f,
		0x0000,
		0x0000,
		0x0000,
		/* 189 0xbd ' ' */
		0x006c,
		0x006c,
		0x006c,
		0x006c,
		0x007f,
		0x0000,
		0x0000,
		0x0000,
		/* 190 0xbe ' ' */
		0x0018,
		0x0018,
		0x001f,
		0x0018,
		0x001f,
		0x0000,
		0x0000,
		0x0000,
		/* 191 0xbf ' ' */
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x001f,
		0x0018,
		0x0018,
		0x0018,
		/* 192 0xc0 ' ' */
		0x0018,
		0x0018,
		0x0018,
		0x0018,
		0x00f8,
		0x0000,
		0x0000,
		0x0000,
		/* 193 0xc1 ' ' */
		0x0018,
		0x0018,
		0x0018,
		0x0018,
		0x00ff,
		0x0000,
		0x0000,
		0x0000,
		/* 194 0xc2 ' ' */
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x00ff,
		0x0018,
		0x0018,
		0x0018,
		/* 195 0xc3 ' ' */
		0x0018,
		0x0018,
		0x0018,
		0x0018,
		0x00f8,
		0x0018,
		0x0018,
		0x0018,
		/* 196 0xc4 ' ' */
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x00ff,
		0x0000,
		0x0000,
		0x0000,
		/* 197 0xc5 ' ' */
		0x0018,
		0x0018,
		0x0018,
		0x0018,
		0x00ff,
		0x0018,
		0x0018,
		0x0018,
		/* 198 0xc6 ' ' */
		0x0018,
		0x0018,
		0x00f8,
		0x0018,
		0x00f8,
		0x0018,
		0x0018,
		0x0018,
		/* 199 0xc7 ' ' */
		0x006c,
		0x006c,
		0x006c,
		0x006c,
		0x00ec,
		0x006c,
		0x006c,
		0x006c,
		/* 200 0xc8 ' ' */
		0x006c,
		0x006c,
		0x00ec,
		0x000c,
		0x00fc,
		0x0000,
		0x0000,
		0x0000,
		/* 201 0xc9 ' ' */
		0x0000,
		0x0000,
		0x00fc,
		0x000c,
		0x00ec,
		0x006c,
		0x006c,
		0x006c,
		/* 202 0xca ' ' */
		0x006c,
		0x006c,
		0x00ef,
		0x0000,
		0x00ff,
		0x0000,
		0x0000,
		0x0000,
		/* 203 0xcb ' ' */
		0x0000,
		0x0000,
		0x00ff,
		0x0000,
		0x00ef,
		0x006c,
		0x006c,
		0x006c,
		/* 204 0xcc ' ' */
		0x006c,
		0x006c,
		0x00ec,
		0x000c,
		0x00ec,
		0x006c,
		0x006c,
		0x006c,
		/* 205 0xcd ' ' */
		0x0000,
		0x0000,
		0x00ff,
		0x0000,
		0x00ff,
		0x0000,
		0x0000,
		0x0000,
		/* 206 0xce ' ' */
		0x006c,
		0x006c,
		0x00ef,
		0x0000,
		0x00ef,
		0x006c,
		0x006c,
		0x006c,
		/* 207 0xcf ' ' */
		0x0018,
		0x0018,
		0x00ff,
		0x0000,
		0x00ff,
		0x0000,
		0x0000,
		0x0000,
		/* 208 0xd0 ' ' */
		0x006c,
		0x006c,
		0x006c,
		0x006c,
		0x00ff,
		0x0000,
		0x0000,
		0x0000,
		/* 209 0xd1 ' ' */
		0x0000,
		0x0000,
		0x00ff,
		0x0000,
		0x00ff,
		0x0018,
		0x0018,
		0x0018,
		/* 210 0xd2 ' ' */
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x00ff,
		0x006c,
		0x006c,
		0x006c,
		/* 211 0xd3 ' ' */
		0x006c,
		0x006c,
		0x006c,
		0x006c,
		0x00fc,
		0x0000,
		0x0000,
		0x0000,
		/* 212 0xd4 ' ' */
		0x0018,
		0x0018,
		0x00f8,
		0x0018,
		0x00f8,
		0x0000,
		0x0000,
		0x0000,
		/* 213 0xd5 ' ' */
		0x0000,
		0x0000,
		0x00f8,
		0x0018,
		0x00f8,
		0x0018,
		0x0018,
		0x0018,
		/* 214 0xd6 ' ' */
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x00fc,
		0x006c,
		0x006c,
		0x006c,
		/* 215 0xd7 ' ' */
		0x006c,
		0x006c,
		0x006c,
		0x006c,
		0x00ff,
		0x006c,
		0x006c,
		0x006c,
		/* 216 0xd8 ' ' */
		0x0018,
		0x0018,
		0x00ff,
		0x0018,
		0x00ff,
		0x0018,
		0x0018,
		0x0018,
		/* 217 0xd9 ' ' */
		0x0018,
		0x0018,
		0x0018,
		0x0018,
		0x001f,
		0x0000,
		0x0000,
		0x0000,
		/* 218 0xda ' ' */
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x00f8,
		0x0018,
		0x0018,
		0x0018,
		/* 219 0xdb ' ' */
		0x00ff,
		0x00ff,
		0x00ff,
		0x00ff,
		0x00ff,
		0x00ff,
		0x00ff,
		0x00ff,
		/* 220 0xdc ' ' */
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x00ff,
		0x00ff,
		0x00ff,
		0x00ff,
		/* 221 0xdd ' ' */
		0x000f,
		0x000f,
		0x000f,
		0x000f,
		0x000f,
		0x000f,
		0x000f,
		0x000f,
		/* 222 0xde ' ' */
		0x00f0,
		0x00f0,
		0x00f0,
		0x00f0,
		0x00f0,
		0x00f0,
		0x00f0,
		0x00f0,
		/* 223 0xdf ' ' */
		0x00ff,
		0x00ff,
		0x00ff,
		0x00ff,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		/* 224 0xe0 ' ' */
		0x0000,
		0x0000,
		0x006e,
		0x003b,
		0x0013,
		0x003b,
		0x006e,
		0x0000,
		/* 225 0xe1 ' ' */
		0x001e,
		0x0033,
		0x0033,
		0x001b,
		0x0033,
		0x0063,
		0x0033,
		0x0000,
		/* 226 0xe2 ' ' */
		0x007f,
		0x0063,
		0x0003,
		0x0003,
		0x0003,
		0x0003,
		0x0003,
		0x0000,
		/* 227 0xe3 ' ' */
		0x0000,
		0x0000,
		0x007f,
		0x0036,
		0x0036,
		0x0036,
		0x0036,
		0x0000,
		/* 228 0xe4 ' ' */
		0x007f,
		0x0063,
		0x0006,
		0x000c,
		0x0006,
		0x0063,
		0x007f,
		0x0000,
		/* 229 0xe5 ' ' */
		0x0000,
		0x0000,
		0x007e,
		0x001b,
		0x001b,
		0x001b,
		0x000e,
		0x0000,
		/* 230 0xe6 ' ' */
		0x0000,
		0x0000,
		0x0066,
		0x0066,
		0x0066,
		0x0066,
		0x003e,
		0x0003,
		/* 231 0xe7 ' ' */
		0x0000,
		0x006e,
		0x003b,
		0x0018,
		0x0018,
		0x0018,
		0x0018,
		0x0000,
		/* 232 0xe8 ' ' */
		0x007e,
		0x0018,
		0x003c,
		0x0066,
		0x0066,
		0x003c,
		0x0018,
		0x007e,
		/* 233 0xe9 ' ' */
		0x001c,
		0x0036,
		0x0063,
		0x007f,
		0x0063,
		0x0036,
		0x001c,
		0x0000,
		/* 234 0xea ' ' */
		0x001c,
		0x0036,
		0x0063,
		0x0063,
		0x0036,
		0x0036,
		0x0077,
		0x0000,
		/* 235 0xeb ' ' */
		0x0070,
		0x0018,
		0x0030,
		0x007c,
		0x0066,
		0x0066,
		0x003c,
		0x0000,
		/* 236 0xec ' ' */
		0x0000,
		0x0000,
		0x007e,
		0x00db,
		0x00db,
		0x007e,
		0x0000,
		0x0000,
		/* 237 0xed ' ' */
		0x0060,
		0x0030,
		0x007e,
		0x00db,
		0x00db,
		0x007e,
		0x0006,
		0x0003,
		/* 238 0xee ' ' */
		0x0078,
		0x000c,
		0x0006,
		0x007e,
		0x0006,
		0x000c,
		0x0078,
		0x0000,
		/* 239 0xef ' ' */
		0x0000,
		0x003e,
		0x0063,
		0x0063,
		0x0063,
		0x0063,
		0x0063,
		0x0000,
		/* 240 0xf0 ' ' */
		0x0000,
		0x007f,
		0x0000,
		0x007f,
		0x0000,
		0x007f,
		0x0000,
		0x0000,
		/* 241 0xf1 ' ' */
		0x0018,
		0x0018,
		0x007e,
		0x0018,
		0x0018,
		0x0000,
		0x007e,
		0x0000,
		/* 242 0xf2 ' ' */
		0x000c,
		0x0018,
		0x0030,
		0x0018,
		0x000c,
		0x0000,
		0x007e,
		0x0000,
		/* 243 0xf3 ' ' */
		0x0030,
		0x0018,
		0x000c,
		0x0018,
		0x0030,
		0x0000,
		0x007e,
		0x0000,
		/* 244 0xf4 ' ' */
		0x0070,
		0x00d8,
		0x00d8,
		0x0018,
		0x0018,
		0x0018,
		0x0018,
		0x0018,
		/* 245 0xf5 ' ' */
		0x0018,
		0x0018,
		0x0018,
		0x0018,
		0x0018,
		0x001b,
		0x001b,
		0x000e,
		/* 246 0xf6 ' ' */
		0x0000,
		0x0018,
		0x0000,
		0x007e,
		0x0000,
		0x0018,
		0x0000,
		0x0000,
		/* 247 0xf7 ' ' */
		0x0000,
		0x006e,
		0x003b,
		0x0000,
		0x006e,
		0x003b,
		0x0000,
		0x0000,
		/* 248 0xf8 ' ' */
		0x001c,
		0x0036,
		0x0036,
		0x001c,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		/* 249 0xf9 ' ' */
		0x0000,
		0x0000,
		0x0000,
		0x0018,
		0x0018,
		0x0000,
		0x0000,
		0x0000,
		/* 250 0xfa ' ' */
		0x0000,
		0x0000,
		0x0000,
		0x0018,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		/* 251 0xfb ' ' */
		0x00f0,
		0x0030,
		0x0030,
		0x0030,
		0x0037,
		0x0036,
		0x003c,
		0x0038,
		/* 252 0xfc ' ' */
		0x0036,
		0x006c,
		0x006c,
		0x006c,
		0x006c,
		0x0000,
		0x0000,
		0x0000,
		/* 253 0xfd ' ' */
		0x001e,
		0x0030,
		0x0018,
		0x000c,
		0x003e,
		0x0000,
		0x0000,
		0x0000,
		/* 254 0xfe ' ' */
		0x0000,
		0x0000,
		0x003c,
		0x003c,
		0x003c,
		0x003c,
		0x0000,
		0x0000,
		/* 255 0xff ' ' */
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
	}
};
