#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

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

#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32
#define WINDOW_WIDTH 960
#define WINDOW_HEIGHT 540

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

uint8 Display[DISPLAY_WIDTH*DISPLAY_HEIGHT] = {};

uint8 FontStartAddress = 0x50;
uint8 Font[] = {
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

    uint8 MemoryIndex = FontStartAddress;
    for(int32 Index = 0; Index < ArrayCount(Font); Index++)
    {
        Memory[MemoryIndex++] = Font[Index];
    }
}

// NOTE(samuel): Clear the display
void Opcode_00E0() 
{
    for(uint32 ScreenIndex = 0; ScreenIndex < ArrayCount(Display); ScreenIndex++)
    {
        Display[ScreenIndex] = 0;
    }
}

// NOTE(samuel): Return from a subroutine
void Opcode_00EE() 
{
    StackPointer--;

    if(StackPointer < 0)
    {
        TraceLog(LOG_WARNING, "Stack underflow");
    }

    ProgramCounter = Stack[StackPointer];
}

// NOTE(samuel): Jump to location nnn
void Opcode_1NNN()
{
    uint16 Address = (uint16)(OpCode & 0x0FFF);
    ProgramCounter = Address;
}

// NOTE(samuel): Call subroutine at nnn
void Opcode_2NNN()
{
    Stack[StackPointer] = ProgramCounter;
    StackPointer++;

    if(StackPointer > ArrayCount(Stack))
    {
        TraceLog(LOG_WARNING, "Stack overflow");
    }
    
    uint16 Address = (uint16)(OpCode & 0x0FFF);
    ProgramCounter = Address;
}

// NOTE(samuel): Skip conditionally
void Opcode_3XNN()
{
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);
    uint8 Value = (uint8)(OpCode & 0x00FF);

    if(Registers[VX] == Value)
    {
        ProgramCounter += 2;
    }
}

// NOTE(samuel): Skip conditionally
void Opcode_4XNN()
{
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);
    uint8 Value = (uint8)(OpCode & 0x00FF);

    if(Registers[VX] != Value)
    {
        ProgramCounter += 2;
    }
}

// NOTE(samuel): Skip conditionally
void Opcode_5XY0()
{
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);
    uint8 VY = (uint8)((OpCode & 0x00F0) >> 4);

    if(Registers[VX] == Registers[VY])
    {
        ProgramCounter += 2;
    }
}

void Opcode_6XNN()
{
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);
    uint8 Value = (uint8)(OpCode & 0x00FF);

    Registers[VX] = Value;
}

void Opcode_7XNN()
{
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);
    uint8 Value = (uint8)(OpCode & 0x00FF);

    Registers[VX] += Value;
}

void Opcode_8XY0()
{
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);
    uint8 VY = (uint8)((OpCode & 0x00F0) >> 4);

    Registers[VX] = Registers[VY];
}

void Opcode_8XY1()
{
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);
    uint8 VY = (uint8)((OpCode & 0x00F0) >> 4);

    Registers[VX] |= Registers[VY];
}

void Opcode_8XY2()
{
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);
    uint8 VY = (uint8)((OpCode & 0x00F0) >> 4);

    Registers[VX] &= Registers[VY];
}

void Opcode_8XY3()
{
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);
    uint8 VY = (uint8)((OpCode & 0x00F0) >> 4);

    Registers[VX] ^= Registers[VY];
}

void Opcode_8XY4()
{
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);
    uint8 VY = (uint8)((OpCode & 0x00F0) >> 4);

    uint8 Flag = ((Registers[VX] + Registers[VY]) > 0xFF);
    Registers[VX] += Registers[VY];
    Registers[0xF] = Flag;
}

void Opcode_8XY5()
{
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);
    uint8 VY = (uint8)((OpCode & 0x00F0) >> 4);

    uint8 Flag = (Registers[VX] >= Registers[VY]);
    Registers[VX] -= Registers[VY];
    Registers[0xF] = Flag;
}

void Opcode_8XY6()
{
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);
    uint8 VY = (uint8)((OpCode & 0x00F0) >> 4);

    //Registers[VX] = Registers[VY];
    uint8 Value = (uint8)((Registers[VX] & 0x1) > 0);

    Registers[VX] = Registers[VY] >> 1;
    Registers[0xF] = Value;
}

