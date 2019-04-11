#ifndef _TRAP_H
#define _TRAP_H

enum{
    TRAP_GETC = 0x20,    /* Get character from keyboard */
    TRAP_OUT = 0x21,     /* Output a character */
    TRAP_PUTS = 0x22,    /* Output a string */
    TRAP_IN = 0x23,      /* Input a string */
    TRAP_PUTSP = 0x24,   /* Output a string with two character in each memory location [15:8] [7:0] */
    TRAP_HALT = 0x25     /* Halt the program */
};

#endif
