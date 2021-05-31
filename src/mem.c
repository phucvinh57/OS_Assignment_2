
#include "mem.h"
#include "stdlib.h"
#include "string.h"
#include <pthread.h>
#include <stdio.h>

static BYTE _ram[RAM_SIZE];

//**Chu thich:
//  - _mem_state: Danh sach cac frame (Vi _mem_state co size
//  la 1 << ADDRESS_LEN - OFFSET_LEN)
//  ? Thanh phan index hoi bi du.

static struct {
	uint32_t proc;	// ID of process currently uses this page
	int index;	// Index of the page in the list of pages allocated
			// to the process
	int next;	// The next page in the list. -1 if it is the last
			// page.
} _mem_stat [NUM_PAGES]; 

static pthread_mutex_t mem_lock;

void init_mem(void) {
	memset(_mem_stat, 0, sizeof(*_mem_stat) * NUM_PAGES);
	memset(_ram, 0, sizeof(BYTE) * RAM_SIZE);
	pthread_mutex_init(&mem_lock, NULL);
}

/* get offset of the virtual address */
static addr_t get_offset(addr_t addr) {
	return addr & ~((~0U) << OFFSET_LEN);
}

/* get the first layer index */
static addr_t get_first_lv(addr_t addr) {
	return addr >> (OFFSET_LEN + PAGE_LEN);
}

/* get the second layer index */
static addr_t get_second_lv(addr_t addr) {
	return (addr >> OFFSET_LEN) - (get_first_lv(addr) << PAGE_LEN);
}

/* Search for page table table from the a segment table */
static struct page_table_t * get_page_table(
		addr_t index, 	// Segment level index
		struct seg_table_t * seg_table) { // first level table
	
	/*
	 * TODO: Given the Segment index [index], you must go through each
	 * row of the segment table [seg_table] and check if the v_index
	 * field of the row is equal to the index
	 *
	 * */

	//**Chu thich:
	//  - Kich thuoc cua segment table nho hon kich thuoc
	//  cua virtual memroy. Do do, index trong segment table
	//  co the dang chua trong page table, hoac khong (Page fault)
	//  - Mot virtual address co the co cung segment table index.
	//  ! Chuong trinh chi dung neu seg_table lan dau khoi tao, 
	//  seg_table->size == 0, neu segment->size > 0, page_table 
	//  phai chac chan khong NULL.
	for (int i = 0; i < seg_table->size; i++) {
		if (index == seg_table->table[i].v_index) {
			return seg_table->table[i].pages;
		}
	}
	return NULL;
}

/* Translate virtual address to physical address. If [virtual_addr] is valid,
 * return 1 and write its physical counterpart to [physical_addr].
 * Otherwise, return 0 */
static int translate(
		addr_t virtual_addr, 	// Given virtual address
		addr_t * physical_addr, //dd Physical aress to be returned
		struct pcb_t * proc) {  // Process uses given virtual address

	/* Offset of the virtual address */
	addr_t offset = get_offset(virtual_addr);
	/* The first layer index */
	addr_t first_lv = get_first_lv(virtual_addr);
	/* The second layer index */
	addr_t second_lv = get_second_lv(virtual_addr);
	
	/* Search in the first level */
	//**Chu thich:
	//  - Khi mot process moi duoc tao, segment table cua process
	//  se empty.
	//  ? size co == 0 mac dinh khong
	//  ? page_table co == NULL hay khong.
	//  ! physical address mac dinh xem da duoc cap bo nho
	struct page_table_t * page_table = NULL;
	page_table = get_page_table(first_lv, proc->seg_table);
	if (page_table == NULL) {
		//Khong tim thay segment index trong seg_table.
		return 0;
	}

	for (int i = 0; i < page_table->size; i++) {
		if (page_table->table[i].v_index == second_lv) {
			/* TODO: Concatenate the offset of the virtual addess
			 * to [p_index] field of page_table->table[i] to 
			 * produce the correct physical address and save it to
			 * [*physical_addr]  */
			
			//Tim thay page tuong ung voi second page address trong page table
			*physical_addr = 
				(page_table->table[i].p_index << OFFSET_LEN) +
				offset; 
			return 1;
		}
	}
	
	//Page fault
	return 0;	
}