void Opcode_8XY7()
{
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);
    uint8 VY = (uint8)((OpCode & 0x00F0) >> 4);

    uint8 Flag = (Registers[VY] >= Registers[VX]);
    Registers[VX] = (Registers[VY] - Registers[VX]);
    Registers[0xF] = Flag;
}

void Opcode_8XYE()
{
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);
    uint8 VY = (uint8)((OpCode & 0x00F0) >> 4);

    //Registers[VX] = Registers[VY];
    uint8 Value = (uint8)((Registers[VX] & (0x1 << 7)) > 0);

    Registers[VX] = Registers[VY] << 1;
    Registers[0xF] = Value;
}

void Opcode_9XY0()
{
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);
    uint8 VY = (uint8)((OpCode & 0x00F0) >> 4);

    if(Registers[VX] != Registers[VY])
    {
        ProgramCounter += 2;
    }
}

void Opcode_ANNN()
{
    uint16 Value = (uint16)(OpCode & 0x0FFF);
    IndexRegister = Value;
}

void Opcode_BNNN()
{
    uint16 Value = (uint16)(OpCode & 0x0FFF);
    ProgramCounter = (uint16)(Value + Registers[0]);
}

void Opcode_CXNN()
{
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);
    uint8 Rand = (uint8)rand();
    uint8 Value = (uint8)(OpCode & 0x00FF);

    Registers[VX] = (uint8)(Rand & Value);
}

void Opcode_DXYN()
{
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);
    uint8 VY = (uint8)((OpCode & 0x00F0) >> 4);
    uint8 Count = (uint8)(OpCode & 0x000F);

    uint8 PosX = Registers[VX];
    uint8 PosY = Registers[VY];

    Registers[0xF] = 0;

    for(uint32 Row = 0; Row < Count; Row++)
    {
        uint32 Sprite = Memory[IndexRegister + Row];

        for(uint32 Column = 0; Column < 8; Column++)
        {
            uint32 Pixel = (Sprite & (0x80 >> Column));
            uint8 *DisplayValue = &Display[(Row + PosY)*DISPLAY_WIDTH + (Column + PosX)];
            if(Pixel != 0)
            {
                if(*DisplayValue == 1)
                {
                    Registers[0xF] = 1;
                }
                
                *DisplayValue ^= 1;
            }
        }
    }
}

void Opcode_EX9E()
{
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);
    
    if(Keypad[VX] == 1)
    {
        ProgramCounter += 2;
    }
}

void Opcode_EXA1()
{
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);
    
    if(Keypad[VX] == 0)
    {
        ProgramCounter += 2;
    }
}

void Opcode_FX07()
{
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);

    Registers[VX] = DelayTimer;
}

void Opcode_FX0A()
{
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);

    for(uint8 KeypadIndex = 0; KeypadIndex < ArrayCount(Keypad); KeypadIndex++)
    {
        if(Keypad[KeypadIndex] == 1)
        {
            Registers[VX] = KeypadIndex;
            return;
        }
    }

    ProgramCounter -= 2;
}

void Opcode_FX15()
{
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);

    DelayTimer = Registers[VX];
}

void Opcode_FX18()
{
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);

    SoundTimer = Registers[VX];
}

void Opcode_FX1E()
{
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);

    if((IndexRegister + Registers[VX]) > 0xFFF)
    {
        Registers[0xF] = 1;
    }

    IndexRegister += Registers[VX];
}

void Opcode_FX29()
{
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);

    IndexRegister = (uint8)(FontStartAddress + Registers[VX]);
}

void Opcode_FX33()
{
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);
    uint8 Value = Registers[VX];

    Memory[IndexRegister + 0] = (uint8)(Value / 100);
    Memory[IndexRegister + 1] = (uint8)((Value / 10) % 10);
    Memory[IndexRegister + 2] = (uint8)(Value % 10);
}

void Opcode_FX55()
{
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);
    for(uint8 Index = 0; Index <= VX; Index++)
    {
        Memory[IndexRegister + Index] = Registers[Index];
    }
    //IndexRegister++;
}

void Opcode_FX65()
{
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);
    for(uint8 Index = 0; Index <= VX; Index++)
    {
        Registers[Index] = Memory[IndexRegister + Index];
    }
    //IndexRegister++;
}

