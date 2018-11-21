/* This function returns the demanded pages in backing store bs_id
 * if the backing store bs_id is not mapped to any other process 
 * otherwise it returns the size of the existing backing store
 * irrespective if it is mapped to current process or not
 */


#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int get_bs(bsd_t bs_id, unsigned int npages) {
	STATWORD ps;
	
	disable(ps);	

  	/* requests a new mapping of npages with ID map_id */

	bs_map_t *bs_entry = &bsm_tab[bs_id];
	
	//return error if invalid npages
	if(npages <= 0 || npages > 256) {
		restore(ps);
		return SYSERR;
	}
	//if the backing store already exists return 
	//size of existing backing store
	else if(bs_entry->bs_status == BSM_MAPPED){
		restore(ps);
		return bs_entry->bs_npages;
	
	}
	//else return npages
	else {
		restore(ps);
		return npages;
	}

}


