/* user.c - main */

//#define NEW2 1

#ifdef NEW
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 *  main  --  user main program
 *------------------------------------------------------------------------
 */
int main()
{
	kprintf("\n\nHello World, Xinu@QEMU lives\n\n");

        /* The hook to shutdown QEMU for process-like execution of XINU.
         * This API call terminates the QEMU process.
         */
        shutdown();
}

#elif NEW2
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>


#define PROC1_VADDR	0x40000000
#define PROC1_VPNO      0x40000
#define PROC2_VADDR     0x80000000
#define PROC2_VPNO      0x80000
#define TEST1_BS	0

void proc1_test1(char *msg, int lck) {
	char *addr;
	int i;

	int ret = get_bs(TEST1_BS, 100);

	//kprintf("Available baking store napges = %d\n", ret);	

	//kprintf("baking store status before xmmap\n");	

	bs_map_t *bs_entry;	

	/*for(i = 0; i < NSTORES; i++) {
		bs_entry = &bsm_tab[i];
		kprintf("bs_status=%d\tbs_pid=%d\tbs_vpno=%d\tbs_npages=%d\tbs_sem=%d\n",
			bs_entry->bs_status, bs_entry->bs_pid, bs_entry->bs_vpno, 
			bs_entry->bs_npages, bs_entry->bs_sem);
	}	
	*/




	if (xmmap(PROC1_VPNO, TEST1_BS, 100) == SYSERR) {
		kprintf("xmmap call failed\n");
		sleep(3);
		return;
	}
/*
	kprintf("xmmap sucess vpno = %d\n", PROC1_VPNO);	
	kprintf("baking store status after xmmap\n");	

	for(i = 0; i < NSTORES; i++) {
		bs_entry = &bsm_tab[i];
		kprintf("bs_status=%d\tbs_pid=%d\tbs_vpno=%d\tbs_npages=%d\tbs_sem=%d\n",
			bs_entry->bs_status, bs_entry->bs_pid, bs_entry->bs_vpno, 
			bs_entry->bs_npages, bs_entry->bs_sem);
	}	
	
*/	
	//kprintf("write begin\n");	

	addr = (char*) PROC1_VADDR;
	for (i = 0; i < 26; i++) {
		//kprintf("write in proc1 i = %d\n", i);
		*(addr + i * NBPG) = 'A' + i;
	}

	//kprintf("unmapping\n");
	xmunmap(PROC1_VPNO);
	//sleep(10);
/*	
	struct pentry *cproc = &proctab[currpid];
        int j;        

        pd_t *ppdir = cproc->pdbr;
        pt_t *pptbl;
        
        kprintf("PDBR for process %d = %lu\n", currpid, ppdir);
        
        for(i = 0; i < 1024; i++) {
                kprintf("pdeno= %d\tptpres = %d\twrite=%d\trefersto=%d\n", 
                        i, ppdir->pd_pres, ppdir->pd_write, ppdir->pd_base);
                if(ppdir->pd_pres == 1) {
                        pptbl = ppdir->pd_base * NBPG;
                        for(j = 0; j < 1024; j++) {
                                kprintf("pteno=%d\tpres=%d\twrite=%d\trefersto=%d\n", 
                                        j, pptbl->pt_pres, pptbl->pt_write, pptbl->pt_base);
                                pptbl++;
                        }
                }
                ppdir++;
                kprintf("\n\n");
        }
        
        fr_map_t *frame;

        kprintf("intial frame status\n\n");
        //print status of all frames 
        for(i = 0 ; i < NFRAMES; i++) {
                frame = &frm_tab[i];
                kprintf("number = %d\tfr_status = %d\t fr_pid = %d\tfr_vpno = %d\t"
                        "fr_refcnt=%d\tfr_type = %d\tfr_dirty=%d\n", i, frame->fr_status, 
                        frame->fr_pid, frame->fr_vpno, frame->fr_refcnt, frame->fr_type, 
                        frame->fr_dirty);
        }

*/
	//kprintf("mapping again\n");
	if (xmmap(PROC1_VPNO, TEST1_BS, 100) == SYSERR) {
		kprintf("xmmap call failed\n");
		sleep(3);
		return;
	}
	
	//kprintf("sleeping\n");
	//sleep(6);

	//kprintf("reading\n");
	for (i = 0; i < 26; i++) {
		kprintf("0x%08x: %c\n", addr + i * NBPG, *(addr + i * NBPG));
	}

	//kprintf("ummapping\n");
	xmunmap(PROC1_VPNO);
	
	//sleep(8);

	/*kprintf("baking store status after xmunmap\n");	

	for(i = 0; i < NSTORES; i++) {
		bs_entry = &bsm_tab[i];
		kprintf("bs_status=%d\tbs_pid=%d\tbs_vpno=%d\tbs_npages=%d\tbs_sem=%d\n",
			bs_entry->bs_status, bs_entry->bs_pid, bs_entry->bs_vpno, 
			bs_entry->bs_npages, bs_entry->bs_sem);
	}*/	
/*
	struct pentry *cproc = &proctab[currpid];
        int j;        

        pd_t *ppdir = cproc->pdbr;
        pt_t *pptbl;
        
        kprintf("PDBR for process %d = %lu\n", currpid, ppdir);
        
        for(i = 0; i < 1024; i++) {
                kprintf("pdeno= %d\tptpres = %d\twrite=%d\trefersto=%d\n", 
                        i, ppdir->pd_pres, ppdir->pd_write, ppdir->pd_base);
                if(ppdir->pd_pres == 1) {
                        pptbl = ppdir->pd_base * NBPG;
                        for(j = 0; j < 1024; j++) {
                                kprintf("pteno=%d\tpres=%d\twrite=%d\trefersto=%d\n", 
                                        j, pptbl->pt_pres, pptbl->pt_write, pptbl->pt_base);
                                pptbl++;
                        }
                }
                ppdir++;
                kprintf("\n\n");
        }
       */ 
        /*fr_map_t *frame;

        kprintf("intial frame status\n\n");
        //print status of all frames 
        for(i = 0 ; i < NFRAMES; i++) {
                frame = &frm_tab[i];
                kprintf("number = %d\tfr_status = %d\t fr_pid = %d\tfr_vpno = %d\t"
                        "fr_refcnt=%d\tfr_type = %d\tfr_dirty=%d\n", i, frame->fr_status, 
                        frame->fr_pid, frame->fr_vpno, frame->fr_refcnt, frame->fr_type, 
                        frame->fr_dirty);
        }
	
	kprintf("unmamapped\n");*/
	return;
}

