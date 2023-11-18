#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"

#define kilo (1024)
int allocateAndMapFrames(uint32 StartOfAllocation , uint32 sizeToAllocate )
{
	uint32 va = StartOfAllocation ;
	int cnt = (int)(ROUNDUP(sizeToAllocate, PAGE_SIZE) / (uint32)PAGE_SIZE) ;
	for(int i=0 ;i<cnt ;i++)
	{
	  //allocate frame
	  struct FrameInfo * ptr_frame_info =NULL;
	  int ret = allocate_frame(&ptr_frame_info);
	  if(ret == E_NO_MEM)
		  return E_NO_MEM ;
	  //map frame
	  map_frame(ptr_page_directory ,ptr_frame_info ,va , PERM_WRITEABLE ); // wee need to ask DR about permission
	  va += PAGE_SIZE ;
	}
	return  0;
}

void dellocateAndUnMapFrames(uint32 StartOfDeallocation , uint32 sizeToDeallocate)
{
	uint32 va =StartOfDeallocation - 4*kilo ;
	int cnt = (int)(ROUNDUP(sizeToDeallocate, PAGE_SIZE) / (uint32)PAGE_SIZE) ;
	for(int i=0 ;i<cnt ;i++)
	{
	  //unmap frames
	  unmap_frame(ptr_page_directory ,va);
	  va -= PAGE_SIZE ;
	}

}

int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit)
{

	//initialization of the variables ( dstart_of_kernal_heap , brk_pointer ,hard_limit)
	dstart_of_kernal_heap = daStart;
	uint32 brk = daStart + ROUNDUP(initSizeToAllocate, PAGE_SIZE) ;
	if(brk>daLimit)
		return E_NO_MEM ;
	brk_pointer = brk;
	hard_limit = daLimit ;
    // allocate And map frames
	int result =  allocateAndMapFrames(daStart ,initSizeToAllocate);
	if(result ==0)
	{
		initialize_dynamic_allocator(daStart,ROUNDUP(initSizeToAllocate, PAGE_SIZE)) ;
	}
	return result ;

	//TODO: [PROJECT'23.MS2 - #01] [1] KERNEL HEAP - initialize_kheap_dynamic_allocator()
	//Initialize the dynamic allocator of kernel heap with the given start address, size & limit
	//All pages in the given range should be allocated
	//Remember: call the initialize_dynamic_allocator(..) to complete the initialization
	//Return:
	//	On success: 0
	//	Otherwise (if no memory OR initial size exceed the given limit): E_NO_MEM

	//Comment the following line(s) before start coding...
	//panic("not implemented yet");
	return 0;
}

void* increment_break_pointer(int increment)
{
	uint32 size_to_increment = increment * kilo ;
	size_to_increment = ROUNDUP(size_to_increment, PAGE_SIZE) ;
	uint32 new_brk = brk_pointer +size_to_increment ;
	if(new_brk>hard_limit)
	{
		 panic("hard limit has been broken !!\n");
	}
	else
	{
		uint32 previous_brk_ptr = brk_pointer ;
		brk_pointer = new_brk ;
		allocateAndMapFrames(previous_brk_ptr ,size_to_increment) ;
		return (void*)previous_brk_ptr ;
	}
}

void* decrement_break_pointer(int increment)
{
	increment*=-1;
	uint32 size_to_decrement = increment * kilo ;
	size_to_decrement = ROUNDUP(size_to_decrement, PAGE_SIZE) ;
	uint32 new_brk = brk_pointer - size_to_decrement ;
	if(new_brk<dstart_of_kernal_heap)
	{
		panic("down of start of kernel heap !!\n");

	}
	else
	{
		uint32 previous_brk_ptr = brk_pointer ;
		brk_pointer =new_brk ;
		uint32 startAddressOfDeallocation = previous_brk_ptr ;
		dellocateAndUnMapFrames(startAddressOfDeallocation ,size_to_decrement) ;
		return (void*)brk_pointer ;
	}
}

