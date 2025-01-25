#include "raylib.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

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
    ProgramCounter = 0x200;
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

// NOTE(samuel): Clear the display
void Opcode_00E0() 
{
    for(uint8 ScreenIndex = 0; ScreenIndex < ArrayCount(Display); ScreenIndex++)
    {
        Display[ScreenIndex] = 0;
    }
}

// NOTE(samuel): Return from a subroutine
void Opcode_00EE() 
{
    ProgramCounter = Stack[StackPointer];
    StackPointer--;
}

// NOTE(samuel): Jump to location nnn
void Opcode_1NNN()
{
    uint16 Address = OpCode & 0x0FFF;
    ProgramCounter = Address;
}

// NOTE(samuel): Call subroutine at nnn
void Opcode_2NNN()
{
    Stack[StackPointer] = ProgramCounter;
    
    uint16 Address = OpCode & 0x0FFF;
    ProgramCounter = Address;
}

// NOTE(samuel): Skip conditionally
void Opcode_3XKK()
{
    uint8 VX = (OpCode & 0x0F00) >> 8;
    uint8 Value = OpCode & 0x00FF;

    if(Registers[VX] == Value)
    {
        ProgramCounter += 2;
    }
}

// NOTE(samuel): Skip conditionally
void Opcode_4XKK()
{
    uint8 VX = (OpCode & 0x0F00) >> 8;
    uint8 Value = OpCode & 0x00FF;

    if(Registers[VX] != Value)
    {
        ProgramCounter += 2;
    }
}

// NOTE(samuel): Skip conditionally
void Opcode_5XY0()
{
    uint16 VX = (OpCode & 0x0F00) >> 8;
    uint16 VY = (OpCode & 0x00F0) >> 4;;

    if(Registers[VX] == Registers[VY])
    {
        ProgramCounter += 2;
    }
}

void Opcode_6XKK()
{
    uint8 VX = (OpCode & 0x0F00) >> 8;
    uint8 Value = OpCode & 0x00FF;

    Registers[VX] = Value;
}

void Opcode_7XKK()
{
    uint8 VX = (OpCode & 0x0F00) >> 8;
    uint8 Value = OpCode & 0x00FF;

    Registers[VX] += Value;
}

void Opcode_8XY0()
{
    uint8 VX = (OpCode & 0x0F00) >> 8;
    uint8 VY = (OpCode & 0x00F0) >> 4;

    Registers[VX] = Registers[VY];
}

void Opcode_8XY1()
{
    uint8 VX = (OpCode & 0x0F00) >> 8;
    uint8 VY = (OpCode & 0x00F0) >> 4;

    Registers[VX] = (Registers[VX] | Registers[VY]);
}

void Opcode_8XY2()
{
    uint8 VX = (OpCode & 0x0F00) >> 8;
    uint8 VY = (OpCode & 0x00F0) >> 4;

    Registers[VX] = (Registers[VX] & Registers[VY]);
}

void Opcode_8XY3()
{
    uint8 VX = (OpCode & 0x0F00) >> 8;
    uint8 VY = (OpCode & 0x00F0) >> 4;

    Registers[VX] = (Registers[VX] ^ Registers[VY]);
}

void Opcode_8XY4()
{
    uint8 VX = (OpCode & 0x0F00) >> 8;
    uint8 VY = (OpCode & 0x00F0) >> 4;

    Registers[VX] = Registers[VX] + Registers[VY];
    if(Registers[VX] > 0xFF)
    {
        Registers[0xF] = 1;
    }
    else
    {
        Registers[0xF] = 0;
    }
}

void Opcode_8XY5()
{
    uint8 VX = (OpCode & 0x0F00) >> 8;
    uint8 VY = (OpCode & 0x00F0) >> 4;

    if(Registers[VX] > Registers[VY])
    {
        Registers[0xF] = 1;
    }
    else
    {
        Registers[0xF] = 0;
    }

    Registers[VX] = Registers[VX] - Registers[VY];
}

void Opcode_8XY6()
{
    uint8 VX = (OpCode & 0x0F00) >> 8;
    uint8 VY = (OpCode & 0x00F0) >> 4;
    uint8 Value = Registers[VX] & 0x1;

    if(Value == 1)
    {
        Registers[0xF] = 1;
    }
    else
    {
        Registers[0xF] = 0;
    }

    Registers[VX] = Registers[VX] >> 1;
}

void Opcode_8XY7()
{
    uint8 VX = (OpCode & 0x0F00) >> 8;
    uint8 VY = (OpCode & 0x00F0) >> 4;

    if(Registers[VY] > Registers[VX])
    {
        Registers[0xF] = 1;
    }
    else
    {
        Registers[0xF] = 0;
    }

    Registers[VX] = Registers[VY] - Registers[VX];
}

