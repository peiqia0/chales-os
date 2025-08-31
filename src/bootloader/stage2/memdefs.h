#pragma once
// memory definitions (i forgot what it did but removing this makes weird errors so i reread the code and added these comments below)
// 0x00000000 to 0x000003FF - interrupt vector table
// 0x00000400 to 0x000004FF - BIOS data area

#define MEMORY_MIN          0x00000500
#define MEMORY_MAX          0x00080000

// 0x00000500 to 0x00010500 - FAT driver
#define MEMORY_FAT_ADDR     ((void far*)0x00500000) // segment:offset (segment 0050 offset 0000 where the first four digits are segment and last 4 is offset)
#define MEMORY_FAT_SIZE     0x00010000

// 0x00020000 to 0x00030000 - stage2

// 0x00030000 to 0x00080000 - free

// 0x00080000 to 0x0009FFFF - Extended BIOS data area
// 0x000A0000 to 0x000C7FFF - Video
// 0x000C8000 to 0x000FFFFF - BIOS