void* sbrk(int increment)
{

	 if(increment==0)
	 {
		 return (void*)brk_pointer ;
	 }
	 else if(increment>0)
	 {
		 return increment_break_pointer(increment);
	 }
	 else if(increment<0)
	 {
		 return decrement_break_pointer(increment);
	 }

	//TODO: [PROJECT'23.MS2 - #02] [1] KERNEL HEAP - sbrk()
	/* increment > 0: move the segment break of the kernel to increase the size of its heap,
	 * 				you should allocate pages and map them into the kernel virtual address space as necessary,
	 * 				and returns the address of the previous break (i.e. the beginning of newly mapped memory).
	 * increment = 0: just return the current position of the segment break
	 * increment < 0: move the segment break of the kernel to decrease the size of its heap,
	 * 				you should deallocate pages that no longer contain part of the heap as necessary.
	 * 				and returns the address of the new break (i.e. the end of the current heap space).
	 *
	 * NOTES:
	 * 	1) You should only have to allocate or deallocate pages if the segment break crosses a page boundary
	 * 	2) New segment break should be aligned on page-boundary to avoid "No Man's Land" problem
	 * 	3) Allocating additional pages for a kernel dynamic allocator will fail if the free frames are exhausted
	 * 		or the break exceed the limit of the dynamic allocator. If sbrk fails, kernel should panic(...)
	 */
	//MS2: COMMENT THIS LINE BEFORE START CODING====
	return (void*)-1 ;
	//panic("not implemented yet");
}

void* kmalloc(unsigned int size) // done!
{
	int num_of_page_to_allocate = (int)(ROUNDUP(size, PAGE_SIZE) / (uint32)PAGE_SIZE) ;
	//cprintf("the size is %d\n",size);
	//cprintf("the num of pages %d\n",num_of_page_to_allocate);
	uint32 arr_of_page_adresses[num_of_page_to_allocate];
     if(size<=DYN_ALLOC_MAX_BLOCK_SIZE)
     {
    	 //cprintf("hereeeeee 1\n");
    	 return alloc_block_FF( size) ;
     }
     else
     {
    	 //cprintf("hereeeeee 2\n");
    	 uint32 va_to_check = hard_limit + PAGE_SIZE ;
    	 int cnt = 0 ;
    	 int is_allocated =0 ;
    	 while(va_to_check<KERNEL_HEAP_MAX)
    	 {

    		// cprintf("hereeeeee 3\n");
    		 if(is_allocated==1)
    			 break;
    		 // get the page table of the va
    		 uint32  *ptr_page_table =NULL ;
    		 int result = get_page_table(ptr_page_directory , va_to_check , &ptr_page_table);
    		 if(result ==TABLE_IN_MEMORY)
    		 {

    		//	 cprintf("hereeeeee 4\n");
    			 // get the page of the va
    			 uint32 target_page_address = ptr_page_table[PTX(va_to_check)];
    			// p=1 can be used
    			 uint32 temp_address = target_page_address & (~PERM_PRESENT);

    			if(target_page_address==temp_address)  // present = 1
    			{
    				//cprintf("hereeeeee 5\n");
    				arr_of_page_adresses[cnt]=va_to_check;
    				cnt++;
    			}
    			if(cnt==num_of_page_to_allocate)
				{
    				//cprintf("hereeeeee 6\n");
					for(int i =0 ; i<num_of_page_to_allocate ;i++)
					{
					//	cprintf("hereeeeee 7\n");
						 int ret = allocateAndMapFrames(arr_of_page_adresses[i] ,PAGE_SIZE);
						 if(ret == E_NO_MEM)
							 return NULL ;
					}
				//	cprintf("hereeeeee 8\n");
					is_allocated = 1;
				}
    			if(target_page_address!=temp_address)
    				cnt=0;
    		 }
    		 va_to_check+=PAGE_SIZE ;
    	 }
    	 if(is_allocated==0)
    		 return NULL ;
    	 else
    		 return (void*)arr_of_page_adresses[0];
    	// cprintf("hereeeeee 9\n");
     }

	//TODO: [PROJECT'23.MS2 - #03] [1] KERNEL HEAP - kmalloc()
	//refer to the project presentation and documentation for details
	// use "isKHeapPlacementStrategyFIRSTFIT() ..." functions to check the current strategy

	//change this "return" according to your answer
	//kpanic_into_prompt("kmalloc() is not implemented yet...!!");
	return NULL;
}

