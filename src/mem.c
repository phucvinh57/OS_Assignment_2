#include "mem.h"
#include "stdlib.h"
#include "string.h"
#include <pthread.h>
#include <stdio.h>

static BYTE _ram[RAM_SIZE];

static struct
{
	uint32_t proc; // ID of process currently uses this page
	int index;	  // Index of the page in the list of pages allocated to the process.
	int next;	 // The next page in the list. -1 if it is the last page.
} _mem_stat[NUM_PAGES];

static pthread_mutex_t mem_lock;

void init_mem(void)
{
	memset(_mem_stat, 0, sizeof(*_mem_stat) * NUM_PAGES);
	memset(_ram, 0, sizeof(BYTE) * RAM_SIZE);
	pthread_mutex_init(&mem_lock, NULL);
}

/* get offset of the virtual address */
static addr_t get_offset(addr_t addr)
{
	return addr & ~((~0U) << OFFSET_LEN);
}

/* get the first layer index */
static addr_t get_first_lv(addr_t addr)
{
	return addr >> (OFFSET_LEN + PAGE_LEN);
}

/* get the second layer index */
static addr_t get_second_lv(addr_t addr)
{
	return (addr >> OFFSET_LEN) - (get_first_lv(addr) << PAGE_LEN);
}

/* Search for page table table from the a segment table */
static struct page_table_t *get_page_table(
	addr_t index, // Segment level index
	struct seg_table_t *seg_table)
{
	int i;
	for (i = 0; i < seg_table->size; i++)
	{
		// Enter your code here
		if (seg_table->table[i].v_index == index)
			return seg_table->table[i].pages;
	}
	return NULL;
}

/* Translate virtual address to physical address. If [virtual_addr] is valid,
 * return 1 and write its physical counterpart to [physical_addr].
 * Otherwise, return 0 */
static int translate(
	addr_t virtual_addr,   // Given virtual address
	addr_t *physical_addr, // Physical address to be returned
	struct pcb_t *proc)
{ // Process uses given virtual address

	/* Offset of the virtual address */
	addr_t offset = get_offset(virtual_addr);
	/* The first layer index */
	addr_t first_lv = get_first_lv(virtual_addr);
	/* The second layer index */
	addr_t second_lv = get_second_lv(virtual_addr);

	/* Search in the first level */
	struct page_table_t *page_table = NULL;
	page_table = get_page_table(first_lv, proc->seg_table);
	if (page_table == NULL)
	{
		return 0;
	}

	int i;
	for (i = 0; i < page_table->size; i++)
	{
		if (page_table->table[i].v_index == second_lv)
		{
			*physical_addr = (page_table->table[i].p_index << OFFSET_LEN) + offset;
			return 1;
		}
	}
	return 0;
}

addr_t alloc_mem(uint32_t size, struct pcb_t *proc)
{
	pthread_mutex_lock(&mem_lock);
	addr_t ret_mem = 0;
	
	/* TODO: Allocate [size] byte in the memory for the
	 * process [proc] and save the address of the first
	 * byte in the allocated memory region to [ret_mem].
	 * */

	uint32_t num_pages = (size % PAGE_SIZE == 0) ? size / PAGE_SIZE : size / PAGE_SIZE + 1; // Number of pages we will use
	int mem_avail = 0;																		// We could allocate new memory region or not?

	/* First we must check if the amount of free memory in
	 * virtual address space and physical address space is
	 * large enough to represent the amount of required 
	 * memory. If so, set 1 to [mem_avail].
	 * Hint: check [proc] bit in each page of _mem_stat
	 * to know whether this page has been used by a process.
	 * For virtual memory space, check bp (break pointer).
	 * */

	// we first check for available physical space
	int num_available_frames = 0;
	for (int i = 0; i < NUM_PAGES; i++)
	{
		if (_mem_stat[i].proc == 0)
		{
			num_available_frames++;
			if (num_available_frames == num_pages)
				break;
		}
	}

	// next, we check for available virtual space of the process
	if (num_available_frames >= num_pages &&
		proc->bp + PAGE_SIZE * num_pages <= RAM_SIZE)
		mem_avail = 1;

	if (mem_avail)
	{
		/* We could allocate new memory region to the process */
		ret_mem = proc->bp;
		proc->bp += num_pages * PAGE_SIZE;
		/* Update status of physical pages which will be allocated
		 * to [proc] in _mem_stat. Tasks to do:
		 * 	- Update [proc], [index], and [next] field
		 * 	- Add entries to segment table page tables of [proc]
		 * 	  to ensure accesses to allocated memory slot is
		 * 	  valid. */

		int num_pages_before = ret_mem / PAGE_SIZE;
		int num_pages_new = 0;
		int prev_frame = -1;

		for (int frame_walker = 0; frame_walker < NUM_PAGES; ++frame_walker)
		{
			if (_mem_stat[frame_walker].proc == 0) // found an available frame
			{
				// first task, update [proc], [index], and [next] field
				_mem_stat[frame_walker].proc = proc->pid;
				_mem_stat[frame_walker].index = num_pages_before + num_pages_new;
				_mem_stat[frame_walker].next = -1;

				if (prev_frame != -1)
					_mem_stat[prev_frame].next = frame_walker;

				prev_frame = frame_walker;

				// second task, we add entries to segment table page tables of [proc]
				// first search for the corresponding page table of the virtual address
				addr_t v_addr = ret_mem + num_pages_new * PAGE_SIZE;
				addr_t first_lv = get_first_lv(v_addr);
				addr_t second_lv = get_second_lv(v_addr);

				struct page_table_t *page_table = get_page_table(first_lv, proc->seg_table); // TODO: check this

				if (page_table == NULL)
				{
					// create the page table // TODO: what if cannot allocate new page table
					page_table = (struct page_table_t *)malloc(sizeof(struct page_table_t));
					page_table->size = 0;
					proc->seg_table->table[proc->seg_table->size].v_index = first_lv;
					proc->seg_table->table[proc->seg_table->size].pages = page_table;
					proc->seg_table->size++;
				}

				// add the entry into page_table
				// if (page_table->size == (1 << PAGE_LEN)) // TODO: page is full, cannot add any entry
				// 	return 0;
				page_table->table[page_table->size].v_index = second_lv;
				page_table->table[page_table->size].p_index = frame_walker;
				page_table->size++;

				// update number of newly allocated pages
				num_pages_new++;
				if (num_pages_new == num_pages)
					break;
			}
		}
	}
	pthread_mutex_unlock(&mem_lock);
	return ret_mem;
}

