#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "register.h"
#include "opcode.h"
#include "lc3.h"

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

/* Getting destination operand */
#define DST(ins) (ins >> 9) & 0x7
/* Getting first operand */
#define SR1_OP(ins) (ins >> 6) & 0x7
/* Getting second operand */
#define SR2_OP(ins) ins & 0x7
/* Getting base register operand */
#define BR_OP(ins) SR1_OP(ins)
/* Getting immediate operand */
#define IMM_OP(ins) ins & 0x1f

/* Getting immediate flag bit */
#define IMM_FL(ins) (ins >> 5) & 0x1
/* Getting condition flag */
#define COND_FL(ins) DST(ins)
/* Getting long flag */
#define LNG_FL(ins) (ins >> 11) & 1
int main(int argc, char* argv[])
{
    enum { PC_START = 0x3000 };
    reg[R_PC] = PC_START;

    int running = 1;
    while (running){
        uint16_t ins = mem_read(reg[R_PC]++);
        uint16_t r0, r1;
        switch(OP_CODE(ins)){
            case OP_BR:
            if(COND_FL(ins) & reg[R_COND])
                reg[R_PC] += sign_extend(ins & 0x1ff, 9);
            break;

            case OP_ADD:
                r0 = DST(ins);
                r1 = SR1_OP(ins);
                if (IMM_FL(ins)) {
                    reg[r0] = reg[r1] + sign_extend(IMM_OP(ins), 5);
                } else {
                    reg[r0] = reg[r1] + reg[SR2_OP(ins)];
                }
                update_flags(r0);
                break;

            case OP_LD:
                break;

            case OP_JSR:
                reg[R_R7] = reg[R_PC];
                if(LNG_FL(ins)){
                    reg[R_PC] += sign_extend(ins & 0x7ff, 11);
                } else {
                    reg[R_PC] = reg[BR_OP(ins)];
                }
                break;

            case OP_AND:
                r0 = DST(ins);
                r1 = SR1_OP(ins);
                if(IMM_FL(ins)) {
                    reg[r0] = reg[r1] & sign_extend(IMM_OP(ins), 5);
                } else {
                    reg[r0] = reg[r1] & reg[SR2_OP(ins)];
                }
                update_flags(r0);
                break;

            case OP_NOT:
                r0 = DST(ins);
                reg[r0] = ~reg[SR1_OP(ins)];
                update_flags(r0);
                break;

            case OP_LDI:
                r0 = DST(ins);
                reg[r0] = mem_read(mem_read(reg[R_PC] + sign_extend(ins & 0x1ff, 9)));
                update_flags(r0);
                break;

            case OP_JMP:
                reg[R_PC] = BR_OP(ins);
                break;

            default:
                printf("Bad OP code encountered, aborting...");
                abort();
                break;
        }
    }
}
