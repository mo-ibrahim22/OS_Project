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
	 ptr_frame_info->va = va;
	 map_frame(ptr_page_directory ,ptr_frame_info ,va , PERM_WRITEABLE ); // wee need to ask DR about permission
	 /*cprintf("va after = %d\n",va);
	 cprintf("the frame after  va %d\n",ptr_frame_info->va);*/
	  va += PAGE_SIZE ;
	}
	return  0;
}

void dellocateAndUnMapFrames(uint32 StartOfDeallocation , uint32 sizeToDeallocate)
{
	uint32 va =StartOfDeallocation - PAGE_SIZE ;
	va = ROUNDUP(va, PAGE_SIZE) ;
	int cnt = (int)(ROUNDUP(sizeToDeallocate, PAGE_SIZE) / (uint32)PAGE_SIZE) ;  // 1
	for(int i=0 ;i<cnt ;i++)
	{
		uint32* ptr_page_table = NULL;
	   //get the page table.
	   get_page_table(ptr_page_directory, va, &ptr_page_table);
	   //uint32 target_page_address = ptr_page_table[PTX((uint32)virtual_address)];
	   // get the frame from page table.
	   struct FrameInfo * target_frame = get_frame_info(ptr_page_directory, va, &ptr_page_table);
	   target_frame->va = (uint32)0;
	   free_frame(target_frame);
	   //unmap frames
	   unmap_frame(ptr_page_directory ,va);
	   va -= PAGE_SIZE ;
	}

}


