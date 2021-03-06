#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <endian.h>

#include "register.h"
#include "kbd.h"
#include "mmap_register.h"
#include "opcode.h"
#include "trap.h"
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

void readff(FILE* file)
{
    uint16_t ori;
    fread(&ori, sizeof(ori), 1, file);

    /* We prefer to check if we're not in big endian since they
     * are rarer nowadays and endian.h is not a C standard.
     */
    #ifndef BIG_ENDIAN
    ori = swap16(ori);
    #endif

    uint16_t maxr = UINT16_MAX - ori;
    uint16_t* ptr = mem + ori;


    size_t r = fread(ptr, sizeof(uint16_t), maxr, file);

    #ifndef BIG_ENDIAN
    while(r-- > 0) {
        *ptr = swap16(*ptr);
        ++ptr;
    }
    #endif
}

int read(const char* path)
{
    FILE* file = fopen(path, "rb");
    if(!file)
        return 0;

    readff(file);

    return !fclose(file);
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
    if(addr == MMR_KBSR) {
        if(check_key()) {
            mem[MMR_KBSR] = (1 << 15);
            mem[MMR_KBDR] = getchar();
	} else {
            mem[MMR_KBSR] = 0;
        }
    }
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


#define DWN_CHAR(car) car & 0xff
#define UP_CHAR(car) car >> 8

int main(int argc, char* argv[])
{
    enum { PC_START = 0x3000 };
    reg[R_PC] = PC_START;

    int running = 1;
    while (running){

        uint16_t ins = mem_read(reg[R_PC]++);

        uint16_t r0, r1;
        uint16_t* car;
        char character;

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
                r0 = DST(ins);
                reg[r0] = mem_read(reg[R_PC] + sign_extend(ins & 0x1ff, 9));
                update_flags(r0);
                break;

            case OP_ST:
                mem_write(reg[R_PC] + sign_extend(ins & 0x1ff, 9), reg[DST(ins)]);
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

            case OP_LDR:
                r0 = DST(ins);
                reg[r0] = mem_read(reg[SR1_OP(ins)] + sign_extend(ins & 0x3f, 6));
                update_flags(r0);
                break;

            case OP_STR:
                mem_write(reg[SR1_OP(ins)] + sign_extend(ins & 0x3f, 6), reg[DST(ins)]);
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

            case OP_STI:
                mem_write(mem_read(reg[R_PC] + sign_extend(ins & 0x1ff, 9)), reg[DST(ins)]);
                break;

            case OP_JMP:
                r0 = BR_OP(ins);
                if(r0 == 0x7) /* RET instruction */
                    reg[R_PC] = reg[R_R7];
                else /* JMP instruction */
                    reg[R_PC] = r0;
                break;

            case OP_LEA:
                r0 = DST(ins);
                reg[r0] = reg[R_PC] + sign_extend(ins & 0x1ff, 9);
                update_flags(r0);
                break;

            case OP_TRAP:
                switch(ins & 0xFF){
                    case TRAP_GETC:
                        reg[R_R0] = (uint16_t) getchar();
                        break;

                    case TRAP_OUT:
                        putc((char)reg[R_R0], stdout);
                        fflush(stdout);
                        break;

                    case TRAP_PUTS:
                        car = mem + reg[R_R0];
                        while(*car) {
                            putc((char)*car, stdout);
                            ++car;
                        }
                        fflush(stdout);
                        break;

                    case TRAP_IN:
                        printf("Type a character: ");
                        reg[R_R0] = (uint16_t) getchar();
                        break;

                    case TRAP_PUTSP:
                        car = mem + reg[R_R0];
                        while(*car) {
                            putc(DWN_CHAR(*car), stdout);
                            character = UP_CHAR(*car);
                            if(character)
                                putc(character, stdout);
                            ++car;
                        }
                        fflush(stdout);
                        break;

                    case TRAP_HALT:
                        running = 0;
                        break;

                    default:
                        printf("Bad trap signal encountered, aborting...");
                        abort();
                        break;
                }
                break;

            default:
                printf("Bad OP code encountered, aborting...");
                abort();
                break;
        }
    }
}
