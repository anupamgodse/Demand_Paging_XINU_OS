#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

SYSCALL release_bs(bsd_t bs_id) {

  	/* release the backing store with ID bs_id */
	
	STATWORD ps;
	disable(ps);

	bs_map_t *bsptr = &bsm_tab[bs_id];
	int owner = bsptr->bs_pid;
	int vpno = bsptr->bs_vpno;
	int flag = 0;
	
	if(bsm_unmap(owner, vpno, flag) == SYSERR) {
		kprintf("release_bs error\n");
		restore(ps);
		return SYSERR;
	}
	restore(ps);
	return OK;
}

