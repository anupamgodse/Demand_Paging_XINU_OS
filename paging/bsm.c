/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_bsm()
{
	int i;
	//for each bsm_tab entry, make its status as unmapped 
	//and set npages to size of bs in pages
	
	bs_map_t *bs_entry;	

	for(i=0 ; i<NSTORES; i++) {
		bs_entry = &bsm_tab[i];
                bs_entry->bs_status = BSM_UNMAPPED;
                bs_entry->bs_npages = STORENFRAMES;
        }
	//never fails
	return OK;
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
	int i;
	//return OK if found else return SYSERR
	for(i = 0; i < NSTORES; i++) {
		if(bsm_tab[i].bs_status == BSM_UNMAPPED) {
			*avail = i;
			return OK;
		}
	}	
	return SYSERR;
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
	//freeing is simply unmapping the entry
	//any process can unmap any bsmtab entry
	
	bs_map_t *bs_entry = &bsm_tab[i];	
	bs_entry->bs_status = BSM_UNMAPPED;
	bs_entry->bs_npages = STORENFRAMES;
	return OK;
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
	//kprintf("looking up for %d processes %d vpno\n", pid, vaddr>>12);
	int i;
	//find store and page corresponding to this vaddr
	for(i = 0; i < NSTORES; i++) {
		if(bsm_tab[i].bs_status == BSM_MAPPED && 
			bsm_tab[i].bs_pid == pid) {
			*store = i;
			break;
		}
	}
	
	//if not found return SYSERROR
	if(i == NSTORES) {
		kprintf("bsm lookup failed: process not mapped to any backing store\n");
		return SYSERR;
	}
	
	//if found then check if page for that virtual address is mapped
	int vpno = bsm_tab[i].bs_vpno;
	long addr_start = vpno * NBPG;
	long addr_end = (vpno+(bsm_tab[i].bs_npages)) * NBPG;
	
	//check if virtual address is mapped to this backing store
	if(vaddr >= addr_start && vaddr < addr_end) {
		*pageth = ((int)(vaddr/NBPG)) - vpno;
		return OK;
	}
	else {	
		kprintf("bsm lookup failed: vaddr dosen't map to backing store\n");
		return SYSERR;
	}
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
	//return error if source is already mapped to 
	//some other pid
	
	bs_map_t *bs_entry = &bsm_tab[source];	
	struct pentry *process; 
	
	if((bs_entry->bs_status == BSM_MAPPED) || 
		npages < 1 || npages > STORENFRAMES) {
		kprintf("bsm_map failed\n");
		return SYSERR;
	}
	else {
		//kprintf("mapping %d proceses vpno %d to %d store\n", pid, vpno, source);
		bs_entry -> bs_pid = pid;
		bs_entry -> bs_vpno = vpno;
		bs_entry -> bs_npages = npages;
		bs_entry -> bs_status = BSM_MAPPED;
	}
	
	return OK;
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
	int source;
	int page;	
	SYSCALL ret;
 	int i;
	fr_map_t *frame;
	unsigned long vaddr;

	
	//iterate through inverted page table which
	//entries mapped to this pid and free those 
	for(i = 0; i < NFRAMES; i++) {
		frame = &frm_tab[i];
		if((frame->fr_status == FRM_MAPPED) && 
			(frame->fr_pid == pid) && (frame->fr_type == FR_PAGE)) {
			vaddr = (frame->fr_vpno) * NBPG;
			if(bsm_lookup(pid, vaddr, &source, 
				&page) == SYSERR) {
				kprintf("bsm unmap failed\n");
				return SYSERR;
			}
			else {
				free_frm(i);
			}
		}
	}
	
	//flag = 1 indicates that the function has been called 
	//from vfreemem and do not free the backing store
	if(flag != 1)
		free_bsm(source);
	
	return OK;
}