int main(int ArgCount, char **ArgValues)
{
    InitializeEmulator();

    if(ArgCount > 1)
    {
        char* Filename = ArgValues[1];
        FILE *File;
        if(fopen_s(&File, Filename, "rb") == 0)
        {
            fread(Memory + ProgramCounter, 1, ArrayCount(Memory) - ProgramCounter, File);
            fclose(File);
        }
    }

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Chip8");
    while(!WindowShouldClose())
    {
        // Input
        Keypad[0] = IsKeyPressed(KEY_ONE);
        Keypad[1] = IsKeyPressed(KEY_TWO);
        Keypad[2] = IsKeyPressed(KEY_THREE);
        Keypad[3] = IsKeyPressed(KEY_FOUR);
        Keypad[4] = IsKeyPressed(KEY_Q);
        Keypad[5] = IsKeyPressed(KEY_W);
        Keypad[6] = IsKeyPressed(KEY_E);
        Keypad[7] = IsKeyPressed(KEY_R);
        Keypad[8] = IsKeyPressed(KEY_A);
        Keypad[9] = IsKeyPressed(KEY_S);
        Keypad[10] = IsKeyPressed(KEY_D);
        Keypad[11] = IsKeyPressed(KEY_F);
        Keypad[12] = IsKeyPressed(KEY_Z);
        Keypad[13] = IsKeyPressed(KEY_X);
        Keypad[14] = IsKeyPressed(KEY_C);
        Keypad[15] = IsKeyPressed(KEY_V);

        for(uint8 KeyIndex = 0; KeyIndex < ArrayCount(Keypad); KeyIndex++)
        {
            if(Keypad[KeyIndex])
            {
                TraceLog(LOG_WARNING, "Key %d is pressed", KeyIndex);
            }
        }

        // Fetching
        OpCode = (uint16)(((uint16)Memory[ProgramCounter] << 8) | Memory[ProgramCounter + 1]);
        if(OpCode)
        {
            ProgramCounter += 2;
        }
        
        BeginDrawing();
        ClearBackground(GRAY);

        switch(OpCode & 0xF000)
        {
            case 0x0 :
            {
                if(OpCode == 0x00E0)
                {
                    //TraceLog(LOG_WARNING, "Clear the screen");
                    Opcode_00E0();
                }
                else if(OpCode == 0x00EE)
                {
                    //TraceLog(LOG_WARNING, "Return from subroutine");
                    Opcode_00EE();
                }
            } break;

            case 0x1000 :
            {
                //TraceLog(LOG_WARNING, "Jump to address");
                Opcode_1NNN();
            } break;

            case 0x2000 :
            {
                //TraceLog(LOG_WARNING, "Call at address");
                Opcode_2NNN();
            } break;

            case 0x3000 :
            {
                //TraceLog(LOG_WARNING, "Skip");
                Opcode_3XNN();
            } break;

            case 0x4000 :
            {
                //TraceLog(LOG_WARNING, "Instruction 4");
                Opcode_4XNN();
            } break;

            case 0x5000 :
            {
                //TraceLog(LOG_WARNING, "Instruction 5");
                Opcode_5XY0();
            } break;

            case 0x6000 :
            {
                //TraceLog(LOG_WARNING, "Instruction 6");
                Opcode_6XNN();
            } break;

            case 0x7000 :
            {
                //TraceLog(LOG_WARNING, "Instruction 7");
                Opcode_7XNN();
            } break;

            case 0x8000 :
            {
                switch(OpCode & 0x000F)
                {
                    case 0x0000 :
                    {
                        //TraceLog(LOG_WARNING, "Instruction 8-0");
                        Opcode_8XY0();
                    } break;
                    
                    case 0x0001 :
                    {
                        //TraceLog(LOG_WARNING, "Instruction 8-1");
                        Opcode_8XY1();
                    } break;
                    
                    case 0x0002 :
                    {
                        //TraceLog(LOG_WARNING, "Instruction 8-2");
                        Opcode_8XY2();
                    } break;
                    
                    case 0x0003 :
                    {
                        //TraceLog(LOG_WARNING, "Instruction 8-3");
                        Opcode_8XY3();
                    } break;
                    
                    case 0x0004 :
                    {
                        //TraceLog(LOG_WARNING, "Instruction 8-4");
                        Opcode_8XY4();
                    } break;
                    
                    case 0x0005 :
                    {
                        //TraceLog(LOG_WARNING, "Instruction 8-5");
                        Opcode_8XY5();
                    } break;
                    
                    case 0x0006 :
                    {
                        //TraceLog(LOG_WARNING, "Instruction 8-6");
                        Opcode_8XY6();
                    } break;
                    
                    case 0x0007 :
                    {
                        //TraceLog(LOG_WARNING, "Instruction 8-7");
                        Opcode_8XY7();
                    } break;
                    
                    case 0x000E :
                    {
                        //TraceLog(LOG_WARNING, "Instruction 8-E");
                        Opcode_8XYE();
                    } break;
                }
            } break;

            case 0x9000 :
            {
                //TraceLog(LOG_WARNING, "Instruction 9");
                Opcode_9XY0();
            } break;

            case 0xA000 :
            {
                //TraceLog(LOG_WARNING, "Instruction A");
                Opcode_ANNN();
            } break;

            case 0xB000 :
            {
                //TraceLog(LOG_WARNING, "Instruction B");
                Opcode_BNNN();
            } break;

            case 0xC000 :
            {
                //TraceLog(LOG_WARNING, "Instruction C");
                Opcode_CXNN();
            } break;

            case 0xD000 :
            {
                //TraceLog(LOG_WARNING, "Instruction D");
                Opcode_DXYN();
            } break;

            case 0xE000 :
            {
                switch(OpCode & 0x000F)
                {
                    case 0x000E :
                    {
                        //TraceLog(LOG_WARNING, "Instruction E-E");
                        Opcode_EX9E();
                    } break;
                    
                    case 0x0001 :
                    {
                        //TraceLog(LOG_WARNING, "Instruction E-1");
                        Opcode_EXA1();
                    } break;
                }
            } break;

            case 0xF000 :
            {
                switch(OpCode & 0x00FF)
                {
                    case 0x0007 :
                    {
                        //TraceLog(LOG_WARNING, "Instruction F-07");
                        Opcode_FX07();
                    } break;

                    case 0x000A :
                    {
                        //TraceLog(LOG_WARNING, "Instruction F-0A");
                        Opcode_FX0A();
                    } break;

                    case 0x0015 :
                    {
                        //TraceLog(LOG_WARNING, "Instruction F-15");
                        Opcode_FX15();
                    } break;

                    case 0x0018 :
                    {
                        //TraceLog(LOG_WARNING, "Instruction F-18");
                        Opcode_FX18();
                    } break;

                    case 0x001E :
                    {
                        //TraceLog(LOG_WARNING, "Instruction F-1E");
                        Opcode_FX1E();
                    } break;

                    case 0x0029 :
                    {
                        //TraceLog(LOG_WARNING, "Instruction F-29");
                        Opcode_FX29();
                    } break;

                    case 0x0033 :
                    {
                        //TraceLog(LOG_WARNING, "Instruction F-33");
                        Opcode_FX33();
                    } break;

                    case 0x0055 :
                    {
                        //TraceLog(LOG_WARNING, "Instruction F-55");
                        Opcode_FX55();
                    } break;

                    case 0x0065 :
                    {
                        //TraceLog(LOG_WARNING, "Instruction F-65");
                        Opcode_FX65();
                    } break;

                }
            } break;
        }

        //DrawRectangle(0, 0, 320, 160, RED);

        int32 PixelSize = 10;
        for(int32 Y = 0; Y < DISPLAY_HEIGHT; Y++)
        {
            for(int32 X = 0; X < DISPLAY_WIDTH; X++)
            {
                uint8 Value = Display[Y*DISPLAY_WIDTH + X];
                Color Pixel = BLACK;
                if(Value)
                {
                    Pixel = RED;
                }
                    
                int32 MinX = X * PixelSize;
                int32 MinY = Y * PixelSize;
                int32 MaxX = MinX + PixelSize;
                int32 MaxY = MinY + PixelSize;
                DrawRectangle(MinX, MinY, MaxX, MaxY, Pixel);
            }
        }

        // Right Column
        int32 PosX = DISPLAY_WIDTH*PixelSize;
        int32 PosY = 0;
        DrawRectangle(PosX, PosY, WINDOW_WIDTH, WINDOW_HEIGHT, RAYWHITE);

        int32 RectSize = 30;
        DrawText("Registers", PosX + 10, 10, 14, BLACK);

        // Bottom Area
        DrawRectangle(0, DISPLAY_HEIGHT*PixelSize, DISPLAY_WIDTH*PixelSize, WINDOW_HEIGHT, Color{255, 0, 255, 100});

        if(DelayTimer > 0)
        {
            DelayTimer--;
        }

        if(SoundTimer > 0)
        {
            SoundTimer--;
        }

        ClearBackground(RAYWHITE);

        EndDrawing();
    }

    return 0;
}
