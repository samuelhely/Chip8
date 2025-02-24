#ifndef CPU_H
#define CPU_H

struct cpu
{
    uint8 Registers[16];
    uint16 IndexRegister;
    uint16 ProgramCounter;
};

#endif // CPU_H
