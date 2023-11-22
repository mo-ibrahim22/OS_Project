#include <inc/lib.h>
//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

int FirstTimeFlag = 1;
void InitializeUHeap()
{
	if(FirstTimeFlag)
	{
#if UHP_USE_BUDDY
		initialize_buddy();
		cprintf("BUDDY SYSTEM IS INITIALIZED\n");
#endif
		FirstTimeFlag = 0;
	}
}

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//=============================================
// [1] CHANGE THE BREAK LIMIT OF THE USER HEAP:
//=============================================
/*2023*/
void* sbrk(int increment)
{
	return (void*) sys_sbrk(increment);
}

//=================================
// [2] ALLOCATE SPACE IN USER HEAP:
//=================================
uint32 arr_user_va [NUM_OF_UHEAP_PAGES];
uint32 size_user_va [NUM_OF_UHEAP_PAGES];
uint32 user_page_status[NUM_OF_UHEAP_PAGES];
struct  Env currenv ;
void* malloc(uint32 size)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	if (size == 0) return NULL ;
	//==============================================================
	//TODO: [PROJECT'23.MS2 - #09] [2] USER HEAP - malloc() [User Side]
	// Write your code here, remove the panic and write your code
	//panic("malloc() is not implemented yet...!!");

	 if(size<=DYN_ALLOC_MAX_BLOCK_SIZE)
	 {
		 //cprintf(" the size========= %d\n", size);
		// cprintf("I'm here 1  and my  size = %d\n" ,size);
		void* pointer_ = alloc_block_FF(size);

		return pointer_ ;
	 }

	 else
	 {
		// cprintf("I'm in the else!!!!!!!!!!!!!!!!!!!!!\n");
		 int num_of_page_to_mark = (int)(ROUNDUP(size, PAGE_SIZE) / (uint32)PAGE_SIZE) ;
		 uint32  hardLimt = (uint32) sys_u_hard_limit();

		 uint32 va_to_check = hardLimt + PAGE_SIZE ;
		// cprintf( "the start address of allocation = %d\n" ,  hardLimt + PAGE_SIZE );
		 int cnt = 0 ;

		 int is_found =0 ;
		 uint32 base_address;
		 while(va_to_check<USER_HEAP_MAX)
		 {

			 if(is_found==1)
			     break;
			     uint32 index = (va_to_check / PAGE_SIZE) - ((hardLimt + PAGE_SIZE) / PAGE_SIZE );
			     //cprintf(" the index now = %d\n",index);
				 uint32 target_value =  user_page_status[index] ;  // target value = 0 then page is free   else page not free


				 if(target_value==(uint32)0)
				 {
					 if(cnt == 0){
						 base_address = va_to_check;
					 }
					cnt++;
				 }
				 if(cnt==num_of_page_to_mark)
				 {
					 is_found=1;
				 }
				 if(target_value!=(uint32)0)
				     cnt=0;

			 va_to_check+=PAGE_SIZE ;
		 }

		 if(is_found==0)
		     return NULL ;
		 else
		 {
			 uint32 index_of_base_address = (base_address / PAGE_SIZE) - ((hardLimt + PAGE_SIZE) / PAGE_SIZE );
			 for(int i=index_of_base_address ;i < index_of_base_address+cnt ;i++)
			 {
				 user_page_status[i]=(uint32)1;
			 }
			 for(int i=0 ;i< NUM_OF_UHEAP_PAGES;i++)
			 {
				 if(arr_user_va[i]==(uint32)0)
				 {
					 arr_user_va[i]=base_address;
					 size_user_va[i]=ROUNDUP(size, PAGE_SIZE);
					 break;
				 }
			 }
			// cprintf( "the actual address of allocation = %d\n" ,base_address );
			 sys_allocate_user_mem(base_address, ROUNDUP(size, PAGE_SIZE));    // don't forget to round in sys_allcate
			 int cnt1 =0 ;

			 for(int i=0 ;i<NUM_OF_UHEAP_PAGES ;i++)
			 {
				 if(user_page_status[i]==1)
					 cnt1++;
			 }
			 //cprintf("cnt1 = %d \n\n" ,cnt1);
			 return (void*)base_address;
		 }
	 }

	return NULL;
	//Use sys_isUHeapPlacementStrategyFIRSTFIT() and	sys_isUHeapPlacementStrategyBESTFIT()
	//to check the current strategy
}

//=================================
// [3] FREE SPACE FROM USER HEAP:
//=================================
void free(void* virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #11] [2] USER HEAP - free() [User Side]
	// Write your code here, remove the panic and write your code
	panic("free() is not implemented yet...!!");
}


//=================================
// [4] ALLOCATE SHARED VARIABLE:
//=================================
void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	if (size == 0) return NULL ;
	//==============================================================
	panic("smalloc() is not implemented yet...!!");
	return NULL;
}

//========================================
// [5] SHARE ON ALLOCATED SHARED VARIABLE:
//========================================
void* sget(int32 ownerEnvID, char *sharedVarName)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	//==============================================================
	// Write your code here, remove the panic and write your code
	panic("sget() is not implemented yet...!!");
	return NULL;
}


//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//=================================
// REALLOC USER SPACE:
//=================================
//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_move_user_mem(...)
//		which switches to the kernel mode, calls move_user_mem(...)
//		in "kern/mem/chunk_operations.c", then switch back to the user mode here
//	the move_user_mem() function is empty, make sure to implement it.
void *realloc(void *virtual_address, uint32 new_size)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	//==============================================================

	// Write your code here, remove the panic and write your code
	panic("realloc() is not implemented yet...!!");
	return NULL;

}


//=================================
// FREE SHARED VARIABLE:
//=================================
//	This function frees the shared variable at the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from main memory then switch back to the user again.
//
//	use sys_freeSharedObject(...); which switches to the kernel mode,
//	calls freeSharedObject(...) in "shared_memory_manager.c", then switch back to the user mode here
//	the freeSharedObject() function is empty, make sure to implement it.

void sfree(void* virtual_address)
{
	// Write your code here, remove the panic and write your code
	panic("sfree() is not implemented yet...!!");
}


//==================================================================================//
//========================== MODIFICATION FUNCTIONS ================================//
//==================================================================================//

void expand(uint32 newSize)
{
	panic("Not Implemented");

}
void shrink(uint32 newSize)
{
	panic("Not Implemented");

}
void freeHeap(void* virtual_address)
{
	panic("Not Implemented");

}