void proc1_test2(char *msg, int lck) {
	char *x, *y;
	int i;

	//kprintf("baking store status after vcreate\n");	

	bs_map_t *bs_entry;	
/*
	for(i = 0; i < NSTORES; i++) {
		bs_entry = &bsm_tab[i];
		kprintf("bs_status=%d\tbs_pid=%d\tbs_vpno=%d\tbs_npages=%d\tbs_sem=%d\n",
			bs_entry->bs_status, bs_entry->bs_pid, bs_entry->bs_vpno, 
			bs_entry->bs_npages, bs_entry->bs_sem);
	}	
*/
	kprintf("ready to allocate heap space\n");
	x = vgetmem(4096*100);
	//sleep(6);
	kprintf("heap allocated at %x\n", x);

	
	for (i = 0; i < 100; i++) {
		kprintf("write vpno = i = %d\n", i);
		*(x + i * NBPG) = 'T';
		
	}
	kprintf("heap variables:\n");
	for (i = 0; i < 100; i++) {
		kprintf("read vpno = i = %d\n", i);
		kprintf("0x%08x: %c\n", x + i * NBPG, *(x + i * NBPG));
	}

	y = vgetmem(4096*156);
	//sleep(6);
	kprintf("heap allocated at %x\n", y);

	
	for (i = 0; i < 156; i++) {
		kprintf("write vpno = i = %d\n", i);
		*(y + i * NBPG) = 'X';
		
	}
	kprintf("heap variables:\n");
	for (i = 0; i < 156; i++) {
		kprintf("read vpno = i = %d\n", i);
		kprintf("0x%08x: %c\n", y + i * NBPG, *(y + i * NBPG));
	}

	vfreemem(x, 4096*100);
	//sleep(6);
	vfreemem(y, 4096*156);
	//sleep(6);

	y = vgetmem(4096*256);
	kprintf("heap allocated at %x\n", y);
	//sleep(6);
	/*kprintf("heap allocated at %x\n", y);

	
	for (i = 0; i < 256; i++) {
		kprintf("write vpno = i = %d\n", i);
		*(y + 10 + i * NBPG) = 'X';
		
	}*/
	kprintf("heap variables:\n");
	for (i = 0; i < 256; i++) {
		kprintf("read vpno = i = %d\n", i);
		kprintf("0x%08x: %c\n", y + i * NBPG, *(y + i * NBPG));
	}
	vfreemem(y, 4096*256);
	//sleep(6);
}

void proc1_test3(char *msg, int lck) {

	char *addr;
	int i;

	addr = (char*) 0x0;

	for (i = 0; i < 1024; i++) {
		*(addr + i * NBPG) = 'B';
	}

	for (i = 0; i < 1024; i++) {
		kprintf("0x%08x: %c\n", addr + i * NBPG, *(addr + i * NBPG));
	}

	return;
}

