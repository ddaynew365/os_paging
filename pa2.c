/**********************************************************************
 * Copyright (c) 2020
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "types.h"
#include "list_head.h"
#include "vm.h"

/**
 * Ready queue of the system
 */
extern struct list_head processes;

/**
 * Currently running process
 */
extern struct process *current;

/**
 * Page Table Base Register that MMU will walk through for address translation
 */
extern struct pagetable *ptbr;


/**
 * The number of mappings for each page frame. Can be used to determine how
 * many processes are using the page frames.
 */
extern unsigned int mapcounts[];


/**
 * alloc_page(@vpn, @rw)
 *
 * DESCRIPTION
 *   Allocate a page frame that is not allocated to any process, and map it
 *   to @vpn. When the system has multiple free pages, this function should
 *   allocate the page frame with the **smallest pfn**.
 *   You may construct the page table of the @current process. When the page
 *   is allocated with RW_WRITE flag, the page may be later accessed for writes.
 *   However, the pages populated with RW_READ only should not be accessed with
 *   RW_WRITE accesses.
 *
 * RETURN
 *   Return allocated page frame number.
 *   Return -1 if all page frames are allocated.
 */
unsigned int alloc_page(unsigned int vpn, unsigned int rw)
{
	int page_num= vpn % 16;
	int outer_page_num = vpn / 16;
	if(ptbr->outer_ptes[outer_page_num] == NULL)
		ptbr->outer_ptes[outer_page_num] = malloc(sizeof(ptbr->outer_ptes[outer_page_num]->ptes));
	int free_page=0;
	for(;free_page<16;free_page++)
	{	
		if(mapcounts[free_page] ==0)
			break;
	}

	if(free_page == 16)
		return -1;
	else{
		mapcounts[free_page]++;
		ptbr->outer_ptes[outer_page_num]->ptes[page_num].valid = 1;
		if(rw==1)
			ptbr->outer_ptes[outer_page_num]->ptes[page_num].writable = 0;
		else 
			ptbr->outer_ptes[outer_page_num]->ptes[page_num].writable = 1;
		ptbr->outer_ptes[outer_page_num]->ptes[page_num].pfn=free_page;
		return ptbr->outer_ptes[outer_page_num]->ptes[page_num].pfn;
	}
	
}


/**
 * free_page(@vpn)
 *
 * DESCRIPTION
 *   Deallocate the page from the current processor. Make sure that the fields
 *   for the corresponding PTE (valid, writable, pfn) is set @false or 0.
 *   Also, consider carefully for the case when a page is shared by two processes,
 *   and one process is to free the page.
 */
void free_page(unsigned int vpn)
{
	int page_num= vpn % 16;
	int outer_page_num = vpn / 16;
	mapcounts[ptbr->outer_ptes[outer_page_num]->ptes[page_num].pfn]--;
	ptbr->outer_ptes[outer_page_num]->ptes[page_num].valid = 0;
	ptbr->outer_ptes[outer_page_num]->ptes[page_num].writable = 0;
	ptbr->outer_ptes[outer_page_num]->ptes[page_num].pfn=0;
}


/**
 * handle_page_fault()
 *
 * DESCRIPTION
 *   Handle the page fault for accessing @vpn for @rw. This function is called
 *   by the framework when the __translate() for @vpn fails. This implies;
 *   0. page directory is invalid
 *   1. pte is invalid
 *   2. pte is not writable but @rw is for write
 *   This function should identify the situation, and do the copy-on-write if
 *   necessary.
 *
 * RETURN
 *   @true on successful fault handling
 *   @false otherwise
 */
bool handle_page_fault(unsigned int vpn, unsigned int rw)
{
	int page_num= vpn % 16;
	int outer_page_num = vpn / 16;
	if(ptbr->outer_ptes[outer_page_num]->ptes[page_num].private == 1 && mapcounts[ptbr->outer_ptes[outer_page_num]->ptes[page_num].pfn]>1)
	{
		
		ptbr->outer_ptes[outer_page_num]->ptes[page_num].writable = 1;
		mapcounts[ptbr->outer_ptes[outer_page_num]->ptes[page_num].pfn]--;
		alloc_page(vpn,rw);
		ptbr->outer_ptes[outer_page_num]->ptes[page_num].private = 0;
		return true;
	}	

	if(ptbr->outer_ptes[outer_page_num]->ptes[page_num].private == 1 && mapcounts[ptbr->outer_ptes[outer_page_num]->ptes[page_num].pfn]==1)
	{
		ptbr->outer_ptes[outer_page_num]->ptes[page_num].writable = 1;
		ptbr->outer_ptes[outer_page_num]->ptes[page_num].private == 0;
		return true;
	}


	return false;
}


/**
 * switch_process()
 *
 * DESCRIPTION
 *   If there is a process with @pid in @processes, switch to the process;
 *   @current should be replaced to the requested process. Make sure that
 *   @ptbr is set properly.
 *
 *   If there is no process with @pid in the @processes list, fork a process
 *   from the @current. This implies the forked child process should have
 *   the identical page table entry 'values' to its parent's (i.e., @current)
 *   page table. 
 *   To implement the copy-on-write feature, you should manipulate the writable
 *   bit in PTE and mapcounts for shared pages. You can use pte->private to
 *   remember whether the PTE was originally writable or not.
 */
void switch_process(unsigned int pid)
{
	/** YOU CAN CHANGE EVERYTING (INCLUDING THE EXAMPLE) IN THIS FUNCTION **/

	struct process *p;
	int isNew=1;
	/* This example shows to iterate @processes to find a process with @pid */
	list_for_each_entry(p, &processes, list) {
		if (p->pid == pid) {
			/* FOUND */
			current = p;
			ptbr = &(p->pagetable);
			
			isNew =0;
	
			break;
		}
	}
	if(isNew == 1){
		p = malloc(sizeof(*p));		/* This example shows to create a process, */
		INIT_LIST_HEAD(&p->list);	/* initialize list_head, */
		list_add_tail(&p->list, &processes);
									/* and add it to the @processes list */
		for(int out=0;out<16;out++)
			{
				if(ptbr->outer_ptes[out] == NULL)	
					continue;

				for(int in = 0; in<16; in++)
				{
					if(ptbr->outer_ptes[out]->ptes[in].writable ==1)
					{
						ptbr->outer_ptes[out]->ptes[in].private = 1;
						ptbr->outer_ptes[out]->ptes[in].writable = 0;
					}
				}
			}

		
		for(int out=0;out<16;out++)
		{
			if(ptbr->outer_ptes[out] == NULL)	
				continue;
			p->pagetable.outer_ptes[out] = malloc(sizeof(p->pagetable.outer_ptes[out]->ptes));	
			for(int in = 0; in<16; in++)
			{
				
				if(ptbr->outer_ptes[out]->ptes[in].valid ==1)
				{
					p->pagetable.outer_ptes[out]->ptes[in].valid = 1;
					p->pagetable.outer_ptes[out]->ptes[in].pfn = ptbr->outer_ptes[out]->ptes[in].pfn;
					p->pagetable.outer_ptes[out]->ptes[in].private = ptbr->outer_ptes[out]->ptes[in].private;
					mapcounts[p->pagetable.outer_ptes[out]->ptes[in].pfn]++;
				}
			}	
		}
		p->pid = pid;
		current = p;
		ptbr = &(p->pagetable);

	
	}
}

