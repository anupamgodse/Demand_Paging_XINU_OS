/* vfreemem.c - vfreemem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];
extern struct mblock vmemlist[];
/*------------------------------------------------------------------------
 *  vfreemem  --  free a virtual memory block, returning it to vmemlist
 *------------------------------------------------------------------------
 */
SYSCALL	vfreemem(block, size)
	struct	mblock	*block;
	unsigned size;
{

	STATWORD ps;    
        struct  mblock  *p, *q;
        unsigned top;
	
	struct pentry *proc;
	proc = &proctab[currpid];
	
	unsigned long min = proc->vhpno*NBPG;
	unsigned long max = (proc->vhpno+proc->vhpnpages)*NBPG - 1;

	
	struct mblock *vmemlist = proctab[currpid].vmemlist;

        if (size==0 || (unsigned)block>(unsigned)max
            || ((unsigned)block)<((unsigned)min)) {
                return(SYSERR);
	}
        size = (unsigned)roundmb(size);
        disable(ps);

	
        for( p=vmemlist->mnext,q= vmemlist;
             p != (struct mblock *) NULL && p < block ;
             q=p,p=p->mnext )
                ;

        if (((top=q->mlen+(unsigned)q)>(unsigned)block && q!= vmemlist) ||
            (p!=NULL && (size+(unsigned)block) > (unsigned)p )) {
                restore(ps);
                return(SYSERR);
        }
        if ( q!= vmemlist && top == (unsigned)block )
                        q->mlen += size;
        else {
                block->mlen = size;
                block->mnext = p;
                q->mnext = block;
                q = block;
        }
        if ( (unsigned)( q->mlen + (unsigned)q ) == (unsigned)p) {
                q->mlen += p->mlen;
                q->mnext = p->mnext;
        }
        restore(ps);
        return(OK);	
}
