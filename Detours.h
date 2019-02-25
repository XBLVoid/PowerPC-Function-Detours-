

// PowerPC Function Detours


void __declspec(naked) GLPR(void)
{
	__asm
	{
		std     r14, -0x98(sp)
		std     r15, -0x90(sp)
		std     r16, -0x88(sp)
		std     r17, -0x80(sp)
		std     r18, -0x78(sp)
		std     r19, -0x70(sp)
		std     r20, -0x68(sp)
		std     r21, -0x60(sp)
		std     r22, -0x58(sp)
		std     r23, -0x50(sp)
		std     r24, -0x48(sp)
		std     r25, -0x40(sp)
		std     r26, -0x38(sp)
		std     r27, -0x30(sp)
		std     r28, -0x28(sp)
		std     r29, -0x20(sp)
		std     r30, -0x18(sp)
		std     r31, -0x10(sp)
		stw     r12, -0x8(sp)
		blr
	}
}

unsigned long RelinkGPLR(unsigned long SFSOffset, unsigned long* SaveStubAddress, unsigned long* OriginalAddress)
{
	unsigned long Instruction;
	unsigned long Replacing;
	unsigned long* Saver = (unsigned long*)GLPR;

	if (SFSOffset & 0x2000000) SFSOffset = SFSOffset | 0xFC000000;

	Replacing = OriginalAddress[SFSOffset / 4];

	for (int i = 0; i < 20; i++)
	{
		if (Replacing == Saver[i])
		{
			unsigned long NewOffset = (unsigned long)&Saver[i] - (unsigned long)SaveStubAddress;
			Instruction = 0x48000001 | (NewOffset & 0x3FFFFFC);
		}
	}
	return Instruction;
}

void PatchInJump(unsigned long *Address, unsigned long *Function)
{
	int FunctionAddress;

		Address ? FunctionAddress = (unsigned long)Function : FunctionAddress = 0;

	if (FunctionAddress)
	{
		Address[0] = 0x3D600000 + (((FunctionAddress >> 16) & 0xFFFF) + (FunctionAddress & 0x8000 ? 1 : 0)); // lis r11, FunctionAddress 
		Address[1] = 0x396B0000 + (FunctionAddress & 0xFFFF); // Addi r11, r11
		Address[2] = 0x7D6903A6; // mtcr r11
		Address[3] = 0x4E800420; // bctr
	}
}

void DetourStart(unsigned long *Address, unsigned long *Function, unsigned long *Stub)
{
    int JumpAddress;

	Address && Stub ? JumpAddress = (unsigned long)(Address + 4) : JumpAddress = 0;
    
    if (JumpAddress)
    {
		Stub[0] = 0x3D600000 + (((JumpAddress >> 16) & 0xFFFF) + (JumpAddress & 0x8000 ?  1 : 0)); // lis r11, JumpAddress
        Stub[1] = 0x396B0000 + (JumpAddress & 0xFFFF); // Addi r11, r11
        Stub[2] = 0x7D6903A6; // mtcr r11
		for (int i = 0; i < 4; i++)
		{
			(Address[i] & 0x48000003) == 0x48000001 ? Stub[i + 3] = RelinkGPLR((Address[i] & ~0x48000003), &Stub[i + 3], &Address[i]) : Stub[i + 3] = Address[i];
		}
        Stub[7] = 0x4E800420; // bctr

        __dcbst(0, Stub); 
		__sync(); 
		__isync(); 
        PatchInJump(Address, Function);
    }
}

/*
	Detour Example:

	void __declspec(naked) ExampleStub(...)
	{
		_asm
		{
			li r3, 1
			nop
			nop
			nop
			nop
			nop
			nop
			blr
		}
	}

	void ExampleHook(int x, int y)
	{
		x += 50;
		y += 50;
		return ExampleStub(x, y);
	}

	DetourStart((unsigned long*)0x1337, (unsigned long*)ExampleHook, (unsigned long*)ExampleStub);
		    Adddress to Hook	     Address of HookFunction	  Address of Save Stub	
*/