int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit)
{
	//cprintf("dstart = %d\n",daStart);

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


	uint32 size_to_increment = (uint32)increment ;
	//cprintf("increment after = %d\n" , size_to_increment);
	//cprintf("size to increment = %d\n",size_to_increment);
	size_to_increment = ROUNDUP(size_to_increment, PAGE_SIZE) ;
	//cprintf("size to increment after round up = %d\n",size_to_increment);
	uint32 new_brk = ROUNDUP(brk_pointer, PAGE_SIZE) +size_to_increment ;
	if(new_brk>hard_limit)
	{
		 panic("hard limit has been broken !!\n");
	}
	else
	{
		uint32 previous_brk_ptr = brk_pointer ;
		brk_pointer = new_brk ;
		allocateAndMapFrames(ROUNDUP(previous_brk_ptr, PAGE_SIZE) ,size_to_increment) ;
		return (void*) ROUNDUP(previous_brk_ptr, PAGE_SIZE) ;
	}
}
void* decrement_break_pointer(int increment)
{
	uint32 hint =(uint32)0 ;
	increment*=-1;
	uint32 size_to_decrement = increment;
	if((brk_pointer - size_to_decrement) %PAGE_SIZE==(uint32)0 && brk_pointer%PAGE_SIZE!=(uint32)0)
	{
		hint = PAGE_SIZE;
	}
	uint32 old_size_to_decrement = size_to_decrement;
	size_to_decrement = size_to_decrement -(size_to_decrement%PAGE_SIZE);    // 8
	uint32 new_brk = brk_pointer - size_to_decrement - (old_size_to_decrement%PAGE_SIZE) ;  //
	if(new_brk<dstart_of_kernal_heap)
	{
		panic("down of start of kernel heap !!\n");

	}
	else
	{
		uint32 previous_brk_ptr = brk_pointer ;
		brk_pointer =new_brk ;
		uint32 startAddressOfDeallocation = previous_brk_ptr ;
		dellocateAndUnMapFrames(startAddressOfDeallocation ,size_to_decrement+hint) ;
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
		 return decrement_break_pointer(increment)  ;
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

uint32 arr_va [NUM_OF_KHEAP_PAGES];
uint32 size_va [NUM_OF_KHEAP_PAGES];
void* kmalloc(unsigned int size)
{
     if(size<=DYN_ALLOC_MAX_BLOCK_SIZE)
     {
    	// cprintf("I'm here 1  and my  size = %d\n" ,size);
    	 void* pointer_ = alloc_block_FF(size);
    	 //cprintf("i'm finsihed with  va %d\n", (void *)pointer_);
    	 return pointer_;
     }
     else
     {
    	 int num_of_page_to_allocate = (int)(ROUNDUP(size, PAGE_SIZE) / (uint32)PAGE_SIZE) ;
    	 //cprintf("the size is %d\n",size);
    	 //cprintf("the num of pages %d\n",num_of_page_to_allocate);
    	 uint32 arr_of_page_adresses[num_of_page_to_allocate];
    	 //cprintf("hereeeeee kmalloc 2\n");
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
						/* else
						 {
							 uint32  *ptr_page_table =NULL ;
							 int result = get_page_table(ptr_page_directory , arr_of_page_adresses[i] , &ptr_page_table);
							 if(result ==TABLE_IN_MEMORY)
							 {
								 ptr_page_table[PTX(arr_of_page_adresses[i])]=ptr_page_table[PTX(arr_of_page_adresses[i])]&(~PERM_PRESENT);
							 }

						 }*/
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
    	 {
    		 for(int i=0 ;i<NUM_OF_KHEAP_PAGES ;i++)
    		 {
    			 if(arr_va[i]==(uint32)0)
    			 {
    				// cprintf("i'm in the target loop \n");
    				 arr_va[i]=arr_of_page_adresses[0] ;
    				 size_va[i]=(uint32)size;
    				 break;
    			 }
    		 }

    		// cprintf("i'm finsihed with  va %d\n",arr_of_page_adresses[0]);
    		 return (void*)arr_of_page_adresses[0]; /*---------><-------*/
    	 }
    	// cprintf("hereeeeee 9\n");
     }

	//TODO: [PROJECT'23.MS2 - #03] [1] KERNEL HEAP - kmalloc()
	//refer to the project presentation and documentation for details
	// use "isKHeapPlacementStrategyFIRSTFIT() ..." functions to check the current strategy

	//change this "return" according to your answer
	//kpanic_into_prompt("kmalloc() is not implemented yet...!!");
	return NULL;
}

void kfree(void* virtual_address)
{
	uint32 new_va =(uint32)virtual_address;
	new_va = ROUNDDOWN(new_va , PAGE_SIZE);
    //TODO: [PROJECT'23.MS2 - #04] [1] KERNEL HEAP - kfree()
    //refer to the project presentation and documentation for details
    // Write your code here, remove the panic and write your code
    // panic("kfree() is not implemented yet...!!");
    // block allocator -> KERNEL_HEAP_START, HARD_LIMIT
    // page allocator -> HARD_LIMIT + PAGE_SIZE, KERNEL_HEAP_MAX
    if((uint32)virtual_address  >= KERNEL_HEAP_START &&(uint32)virtual_address  < hard_limit)
    {
    	//cprintf("here 1\n");
    	//cprintf("current virtual address is %d\n" , (uint32)virtual_address);
        free_block(virtual_address);
    }
    else if((uint32)virtual_address >= (hard_limit + PAGE_SIZE) &&(uint32)virtual_address  <KERNEL_HEAP_MAX)
    {
    	//cprintf("here 2\n");
    	//cprintf("current virtual address is %d" , (uint32)virtual_address);

    	uint32 targrt_va ,target_size;
    	for(int i=0 ;i<NUM_OF_KHEAP_PAGES ;i++)
    	{
    		if(arr_va[i]==(uint32)new_va)
    		{
    			//cprintf("found in the arrray  with  the value = %d\n",arr_va[i]);
    			targrt_va =(uint32)new_va;
    			target_size =size_va[i];
    			size_va[i]=(uint32)0 ;
    			arr_va[i]=(uint32)0;
    			break ;
    		}
    	}

    	int cnt = (int)(ROUNDUP(target_size, PAGE_SIZE) / (uint32)PAGE_SIZE) ;
    	// unmap

    	for(int i = 0 ; i < cnt; i++){
    		 uint32* ptr_page_table = NULL;
			  // get the page table.
			   get_page_table(ptr_page_directory, targrt_va, &ptr_page_table);
			   //uint32 target_page_address = ptr_page_table[PTX((uint32)virtual_address)];
			  // get the frame from page table.
			  struct FrameInfo * target_frame = get_frame_info(ptr_page_directory, targrt_va, &ptr_page_table);
			  target_frame->va = (uint32)0;
			  free_frame(target_frame);
			  unmap_frame(ptr_page_directory,targrt_va);

    	   targrt_va += PAGE_SIZE;
    	 }
    	/*cprintf("here 2\n");
    	uint32 new_va =(uint32)virtual_address;
    	//new_va = ROUNDDOWN(new_va , PAGE_SIZE);

         uint32* ptr_page_table = NULL;
        // get the page table.
         get_page_table(ptr_page_directory, new_va, &ptr_page_table);
         //uint32 target_page_address = ptr_page_table[PTX((uint32)virtual_address)];
        // get the frame from page table.
        struct FrameInfo * target_frame = get_frame_info(ptr_page_directory, new_va, &ptr_page_table);
        uint32 pa = to_physical_address(target_frame);
        cprintf("NUmber of frames = %d\n" , PPN(pa));
    	//free_frame(target_frame);

        for(int i = 0 ; i < PPN(pa); i++){
        	unmap_frame(ptr_page_directory,new_va);
        	new_va += PAGE_SIZE;
        }
*/        // free this frame.
        //unmap_frame(ptr_page_directory ,new_va);
        // ptr_page_table[PTX((uint32)virtual_address)] = ptr_page_table[PTX((uint32)virtual_address)] &(~PERM_PRESENT) ;
        //free_frame(ptr_frame_info);

    }
    else
    {
    	//cprintf("here 3\n");
        panic("Address is invalid!!");
    }
}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT'23.MS2 - #05] [1] KERNEL HEAP - kheap_virtual_address()

		struct FrameInfo* target_frame = to_frame_info(physical_address);
		if(target_frame->va == 0){
			return 0;
		}
		return (target_frame->va& 0xFFFFF000) + (physical_address & 0x00000FFF);


}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #06] [1] KERNEL HEAP - kheap_physical_address()

	// get the page table
	uint32* ptr_page_table = NULL;
	get_page_table(ptr_page_directory,virtual_address,&ptr_page_table);

	uint32 pa = (ptr_page_table[PTX(virtual_address)] & 0xFFFFF000)+(virtual_address & 0x00000FFF);
	return pa;

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

int page_is_mapped(uint32 va ,uint32 * ptr_page_table)
{
	uint32 old_entry = ptr_page_table[PTX(va)] ;
	uint32 check_entry = ptr_page_table[PTX(va)] & (~PERM_PRESENT);
	if(old_entry == check_entry )
		return 0 ;
	return 1 ;
}
void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT'23.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc()
	if(virtual_address !=NULL && new_size == 0)
	{
		kfree(virtual_address);
		return NULL;
	}
	else if(virtual_address==NULL && new_size!=0)
	{
		return kmalloc(new_size);
	}
	else if (virtual_address == NULL && new_size==0)
	{
		return NULL;
	}

	// The virtual address in Block Range
	if((uint32)virtual_address  >= KERNEL_HEAP_START && (uint32)virtual_address  < hard_limit)
	{
		if(new_size<=DYN_ALLOC_MAX_BLOCK_SIZE)
		{
			// new size in Block range
			void* pointer_ = realloc_block_FF(virtual_address , new_size);
			return pointer_;
		}
		else
		{
			// new size in Page Range
			kfree(virtual_address);
			return kmalloc(new_size);
		}
	}
	else
	{
		// in Page Range
		// Get the current size
		uint32 new_va =(uint32)virtual_address;
		new_va = ROUNDDOWN(new_va , PAGE_SIZE);
		uint32 current_size ;
		int index = -1;
		for(int i=0 ;i<NUM_OF_KHEAP_PAGES ;i++)
		{
			if(arr_va[i]==(uint32)new_va)
			{
				index = i;
				current_size =size_va[i];
				break ;
			}
		}
		//
		if(index == -1)
			return NULL;
		//

		if(current_size == new_size)
		{
			// the current size = the new size
			return virtual_address;
		}
		// if new size < current size
		else if(new_size < current_size)
		{
			// new size in Block range
			if(new_size<=DYN_ALLOC_MAX_BLOCK_SIZE)
			{
				kfree(virtual_address);
				return kmalloc(new_size);
			}
			// still in Page Range
			else
			{
				new_size = ROUNDUP(new_size , PAGE_SIZE);
				uint32 size_to_deallocate = current_size - new_size ;
				dellocateAndUnMapFrames(new_va + current_size , size_to_deallocate);
				size_va[index] = new_size;
				return virtual_address;
			}
		}
		else
		{
			// new size > current size

			new_size = ROUNDUP(new_size , PAGE_SIZE);
			uint32 size_to_allocate = new_size - current_size ;

			int cnt =0 ;
			int num_of_pages_to_allocate = (int)(ROUNDUP(size_to_allocate, PAGE_SIZE) / (uint32)PAGE_SIZE) ;
			// calculate num of free pages after me
			uint32 start_address_to_check = new_va + current_size ;
			for(int i=0 ;i<num_of_pages_to_allocate;i++)
			{
				uint32  *ptr_page_table =NULL ;
				int result = get_page_table(ptr_page_directory , start_address_to_check , &ptr_page_table);
				if(result==TABLE_IN_MEMORY)
				{
					if(page_is_mapped(start_address_to_check,ptr_page_table)==0)
						cnt++;
					else
						break;
				}
				start_address_to_check+=PAGE_SIZE;
			}
			// size free after me fits me
			if(cnt==num_of_pages_to_allocate)
			{
				allocateAndMapFrames(new_va+ current_size ,size_to_allocate);
				size_va[index]=new_size;
				return virtual_address;
			}
			// size free after me doesn't fit me
			else
			{
				arr_va[index]=0;
				size_va[index]=0;
				kfree(virtual_address);
				return kmalloc(new_size);

			}

		}
	}
	return NULL;
}

