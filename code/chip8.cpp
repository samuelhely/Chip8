#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#include "chip8.h"
#include "cpu.h"
#include "cpu.cpp"

static uint8 ReadMemory(memory *Memory, uint32 Address)
{
    assert(Address < ArrayCount(Memory->Data));
    uint8 Result = Memory->Data[Address];
    return Result;
}

static void WriteToMemory(memory *Memory, uint8 *Data, size_t Size, uint32 BaseAddress)
{
    assert(BaseAddress < ArrayCount(Memory->Data));
    assert(Size < ArrayCount(Memory->Data));

    for(size_t DataIndex = 0; DataIndex < Size; DataIndex++)
    {
        Memory->Data[BaseAddress + DataIndex] = Data[DataIndex];
    }
}

static void StackPush(chip8 *Chip8, uint16 Address)
{
    Chip8->Stack[Chip8->StackPointer++] = Address;

    if(Chip8->StackPointer >= STACK_SIZE)
    {
        TraceLog(LOG_WARNING, "Stack overflow");
    }
}

static uint16 StackPop(chip8 *Chip8)
{
    Chip8->StackPointer--;

    if(Chip8->StackPointer < 0)
    {
        TraceLog(LOG_WARNING, "Stack underflow");
    }

    return Chip8->Stack[Chip8->StackPointer];
}

static void InitEmulator(chip8 *Chip8, memory *Memory)
{
    Chip8->DelayTimer = 0;
    Chip8->SoundTimer = 0;

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
        0xF0, 0x80, 0xF0, 0x80, 0x80, // F
    };

    WriteToMemory(Memory, Font, ArrayCount(Font), FONT_ADDRESS);
}
    
void InputAudioCallback(void *Buffer, uint32 Frames)
{
    uint16 *Data = (uint16*)Buffer;

    int32 ToneHz = 256;
    int32 WavePeriod = 48000 / ToneHz;

    for(uint32 Index = 0; Index < Frames; Index++)
    {
        real32 SineValue = sinf(tSine);
        int16 SampleValue = (int16)(SineValue*2000);

        *Data++ = SampleValue;
        *Data++ = SampleValue;

        tSine += 2.0f*PI*1.0f/(real32)WavePeriod;
        if(tSine > 2.0f*PI)
        {
            tSine -= 2.0f*PI;
        }
    }

}

int main(int ArgCount, char **ArgValues)
{
    cpu CPU = {};
    CPU.ProgramCounter = PC_START_ADDRESS;

    chip8 Chip8 = {};

    memory Memory = {};

    InitEmulator(&Chip8, &Memory);

    char *Filename = "Beep.ch8";
    int FileSize = GetFileLength(Filename);
    uint8 *ROMData = LoadFileData(Filename, &FileSize);
    if(FileSize > 0 && ROMData)
    {
        WriteToMemory(&Memory, ROMData, FileSize, PC_START_ADDRESS);
    }
    
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Chip8");
    
    InitAudioDevice();
    AudioStream Stream = {};
    if(IsAudioDeviceReady())
    {
        Stream = LoadAudioStream(44100, 16, 2);
        SetAudioStreamBufferSizeDefault(4096);
        SetAudioStreamCallback(Stream, InputAudioCallback);
    }

    real32 AccumulatorRenderTime = 0.0f;
    
    uint32 CPUClockHz = 480;
    uint32 TimingHz = 60;

    real32 InstructionsPerFrame = (real32)CPUClockHz/(real32)TimingHz;
    real32 TimingPeriod = 1.0f/(real32)TimingHz;
    
    real64 LastTime = GetTime();
    while(!WindowShouldClose())
    {
        real64 EndTime = GetTime();
        real32 DeltaTime = (real32)(EndTime - LastTime);
        LastTime = EndTime;

        AccumulatorRenderTime += DeltaTime;

        while(AccumulatorRenderTime > TimingPeriod)
        {
            // Input
            Chip8.Keypad[1] = IsKeyDown(KEY_ONE) ? 1 : 0;
            Chip8.Keypad[2] = IsKeyDown(KEY_TWO) ? 1 : 0;
            Chip8.Keypad[3] = IsKeyDown(KEY_THREE) ? 1 : 0;
            Chip8.Keypad[4] = IsKeyDown(KEY_Q) ? 1 : 0;
            Chip8.Keypad[5] = IsKeyDown(KEY_W) ? 1 : 0;
            Chip8.Keypad[6] = IsKeyDown(KEY_E) ? 1 : 0;
            Chip8.Keypad[7] = IsKeyDown(KEY_A) ? 1 : 0;
            Chip8.Keypad[8] = IsKeyDown(KEY_S) ? 1 : 0;
            Chip8.Keypad[9] = IsKeyDown(KEY_D) ? 1 : 0;
            Chip8.Keypad[0] = IsKeyDown(KEY_X) ? 1 : 0;
            Chip8.Keypad[10] = IsKeyDown(KEY_Z) ? 1 : 0;
            Chip8.Keypad[11] = IsKeyDown(KEY_C) ? 1 : 0;
            Chip8.Keypad[12] = IsKeyDown(KEY_FOUR) ? 1 : 0;
            Chip8.Keypad[13] = IsKeyDown(KEY_R) ? 1 : 0;
            Chip8.Keypad[14] = IsKeyDown(KEY_F) ? 1 : 0;
            Chip8.Keypad[15] = IsKeyDown(KEY_V) ? 1 : 0;

            for(int32 InstructionCount = 0; InstructionCount < InstructionsPerFrame; InstructionCount++)
            {
                Cycle(&CPU, &Chip8, &Memory);
            }

            if(Chip8.DelayTimer > 0)
            {
                Chip8.DelayTimer--;
            }

            if(Chip8.SoundTimer > 0)
            {
                Chip8.SoundTimer--;
                PlayAudioStream(Stream);
            }
            else
            {
                IsAudioStreamPlaying(Stream);
                StopAudioStream(Stream);
            }

            int32 PixelSize = 10;
            int32 ScreenWidth = DISPLAY_WIDTH*PixelSize;
            int32 ScreenHeight = DISPLAY_HEIGHT*PixelSize;

            BeginDrawing();
            ClearBackground(GRAY);

            uint8 *Row = (uint8*)Chip8.Display; 

            for(uint8 Y = 0; Y < DISPLAY_HEIGHT; Y++)
            {
                for(uint8 X = 0; X < DISPLAY_WIDTH; X++)
                {
                    uint8 Value = (uint8)Chip8.Display[Y*DISPLAY_WIDTH + X];
                    Color Pixel = BLACK;
                    if(Value == 1)
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

            DrawRectangle(DISPLAY_WIDTH*PixelSize, 0, WINDOW_WIDTH, WINDOW_HEIGHT, WHITE);
            DrawRectangle(0, DISPLAY_HEIGHT*PixelSize, DISPLAY_WIDTH*PixelSize, WINDOW_HEIGHT, WHITE);

            EndDrawing();

            AccumulatorRenderTime -= TimingPeriod;
        }
    }

    CloseAudioDevice();
    return 0;
}
