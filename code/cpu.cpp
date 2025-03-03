static void Operations(cpu *CPU, uint8 VX, uint8 VY, uint8 N4)
{
    // NOTE(samuel): Logical and arithmetics operations.
    switch(N4)
    {
        case 0x0 :
        {
            CPU->Registers[VX] = CPU->Registers[VY];
        } break;

        case 0x1 :
        {
            CPU->Registers[VX] |= CPU->Registers[VY];
            CPU->Registers[0xF] = 0;
        } break;

        case 0x2 :
        {
            CPU->Registers[VX] &= CPU->Registers[VY];
            CPU->Registers[0xF] = 0;
        } break;

        case 0x3 :
        {
            CPU->Registers[VX] ^= CPU->Registers[VY];
            CPU->Registers[0xF] = 0;
        } break;

        case 0x4 :
        {
            uint8 Flag = ((CPU->Registers[VX] + CPU->Registers[VY]) > 0xFF);
            CPU->Registers[VX] += CPU->Registers[VY];
            CPU->Registers[0xF] = Flag;
        } break;

        case 0x5 :
        {
            uint8 Flag = (CPU->Registers[VX] >= CPU->Registers[VY]);
            CPU->Registers[VX] -= CPU->Registers[VY];
            CPU->Registers[0xF] = Flag;
        } break;

        case 0x6 :
        {
            uint8 Value = (uint8)((CPU->Registers[VX] & 0x1) > 0);
            CPU->Registers[VX] = CPU->Registers[VY] >> 1;
            CPU->Registers[0xF] = Value;
        } break;

        case 0x7 :
        {
            uint8 Flag = (CPU->Registers[VY] >= CPU->Registers[VX]);
            CPU->Registers[VX] = (CPU->Registers[VY] - CPU->Registers[VX]);
            CPU->Registers[0xF] = Flag;
        } break;

        case 0xE :
        {
            uint8 Value = (uint8)((CPU->Registers[VX] & (0x1 << 7)) > 0);
            CPU->Registers[VX] = CPU->Registers[VY] << 1;
            CPU->Registers[0xF] = Value;
        } break;
    }
}

void Draw(cpu *CPU, chip8 *Chip8, memory *Memory, uint8 VX, uint8 VY, uint8 N4)
{
    uint8 NumRows = N4;

    uint8 X = CPU->Registers[VX] % DISPLAY_WIDTH;
    uint8 Y = CPU->Registers[VY] % DISPLAY_HEIGHT;

    CPU->Registers[0xF] = 0;

    for(uint8 Row = 0; Row < NumRows; Row++)
    {
        uint8 Sprite = ReadMemory(Memory, CPU->IndexRegister + Row);

        if((Y + Row) > DISPLAY_HEIGHT)
        {
            break;
        }

        for(uint8 Bit = 0; Bit < 8; Bit++)
        {
            if((X + Bit) > DISPLAY_WIDTH)
            {
                break;
            }

            uint8 Pixel = (Sprite & (0x80 >> Bit)) >> (7 - Bit);
            uint8 *DisplayValue = &Chip8->Display[(Y + Row)*DISPLAY_WIDTH + (X + Bit)];

            if(Pixel && *DisplayValue)
            {
                CPU->Registers[0xF] = 1;
            }

            *DisplayValue ^= Pixel;
        }
    }
}

static void KeyPress(cpu *CPU, uint8 *Keypad, uint8 VX, uint8 NN)
{
    switch(NN)
    {
        case 0x9E :
        {
            if(Keypad[CPU->Registers[VX]])
            {
                CPU->ProgramCounter += 2;
            }
        } break;

        case 0xA1 :
        {
            if(!Keypad[CPU->Registers[VX]])
            {
                CPU->ProgramCounter += 2;
            }
        } break;
    }
}

