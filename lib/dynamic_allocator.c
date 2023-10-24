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
		total_size_to_be_allocated= (uint32)size+(uint32)meta_data_size;
		uint8 is_current_block_free= cr_Block->is_free;

		/*current_block_size=9;
		total_size_to_be_allocated=5;*/

		if(is_current_block_free==1)
		{
			if(total_size_to_be_allocated==current_block_size)
			{
				cr_Block->is_free=0;
				is_block_allocated=1;
				return (void*) ((uint32)cr_Block+meta_data_size);
			}
			else if(total_size_to_be_allocated<current_block_size)
			{
				//--> allocate space
				//--> modify meta data
				//create meta data for free space
				 uint32 remaing_space_size = (uint32)((uint32)current_block_size-(uint32)total_size_to_be_allocated);
				 struct BlockMetaData * remaining_space =(struct BlockMetaData*)((uint32)cr_Block+(uint32)total_size_to_be_allocated);
				 remaining_space->is_free=1;
				 remaining_space->size=remaing_space_size;
				 LIST_INSERT_AFTER(&blockList,cr_Block,remaining_space);
				 cr_Block->size=total_size_to_be_allocated;
				 cr_Block->is_free=0;
				 is_block_allocated=1;
				 return   (void*)((uint32)cr_Block+meta_data_size);
			}
		}
		cr_Block = LIST_NEXT(cr_Block);
	}

    if(is_block_allocated==0)
	{
		if(is_last_block_free==1)
		{
			uint32 space_to_sbrk = (uint32)total_size_to_be_allocated-(uint32)size_of_last_block;
			void* adrs =  sbrk(space_to_sbrk);
			if(adrs!=(void*)-1)
			{
				last_Block->is_free=0;
				last_Block->size =total_size_to_be_allocated;
				return (void*)((uint32)last_Block+meta_data_size);
			}
			return NULL;
		}

		else if(is_last_block_free==0)
		{
			void* adrs = sbrk(total_size_to_be_allocated);
			if(adrs!=(void*)-1)
			{
				struct BlockMetaData * block_in_extended_area =(struct BlockMetaData*)adrs;
				block_in_extended_area->is_free=0;
				block_in_extended_area->size=total_size_to_be_allocated;
				LIST_INSERT_TAIL(&blockList,block_in_extended_area);
				return (void*)((uint32)adrs+meta_data_size);
			}
			return NULL;
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
    if (size == 0)
        return NULL;

    uint8 is_last_block_free = 0;
    uint32 size_of_last_block;
    struct BlockMetaData *last_Block = NULL;
    struct BlockMetaData *cr_Block = LIST_FIRST(&blockList);
    uint32 meta_data_size = sizeOfMetaData();
    int is_block_allocated = 0;
    uint32 total_size_to_be_allocated;
    struct BlockMetaData *best_fit_block = NULL;
    uint32 best_fit_block_size = (uint32)DYN_ALLOC_MAX_SIZE;

    while (cr_Block)
    {
        is_last_block_free = cr_Block->is_free;
        size_of_last_block = cr_Block->size;
        last_Block = cr_Block;

        uint32 current_block_size = cr_Block->size;
        total_size_to_be_allocated = (uint32)size + (uint32)meta_data_size;
        uint8 is_current_block_free = cr_Block->is_free;

        if (is_current_block_free == 1)
        {
            if (total_size_to_be_allocated == current_block_size)
            {
                cr_Block->is_free = 0;
                is_block_allocated = 1;
                return (void *)((uint32)cr_Block + meta_data_size);
            }
            else if (total_size_to_be_allocated < current_block_size)
            {
                if (current_block_size < best_fit_block_size)
                {
                    best_fit_block = cr_Block;
                    best_fit_block_size = current_block_size;
                }
            }
        }
        cr_Block = LIST_NEXT(cr_Block);
    }

    if (is_block_allocated == 0)
    {
        if (best_fit_block != NULL)
        {
            if (best_fit_block_size > total_size_to_be_allocated + meta_data_size)
            {
                struct BlockMetaData *remaining_space = (struct BlockMetaData *)((uint32)best_fit_block + total_size_to_be_allocated);
                remaining_space->is_free = 1;
                remaining_space->size = best_fit_block_size - total_size_to_be_allocated;
                LIST_INSERT_AFTER(&blockList, best_fit_block, remaining_space);
                best_fit_block->size = total_size_to_be_allocated;
            }
            best_fit_block->is_free = 0;
            return (void *)((uint32)best_fit_block + meta_data_size);
        }
        else if (is_last_block_free == 1)
        {
            uint32 space_to_sbrk = total_size_to_be_allocated - size_of_last_block;
            void *adrs = sbrk(space_to_sbrk);
            if (adrs != (void *)-1)
            {
                last_Block->is_free = 0;
                last_Block->size = total_size_to_be_allocated;
                return (void *)((uint32)last_Block + meta_data_size);
            }
        }
        else
        {
            void *adrs = sbrk(total_size_to_be_allocated);
            if (adrs != (void *)-1)
            {
                struct BlockMetaData *block_in_extended_area = (struct BlockMetaData *)adrs;
                block_in_extended_area->is_free = 0;
                block_in_extended_area->size = total_size_to_be_allocated;
                LIST_INSERT_TAIL(&blockList, block_in_extended_area);
                return (void *)((uint32)adrs + meta_data_size);
            }
        }
    }

    //TODO: [PROJECT'23.MS1 - BONUS] [3] DYNAMIC ALLOCATOR - alloc_block_BF()
    //panic("alloc_block_BF is not implemented yet");

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


void Handle_Case_If_The_Block_Is_First_Block(struct BlockMetaData * selected_block ,struct BlockMetaData * next_block )
{
	if(next_block->is_free==0)
	{
		selected_block->is_free=1;
	}
	else
	{
		selected_block->is_free=1;
		selected_block->size+= next_block->size;
		struct BlockMetaData * next_of_next_block = LIST_NEXT(next_block);
		selected_block->prev_next_info.le_next=next_of_next_block;
		next_block->is_free=0;
		next_block->size=0;
		next_block=NULL;
	}
}
void Handle_Case_If_The_Block_Is_Last_Block(struct BlockMetaData * selected_block , struct BlockMetaData * prev_block)
{
	if(prev_block->is_free==0)
	{
		selected_block->is_free=1;
	}
	else
	{
		prev_block->size+= selected_block->size;
		prev_block->prev_next_info.le_next=NULL;
		selected_block->is_free=0;
		selected_block->size=0;
		selected_block=NULL;
	}
}
void Handle_Case_If_Previous_And_Next_are_Full(struct BlockMetaData * selected_block)
{
	selected_block->is_free=1;
}
void Handle_Case_If_Previous_And_Next_are_Free(struct BlockMetaData * selected_block ,struct BlockMetaData * prev_block ,struct BlockMetaData * next_block)
{
	struct BlockMetaData * next_of_next_block = LIST_NEXT(next_block);
	prev_block->size+=(selected_block->size+next_block->size);
	prev_block->prev_next_info.le_next=next_of_next_block;
	selected_block->is_free=0;
	selected_block->size=0;
	next_block->is_free=0;
	next_block->size=0;
	selected_block=NULL;
	next_block=NULL;
}
void Handle_Case_If_Only_Next_Is_Free(struct BlockMetaData * selected_block , struct BlockMetaData * next_block)
{
	uint32 size_of_next_block = next_block->size;
	struct BlockMetaData * block = LIST_NEXT(next_block);
	selected_block->is_free=1;
	selected_block->size += size_of_next_block;
	selected_block->prev_next_info.le_next = block;
	next_block->is_free=0;
	next_block->size=0;
	next_block=NULL;
}
void Handle_Case_If_Only_Previous_Is_Free(struct BlockMetaData * selected_block,struct BlockMetaData * prev_block ,struct BlockMetaData * next_block)
{
	prev_block->size += selected_block->size;
	prev_block->prev_next_info.le_next=next_block;
	selected_block->is_free=0;
	selected_block->size=0;
	selected_block=NULL;
}

//===================================================
// [8] FREE BLOCK WITH COALESCING:
//===================================================
void free_block(void *va)
{
	//TODO: [PROJECT'23.MS1 - #7] [3] DYNAMIC ALLOCATOR - free_block()
	//panic("free_block is not implemented yet");
	uint32 meta_data_size = sizeOfMetaData();
	if(!va) // if the address is NUll then return
	{
		return ;
	}
    // predefined data to use
	uint32 address_of_block = (uint32)va-(uint32)meta_data_size;
	struct BlockMetaData * selected_block =(struct BlockMetaData*)address_of_block;
	struct BlockMetaData * prev_block = LIST_PREV(selected_block);
	struct BlockMetaData * next_block = LIST_NEXT(selected_block);
	struct BlockMetaData * first_block = LIST_FIRST(&blockList);
	struct BlockMetaData * last_block = LIST_LAST(&blockList);
	// handling of corner case :  if the block already free then return
    if(selected_block->is_free==1)
    	return;
    // handling of all cases
	if(selected_block==first_block)
	{
		Handle_Case_If_The_Block_Is_First_Block(selected_block,next_block);
		return;
	}
	else if(selected_block==last_block)
	{
		Handle_Case_If_The_Block_Is_Last_Block(selected_block,prev_block);
		return;
	}
	else if(prev_block->is_free==0 && next_block->is_free==0)
	{
		Handle_Case_If_Previous_And_Next_are_Full(selected_block);
		return ;
	}
	else if(prev_block->is_free==0 && next_block->is_free==1)
	{
		Handle_Case_If_Only_Next_Is_Free(selected_block ,next_block);
		return;
	}
	else if(prev_block->is_free==1 && next_block->is_free==0)
	{
		Handle_Case_If_Only_Previous_Is_Free(selected_block,prev_block,next_block);
		return;
	}
	else if (prev_block->is_free==1 && next_block->is_free==1 )
	{
		Handle_Case_If_Previous_And_Next_are_Free(selected_block,prev_block,next_block);
		return;
	}
}

//=========================================
// [4] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void* va, uint32 new_size) // not completed (if small size and next is free)
{

	// corner cases
	if(va!=NULL &&new_size==0)
	{
		cprintf("now here  1\n");
		free_block(va);
		return NULL;
	}
	else if(va==NULL &&new_size!=0)
	{
		cprintf("now here  2\n");
		return alloc_block_FF(new_size);
	}
	else if(va==NULL && new_size==0)
	{
		cprintf("now here  3\n");
		return alloc_block_FF(0);
	}

	uint32 meta_data_size = sizeOfMetaData();
	uint32 address_of_block = (uint32)va-(uint32)meta_data_size;
	//TODO: [PROJECT'23.MS1 - #8] [3] DYNAMIC ALLOCATOR - realloc_block_FF()
	//panic("realloc_block_FF is not implemented yet");
	struct BlockMetaData * selected_block =(struct BlockMetaData*)address_of_block;
	struct BlockMetaData * next_block = LIST_NEXT(selected_block);
	uint32 old_size = selected_block->size;
	uint32 next_block_size = next_block->size;
	uint8 is_next_free = next_block->is_free;
	uint32 actual_old_size = old_size-meta_data_size;

	if(new_size==actual_old_size)
	{
		cprintf("now here  4\n");
		return va;
	}

	else if(new_size>actual_old_size) // new size bigger than me
	{
		cprintf("now here  5\n");
		uint32 taken_size = new_size - actual_old_size;
		if(is_next_free == 1)   // next is free
		{
			cprintf("now here  6\n");
			if(next_block_size==taken_size) // fit and taken size equal next block size
			{
				cprintf("now here  7\n");
				struct BlockMetaData * next_of_next = LIST_NEXT(next_block);
				selected_block->size = new_size+meta_data_size;
				next_block->size=0;
				next_block->is_free=0;
				next_block=NULL;
				selected_block->prev_next_info.le_next=next_of_next;
				return (void*)((uint32)selected_block + meta_data_size);
			}
			else if(next_block_size>taken_size) // fit and taken size is smaller than next_block size
			{
				cprintf("now here  8\n");
				selected_block->size = new_size+meta_data_size;
				next_block->size = 0;
				next_block->is_free=0;
				uint32  free_block_address = (uint32)((uint32)next_block+(uint32)taken_size);
				struct BlockMetaData * free_block =(struct BlockMetaData*)free_block_address;
				free_block->is_free=1;
				free_block->size=next_block_size-taken_size;
				LIST_INSERT_AFTER(&blockList,selected_block,free_block);
				next_block=NULL;
				return (void*)((uint32)selected_block + meta_data_size);
			}
			else // next not fit me
			{
				cprintf("now here  9\n");
				free_block(va);
				return alloc_block_FF(new_size);
			}
		}

		else // next block is not free
		{
			cprintf("now here  10\n");
			free_block(va);
			return alloc_block_FF(new_size);
		}
	}
	else  // new size smaller than old size
	{
		cprintf("now here  11\n");
		if(is_next_free==0) // next is not free
		{
			cprintf("now here  12\n");
			selected_block->size = new_size+meta_data_size;
			uint32 selected_block_new_size = new_size+meta_data_size;
			uint32  free_block_address = (uint32) selected_block + selected_block_new_size;
			struct BlockMetaData * free_block =(struct BlockMetaData*)free_block_address;
			free_block->is_free=1;
			free_block->size = old_size-selected_block_new_size;
			LIST_INSERT_AFTER(&blockList,selected_block,free_block);
			return (void*)((uint32)selected_block + meta_data_size);
		}

		else // next is free
		{
			cprintf("now here  13\n");
			selected_block->size = new_size+meta_data_size;
			uint32 free_size = actual_old_size-new_size;
			uint32 new_address = (uint32)next_block -(uint32)free_size;
			next_block->is_free=0;
			next_block->size=0;
			LIST_REMOVE(&blockList, next_block);
			struct BlockMetaData * new_block =(struct BlockMetaData*)new_address;
			new_block->is_free=1;
			new_block->size=next_block_size+free_size;
			LIST_INSERT_AFTER(&blockList,selected_block,new_block);
			return (void*)((uint32)selected_block + meta_data_size);
		}
	}
}