void Opcode_8XYE()
{
    uint8 VX = (OpCode & 0x0F00) >> 8;
    uint8 VY = (OpCode & 0x00F0) >> 4;
    uint8 Value = (Registers[VX] & (0x1 << 7));

    if(Value == 1)
    {
        Registers[0xF] = 1;
    }
    else
    {
        Registers[0xF] = 0;
    }

    Registers[VX] = Registers[VX] << 1;
}

void Opcode_9XY0()
{
    uint8 VX = (OpCode & 0x0F00) >> 8;
    uint8 VY = (OpCode & 0x00F0) >> 4;

    if(Registers[VX] != Registers[VY])
    {
        ProgramCounter += 2;
    }
}

void Opcode_ANNN()
{
    uint16 Value = OpCode & 0x0FFF;
    IndexRegister = Value;
}

void Opcode_BNNN()
{
    uint16 Value = OpCode & 0x0FFF;
    ProgramCounter = Value + Registers[0];
}

void Opcode_CXKK()
{
    uint8 VX = OpCode & 0x0F00;
    uint8 Rand = (uint8)rand();
    uint8 Value = OpCode & 0x00FF;

    Registers[VX] = Rand & Value;
}

void Opcode_DXYN()
{
    uint8 VX = OpCode & 0x0F00;
    uint8 VY = OpCode & 0x00F0;
    uint8 Value = OpCode & 0x000F;

    // NOTE(samuel): The interpreter reads n bytes from memory, starting at the address stored in I. 
    // These bytes are then displayed as sprites on screen at coordinates (Vx, Vy). 
    // Sprites are XORed onto the existing screen. 
    // If this causes any pixels to be erased, VF is set to 1, otherwise it is set to 0. 
    // If the sprite is positioned so part of it is outside the coordinates of the display, 
    // it wraps around to the opposite side of the screen. 
}

int main(int ArgCount, char **ArgValues)
{
    InitializeEmulator();

    uint16 Offset = 0x200;
    if(ArgCount > 1)
    {
        char* Filename = ArgValues[1];
        FILE *File = fopen(Filename, "rb");
        if(File)
        {
            fread(Memory + ProgramCounter, 1, ArrayCount(Memory) - ProgramCounter, File);
            fclose(File);
        }
    }

    uint16 Instruction;

    InitWindow(640, 480, "Chip8");
    while(!WindowShouldClose())
    {
        // Fetching
        Instruction = (uint16)(((uint16)Memory[ProgramCounter] << 8) | Memory[ProgramCounter + 1]);
        ProgramCounter += 2;
        
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        uint8 OpCode = (Instruction & 0xF000) >> 8;
        switch(OpCode)
        {
            case 0x0 :
            {
                if(Instruction == 0x00E0)
                {
                    TraceLog(LOG_WARNING, "Clear the screen");
                }
                else if(Instruction == 0x00EE)
                {
                    TraceLog(LOG_WARNING, "Clear the screen");
                }
            } break;

            case 0x10 :
            {
                TraceLog(LOG_WARNING, "Jump to address");
            } break;

            case 0x20 :
            {
                TraceLog(LOG_WARNING, "Call at address");
            } break;

            case 0x30 :
            {
                TraceLog(LOG_WARNING, "Skip");
            } break;

            case 0x40 :
            {
                TraceLog(LOG_WARNING, "Instruction 4");
            } break;

            case 0x50 :
            {
                TraceLog(LOG_WARNING, "Instruction 5");
            } break;

            case 0x60 :
            {
                TraceLog(LOG_WARNING, "Instruction 6");
            } break;

            case 0x70 :
            {
                TraceLog(LOG_WARNING, "Instruction 7");
            } break;

            case 0x80 :
            {
                TraceLog(LOG_WARNING, "Instruction 8");
            } break;

            case 0x90 :
            {
                TraceLog(LOG_WARNING, "Instruction 9");
            } break;

            case 0xA0 :
            {
                TraceLog(LOG_WARNING, "Instruction A");
            } break;

            case 0xB0 :
            {
                TraceLog(LOG_WARNING, "Instruction B");
            } break;

            case 0xC0 :
            {
                TraceLog(LOG_WARNING, "Instruction C");
            } break;

            case 0xD0 :
            {
                TraceLog(LOG_WARNING, "Instruction D");
            } break;

            case 0xE0 :
            {
                TraceLog(LOG_WARNING, "Instruction E");
            } break;

            case 0xF0 :
            {
                TraceLog(LOG_WARNING, "Instruction F");
            } break;
        }

        EndDrawing();
    }

    return 0;
}