int free_mem(addr_t address, struct pcb_t *proc)
{
	/*TODO: Release memory region allocated by [proc]. The first byte of
	 * this region is indicated by [address]. Task to do:
	 * 	- Set flag [proc] of physical page use by the memory block
	 * 	  back to zero to indicate that it is free.
	 * 	- Remove unused entries in segment table and page tables of
	 * 	  the process [proc].
	 * 	- Remember to use lock to protect the memory from other
	 * 	  processes.  */

	addr_t physical_addr;
	if (translate(address, &physical_addr, proc)) // valid virtual address
	{
		pthread_mutex_lock(&mem_lock);
		physical_addr = physical_addr >> OFFSET_LEN;
		while (physical_addr != -1)
		{
			addr_t first_lv = get_first_lv(address);
			addr_t second_lv = get_second_lv(address);
			addr_t temp = _mem_stat[physical_addr].next;

			_mem_stat[physical_addr].proc = 0;
			_mem_stat[physical_addr].next = -1;
			_mem_stat[physical_addr].index = -1;

			struct page_table_t * page_table;
			page_table = get_page_table(first_lv, proc->seg_table);
			// search the page_table for victim entry
			int victim = 0;
			for (; victim < page_table->size; victim++)
				if (page_table->table[victim].v_index == second_lv)
					break;

			// delete the victim entry
			// for (int i = victim; i < page_table->size - 1; ++i)
			// {
			// 	page_table->table[i].p_index = page_table->table[i + 1].p_index;
			// 	page_table->table[i].v_index = page_table->table[i + 1].v_index;
			// }
			
			int tail_index = page_table->size - 1;
			page_table->table[victim].p_index = page_table->table[tail_index].p_index;
			page_table->table[victim].v_index = page_table->table[tail_index].v_index;

			physical_addr = temp;
			page_table->size--;
			address += PAGE_SIZE;
		}
		pthread_mutex_unlock(&mem_lock);
		return 0;
	}
	else
		return 1;
}

int read_mem(addr_t address, struct pcb_t *proc, BYTE *data)
{
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc))
	{
		*data = _ram[physical_addr];
		return 0;
	}
	else
	{
		return 1;
	}
}

int write_mem(addr_t address, struct pcb_t *proc, BYTE data)
{
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc))
	{
		_ram[physical_addr] = data;
		return 0;
	}
	else
	{
		return 1;
	}
}

void dump(void)
{
	int i;
	for (i = 0; i < NUM_PAGES; i++)
	{
		if (_mem_stat[i].proc != 0)
		{
			printf("%03d: ", i);
			printf("%05x-%05x - PID: %02d (idx %03d, nxt: %03d)\n",
				   i << OFFSET_LEN,
				   ((i + 1) << OFFSET_LEN) - 1,
				   _mem_stat[i].proc,
				   _mem_stat[i].index,
				   _mem_stat[i].next);
			int j;
			for (j = i << OFFSET_LEN;
				 j < ((i + 1) << OFFSET_LEN) - 1;
				 j++)
			{

				if (_ram[j] != 0)
				{
					printf("\t%05x: %02x\n", j, _ram[j]);
				}
			}
		}
	}
}
