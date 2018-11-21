/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
	STATWORD ps;
	disable(ps);
	if(bsm_map(currpid, virtpage, source, npages) == SYSERR) {
		kprintf("xmmap error\n");
		restore(ps);
		return SYSERR;
	}
	restore(ps);
	return OK;
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage)
{
	STATWORD ps;
	disable(ps);
	int store;
	int pageth;
	int flag = 0; 
	//check if the process has mapping to any of the stores
	if(bsm_lookup(currpid, virtpage*NBPG, &store, &pageth) == SYSERR 
		|| bsm_unmap(currpid, virtpage, flag) == SYSERR) {
		kprintf("xmunmap error\n");
		restore(ps);
		return SYSERR;
	}
	restore(ps);
	return OK;
}

