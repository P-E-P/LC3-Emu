#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "register.h"
#include "opcode.h"

enum {
    FL_POS = 1 << 0,
    FL_ZRO = 1 << 1,
    FL_NEG = 1 << 2
};


uint16_t mem[UINT16_MAX];
uint16_t reg[R_COUNT];

uint16_t sign_extend(uint16_t x, int bit_count)
{
    if((x >> (bit_count - 1)) & 1){
        x |= (0xffff << bit_count);
    }
    return x;
}

uint16_t swap16(uint16_t x)
{
    return (x << 8) | (x >> 8);
}


void update_flags(uint16_t r)
{
    if (reg[r] == 0) {
        reg[R_COND] = FL_ZRO;
    }
    else if (reg[r] >> 15) {
        reg[R_COND] = FL_NEG;
    } else {
        reg[R_COND] = FL_POS;
    }
}

inline void mem_write(uint16_t addr, uint16_t val)
{
    mem[addr] = val;
}

inline uint16_t mem_read(uint16_t addr)
{
    return mem[addr];
}

/* Getting OP code */
#define OP_CODE(ins) ins >> 12
/* Getting destination */
#define DEST(ins) (ins >> 9) & 0x7
/* Getting first operand */
#define FST_OP(ins) (ins >> 6) & 0x7
/* Getting second operand */ 
#define SND_OP(ins) ins & 0x7

int main(int argc, char* argv[])
{
    enum { PC_START = 0x3000 };
    reg[R_PC] = PC_START;

    int running = 1;
    while (running){
        uint16_t ins = mem_read(reg[R_PC]++);
        switch(OP_CODE(ins)){
            case OP_ADD:;
                uint16_t r0 = DEST(ins);
                uint16_t r1 = FST_OP(ins);
                /* If we add an immediate value */
                if ((ins >> 5) & 0x1) {
                    reg[r0] = reg[r1] + sign_extend(ins & 0x1F, 5);
                } else {
                    reg[r0] = reg[r1] + reg[SND_OP(ins)];
                }
                update_flags(r0);
                break;
            case OP_AND:
                break;
            default:
                printf("Bad OP code encountered, aborting...");
                abort();
                break;
        }
    }
}
