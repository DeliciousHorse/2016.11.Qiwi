enum Register {
	Rax, Rcx, Rdx, Rbx, Rsp, Rbp, Rsi, Rdi
};

void genPush(uint8_t*& dst, Register r) {
	*dst++ = 0x50 | r;
}

void genPop(uint8_t*& dst, Register r) {
	*dst++ = 0x58 | r;
}

void genMovVal64(uint8_t*& dst, Register r, uint64_t value) {
	*dst++ = 0x48;
	*dst++ = 0xB8 | r;
	*(uint64_t*)dst = value;
	dst += 8;
}

void genRet(uint8_t*& dst) {
	*dst++ = 0xC3;
}

void genInc(uint8_t*& dst, Register r) {
	*dst++ = 0x48;
	*dst++ = 0xFF;
	*dst++ = 0xC0 | r;
}

void genDec(uint8_t*& dst, Register r) {
	*dst++ = 0x48;
	*dst++ = 0xFF;
	*dst++ = 0xC8 | r;
}

void genIncPtr(uint8_t*& dst, Register r) {
	*dst++ = 0xFE;
	*dst++ = r;
}

void genDecPtr(uint8_t*& dst, Register r) {
	*dst++ = 0xFE;
	*dst++ = 0x08 | r;
}


void genCmpPtr(uint8_t*& dst, Register r, uint8_t value) {
	if (Rsp == r || Rbp == r)
		throw std::runtime_error("I'm to lazy to add modr/m encoding");

	*dst++ = 0x80;
	// 0011 1...
	*dst++ = 0x38 | r;
	*dst++ = value;
}

void genShortJnz(uint8_t*& dst, uint8_t value) {
	*dst++ = 0x75;
	*dst++ = value;
}

void genRelJmp(uint8_t*& dst, uint32_t value) {
	*dst++ = 0xE9;
	*(uint32_t*)dst = value;
	dst += 4;
}

void genSub(uint8_t*& dst, Register r, uint8_t value) {
	*dst++ = 0x48;
	*dst++ = 0x83;
	// 11 | 101 | reg
	*dst++ = 0xC0 | (5 << 3) | r;
	*dst++ += value;
}

void genAdd(uint8_t*& dst, Register r, uint8_t value) {
	*dst++ = 0x48;
	*dst++ = 0x83;
	// 11 | 000 | reg
	*dst++ = 0xC0 | (0 << 3) | r;
	*dst++ += value;
}

void realGenerate(uint8_t* dst, const uint8_t* program, size_t programSize, uint8_t* memory, size_t memorySize, FILE* output)
{
	uint8_t* originalDst = dst;
	size_t start = 0;
	size_t locGpc = start;
	std::stack<uint8_t*> loopStart;

	// save registers
	genPush(dst, Rdi);
	genMovVal64(dst, Rdi, (uint64_t)memory);
	genSub(dst, Rsp, 0x20);

	uint8_t* jumpDestination;
	while (locGpc < programSize)
	{
		switch (program[locGpc])
		{
		case '+':
			genIncPtr(dst, Rdi);
			locGpc++; break;
		case '-':
			genDecPtr(dst, Rdi);
			locGpc++; break;
		case '>':
			genInc(dst, Rdi);
			locGpc++; break;
		case '<':
			genDec(dst, Rdi);
			locGpc++; break;
		case '.':
			//| int3
			genMovVal64(dst, Rax, (uint64_t)&fputc);
			genMovVal64(dst, Rdx, (uint64_t)output);
			// mov cl, [rdi]
			*dst++ = 0x8a;
			*dst++ = 0xf;
			// call rax
			*dst++ = 0xff;
			*dst++ = 0xd0;
			locGpc++; break;
		case ',':
			*dst++ = 0xCC;
			locGpc++; break;

		case '[':
			// we'll change logic here a bit if data under rdi is NOT zero we skip
			// long jmp, and if it's zero this jmp will be taken
			loopStart.push(dst);

			genCmpPtr(dst, Rdi, 0);
			// next jump is 5 bytes long
			genShortJnz(dst, 5);
			// we will fill this later when processing ']'
			genRelJmp(dst, 0);

			locGpc++; break;

		case ']':
			jumpDestination = loopStart.top();
			loopStart.pop();
			// first let's generate jump back... additional 5 is the length of current jmp instruction
			genRelJmp(dst, jumpDestination - dst - 5);

			// now, let's fix dummy jump
			// skip cmp and jnz
			jumpDestination += 5;
			// additional 5 for jmp
			genRelJmp(jumpDestination, dst - jumpDestination - 5);

			locGpc++; break;
		}
	}

	genAdd(dst, Rsp, 0x20);
	genPop(dst, Rdi);

	*dst++ = 0xC3;
	*dst++ = 0xCC;
}
