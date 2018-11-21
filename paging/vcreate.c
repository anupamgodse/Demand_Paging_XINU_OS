/* vcreate.c - vcreate */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

/*
static unsigned long esp;
*/

extern struct mblock vmemlist[];


LOCAL	newpid();
/*------------------------------------------------------------------------
 *  create  -  create a process to start running a procedure
 *------------------------------------------------------------------------
 */
SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	hsize;			/* virtual heap size in pages	*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{


	unsigned long	savsp, *pushsp;
	STATWORD 	ps;    
	int		pid;		/* stores new process id	*/
	struct	pentry	*pptr;		/* pointer to proc. table entry */
	int		i;
	unsigned long	*a;		/* points to list of args	*/
	unsigned long	*saddr;		/* stack address		*/
	int		INITRET();

	int store;			//backing store for heap
	int page_directory_frame;
        fr_map_t *fr_ptr = NULL;
	
	//size of page directory entry
        int pde_size = sizeof(pd_t);

        //number of page table entries per frame
        int npde_frame = NBPG / pde_size;
	
        pd_t *pde_address;
        pd_t pd_entry = {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};	

	//vmemlist ptr
	struct mblock *mptr;

	disable(ps);
	if (ssize < MINSTK)
		ssize = MINSTK;
	ssize = (int) roundew(ssize);
	if (((saddr = (unsigned long *)getstk(ssize)) ==
	    (unsigned long *)SYSERR ) ||
	    (pid=newpid()) == SYSERR || priority < 1 ) {
		restore(ps);
		return(SYSERR);
	}

	//for private heap	
	if(hsize == 0) {
		//has no backing store
		proctab[pid].store = -1;
		//no free memeory
		proctab[pid].vmemlist = NULL; 
		//rest same as create
		restore(ps);
		return pid;
	}
	else {
		//try and get backing store if available
		//if not available then return SYSERR
		if(get_bsm(&store) == SYSERR) {
			proctab[pid].store = -1;
			//no free memeory
			proctab[pid].vmemlist = NULL;
			kprintf("vcreate error: no free backing store available\n");
			restore(ps);
			return SYSERR;
		}
		else {
			//now map this store to this process heap
			//kprintf("mapping %d\s vpage 4096 to %d store\n", pid, store);
			if(bsm_map(pid, 4096, store, hsize) 
				== SYSERR) {
				kprintf("vcreate error: backing store mapping failed\n");
				proctab[pid].store = -1;
				//no free memeory
				proctab[pid].vmemlist = NULL; 
				restore(ps);
				return SYSERR;
			}
			//now you have baking store and now 
			//initialize the vmemlist for this process
			else {

				proctab[pid].vmemlist = &vmemlist[store];

				vmemlist[store].mnext = mptr = (struct mblock *) roundmb(BACKING_STORE_BASE+
									store*BACKING_STORE_UNIT_SIZE);
		                mptr->mnext = NULL;
                		mptr->mlen = (int) truncew(hsize*NBPG);
                
				proctab[pid].store = store;
				proctab[pid].vhpno = 4096;
				proctab[pid].vhpnpages = hsize;
				//vmemlist[store].mnext = BACKING_STORE_BASE + BACKING_STORE_UNIT_SIZE * store;
				//vmemlist[store].mlen = hsize * NBPG;
				proctab[pid].vmemlist->mnext = 4096*NBPG;
				//proctab[pid].vmemlist = 4096*NBPG;
			}
		}
	}
	
	//kprintf("process has been mapped to %d store\n", store);
	//rest of create vsame as create
	//no more erros possibles then increament numprocs
	numproc++;
	pptr = &proctab[pid];

	pptr->fildes[0] = 0;	/* stdin set to console */
	pptr->fildes[1] = 0;	/* stdout set to console */
	pptr->fildes[2] = 0;	/* stderr set to console */

	for (i=3; i < _NFILE; i++)	/* others set to unused */
		pptr->fildes[i] = FDFREE;

	pptr->pstate = PRSUSP;
	for (i=0 ; i<PNMLEN && (int)(pptr->pname[i]=name[i])!=0 ; i++)
		;
	pptr->pprio = priority;
	pptr->pbase = (long) saddr;
	pptr->pstklen = ssize;
	pptr->psem = 0;
	pptr->phasmsg = FALSE;
	pptr->plimit = pptr->pbase - ssize + sizeof (long);	
	pptr->pirmask[0] = 0;
	pptr->pnxtkin = BADPID;
	pptr->pdevs[0] = pptr->pdevs[1] = pptr->ppagedev = BADDEV;

		/* Bottom of stack */
	*saddr = MAGIC;
	savsp = (unsigned long)saddr;

	/* push arguments */
	pptr->pargs = nargs;
	a = (unsigned long *)(&args) + (nargs-1); /* last argument	*/
	for ( ; nargs > 0 ; nargs--)	/* machine dependent; copy args	*/
		*--saddr = *a--;	/* onto created process' stack	*/
	*--saddr = (long)INITRET;	/* push on return address	*/

	*--saddr = pptr->paddr = (long)procaddr; /* where we "ret" to	*/
	*--saddr = savsp;		/* fake frame ptr for procaddr	*/
	savsp = (unsigned long) saddr;

/* this must match what ctxsw expects: flags, regs, old SP */
/* emulate 386 "pushal" instruction */
	*--saddr = 0;
	*--saddr = 0;	/* %eax */
	*--saddr = 0;	/* %ecx */
	*--saddr = 0;	/* %edx */
	*--saddr = 0;	/* %ebx */
	*--saddr = 0;	/* %esp; fill in below */
	pushsp = saddr;
	*--saddr = savsp;	/* %ebp */
	*--saddr = 0;		/* %esi */
	*--saddr = 0;		/* %edi */
	*pushsp = pptr->pesp = (unsigned long)saddr;
	

	//get free frame for page directory of this process

        get_frm(&page_directory_frame);

        //map this frame to this pid's page directory
        fr_ptr = &frm_tab[page_directory_frame];
        fr_ptr->fr_status = FRM_MAPPED;
        fr_ptr->fr_pid = pid;
        fr_ptr->fr_type = FR_DIR;
        fr_ptr->fr_vpno = FRAME0+page_directory_frame;

        //page directory references 4 global page tables
        fr_ptr->fr_refcnt = 4;

        //this page directory has address (FRAME0 + page_directory_frame) * NBPG
        pde_address = (FRAME0 + page_directory_frame)*NBPG;

        //set process's pdbr to this address
        proctab[pid].pdbr = pde_address;

        //let this page directory point to global page tables
        for(i = 0; i < 4; i++) {
                //setting global page table base address
                pd_entry.pd_base = FRAME0 + i;

                //setting pde
                *pde_address = pd_entry;

                //increment address by pde factor
                pde_address++;

        }

	//mark rest of the page tables as not present
        for( ;i < npde_frame; i++) {
                pd_entry.pd_pres = 0;

                //setting pde
                *pde_address = pd_entry;

                //increment address by pde factor
                pde_address++;
        }	

	restore(ps);

	return(pid);	
}

/*------------------------------------------------------------------------
 * newpid  --  obtain a new (free) process id
 *------------------------------------------------------------------------
 */
LOCAL	newpid()
{
	int	pid;			/* process id to return		*/
	int	i;

	for (i=0 ; i<NPROC ; i++) {	/* check all NPROC slots	*/
		if ( (pid=nextproc--) <= 0)
			nextproc = NPROC-1;
		if (proctab[pid].pstate == PRFREE)
			return(pid);
	}
	return(SYSERR);
}
