#include <stdio.h>
#include <stdint.h>

#define Kilobytes(Value)((Value) * 1024)
#define Megabytes(Value)(Kilobytes(Value) * 1024)

uint8_t Registers[16];
uint16_t OpCode;
uint8_t Memory[Kilobytes(4)];
uint16_t IndexRegister;
uint16_t ProgramCounter;
uint16_t Stack[16];
uint8_t StackPointer;
uint8_t DelayTimer;
uint8_t SoundTimer;
uint8_t Keypad[16];
uint8_t Display[64*32];

int main(int ArgCount, char **ArgValues)
{
    return 0;
}
