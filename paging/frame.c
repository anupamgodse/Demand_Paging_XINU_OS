/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>
#include <q.h>

unsigned long invalidate;
extern int page_replace_policy;
extern struct qent sc_queue[];
extern int sc_qtail;
extern int sc_qhead;

/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */

SYSCALL print_frm() {
	int i;
	fr_map_t *frame;
	kprintf("frame status\n");
	for(i = 0 ; i < NFRAMES; i++) {
                frame = &frm_tab[i];
		if(frame->fr_status == FRM_MAPPED)
                kprintf("number = %d\tfr_status = %d\t fr_pid = %d\tfr_vpno = %d\t"
                        "fr_refcnt=%d\tfr_type = %d\tfr_dirty=%d\n", i, frame->fr_status, 
                        frame->fr_pid, frame->fr_vpno, frame->fr_refcnt, frame->fr_type, 
                        frame->fr_dirty);
        }

	return OK;	
}



SYSCALL init_frm()
{		
	int i;
	fr_map_t *fr_ptr;
	for(i=0 ; i < NFRAMES; i++) {
		fr_ptr = &frm_tab[i];
                fr_ptr->fr_status = FRM_UNMAPPED;
                fr_ptr->fr_refcnt = 0;
                fr_ptr->fr_dirty = 0;
        }
  	return OK;
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
	int i;
	int victim;
	fr_map_t *fr_ptr;
  	
  	//check if free frame exists in memory
  	for(i = 0; i < NFRAMES; i++) {
		if(frm_tab[i].fr_status == FRM_UNMAPPED) {
			*avail = i;
			return OK;
		}
		
	}
	
	//if free frame dosent exit then chooose a victim
	//frame to be replaced using the replacement
	//policy

	if(page_replace_policy == SC) {
		//check from head of queue if the ref_cnt is 0
		//if not then make it 0 and go to next until tail
		//on next iteration guaranteed 0

		struct	qent	*tptr = &sc_queue[sc_qtail];		/* points to tail entry		*/
		struct	qent	*temp = &((sc_queue[sc_qhead]));


		fr_ptr = &frm_tab[temp->qnext];
		while(fr_ptr->fr_refcnt != 0) {
			fr_ptr->fr_refcnt = 0;
			temp = &sc_queue[temp->qnext];
			
			//if reaches tail then restart the iteration
			if(&sc_queue[temp->qnext] == tptr) {
				temp = &((sc_queue[sc_qhead]));
			}
			
			fr_ptr = &frm_tab[temp->qnext];
		}
	
		//Now we point to frame whose refcnt is 0 and has index
		//temp->qkey		

		//this is victim frame and  we should free it
		
		victim = sc_dequeue(temp->qnext);
		free_frm(victim);

		*avail = victim;

	}
	
  	return OK;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{
	fr_map_t *frame = &frm_tab[i], *pt_frame, *pd_frame;
	//return if frame is already unmapped
	if(frame->fr_status == FRM_UNMAPPED) {
		return OK;
	}

	//just a precaution
	if(frame->fr_type != FR_PAGE) {
		kprintf("why free is called if frame type is not FR_PAGE\n");
		return SYSERR;
	}


	int owner; 	//owner pid of the frame
	int vp; 	//virtual page number of frame
	int is_dirty;	//is page to be replaced dirty?
	int store;	//backing store of page to be replaced
	int pageth;	//backing store index of page to be replaced
	unsigned long vaddr;	//vaddr of st vpno	
	//unsigned long *pd; 	//page directory ptr;
	unsigned long pt;	//page table ptr;
	int page_table_frame;	//frame occupied by page table
	int page_dir_frame;	//frame occupied by page dir
	pt_t *pte;	//page table entry ptr
	pd_t *pde;	//page directory entry ptr


	vp = frame->fr_vpno;
	vaddr = vp*NBPG;
	owner =	frame->fr_pid;
	pde = (pd_t *)proctab[owner].pdbr;
	page_dir_frame = (((unsigned long int)pde)>>12) - FRAME0;
	pd_frame = &frm_tab[page_dir_frame];



	virt_addr_t a;

	a.pg_offset = vaddr & 0xFFF;
	a.pt_offset = (vaddr >> 12) & 0x3FF;
	a.pd_offset = (vaddr >> 22) & 0x3FF;

	//get page table base address to which frame belongs
	pt = ((pde + a.pd_offset)->pd_base) << 12;
	
	//offset to page table entry of that page in page table
	pte = ((pt_t *)pt) + a.pt_offset;	

	//mark page as not present	
	pte->pt_pres = 0;
	is_dirty = pte->pt_dirty;

	//invalidate tlb entry	
	if(frame->fr_pid == currpid) {
		invalidate = (FRAME0+i)*NBPG;
		asm("invlpg invalidate");
	}

	//frame occupied by pt will be pt/NBPG
	page_table_frame = (pt>>12) - FRAME0;

	//decrement reference count of that page table by 1
	pt_frame = &frm_tab[page_table_frame];
	pt_frame->fr_refcnt--;
	

	//if reference count is 0 then mark the correponding
	//entry in page directory as not present and mark 
	//the frame which was allocated to this page table
	//as unmapped
	if(pt_frame->fr_refcnt == 0) {
		(pde + a.pd_offset)->pd_pres = 0;
		pt_frame->fr_status = FRM_UNMAPPED;
		//reduce the fr_refcnt of frame containing the 
		//page directory of this process
		pd_frame->fr_refcnt--;
	}
		
	
	


	//check to which process the frame is mapped and write this to processe's
	//baking store if dirty
	if(is_dirty == 1) {
		if(bsm_lookup(owner, vaddr, &store, 
			&pageth) == SYSERR) {
			kill(owner);
			kprintf("bsm lookup failed in free frame\n");
			return SYSERR;
		}
		else {
			write_bs((FRAME0+i) * NBPG, store, pageth);
		}
	}
	
	//mark the frame as unmapped
	frame->fr_status = FRM_UNMAPPED;
	
	//remove it from SC queue if the replacement policy is SC
	if(page_replace_policy == SC) {
		//it will not exist in SCqueue if it is page table or page directory
		if(frame->fr_type == FR_PAGE) {
			sc_dequeue(i);
		}
	}
		
  	return OK;
}



