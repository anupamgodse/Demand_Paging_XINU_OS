/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

extern int page_replace_policy;
extern int sc_qtail;
extern int sc_qhead;
extern long pferrcode;

/*------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */

void print_page_dir(unsigned long pdbr) {
	pd_t *pde = pdbr;
	pt_t *pte;
	int i, j;

	pde+=4;
	for(i=4; i<1024; i++) {
		if(pde->pd_pres == 1) {
			kprintf("pdeno=%d\tpd_pres=%d\tpd_write=%d\tpd_base=%d\n", 
				i, pde->pd_pres, pde->pd_write, pde->pd_base);
			pte = pde->pd_base << 12;
			for(j=0; j<1024; j++) {
				if(pte->pt_pres == 1)
					kprintf("pteno=%d\tpt_pres=%d\tpt_write=%d\tpt_base=%d\n", 
						j, pte->pt_pres, pte->pt_write, pte->pt_base);
					pte++;
			}
		}
		pde++;
	}

}



SYSCALL pfint()
{
	//pferrcode	
	//kprintf("pferrcode = %08x\n", pferrcode);
	unsigned long vaddr; 	//address causing page fault
	int vpno;		//page containing vaddr
	unsigned long pd;	// page directory ptr
	int store;		//backing store to which this process is mapped
	int pageth;		//page to which faulted page is mapped in backing store
	int page_dir_frame;
	int page_table_frame;
	int page_frame;
	pd_t *pde;		//page directory entry
	pt_t *pte;		//page table entry
	int pd_pres;		//page table present
	fr_map_t *fr_table_ptr;	//frame table ptr
	fr_map_t *fr_dir_ptr; //frame directory ptr
	fr_map_t *fr_page_ptr; //frame directory ptr
	
	int i;

	pt_t *pte_address;
        pt_t pt_entry  = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	

	vaddr = read_cr2();
	vpno = vaddr>>12;
	pd = (proctab[currpid]).pdbr;
	page_dir_frame = ((int)(pd/NBPG)) - FRAME0;
	fr_dir_ptr = &frm_tab[page_dir_frame];


	//kprintf("faulted on %lu %d for %d proc\n", vaddr, vpno, currpid);

	//check if vaddr is legal
	//it is legal iff it is mapped to one of the backing stores
	if(bsm_lookup(currpid, vaddr, &store, &pageth) == SYSERR) {
		kprintf("illegal address access: killing process\n");
		kill(currpid);
		return SYSERR;
	}
		
		
	virt_addr_t a;
        a.pg_offset = vaddr & 0xFFF;
        a.pt_offset = (vaddr >> 12) & 0x3FF;
        a.pd_offset = (vaddr >> 22) & 0x3FF;	

	
	//check if page table exists in memory
	pde = ((pd_t *)pd + a.pd_offset);
		
	pd_pres = pde->pd_pres;

	//if not present then we need to allocate page table itself
	//in memory first
	if(pd_pres == 0) {
		get_frm(&page_table_frame);
		fr_table_ptr = &frm_tab[page_table_frame];
		fr_table_ptr->fr_status = FRM_MAPPED;
		fr_table_ptr->fr_pid = currpid;
		fr_table_ptr->fr_type = FR_TBL;
		fr_table_ptr->fr_vpno = FRAME0 + page_table_frame; //nosense
		
		pte_address = (FRAME0+page_table_frame) * NBPG;
		//mark all pages in page table as not present
		for(i = 0; i < NPTEFRAME; i++) {
			*pte_address = pt_entry;
			pte_address++;		
		}
		//mark corresponding page directory entry as available
		//and assign pd_base
		pde->pd_pres = 1;
		pde->pd_base = FRAME0 + page_table_frame;
		pde->pd_write = 1;
	
		//now as you have page directory pointing 
		//to one more page in memory increase its ref_cnt
		//increase ref count of page directory
		//this is absolutely not necessory
		//i dont know why i am doing this
		fr_dir_ptr->fr_refcnt += 1;
	}		
	else {
		page_table_frame = (pde->pd_base)-FRAME0;
		fr_table_ptr = &frm_tab[page_table_frame];
	}
				
	
	
	//get frame for page
	get_frm(&page_frame);
	
	read_bs((FRAME0+page_frame)*NBPG, store, pageth);

	//mark this page as present in page table
	//and assign its location
	pte_address = (FRAME0 + page_table_frame) * NBPG;	
	pte = pte_address + a.pt_offset;	
	
	pte->pt_pres = 1;
	pte->pt_write = 1;
	pte->pt_base = FRAME0 + page_frame;

	//set all frame parameters
	fr_page_ptr = &frm_tab[page_frame];
	fr_page_ptr->fr_status = FRM_MAPPED;
	fr_page_ptr->fr_pid = currpid;
	fr_page_ptr->fr_type = FR_PAGE;
	fr_page_ptr->fr_vpno = vpno; 

	
	//increase page table frame refcount
	//as it now points to one more page
	//which is present in memory
	fr_table_ptr->fr_refcnt += 1;



	if(page_replace_policy == SC) {
		//append this to the SR policy queue
		sc_enqueue(page_frame, sc_qtail);
		//also make reference bit as 1
		fr_page_ptr->fr_refcnt = 1;
	}	
	return OK;
}


