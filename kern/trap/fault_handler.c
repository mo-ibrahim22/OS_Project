/*
 * fault_handler.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include "trap.h"
#include <kern/proc/user_environment.h>
#include "../cpu/sched.h"
#include "../disk/pagefile_manager.h"
#include "../mem/memory_manager.h"


//2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
// 0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;

//===============================
// REPLACEMENT STRATEGIES
//===============================
//2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE)
{
	assert(LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE ;
}
void setPageReplacmentAlgorithmCLOCK(){_PageRepAlgoType = PG_REP_CLOCK;}
void setPageReplacmentAlgorithmFIFO(){_PageRepAlgoType = PG_REP_FIFO;}
void setPageReplacmentAlgorithmModifiedCLOCK(){_PageRepAlgoType = PG_REP_MODIFIEDCLOCK;}
/*2018*/ void setPageReplacmentAlgorithmDynamicLocal(){_PageRepAlgoType = PG_REP_DYNAMIC_LOCAL;}
/*2021*/ void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps){_PageRepAlgoType = PG_REP_NchanceCLOCK;  page_WS_max_sweeps = PageWSMaxSweeps;}

//2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE){return _PageRepAlgoType == LRU_TYPE ? 1 : 0;}
uint32 isPageReplacmentAlgorithmCLOCK(){if(_PageRepAlgoType == PG_REP_CLOCK) return 1; return 0;}
uint32 isPageReplacmentAlgorithmFIFO(){if(_PageRepAlgoType == PG_REP_FIFO) return 1; return 0;}
uint32 isPageReplacmentAlgorithmModifiedCLOCK(){if(_PageRepAlgoType == PG_REP_MODIFIEDCLOCK) return 1; return 0;}
/*2018*/ uint32 isPageReplacmentAlgorithmDynamicLocal(){if(_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL) return 1; return 0;}
/*2021*/ uint32 isPageReplacmentAlgorithmNchanceCLOCK(){if(_PageRepAlgoType == PG_REP_NchanceCLOCK) return 1; return 0;}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt){_EnableModifiedBuffer = enableIt;}
uint8 isModifiedBufferEnabled(){  return _EnableModifiedBuffer ; }

void enableBuffering(uint32 enableIt){_EnableBuffering = enableIt;}
uint8 isBufferingEnabled(){  return _EnableBuffering ; }

void setModifiedBufferLength(uint32 length) { _ModifiedBufferLength = length;}
uint32 getModifiedBufferLength() { return _ModifiedBufferLength;}

//===============================
// Added By Farouk
//===============================
void add_WS_element_FIFO_placement(struct WorkingSetElement* new_workingset_element, uint32 wsSize)
{
	if(curenv->page_last_WS_element)
	{
		if(curenv->page_last_WS_element == LIST_FIRST(&curenv->page_WS_list))
		{
			curenv->page_last_WS_element = NULL;
		}
		else
		{
			struct WorkingSetElement *current_First, *current_Last, *page_last_WS_element_previous;
			current_First = LIST_FIRST(&curenv->page_WS_list);
			current_Last = LIST_LAST(&curenv->page_WS_list);
			page_last_WS_element_previous = curenv->page_last_WS_element->prev_next_info.le_prev;


			// Logic
			current_First->prev_next_info.le_prev = current_Last;
			current_Last->prev_next_info.le_next = current_First;

			page_last_WS_element_previous->prev_next_info.le_next = NULL;
			curenv->page_last_WS_element->prev_next_info.le_prev = NULL;

			curenv->page_WS_list.lh_first = curenv->page_last_WS_element;
			curenv->page_WS_list.lh_last = page_last_WS_element_previous;

			curenv->page_last_WS_element = NULL;
		}
	}

	LIST_INSERT_TAIL(&curenv->page_WS_list,new_workingset_element);
	wsSize = LIST_SIZE(&(curenv->page_WS_list));
	if(wsSize == (curenv->page_WS_max_size)){
		curenv->page_last_WS_element = LIST_FIRST(&curenv->page_WS_list);
	}
}

void add_WS_element_LRU_placement(uint32 fault_va)
{
	struct FrameInfo * ptr_frame_info = NULL;
	allocate_frame(&ptr_frame_info);
	ptr_frame_info->va = fault_va;
	map_frame(curenv->env_page_directory ,ptr_frame_info ,fault_va , PERM_WRITEABLE|PERM_USER|PERM_PRESENT);

	int Page_file_val = pf_read_env_page(curenv,(void *)fault_va);
	if(Page_file_val == E_PAGE_NOT_EXIST_IN_PF)
	{
		if( (fault_va >= USTACKBOTTOM && fault_va <= USTACKTOP) ||(fault_va >= USER_HEAP_START && fault_va <= USER_HEAP_MAX) )
		{
			struct WorkingSetElement *new_workingset_element =env_page_ws_list_create_element(curenv,fault_va);
			LIST_INSERT_HEAD(&curenv->ActiveList, new_workingset_element);
		}
		else
		{
			sched_kill_env(curenv->env_id);
		}
	}
	else
	{
		struct WorkingSetElement *new_workingset_element =env_page_ws_list_create_element(curenv,fault_va);
		LIST_INSERT_HEAD(&curenv->ActiveList, new_workingset_element);
	}

}

