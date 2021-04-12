#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "types.h"
#include "list_head.h"
#include "vm.h"


extern struct list_head processes;

extern struct process *current;

extern struct pagetable *ptbr;



extern unsigned int mapcounts[];



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


void free_page(unsigned int vpn)
{
	int page_num= vpn % 16;
	int outer_page_num = vpn / 16;
	mapcounts[ptbr->outer_ptes[outer_page_num]->ptes[page_num].pfn]--;
	ptbr->outer_ptes[outer_page_num]->ptes[page_num].valid = 0;
	ptbr->outer_ptes[outer_page_num]->ptes[page_num].writable = 0;
	ptbr->outer_ptes[outer_page_num]->ptes[page_num].pfn=0;
}



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



void switch_process(unsigned int pid)
{


	struct process *p;
	int isNew=1;

	list_for_each_entry(p, &processes, list) {
		if (p->pid == pid) {
			current = p;
			ptbr = &(p->pagetable);
			
			isNew =0;
	
			break;
		}
	}
	if(isNew == 1){
		p = malloc(sizeof(*p));		
		INIT_LIST_HEAD(&p->list);	
		list_add_tail(&p->list, &processes);

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