int main() {
//	srpolicy(SC);
	
	kprintf("main creation good and main has started\n");
	
	int pid1;
	int pid2;

	/*kprintf("\n1: shared memory\n");
	pid1 = create(proc1_test1, 2000, 30, "proc1_test1", 0, NULL);
	kprintf("process %d created\n", pid1);
	resume(pid1);
	sleep(10);*/
/*
        fr_map_t *frame;

	int i;
        kprintf("intial frame status\n\n");
        //print status of all frames 
        for(i = 0 ; i < NFRAMES; i++) {
                frame = &frm_tab[i];
                kprintf("number = %d\tfr_status = %d\t fr_pid = %d\tfr_vpno = %d\t"
                        "fr_refcnt=%d\tfr_type = %d\tfr_dirty=%d\n", i, frame->fr_status, 
                        frame->fr_pid, frame->fr_vpno, frame->fr_refcnt, frame->fr_type, 
                        frame->fr_dirty);
        }

	kprintf("baking store status before vcreate\n");	

	bs_map_t *bs_entry;	

	for(i = 0; i < NSTORES; i++) {
		bs_entry = &bsm_tab[i];
		kprintf("bs_status=%d\tbs_pid=%d\tbs_vpno=%d\tbs_npages=%d\tbs_sem=%d\n",
			bs_entry->bs_status, bs_entry->bs_pid, bs_entry->bs_vpno, 
			bs_entry->bs_npages, bs_entry->bs_sem);
	}	

        kprintf("intial frame status before vcreate\n\n");
        //print status of all frames 
        for(i = 0 ; i < NFRAMES; i++) {
                frame = &frm_tab[i];
                kprintf("number = %d\tfr_status = %d\t fr_pid = %d\tfr_vpno = %d\t"
                        "fr_refcnt=%d\tfr_type = %d\tfr_dirty=%d\n", i, frame->fr_status, 
                        frame->fr_pid, frame->fr_vpno, frame->fr_refcnt, frame->fr_type, 
                        frame->fr_dirty);
        }
*/

	kprintf("\n2: vgetmem/vfreemem\n");
	pid1 = vcreate(proc1_test2, 2000, 256, 20, "proc1_test2", 0, NULL);
	kprintf("pid %d has private heap\n", pid1);
	resume(pid1);
	sleep(5);

/*
	kprintf("baking store status after proc2 completion\n");	


	for(i = 0; i < NSTORES; i++) {
		bs_entry = &bsm_tab[i];
		kprintf("bs_status=%d\tbs_pid=%d\tbs_vpno=%d\tbs_npages=%d\tbs_sem=%d\n",
			bs_entry->bs_status, bs_entry->bs_pid, bs_entry->bs_vpno, 
			bs_entry->bs_npages, bs_entry->bs_sem);
	}	

        kprintf("intial frame status after vcreate before task 3\n\n");
        //print status of all frames 
        for(i = 0 ; i < NFRAMES; i++) {
                frame = &frm_tab[i];
                kprintf("number = %d\tfr_status = %d\t fr_pid = %d\tfr_vpno = %d\t"
                        "fr_refcnt=%d\tfr_type = %d\tfr_dirty=%d\n", i, frame->fr_status, 
                        frame->fr_pid, frame->fr_vpno, frame->fr_refcnt, frame->fr_type, 
                        frame->fr_dirty);
        }*/
	/*kprintf("\n3: Frame test\n");
	pid1 = create(proc1_test3, 2000, 20, "proc1_test3", 0, NULL);
	resume(pid1);
	sleep(10);*/
}
#else
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>


#define PROC1_VADDR	0x40000000
#define PROC1_VPNO      0x40000
#define PROC2_VADDR     0x80000000
#define PROC2_VPNO      0x80000
#define TEST1_BS	1

void proc1_test1(char *msg, int lck) {
	char *addr;
	int i;

	get_bs(TEST1_BS, 100);

	if (xmmap(PROC1_VPNO, TEST1_BS, 100) == SYSERR) {
		kprintf("xmmap call failed\n");
		sleep(3);
		return;
	}

	addr = (char*) PROC1_VADDR;
	for (i = 0; i < 26; i++) {
		*(addr + i * NBPG) = 'A' + i;
	}

	sleep(6);

	for (i = 0; i < 26; i++) {
		kprintf("0x%08x: %c\n", addr + i * NBPG, *(addr + i * NBPG));
	}

	xmunmap(PROC1_VPNO);
	return;
}

void proc1_test2(char *msg, int lck) {
	int *x;

	kprintf("ready to allocate heap space\n");
	x = vgetmem(1024);
	kprintf("heap allocated at %x\n", x);
	*x = 100;
	*(x + 1) = 200;

	kprintf("heap variable: %d %d\n", *x, *(x + 1));
	vfreemem(x, 1024);
}

void proc1_test3(char *msg, int lck) {

	char *addr;
	int i;

	addr = (char*) 0x0;

	for (i = 0; i < 1024; i++) {
		*(addr + i * NBPG) = 'B';
	}

	for (i = 0; i < 1024; i++) {
		kprintf("0x%08x: %c\n", addr + i * NBPG, *(addr + i * NBPG));
	}

	return;
}

int main() {
	int pid1;
	int pid2;

	kprintf("\n1: shared memory\n");
	pid1 = create(proc1_test1, 2000, 20, "proc1_test1", 0, NULL);
	resume(pid1);
	sleep(10);

	kprintf("\n2: vgetmem/vfreemem\n");
	pid1 = vcreate(proc1_test2, 2000, 100, 20, "proc1_test2", 0, NULL);
	kprintf("pid %d has private heap\n", pid1);
	resume(pid1);
	sleep(3);

	kprintf("\n3: Frame test\n");
	pid1 = create(proc1_test3, 2000, 20, "proc1_test3", 0, NULL);
	resume(pid1);
	sleep(3);
}

#endif
