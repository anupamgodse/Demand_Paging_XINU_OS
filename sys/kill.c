/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <stdio.h>
#include <paging.h>

/*------------------------------------------------------------------------
 * kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */

SYSCALL unmap_proc_frames(int pid) {
	int i;
	int store = -1;
	int vpno;

	//write back all dirty frames to backing store (same as bsm_unmap)
	//only if atall this process is mapped to some backing store
	//here bsm_unmap covers 1,2, 3 points given in description of 
	//process destruction behaviour in PA2 description
	
	//first check to which backing store is this pid mapped
	for(i=0; i < NSTORES; i++) {
		if(bsm_tab[i].bs_status == BSM_MAPPED && 
			bsm_tab[i].bs_pid == pid) {
			store = i;
			vpno = bsm_tab[i].bs_vpno;
			break;
		}
	}

	if(store != -1)
		bsm_unmap(pid, vpno, 0);

	//free the frame taken by page pid's page directory
	frm_tab[((proctab[pid].pdbr)/NBPG) - FRAME0].fr_status = FRM_UNMAPPED;
	return OK;
}


SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev;

	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	if (--numproc == 0)
		xdone();

	dev = pptr->pdevs[0];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->pdevs[1];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->ppagedev;
	if (! isbaddev(dev) )
		close(dev);
	
	send(pptr->pnxtkin, pid);

	freestk(pptr->pbase, pptr->pstklen);
	switch (pptr->pstate) {

	case PRCURR:	pptr->pstate = PRFREE;	/* suicide */
			unmap_proc_frames(pid);
			resched();

	case PRWAIT:	semaph[pptr->psem].semcnt++;

	case PRREADY:	dequeue(pid);
			pptr->pstate = PRFREE;
			break;

	case PRSLEEP:
	case PRTRECV:	unsleep(pid);
						/* fall through	*/
	default:	pptr->pstate = PRFREE;
	}

	unmap_proc_frames(pid);	
	restore(ps);
	return(OK);
}