static void Misc(cpu *CPU, chip8 *Chip8, memory *Memory, uint8 VX, uint8 NN)
{
    switch(NN)
    {
        case 0x07 :
        {
            CPU->Registers[VX] = Chip8->DelayTimer;
        } break;

        case 0x0A :
        {
            for(uint8 KeypadIndex = 0; KeypadIndex < KEYPAD_SIZE; KeypadIndex++)
            {
                if(Chip8->Keypad[KeypadIndex])
                {
                    CPU->Registers[VX] = KeypadIndex;
                    return;
                }
            }

            CPU->ProgramCounter -= 2;
        } break;

        case 0x15 :
        {
            Chip8->DelayTimer = CPU->Registers[VX];
        } break;

        case 0x18 :
        {
            Chip8->SoundTimer = CPU->Registers[VX];
        } break;

        case 0x1E :
        {
            // TODO(samuel): Is this a quirk?
            if((CPU->IndexRegister + CPU->Registers[VX]) > 0xFFF)
            {
                CPU->Registers[0xF] = 1;
            }

            CPU->IndexRegister += CPU->Registers[VX];
        } break;

        case 0x29 :
        {
            uint8 FontSize = 5;
            CPU->IndexRegister = (uint8)(FONT_ADDRESS + CPU->Registers[VX]*FontSize);
        } break;

        case 0x33 :
        {
            uint8 Value = CPU->Registers[VX];
            uint8 Data[] = {
                (uint8)(Value / 100),
                (uint8)((Value / 10) % 10),
                (uint8)(Value % 10)
            };
            WriteToMemory(Memory, Data, ArrayCount(Data), CPU->IndexRegister);
            //Memory[CPU->IndexRegister + 0] = (uint8)(Value / 100);
            //Memory[CPU->IndexRegister + 1] = (uint8)((Value / 10) % 10);
            //Memory[CPU->IndexRegister + 2] = (uint8)(Value % 10);
        } break;

        case 0x55 :
        {
            // TODO(samuel): Implement quirks
            for(uint8 Index = 0; Index <= VX; Index++)
            {
                WriteToMemory(Memory, &CPU->Registers[Index], sizeof(uint8), CPU->IndexRegister++);
                //Memory[CPU->IndexRegister++] = CPU->Registers[Index];
            }
            //CPU->IndexRegister++;
        } break;

        case 0x65 :
        {
            // TODO(samuel): Implement quirks
            for(uint8 Index = 0; Index <= VX; Index++)
            {
                CPU->Registers[Index] = ReadMemory(Memory, CPU->IndexRegister++);
                //CPU->Registers[Index] = Memory[CPU->IndexRegister++];
            }
            //CPU->IndexRegister++;
        } break;

    }
}

static void Cycle(cpu *CPU, chip8 *Chip8, memory *Memory)
{
    // Fetching
    uint16 OpCode = (uint16)(((uint16)ReadMemory(Memory, CPU->ProgramCounter) << 8) | ReadMemory(Memory, CPU->ProgramCounter + 1));
    if(OpCode)
    {
        CPU->ProgramCounter += 2;
    }

    // NOTE(samuel): Separate by nimbles
    uint8 N1 = (uint8)((OpCode & 0xF000) >> 12);
    uint8 VX = (uint8)((OpCode & 0x0F00) >> 8);
    uint8 VY = (uint8)((OpCode & 0x00F0) >> 4);
    uint8 N4 = (uint8)((OpCode & 0x000F) >> 0);

    uint8 NN = (uint8)(OpCode & 0x00FF);
    uint16 NNN = (OpCode & 0x0FFF);

    switch(N1)
    {
        case 0x0 :
        {
            if(OpCode == 0x00E0)
            {
                // NOTE(samuel): Clear the display
                for(uint32 ScreenIndex = 0; ScreenIndex < ArrayCount(Chip8->Display); ScreenIndex++)
                {
                    Chip8->Display[ScreenIndex] = 0;
                }
            }
            else if(OpCode == 0x00EE)
            {
                CPU->ProgramCounter = StackPop(Chip8);
            }
        } break;

        case 0x1 :
        {
            uint16 Address = NNN;
            CPU->ProgramCounter = Address;
        } break;

        case 0x2 :
        {
            StackPush(Chip8, CPU->ProgramCounter);

            uint16 Address = NNN;
            CPU->ProgramCounter = Address;
        } break;

        case 0x3 :
        {
            uint8 Value = NN;

            if(CPU->Registers[VX] == Value)
            {
                CPU->ProgramCounter += 2;
            }
        } break;

        case 0x4 :
        {
            uint8 Value = NN;

            if(CPU->Registers[VX] != Value)
            {
                CPU->ProgramCounter += 2;
            }
        } break;

        case 0x5 :
        {
            if(CPU->Registers[VX] == CPU->Registers[VY])
            {
                CPU->ProgramCounter += 2;
            }
        } break;

        case 0x6 :
        {
            uint8 Value = NN;

            CPU->Registers[VX] = Value;
        } break;

        case 0x7 :
        {
            uint8 Value = NN;

            CPU->Registers[VX] += Value;
        } break;

        case 0x8 :
        {
            Operations(CPU, VX, VY, N4);
        } break;

        case 0x9 :
        {
            if(CPU->Registers[VX] != CPU->Registers[VY])
            {
                CPU->ProgramCounter += 2;
            }
        } break;

        case 0xA :
        {
            uint16 Value = NNN;
            CPU->IndexRegister = Value;
        } break;

        case 0xB :
        {
            uint16 Value = NNN;
            CPU->ProgramCounter = (uint16)(Value + CPU->Registers[0]);
        } break;

        case 0xC :
        {
            uint8 Rand = (uint8)rand();
            uint8 Value = NN;
            CPU->Registers[VX] = (uint8)(Rand & Value);
        } break;

        case 0xD :
        {
            Draw(CPU, Chip8, Memory, VX, VY, N4);
        } break;

        case 0xE :
        {
            KeyPress(CPU, Chip8->Keypad, VX, NN);
        } break;

        case 0xF :
        {
            Misc(CPU, Chip8, Memory, VX, NN);
        } break;
    }
}
