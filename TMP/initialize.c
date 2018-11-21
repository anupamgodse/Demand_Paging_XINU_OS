/* initialize.c - nulluser, sizmem, sysinit */

#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <sleep.h>
#include <mem.h>
#include <tty.h>
#include <q.h>
#include <io.h>
#include <paging.h>

/*#define DETAIL */
#define HOLESIZE	(600)	
#define	HOLESTART	(640 * 1024)
#define	HOLEEND		((1024 + HOLESIZE) * 1024)  
/* Extra 600 for bootp loading, and monitor */

extern	int	main();	/* address of user's main prog	*/

extern	int	start();

LOCAL		sysinit();

/* Declarations of major kernel variables */
struct	pentry	proctab[NPROC]; /* process table			*/
int	nextproc;		/* next process slot to use in create	*/
struct	sentry	semaph[NSEM];	/* semaphore table			*/
int	nextsem;		/* next sempahore slot to use in screate*/
struct	qent	q[NQENT];	/* q table (see queue.c)		*/
int	nextqueue;		/* next slot in q structure to use	*/
char	*maxaddr;		/* max memory address (set by sizmem)	*/
struct	mblock	memlist;	/* list of free memory blocks		*/
#ifdef	Ntty
struct  tty     tty[Ntty];	/* SLU buffers and mode control		*/
#endif

/* active system status */
int	numproc;		/* number of live user processes	*/
int	currpid;		/* id of currently running process	*/
int	reboot = 0;		/* non-zero after first boot		*/

int	rdyhead,rdytail;	/* head/tail of ready list (q indicies)	*/
char 	vers[80];
int	console_dev;		/* the console device			*/

/*  added for the demand paging */
int page_replace_policy = SC;
/*struct qent sc_queue[NFRAMES+2];
int sc_qtail = NFRAMES+2;
int sc_qhead = NFRAMES+1;*/
struct mblock vmemlist[NSTORES];//backing store private heap related memlist

bs_map_t bsm_tab[NSTORES];
fr_map_t frm_tab[NFRAMES];


/************************************************************************/
/***				NOTE:				      ***/
/***								      ***/
/***   This is where the system begins after the C environment has    ***/
/***   been established.  Interrupts are initially DISABLED, and      ***/
/***   must eventually be enabled explicitly.  This routine turns     ***/
/***   itself into the null process after initialization.  Because    ***/
/***   the null process must always remain ready to run, it cannot    ***/
/***   execute code that might cause it to be suspended, wait for a   ***/
/***   semaphore, or put to sleep, or exit.  In particular, it must   ***/
/***   not do I/O unless it uses kprintf for polled output.           ***/
/***								      ***/
/************************************************************************/

/*------------------------------------------------------------------------
 *  nulluser  -- initialize system and become the null process (id==0)
 *------------------------------------------------------------------------
 */
nulluser()				/* babysit CPU when no one is home */
{
        int userpid;

	console_dev = SERIAL0;		/* set console to COM0 */

	initevec();

	kprintf("system running up!\n");
	sysinit();

	enable();		/* enable interrupts */

	sprintf(vers, "PC Xinu %s", VERSION);
	kprintf("\n\n%s\n", vers);
	if (reboot++ < 1)
		kprintf("\n");
	else
		kprintf("   (reboot %d)\n", reboot);


	kprintf("%d bytes real mem\n",
		(unsigned long) maxaddr+1);
#ifdef DETAIL	
	kprintf("    %d", (unsigned long) 0);
	kprintf(" to %d\n", (unsigned long) (maxaddr) );
#endif	

	kprintf("%d bytes Xinu code\n",
		(unsigned long) ((unsigned long) &end - (unsigned long) start));
#ifdef DETAIL	
	kprintf("    %d", (unsigned long) start);
	kprintf(" to %d\n", (unsigned long) &end );
#endif

#ifdef DETAIL	
	kprintf("%d bytes user stack/heap space\n",
		(unsigned long) ((unsigned long) maxaddr - (unsigned long) &end));
	kprintf("    %d", (unsigned long) &end);
	kprintf(" to %d\n", (unsigned long) maxaddr);
#endif	
	
	kprintf("clock %sabled\n", clkruns == 1?"en":"dis");


	/* create a process to execute the user's main program */
	userpid = create(main,INITSTK,INITPRIO,INITNAME,INITARGS);
	resume(userpid);

	while (TRUE)
		/* empty */;
}

/*------------------------------------------------------------------------
 *  sysinit  --  initialize all Xinu data structeres and devices
 *------------------------------------------------------------------------
 */