void add_WS_element_LRU_RE_placement(struct WorkingSetElement* new_workingset_element, uint32 fault_va)
{
	struct FrameInfo * ptr_frame_info = NULL;
	allocate_frame(&ptr_frame_info);
	ptr_frame_info->va = fault_va;
	map_frame(curenv->env_page_directory ,ptr_frame_info ,fault_va , PERM_WRITEABLE|PERM_USER|PERM_PRESENT);

	int Page_file_val = pf_read_env_page(curenv,(void *)fault_va);
	if(Page_file_val == E_PAGE_NOT_EXIST_IN_PF)
	{
		if( (fault_va >= USTACKBOTTOM && fault_va <= USTACKTOP) ||(fault_va >= USER_HEAP_START && fault_va <= USER_HEAP_MAX) )
		{
			new_workingset_element->virtual_address = fault_va;
			new_workingset_element->empty = 1;
			new_workingset_element->time_stamp = 0;

			ptr_frame_info->element = new_workingset_element;
			LIST_INSERT_HEAD(&curenv->ActiveList, new_workingset_element);
		}
		else
		{
			sched_kill_env(curenv->env_id);
		}
	}
	else
	{
		new_workingset_element->virtual_address = fault_va;
		new_workingset_element->empty = 1;
		new_workingset_element->time_stamp = 0;

		ptr_frame_info->element = new_workingset_element;
		LIST_INSERT_HEAD(&curenv->ActiveList, new_workingset_element);
	}
}

//===============================
// FAULT HANDLERS
//===============================

//Handle the table fault
void table_fault_handler(struct Env * curenv, uint32 fault_va)
{
	//panic("table_fault_handler() is not implemented yet...!!");
	//Check if it's a stack page
	uint32* ptr_table;
#if USE_KHEAP
	{
		ptr_table = create_page_table(curenv->env_page_directory, (uint32)fault_va);
	}
#else
	{
		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
	}
#endif
}

//Handle the page fault