addr_t alloc_mem(uint32_t size, struct pcb_t * proc) {
	printf("Allocate: Begin\n");
	pthread_mutex_lock(&mem_lock);
	addr_t ret_mem = 0;
	/* TODO: Allocate [size] byte in the memory for the
	 * process [proc] and save the address of the first
	 * byte in the allocated memory region to [ret_mem].
	 * */

	uint32_t num_pages = (size % PAGE_SIZE) ? size / PAGE_SIZE + 1:
		size / PAGE_SIZE; // Number of pages we will use
	int mem_avail = 0; // We could allocate new memory region or not?
	/* First we must check if the amount of free memory in
	 * virtual address space and physical address space is
	 * large enough to represent the amount of required 
	 * memory. If so, set 1 to [mem_avail].
	 * Hint: check [proc] bit in each page of _mem_stat
	 * to know whether this page has been used by a process.
	 * For virtual memory space, check bp (break pointer).
	 * */

	//**Chu thich:
	//  - memory allocate strategy: First fit.
	//  - _mem_state->proc == 0 nghia la khong co process nao dang giu page nay
	
	uint8_t isVirtualAddressEnough = 0;
	uint8_t isPageEnough = 0;
	
	//Check virtual address space:
	if ((proc->bp + num_pages * PAGE_SIZE) < (1 << 20)){
		isVirtualAddressEnough = 1;
	}

	//Check free page:
	uint32_t count = 0; //To count number of page is available
	uint32_t indexOfAvailableFrame[num_pages]; //index cua cac page available.
	for (int i = 0; i < NUM_PAGES; ++i) {
		if (_mem_stat[i].proc == 0) {
			indexOfAvailableFrame[count] = i;
			count++;
			if (count == num_pages) {
				isPageEnough = 1;
				break;
			}
		}
	}
	
	if (isVirtualAddressEnough && isPageEnough) {
		mem_avail = 1;
	}
	
	uint32_t lastVirtualAddressBeforUpdate = proc->bp;
	if (mem_avail) {
		/* We could allocate new memory region to the process */
		//Tra ve physical address cua page dau tien:
		ret_mem = indexOfAvailableFrame[0] << OFFSET_LEN;
		proc->bp += num_pages * PAGE_SIZE;
		/* Update status of physical pages which will be allocated
		 * to [proc] in _mem_stat. Tasks to do:
		 * 	- Update [proc], [index], and [next] field
		 * 	- Add entries to segment table page tables of [proc]
		 * 	  to ensure accesses to allocated memory slot is
		 * 	  valid. */

		//Update _mem_state:
		//**Tim index cua page cuoi cung truoc khi cap nhat de chuong trinh cap nhat
		//next_page cua index do = index dau tien cua danh sach page sap duoc cap
		//nhat.

		//*Tim index cua page dau tien:
		for (int i = 0; i < num_pages ; ++i) {
			_mem_stat[indexOfAvailableFrame[i]].proc = proc->pid;
			_mem_stat[indexOfAvailableFrame[i]].index = i;
			if (i + 1 < num_pages) {
				_mem_stat[indexOfAvailableFrame[i]].next = indexOfAvailableFrame[i + 1];
			}
			else {
				_mem_stat[indexOfAvailableFrame[i]].next = -1;
			}
		}

		uint8_t segmentIndex, pageIndex;
		for (int i = 0; i < num_pages; ++i) {
			segmentIndex = (lastVirtualAddressBeforUpdate >> 5) % SEGMENT_LEN;
			pageIndex = lastVirtualAddressBeforUpdate % PAGE_LEN;
			proc->seg_table->table[segmentIndex].v_index = segmentIndex;
			if (proc->seg_table->table[segmentIndex].pages == NULL) {
				proc->seg_table->table[segmentIndex].pages = 
					(struct page_table_t*)malloc(sizeof(struct page_table_t));
			}
			proc->seg_table->table[segmentIndex].pages->table[pageIndex].v_index = pageIndex;
			proc->seg_table->table[segmentIndex].pages->table[pageIndex].p_index = indexOfAvailableFrame[i];
			lastVirtualAddressBeforUpdate += PAGE_SIZE;
		}
	}
	pthread_mutex_unlock(&mem_lock);
	printf("--------------DONE----------------\n");
	dump();
	return ret_mem;
}

int free_mem(addr_t address, struct pcb_t * proc) {
	/*TODO: Release memory region allocated by [proc]. The first byte of
	 * this region is indicated by [address]. Task to do:
	 * 	- Set flag [proc] of physical page use by the memory block
	 * 	  back to zero to indicate that it is free.
	 * 	- Remove unused entries in segment table and page tables of
	 * 	  the process [proc].
	 * 	- Remember to use lock to protect the memory from other
	 * 	  processes.  */

	// ********************************************
	// ** Fix: free_mem nhan vao physical address:
	// *******************************************
	//Free trong _mem_state
	pthread_mutex_lock(&mem_lock);
	uint32_t indexInMemState = address >> OFFSET_LEN;
	int temp = _mem_stat[indexInMemState].index;
	while (indexInMemState != -1) {
		temp = _mem_stat[indexInMemState].next;
		_mem_stat[indexInMemState].proc = 0;
		_mem_stat[indexInMemState].index = -1;
		_mem_stat[indexInMemState].next = -1;
		indexInMemState = temp;
	}

	//TODO:Free in page table:
	// /* The first layer index */
	addr_t first_lv = get_first_lv(address);
	// /* The second layer index */
	addr_t second_lv = get_second_lv(address);
	// //Clear v-index to p-index
	proc->seg_table->table[first_lv].pages->table[second_lv].p_index = 0;
	

	//Dummy code:
	addr_t dummyPhysicAddress;
	translate(address, &dummyPhysicAddress, proc);

	pthread_mutex_unlock(&mem_lock);
	return 0;
}

int read_mem(addr_t address, struct pcb_t * proc, BYTE * data) {
	*data = _ram[address];
	return 0;
}

int write_mem(addr_t address, struct pcb_t * proc, BYTE data) {
	printf("data: %d\n", (uint32_t)(data));
	printf("address: %d\n", address);
	_ram[address] = data;
	return 1;
}

void dump(void) {
	for (int i = 0; i < NUM_PAGES; i++) {
		if (_mem_stat[i].proc != 0) {
			printf("%03d: ", i);
			printf("%05x-%05x - PID: %02d (idx %03d, nxt: %03d)\n",
				i << OFFSET_LEN,
				((i + 1) << OFFSET_LEN) - 1,
				_mem_stat[i].proc,
				_mem_stat[i].index,
				_mem_stat[i].next
			);
			for (int j = i << OFFSET_LEN;
				j < ((i+1) << OFFSET_LEN) - 1;
				j++) {
				
				if (_ram[j] != 0) {
					printf("\t%05x: %02x\n", j, _ram[j]);
				}
					
			}
		}
	}
}


