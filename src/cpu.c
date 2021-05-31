
#include "cpu.h"
#include "mem.h"
#include <stdio.h>

static int calc(struct pcb_t * proc) {
	return ((unsigned long)proc & 0UL);
}

static int alloc(struct pcb_t * proc, uint32_t size, uint32_t reg_index) {
	addr_t addr = alloc_mem(size, proc);
	if (addr == 0) {
		return 1;
	}else{
		//register luu physical address truoc khi allocate.
		proc->regs[reg_index] = addr;
		return 0;
	}
}

static int free_data(struct pcb_t * proc, uint32_t reg_index) {
	return free_mem(proc->regs[reg_index], proc);
}

static int read(
		struct pcb_t * proc, // Process executing the instruction
		uint32_t source, // Index of source register
		uint32_t offset, // Source address = [source] + [offset]
		uint32_t destination) { // Index of destination register
	//**Chu thich: 
	//  - Source register chua dia chi (index) cua page can doc.
	//  - Destination register chua dia chi (index) cua page can duoc luu
	//  - read_mem: Luu gia tri tai dia chi source + offset, luu vao data.
	BYTE data;
	if (read_mem(proc->regs[source] + offset, proc,	&data)) {
		proc->regs[destination] += data;
		return 0;		
	}else{
		return 1;
	}
}

static int write(
		struct pcb_t * proc, // Process executing the instruction
		BYTE data, // Data to be wrttien into memory
		uint32_t destination, // Index of destination register
		uint32_t offset) { 	// Destination address =
					// [destination] + [offset]
	printf("Write-data: %d \n", (uint32_t)data);
	printf("address: %d\n", proc->regs[destination]);

	return write_mem(proc->regs[destination] + offset, proc, data);
} 

int run(struct pcb_t * proc) {
	/* Check if Program Counter point to the proper instruction */
	if (proc->pc >= proc->code->size) {
		printf("PC is full\n");
		return 1;
	}
	
	//**Chu thich:
	//  - State tra ve la 0 neu success, con lai la fault

	struct inst_t ins = proc->code->text[proc->pc];
	proc->pc++;
	int stat = 1;
	printf("Process: pc = %d, code-size = %d\n", proc->pc, proc->code->size);
	switch (ins.opcode) {
	case CALC:
		stat = calc(proc);
		break;
	case ALLOC:
		stat = alloc(proc, ins.arg_0, ins.arg_1);
		break;
	case FREE:
		stat = free_data(proc, ins.arg_0);
		break;
	case READ:
		stat = read(proc, ins.arg_0, ins.arg_1, ins.arg_2);
		break;
	case WRITE:
		printf("WRITE-LABEL: %d\n", (int)ins.arg_2);
		stat = write(proc, ins.arg_0, ins.arg_1, ins.arg_2);
		break;
	default:
		stat = 1;
	}
	
	return stat;

}


