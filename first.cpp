// NOTE: this is NOT a cpp file, this is input file for dynasm
//
|.arch x64
|.section code
|.globals GLOB_
|.actionlist actions
|
|.macro saveregs
|    push rdi;
|.endmacro
|.macro restoreregs
|    pop rdi;
|.endmacro

#include "dynasm-helper.h"
#include <iostream>
#include <stack>
#include <stdint.h>


int labelsCount = 1;
FILE* g_output = nullptr;

bool dynasmGenerator(Dst_DECL, const uint8_t* program, size_t programSize, uint8_t* memory, size_t memorySize, FILE* output)
{
	// it's actually easier to just count the labels, but generate them dynamically as we'll go...
	for (size_t i = 0; i < programSize; ++i)
		if (program[i] == '[' || program[i] == ']')
			labelsCount++;

	Dst->growPc(labelsCount);
	std::cerr << "labels count: " << labelsCount << std::endl;

	uint64_t natMachineMem = (uint64_t)memory;
	uint64_t uPrinter = (uint64_t)&fputc;
	uint64_t uOutput = (uint64_t)g_output;
	g_output = output;

	| saveregs
	| sub rsp, 32
	|
	| mov64 rdi, natMachineMem
	| xor rax, rax

	size_t start = 0;
	size_t locGpc = start;
	std::stack<int> loopStart;
	int dynamicLabelId = 1;
	int jumpLabel;
	std::cerr << programSize << std::endl;
	while (locGpc < programSize)
	{
		switch (program[locGpc])
		{
		case '+':
			| inc byte [rdi]
			locGpc++; break;
		case '-':
			| dec byte [rdi]
			locGpc++; break;
		case '>':
			| inc rdi
			locGpc++; break;
		case '<':
			| dec rdi
			locGpc++; break;
		case '.':
			| mov64 rax, uPrinter
			| mov64 rdx, uOutput
			| mov rcx, [rdi]
			| call rax
			locGpc++; break;
		case ',':
			| int3
			locGpc++; break;

		case '[':
			|=> dynamicLabelId:
			| cmp byte [rdi], 0
			| jz => (dynamicLabelId+1)
			loopStart.push(dynamicLabelId);
			dynamicLabelId += 2;
			locGpc++; break;
			break;

		case ']':
			jumpLabel = loopStart.top();
			loopStart.pop();
			| jmp => jumpLabel
			|=> (jumpLabel + 1):

			locGpc++; break;
		}
	}

| add rsp, 32
| restoreregs
| ret

	std::cerr << "zomg, aot done" << std::endl;
	return true;
}

