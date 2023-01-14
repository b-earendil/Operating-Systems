/*
 * File:  pager-lru.c
 *
 * Author:  Ben Adams
 *
 * Project:  CSPB Operating Systems
 * 
 * Description: This file contains an lru pageit implementation.
 */

#include <stdio.h> 
#include <stdlib.h>

#include "simulator.h"


int prev_page[MAXPROCESSES];

static int lru(Pentry q, int timestamps[], int curr_tick) {
    int victim;
    int least_tick = curr_tick;
    for(int pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++) {
        // if the page is in the page table 
        // and the timestamp for the page is smaller than least_tick
        // update least_tick and victim page accordingly
        if(q.pages[pagetmp] && timestamps[pagetmp] < least_tick) {
            least_tick = timestamps[pagetmp];
            victim = pagetmp;
        }
    }
    return victim; // return the victim page to be replaced
}

void pageit(Pentry q[MAXPROCESSES]) { 
    
    /* Static vars */
    static int initialized = 0;
    static int tick = 1; // artificial time

    /* Local vars */
    int proctmp;
    int pagetmp;
    int victim;

    /* initialize static vars on first run */
    if(!initialized){
	for(proctmp=0; proctmp < MAXPROCESSES; proctmp++){
	    for(pagetmp=0; pagetmp < MAXPROCPAGES; pagetmp++){
		    timestamps[proctmp][pagetmp] = 0; 
	    }
	}
	    initialized = 1;
    }
    
    /* TODO: Implement LRU Paging */
    // for each active process, calculate the requested page and load it into memory
    for(proctmp = 0; proctmp < MAXPROCESSES; proctmp++) {

        if(q[proctmp].active) {     // if process is active
            pagetmp = (q[proctmp].pc / PAGESIZE);    // calculate the requested page
            timestamps[proctmp][pagetmp] = tick;    // set timestamp 

            if(pagetmp != prev_page[proctmp]) { // if there is a transition
                transitions[prev_page[proctmp]][pagetmp] += 1; // count transition from prev to req page
                prev_page[proctmp] = pagetmp; // set prev_page to be equal to requested_page
            }

            // if process not in page table, page it in 
            // if page in fails, use least recently used replacement algorithm
            if(!q[proctmp].pages[pagetmp] && !pagein(proctmp, pagetmp)) {
                // least recently used replacement algorithm
                victim = lru(q[proctmp], timestamps[proctmp], tick);
                // out with the old
                pageout(proctmp, victim);
                // in with the new
                pagein(proctmp, pagetmp);
            }
        }
    }

    /* advance time for next pageit iteration */
    tick++;
} 
