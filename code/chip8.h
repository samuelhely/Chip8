#ifndef CHIP8_H
#define CHIP8_H

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;

typedef uint32_t bool32;

typedef float real32;
typedef double real64;

#define Kilobytes(Value)((Value) * 1024)
#define Megabytes(Value)(Kilobytes(Value) * 1024)

#define ArrayCount(Array)(sizeof(Array)/sizeof((Array)[0]))

#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32
#define WINDOW_WIDTH 960
#define WINDOW_HEIGHT 540

#define STACK_SIZE 16
#define KEYPAD_SIZE 16

#define PC_START_ADDRESS 0x200
#define FONT_ADDRESS 0x50

struct key
{
    int32 Value;
    bool32 IsPressed;;
};

struct chip8
{
    uint16 Stack[STACK_SIZE];
    uint8 StackPointer;
    uint8 DelayTimer;
    uint8 SoundTimer;
    uint8 Keypad[KEYPAD_SIZE];
    uint8 Display[DISPLAY_WIDTH*DISPLAY_HEIGHT];
};

struct memory
{
    uint8 Data[Kilobytes(4)];
    uint8 Address;
};

real32 tSine = 0.0f;

static void InitEmulator(chip8 *Chip8, memory *Memory);

static uint8 ReadMemory(memory *Memory, uint32 Address);
static void WriteToMemory(memory *Memory, uint8 *Data, size_t Size, uint32 BaseAddress);

static void StackPush(chip8 *Chip8, uint16 Address);
static uint16 StackPop(chip8 *Chip8);

#endif // CHIP8_H