LOCAL
sysinit()
{
	static	long	currsp;
	int	i,j;
	struct	pentry	*pptr;
	struct	sentry	*sptr;
	struct	mblock	*mptr;
	SYSCALL pfintr();

	//size of page table entry
	int pte_size = sizeof(pt_t);	

	//number of page table entries per frame
	int npte_frame = NBPG / pte_size;

	//size of page directory entry
	int pde_size = sizeof(pd_t);	

	//number of page table entries per frame
	int npde_frame = NBPG / pde_size;
	
	//global page table entry
	pt_t *pte_address;
	pt_t pt_entry  = {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};	
	
	pd_t *pde_address;
	pd_t pd_entry = {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	//frame pointers
	fr_map_t *fr_ptr = NULL;

	numproc = 0;			/* initialize system variables */
	nextproc = NPROC-1;
	nextsem = NSEM-1;
	nextqueue = NPROC;		/* q[0..NPROC-1] are processes */

	/* initialize free memory list */
	/* PC version has to pre-allocate 640K-1024K "hole" */
	if (maxaddr+1 > HOLESTART) {
		memlist.mnext = mptr = (struct mblock *) roundmb(&end);
		mptr->mnext = (struct mblock *)HOLEEND;
		mptr->mlen = (int) truncew(((unsigned) HOLESTART -
	     		 (unsigned)&end));
        	
		mptr->mlen -= 4;

		mptr = (struct mblock *) HOLEEND;
		mptr->mnext = 0;
		mptr->mlen = (int) truncew((unsigned)maxaddr - HOLEEND -
	      		NULLSTK);
/*
		mptr->mlen = (int) truncew((unsigned)maxaddr - (4096 - 1024 ) *  4096 - HOLEEND - NULLSTK);
*/
	} else {
		/* initialize free memory list */
		memlist.mnext = mptr = (struct mblock *) roundmb(&end);
		mptr->mnext = 0;
		mptr->mlen = (int) truncew((unsigned)maxaddr - (int)&end -
			NULLSTK);
	}
	

	for (i=0 ; i<NPROC ; i++)	/* initialize process table */
		proctab[i].pstate = PRFREE;


#ifdef	MEMMARK
	_mkinit();			/* initialize memory marking */
#endif

#ifdef	RTCLOCK
	clkinit();			/* initialize r.t.clock	*/
#endif

	mon_init();     /* init monitor */

#ifdef NDEVS
	for (i=0 ; i<NDEVS ; i++ ) {	    
	    init_dev(i);
	}
#endif

	pptr = &proctab[NULLPROC];	/* initialize null process entry */
	pptr->pstate = PRCURR;
	for (j=0; j<7; j++)
		pptr->pname[j] = "prnull"[j];
	pptr->plimit = (WORD)(maxaddr + 1) - NULLSTK;
	pptr->pbase = (WORD) maxaddr - 3;
/*
	pptr->plimit = (WORD)(maxaddr + 1) - NULLSTK - (4096 - 1024 )*4096;
	pptr->pbase = (WORD) maxaddr - 3 - (4096-1024)*4096;
*/
	pptr->pesp = pptr->pbase-4;	/* for stkchk; rewritten before used */
	*( (int *)pptr->pbase ) = MAGIC;
	pptr->paddr = (WORD) nulluser;
	pptr->pargs = 0;
	pptr->pprio = 0;
	currpid = NULLPROC;

	for (i=0 ; i<NSEM ; i++) {	/* initialize semaphores */
		(sptr = &semaph[i])->sstate = SFREE;
		sptr->sqtail = 1 + (sptr->sqhead = newqueue());
	}

	rdytail = 1 + (rdyhead=newqueue());/* initialize ready list */

	//initialize data structures for paging
	
	//all backing stores are unmapped initially
	init_bsm();

	//initialize frame table
	init_frm();	
	
	//map frames0-4 to global page tables
	for(i=0 ; i < 4; i++) {
		fr_ptr = &frm_tab[i];
		fr_ptr->fr_status = FRM_MAPPED;
		fr_ptr->fr_pid = NULLPROC;
		fr_ptr->fr_type = FR_TBL;
		fr_ptr->fr_vpno = FRAME0 + i;
	
		pte_address = (FRAME0 + i) * NBPG;
		for(j = i * npte_frame ; j < (i+1) * npte_frame; j++) {
			
			//setting page number this entry points to 
			pt_entry.pt_base = j;
			
			//copying pt_entry to jth(0-1023) page table entry in FRAMEi(0-4)
			*pte_address = pt_entry;
			
			//incrementing address by pte factor
			pte_address++;
			
		}

		//this page table references npte_frame pages
		fr_ptr->fr_refcnt = npte_frame;

	}

	fr_ptr = &frm_tab[4];
	
	//map frame4 to NULL processe's page directory
	fr_ptr->fr_status = FRM_MAPPED;
	fr_ptr->fr_pid = NULLPROC;
	fr_ptr->fr_type = FR_DIR;
	fr_ptr->fr_vpno = FRAME0 + 4;
		
	//page directory references 4 global page tables
	fr_ptr->fr_refcnt = 4;
	
	//point to global page tables
	pde_address = (FRAME0+4) * NBPG;
	
	//setting PDBR for NULLPROC
	pptr->pdbr = pde_address;

	//setting cr3 register
	unsigned long int temp = read_cr3();
	temp &= 0xfff;	
	temp |= ((FRAME0+4) << 12);
	write_cr3(temp);
	
	for(i = 0; i < 4; i++) {
		//setting page table base address
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
	
	//install interrupt routine for page fault interrupt
	set_evec(14, pfintr);	
	

	srpolicy(page_replace_policy);

	//enable paging
	enable_paging();


	return(OK);
}

stop(s)
char	*s;
{
	kprintf("%s\n", s);
	kprintf("looping... press reset\n");
	while(1)
		/* empty */;
}

delay(n)
int	n;
{
	DELAY(n);
}


#define	NBPG	4096

/*------------------------------------------------------------------------
 * sizmem - return memory size (in pages)
 *------------------------------------------------------------------------
 */
long sizmem()
{
	unsigned char	*ptr, *start, stmp, tmp;
	int		npages;

	/* at least now its hacked to return
	   the right value for the Xinu lab backends (16 MB) */

	return 4096; 

	start = ptr = 0;
	npages = 0;
	stmp = *start;
	while (1) {
		tmp = *ptr;
		*ptr = 0xA5;
		if (*ptr != 0xA5)
			break;
		*ptr = tmp;
		++npages;
		ptr += NBPG;
		if ((int)ptr == HOLESTART) {	/* skip I/O pages */
			npages += (1024-640)/4;
			ptr = (unsigned char *)HOLEEND;
		}
	}
	return npages;
}