void page_fault_handler(struct Env * curenv, uint32 fault_va)
{

	//
	fault_va = ROUNDDOWN(fault_va, PAGE_SIZE);
	//

#if USE_KHEAP
		struct WorkingSetElement *victimWSElement = NULL;
		uint32 wsSize = LIST_SIZE(&(curenv->page_WS_list));
#else
		int iWS =curenv->page_last_WS_index;
		uint32 wsSize = env_page_ws_get_size(curenv);
#endif


	/*FIFO Re/placement*/
	if(isPageReplacmentAlgorithmFIFO())
	{
		/*Placement*/
		if(wsSize < (curenv->page_WS_max_size))
		{
			//TODO: [PROJECT'23.MS2 - #15] [3] PAGE FAULT HANDLER - Placement
			/*cprintf("\nPLACEMENT -> Working Set Size = %d\n", wsSize );
			env_page_ws_print(curenv);*/

			struct FrameInfo * ptr_frame_info = NULL;
			allocate_frame(&ptr_frame_info);
			ptr_frame_info->va = fault_va;
			map_frame(curenv->env_page_directory ,ptr_frame_info ,fault_va , PERM_WRITEABLE|PERM_USER|PERM_PRESENT);

			int Page_file_val = pf_read_env_page(curenv,(void *)fault_va);
			if(Page_file_val == E_PAGE_NOT_EXIST_IN_PF)
			{
				if( (fault_va >= USTACKBOTTOM && fault_va <= USTACKTOP) ||(fault_va >= USER_HEAP_START && fault_va <= USER_HEAP_MAX) )
				{
					/*if(fault_va >= USTACKBOTTOM && fault_va <= USTACKTOP )
				{
					cprintf("\nHandle Fault_va in stack\n");
				}

				if(fault_va >= USER_HEAP_START && fault_va <= USER_HEAP_MAX)
				{
					cprintf("\nHandle Fault_va in heap\n");
				}*/

					struct WorkingSetElement *new_workingset_element =env_page_ws_list_create_element(curenv,fault_va);
					add_WS_element_FIFO_placement(new_workingset_element, wsSize);
				}
				else
				{
					/*cprintf("Kill the environment Fault Handler\n");*/
					sched_kill_env(curenv->env_id);
				}
			}
			else
			{
				/*cprintf("============== Handle Fault_va in Page File ======================\n");*/
				struct WorkingSetElement *new_workingset_element =env_page_ws_list_create_element(curenv,fault_va);
				add_WS_element_FIFO_placement(new_workingset_element, wsSize);
			}
			//env_page_ws_print(curenv);
		}
		/*Replacement*/
		else
		{
			//TODO: [PROJECT'23.MS3 - #1] [1] PAGE FAULT HANDLER - FIFO Replacement

			/*Deal With Victim*/
			// get the FIFO Pointer
			struct WorkingSetElement *FIFO_Pointer = curenv->page_last_WS_element;

			// get page permissions
			uint32 permissions = pt_get_page_permissions( curenv->env_page_directory, FIFO_Pointer->virtual_address);

			// If Modified -> Then update it
			if((permissions & PERM_MODIFIED) == PERM_MODIFIED){
				// get the page table.
				uint32* ptr_page_table = NULL;
				get_page_table(curenv->env_page_directory, FIFO_Pointer->virtual_address, &ptr_page_table);

				// Get Pointer To Frame Info
				struct FrameInfo *ptr_frame_info = get_frame_info(curenv->env_page_directory, FIFO_Pointer->virtual_address,&ptr_page_table);

				// Update the page in page file
				int ret = pf_update_env_page(curenv, FIFO_Pointer->virtual_address, ptr_frame_info);
			}

			unmap_frame(curenv->env_page_directory, FIFO_Pointer->virtual_address);

			/*We will use The Current Working set element rather than create another one*/
			struct FrameInfo * ptr_frame_info;
			allocate_frame(&ptr_frame_info);
			ptr_frame_info->va = fault_va;
			map_frame(curenv->env_page_directory ,ptr_frame_info ,fault_va , PERM_WRITEABLE|PERM_USER|PERM_PRESENT);

			int Page_file_val = pf_read_env_page(curenv,(void *)fault_va);
			if(Page_file_val == E_PAGE_NOT_EXIST_IN_PF)
			{
				if( (fault_va >= USTACKBOTTOM && fault_va <= USTACKTOP) ||(fault_va >= USER_HEAP_START && fault_va <= USER_HEAP_MAX) )
				{
					curenv->page_last_WS_element->virtual_address = fault_va;
					ptr_frame_info->element = curenv->page_last_WS_element;
					/*curenv->page_last_WS_element->empty = 1;
					curenv->page_last_WS_element->time_stamp = 0;*/
				}
				else
				{
					sched_kill_env(curenv->env_id);
				}
			}
			else
			{
				curenv->page_last_WS_element->virtual_address = fault_va;
				ptr_frame_info->element = curenv->page_last_WS_element;
				/*curenv->page_last_WS_element->empty = 1;
				curenv->page_last_WS_element->time_stamp = 0;*/
			}

			/* Update FIFO Pointer*/
			if(curenv->page_last_WS_element == LIST_LAST(&curenv->page_WS_list))
			{
				curenv->page_last_WS_element = LIST_FIRST(&curenv->page_WS_list);
			}
			else
			{
				curenv->page_last_WS_element = curenv->page_last_WS_element->prev_next_info.le_next;
			}
			//env_page_ws_print(curenv);
		}
	}

	/*LRU Re/placement*/
	if(isPageReplacmentAlgorithmLRU(PG_REP_LRU_LISTS_APPROX))
	{
		//cprintf("\nFaulted Va is %x\n" , fault_va);
		//env_page_ws_print(curenv);

		//TODO: [PROJECT'23.MS3 - #2] [1] PAGE FAULT HANDLER - LRU Replacement
		//TODO: [PROJECT'23.MS3 - BONUS] [1] PAGE FAULT HANDLER - O(1) implementation of LRU replacement
		int current_active_list_size = LIST_SIZE(&curenv->ActiveList);
		/*
		 * The activeList is not full
		 * So:-
		 * Surely: the faulted page is not exist in secondChanceList
		 * */
		if(current_active_list_size < curenv->ActiveListSize)
		{
			//cprintf("IN 1\n");
			add_WS_element_LRU_placement(fault_va);
		}
		/*The activeList is full
		 * So:-
		 * The faulted page may be in secondChanceList or in disk*/
		else
		{
			uint32 perms = pt_get_page_permissions( curenv->env_page_directory,fault_va);
			/*Check if the faulted page in secondChanceList or not*/
			int found = 0;
			if((perms & PERM_IN_LRU_SECOND_LIST) == PERM_IN_LRU_SECOND_LIST)
			{
				//cprintf("In Second List\n");
				found = 1;
			}

			/*If not found*/
			if(found == 0)
			{
				/*If the secondChanceList is not full*/
				int current_secondChance_list_size = LIST_SIZE(&curenv->SecondList);
				if(current_secondChance_list_size < curenv->SecondListSize)
				{
					//cprintf("IN 2\n");

					struct WorkingSetElement *activateList_old_WSE = LIST_LAST(&curenv->ActiveList);
					// set PERM_PRESENT to 0 & PERM_IN_LRU_SECOND_LIST to 1
					pt_set_page_permissions(curenv->env_page_directory, activateList_old_WSE->virtual_address, PERM_IN_LRU_SECOND_LIST, PERM_PRESENT);

					LIST_REMOVE(&curenv->ActiveList, activateList_old_WSE);
					LIST_INSERT_HEAD(&curenv->SecondList, activateList_old_WSE);

					add_WS_element_LRU_placement(fault_va);
				}
				/*The secondChanceList is full*/
				else
				{
					//cprintf("IN 3\n");

					/*
					 * 1- remove the tail of secondList
					 * 2- update the page in disk
					 * 3- remove the tail of activateList & add it to the secondList
					 * 4- add the new page in the head of the activate list
					 * */
					struct WorkingSetElement* ptr_tmp_WS_element = LIST_LAST(&curenv->SecondList);

					LIST_REMOVE(&curenv->SecondList, ptr_tmp_WS_element);
					// get page permissions
					uint32 permissions = pt_get_page_permissions( curenv->env_page_directory, ptr_tmp_WS_element->virtual_address);

					// If Modified -> Then update it
					if((permissions & PERM_MODIFIED) == PERM_MODIFIED){
						// get the page table.
						uint32* ptr_page_table = NULL;
						get_page_table(curenv->env_page_directory, ptr_tmp_WS_element->virtual_address, &ptr_page_table);

						// Get Pointer To Frame Info
						struct FrameInfo *ptr_frame_info = get_frame_info(curenv->env_page_directory, ptr_tmp_WS_element->virtual_address,&ptr_page_table);

						// Update the page in page file
						int ret = pf_update_env_page(curenv, ptr_tmp_WS_element->virtual_address, ptr_frame_info);
					}

					unmap_frame(curenv->env_page_directory, ptr_tmp_WS_element->virtual_address);
					pt_set_page_permissions(curenv->env_page_directory, ptr_tmp_WS_element->virtual_address, 0,PERM_IN_LRU_SECOND_LIST);

					struct WorkingSetElement *activateList_old_WSE = LIST_LAST(&curenv->ActiveList);
					pt_set_page_permissions(curenv->env_page_directory, activateList_old_WSE->virtual_address, PERM_IN_LRU_SECOND_LIST, PERM_PRESENT);
					LIST_REMOVE(&curenv->ActiveList, activateList_old_WSE);
					LIST_INSERT_HEAD(&curenv->SecondList, activateList_old_WSE);

					add_WS_element_LRU_RE_placement(ptr_tmp_WS_element, fault_va);
				}
			}
			/*Faulted page in second list*/
			else
			{
				//cprintf("IN 4\n");

				/*
				 * 1- remove the target page from the secondList
				 * 2- remove the tail of activateList & add it to the secondList
				 * 3- put the target page at the head of the list
				 * 4- make it's present to 1*/

				// get the the target working set element From FRAME_INFO
				uint32* ptr_page_table = NULL;
				//get the page table.
				get_page_table(curenv->env_page_directory, fault_va, &ptr_page_table);
				// get the frame from page table.
				struct FrameInfo * target_frame = get_frame_info(curenv->env_page_directory, fault_va, &ptr_page_table);

				struct WorkingSetElement *ptr_WS_element = target_frame->element;

				LIST_REMOVE(&curenv->SecondList, ptr_WS_element);

				struct WorkingSetElement *activateList_old_WSE = LIST_LAST(&curenv->ActiveList);
				pt_set_page_permissions(curenv->env_page_directory, activateList_old_WSE->virtual_address, PERM_IN_LRU_SECOND_LIST, PERM_PRESENT);
				LIST_REMOVE(&curenv->ActiveList, activateList_old_WSE);
				LIST_INSERT_HEAD(&curenv->SecondList, activateList_old_WSE);

				LIST_INSERT_HEAD(&(curenv->ActiveList), ptr_WS_element);
				pt_set_page_permissions(curenv->env_page_directory, ptr_WS_element->virtual_address, PERM_PRESENT, PERM_IN_LRU_SECOND_LIST);
			}
		}

		//env_page_ws_print(curenv);
	}
}

void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va)
{
	panic("this function is not required...!!");
}



