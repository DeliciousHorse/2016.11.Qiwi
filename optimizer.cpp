#include "ex4bfoptimizer.h"

enum State {
	Find_Loop,
	Store_Sym,
	Check_Sym,
	Check_Loop_End
};

void optimizeBf(uint8_t* program, size_t programSize) {
	size_t start = 0;
	size_t locGpc = start;

	State state = Find_Loop;
	size_t loopStartGpc = 0;
	int symId = 0;
	int sym[2];
	int symCount[2];

	while (locGpc < programSize) {
		switch (state) {
		case Find_Loop:
			if ('[' == program[locGpc]) {
				loopStartGpc = locGpc;
				symId = 0;
				sym[0] = 0;
				state = Store_Sym;
			}
			locGpc++;
			break;
		case Store_Sym:
			if ('+' == program[locGpc] || '-' == program[locGpc]) {
				sym[symId] = program[locGpc];
				symCount[symId] = 1;
				state = Check_Sym;
				locGpc++;
			}
			else {
				state = Find_Loop;
			}
			break;
		case Check_Sym:
			if (sym[symId] == program[locGpc]) {
				++symCount[symId];
				if (symCount[symId] > 126) {
					state = Find_Loop;
				} else {
					locGpc++;
				}
			}
			else if ('>' == program[locGpc] && 0 == symId) {
				symId = 1;
				sym[1] = 0;
				state = Store_Sym;
				locGpc++;
			}
			else if ('<' == program[locGpc] && 1 == symId) {
				state = Check_Loop_End;
				locGpc++;
			}
			else {
				state = Find_Loop;
			}
			break;
		case Check_Loop_End:
			if (']' == program[locGpc]) {
				program[loopStartGpc++] = 'M';
				program[loopStartGpc++] = (sym[0] == '-') ? -symCount[0] : symCount[0];
				program[loopStartGpc++] = (sym[1] == '-') ? -symCount[1] : symCount[1];
				locGpc++;

				for (; loopStartGpc < locGpc; ++loopStartGpc)
					program[loopStartGpc] = ' ';

			}
			state = Find_Loop;
			break;
		}
	}
}
