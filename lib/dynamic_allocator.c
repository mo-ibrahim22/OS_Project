/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//=====================================================
// 1) GET BLOCK SIZE (including size of its meta data):
//=====================================================
uint32 get_block_size(void* va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1) ;
	return curBlkMetaData->size ;
}

//===========================
// 2) GET BLOCK STATUS:
//===========================
int8 is_free_block(void* va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1) ;
	return curBlkMetaData->is_free ;
}

//===========================================
// 3) ALLOCATE BLOCK BASED ON GIVEN STRATEGY:
//===========================================
void *alloc_block(uint32 size, int ALLOC_STRATEGY)
{
	void *va = NULL;
	switch (ALLOC_STRATEGY)
	{
	case DA_FF:
		va = alloc_block_FF(size);
		break;
	case DA_NF:
		va = alloc_block_NF(size);
		break;
	case DA_BF:
		va = alloc_block_BF(size);
		break;
	case DA_WF:
		va = alloc_block_WF(size);
		break;
	default:
		cprintf("Invalid allocation strategy\n");
		break;
	}
	return va;
}

//===========================
// 4) PRINT BLOCKS LIST:
//===========================

void print_blocks_list(struct MemBlock_LIST list)
{
	cprintf("=========================================\n");
	struct BlockMetaData* blk ;
	cprintf("\nDynAlloc Blocks List:\n");
	LIST_FOREACH(blk, &list)
	{
		cprintf("(size: %d, isFree: %d)\n", blk->size, blk->is_free) ;
	}
	cprintf("=========================================\n");

}
//
////********************************************************************************//
////********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================
void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{
	//=========================================
	//DON'T CHANGE THESE LINES=================
	if (initSizeOfAllocatedSpace == 0)
		        return ;
	//=========================================
	 struct BlockMetaData * memblock= (struct BlockMetaData*)daStart;
	 memblock->is_free=1;
	 memblock->size=initSizeOfAllocatedSpace;
	 LIST_INIT(&blockList);
	 LIST_INSERT_TAIL(&blockList,memblock);    // may be removed if needed
	 //panic("initialize_dynamic_allocator is not implemented yet");
}

//=========================================
// [4] ALLOCATE BLOCK BY FIRST FIT:
//=========================================

void *alloc_block_FF(uint32 size)
{

	if(size==0)
		return NULL;

    uint8 is_last_block_free=0;
    uint32 size_of_last_block;
    struct BlockMetaData * last_Block;

	struct BlockMetaData * cr_Block = LIST_FIRST(&blockList);
	uint32 meta_data_size = sizeOfMetaData();
	int is_block_allocated=0;
	uint32 total_size_to_be_allocated;
	while(cr_Block)
	{

		is_last_block_free=cr_Block->is_free;
		size_of_last_block=cr_Block->size;
	    last_Block=cr_Block;

		uint32 current_block_size = cr_Block->size; // block to allocate in
		total_size_to_be_allocated= size+meta_data_size;
		uint8 is_current_block_free= cr_Block->is_free;

		/*current_block_size=9;
		total_size_to_be_allocated=5;*/

		if(is_current_block_free==1)
		{
			if(total_size_to_be_allocated==current_block_size)
			{
				cprintf("%s\n","sssssssss1");
				cr_Block->is_free=0;
				is_block_allocated=1;
				return (void*) (uint32)cr_Block+meta_data_size;
			}
			else if(total_size_to_be_allocated<current_block_size)
			{
				cprintf("%s\n","sssssssss2");
				//--> allocate space
				//--> modify meta data
				//create meta data for free space
				 uint32 remaing_space_size = (uint32)((unsigned int)current_block_size-(unsigned int)total_size_to_be_allocated);
				 struct BlockMetaData * remaining_space =(struct BlockMetaData*)((uint32)cr_Block+total_size_to_be_allocated);
				 remaining_space->is_free=1;
				 remaining_space->size=remaing_space_size;


				 cr_Block->size=total_size_to_be_allocated;
				 cr_Block->is_free=0;
				 is_block_allocated=1;
				 return   (void*)(uint32)(cr_Block+meta_data_size);
			}
		}
		cr_Block = LIST_NEXT(cr_Block);
	}

    if(is_block_allocated==0)
	{
		cprintf("%s\n","sssssssss3");
		if(is_last_block_free==1)
		{
			cprintf("%s\n","sssssssss444");
			uint32 space_to_sbrk = total_size_to_be_allocated-size_of_last_block;
			void* adrs =  sbrk(space_to_sbrk);
			if(adrs!=(void*)-1)
			{
				cprintf("%s\n","sssssssss444llllllllllllllllllllllllllllfdfdsfasf");
				last_Block->is_free=0;
				last_Block->size =total_size_to_be_allocated;
				return last_Block+meta_data_size;
			}
			return NULL;
		}

		else if(is_last_block_free==0)
		{
			void* adrs = sbrk(total_size_to_be_allocated);
			if(adrs!=(void*)-1)
			{
				cprintf("%s\n","sssssssss5555");
				cprintf("%s\n","sssssssss89789798");
				struct BlockMetaData * block_in_extended_area =(struct BlockMetaData*)adrs;
				block_in_extended_area->is_free=0;
				block_in_extended_area->size=total_size_to_be_allocated;
				LIST_INSERT_TAIL(&blockList,block_in_extended_area);
				return (void*)((uint32)adrs+meta_data_size);
			}
			return NULL;
			/*struct BlockMetaData * First_Block = LIST_FIRST(&blockList);
			  First_Block->size += total_size_to_be_allocated;*/
		}
	}
	//TODO: [PROJECT'23.MS1 - #6] [3] DYNAMIC ALLOCATOR - alloc_block_FF()
	//panic("alloc_block_FF is not implemented yet");
    return NULL ;
}
//=========================================
// [5] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void *alloc_block_BF(uint32 size)
{
	//TODO: [PROJECT'23.MS1 - BONUS] [3] DYNAMIC ALLOCATOR - alloc_block_BF()
	panic("alloc_block_BF is not implemented yet");
	return NULL;
}

//=========================================
// [6] ALLOCATE BLOCK BY WORST FIT:
//=========================================
void *alloc_block_WF(uint32 size)
{
	panic("alloc_block_WF is not implemented yet");
	return NULL;
}

//=========================================
// [7] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
void *alloc_block_NF(uint32 size)
{
	panic("alloc_block_NF is not implemented yet");
	return NULL;
}

//===================================================
// [8] FREE BLOCK WITH COALESCING:
//===================================================
void free_block(void *va)
{
	//TODO: [PROJECT'23.MS1 - #7] [3] DYNAMIC ALLOCATOR - free_block()
	panic("free_block is not implemented yet");
}

//=========================================
// [4] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void* va, uint32 new_size)
{
	//TODO: [PROJECT'23.MS1 - #8] [3] DYNAMIC ALLOCATOR - realloc_block_FF()
	panic("realloc_block_FF is not implemented yet");
	return NULL;
}
