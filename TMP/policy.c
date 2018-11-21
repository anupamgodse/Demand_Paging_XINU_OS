/* policy.c = srpolicy*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <q.h>


extern int page_replace_policy;
struct qent sc_queue[NFRAMES+2];
int sc_qtail = NFRAMES+1;
int sc_qhead = NFRAMES;


//enqueue to sr queue
int sc_enqueue(int item, int tail)
/*      int     item;                   - item to enqueue on a list     */
/*      int     tail;                   - index in q of list tail       */
{
        struct  qent    *tptr;          /* points to tail entry         */
        struct  qent    *mptr;          /* points to item entry         */

        tptr = &sc_queue[tail];
        mptr = &sc_queue[item];
        mptr->qnext = tail;
        mptr->qprev = tptr->qprev;
        sc_queue[tptr->qprev].qnext = item;
        tptr->qprev = item;
        return(item);
}


/*------------------------------------------------------------------------
 *  *  dequeue  --  remove an item from the head of a list and return it
 *   *------------------------------------------------------------------------
 *    */

//dequeue from sr_queue
int sc_dequeue(int item)
{
        struct  qent    *mptr;          /* pointer to q entry for item  */

        mptr = &sc_queue[item];
        sc_queue[mptr->qprev].qnext = mptr->qnext;
        sc_queue[mptr->qnext].qprev = mptr->qprev;
        return(item);
}





/*-------------------------------------------------------------------------
 * srpolicy - set page replace policy 
 *-------------------------------------------------------------------------
 */
SYSCALL srpolicy(int policy)
{
  	/* sanity check ! */
	if(policy == SC) {
		//init sr_queue
		struct  qent    *hptr;
		struct  qent    *tptr;
		int     hindex, tindex;

		hptr = &sc_queue[ hindex=NFRAMES]; /* assign and rememeber queue   */
		tptr = &sc_queue[ tindex=NFRAMES+1]; /* index values for head&tail   */
		hptr->qnext = tindex;
		hptr->qprev = EMPTY;
		hptr->qkey  = MININT;
		tptr->qnext = EMPTY;
		tptr->qprev = hindex;
		tptr->qkey  = MAXINT;
	}


  	if(policy == SC || policy == AGING) {
		page_replace_policy = policy;
  		return OK;
	}
	else {
		return SYSERR;
	}
}

/*-------------------------------------------------------------------------
 * grpolicy - get page replace policy 
 *-------------------------------------------------------------------------
 */
SYSCALL grpolicy()
{
  return page_replace_policy;
}
