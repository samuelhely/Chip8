#include <stdio.h>
#include <stdint.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;

#define Kilobytes(Value)((Value) * 1024)
#define Megabytes(Value)(Kilobytes(Value) * 1024)

#define ArrayCount(Array)(sizeof(Array)/sizeof((Array)[0]))

uint8 Memory[Kilobytes(4)];
uint16 Stack[16] = {};
uint8 Registers[16] = {};
uint16 IndexRegister;
uint16 ProgramCounter;
uint8 StackPointer;
uint16 OpCode;
uint8 DelayTimer;
uint8 SoundTimer;
uint8 Keypad[16] = {};
uint8 Display[64*32] = {};

uint8_t Font[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void InitializeEmulator() {
    IndexRegister = 0;
    ProgramCounter = 0;
    StackPointer = 0;
    OpCode = 0;
    DelayTimer = 0;
    SoundTimer = 0;

    uint8 MemoryIndex = 80;
    for(int32 Index = 0; Index < ArrayCount(Font); Index++)
    {
        Memory[MemoryIndex++] = Font[Index];
    }
}

int main(int ArgCount, char **ArgValues)
{
    InitializeEmulator();

    if(ArgCount > 1)
    {
        uint32 Offset = 0x200;
        char* Filename = ArgValues[1];
        FILE *File = fopen(Filename, "rb");
        if(File)
        {
            fread(Memory + Offset, 1, ArrayCount(Memory) - Offset, File);
            fclose(File);
        }
    }

    printf("HI");
    return 0;
}