void kfree(void* virtual_address) // gets 15% eval
{
	//TODO: [PROJECT'23.MS2 - #04] [1] KERNEL HEAP - kfree()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	// panic("kfree() is not implemented yet...!!");
	// block allocator -> KERNEL_HEAP_START, HARD_LIMIT
	// page allocator -> HARD_LIMIT + PAGE_SIZE, KERNEL_HEAP_MAX
	if((uint32)virtual_address >= KERNEL_HEAP_START && (uint32)virtual_address <= hard_limit){
		free_block(virtual_address);
	}else if((uint32)virtual_address >= (hard_limit + PAGE_SIZE) && (uint32)virtual_address <= KERNEL_HEAP_MAX){
		// declare a null pointer for page table for future use.
		uint32* ptr_page_table = NULL;

		// get the page table.
		get_page_table(ptr_page_directory, (uint32)virtual_address, &ptr_page_table);

		// get the frame from page table.
		struct FrameInfo * target_frame = get_frame_info(ptr_page_directory, (uint32)virtual_address, &ptr_page_table);

		// free this frame.
		free_frame(target_frame);
	}else{
		panic("Address is invalid!!");
	}
}

unsigned int kheap_virtual_address(unsigned int physical_address) //gets 20% eval.
{
	//TODO: [PROJECT'23.MS2 - #05] [1] KERNEL HEAP - kheap_virtual_address()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	//panic("kheap_virtual_address() is not implemented yet...!!");

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================
	//change this "return" according to your answer

	if(physical_address >= KERNEL_HEAP_START && physical_address <= hard_limit)
	{
			// Block Allocator
			// V.A = P.A + Kernel_base
			unsigned int va = physical_address + KERNEL_BASE;
			return va;
	}
	else if(physical_address >= (hard_limit + PAGE_SIZE) && physical_address <= KERNEL_HEAP_MAX)
	{
			// Page Allocator
			struct FrameInfo* target_frame = to_frame_info(physical_address);
			// doesn't work because v.a always equal to zero!
			return target_frame->va;
	}
	return 0;
}

unsigned int kheap_physical_address(unsigned int virtual_address) //gets 40% eval.
{
	//TODO: [PROJECT'23.MS2 - #06] [1] KERNEL HEAP - kheap_physical_address()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	//panic("kheap_physical_address() is not implemented yet...!!");
	// block allocator -> KERNEL_HEAP_START, HARD_LIMIT
	// page allocator -> HARD_LIMIT + PAGE_SIZE, KERNEL_HEAP_MAX

	// Block Allocator
	if(virtual_address >= KERNEL_HEAP_START && virtual_address <= hard_limit){
		// P.A = V.A - Kernel_base
		uint32 pa = virtual_address - KERNEL_BASE;
		return pa;
	}else if(virtual_address >= (hard_limit + PAGE_SIZE) && virtual_address <= KERNEL_HEAP_MAX){

		// get the page table
		uint32* ptr_page_table = NULL;
		get_page_table(ptr_page_directory,virtual_address,&ptr_page_table);

		// if there is no page table for this virtual address.
		if(ptr_page_table[PTX(virtual_address)] <= 0){
			return 0;
		}

		uint32 pa = (ptr_page_table[PTX(virtual_address)] & 0xFFFFF000)+(virtual_address & 0x00000FFF);

		return pa;
	}

	return 0;
}

void kfreeall()
{
	panic("Not implemented!");

}

void kshrink(uint32 newSize)
{
	panic("Not implemented!");
}

void kexpand(uint32 newSize)
{
	panic("Not implemented!");
}




//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT'23.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc()
	// Write your code here, remove the panic and write your code
	return NULL;
	panic("krealloc() is not implemented yet...!!");
}
