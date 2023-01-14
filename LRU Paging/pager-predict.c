/*
 * File:  pager-lru.c
 *
 * Author:  Ben Adams
 *
 * Project:  CSPB Operating Systems
 * 
 * Description: This file contains a predictive pageit implmentation.
 */

#include <stdio.h> 
#include <stdlib.h>

#include "simulator.h"
#include "pager-predict.h"


void pageit(Pentry q[MAXPROCESSES]) { 
    
    /* This file contains the stub for a predictive pager */
    /* You may need to add/remove/modify any part of this file */

    /* Static vars */
    static int initialized = 0;
    static int who_to_pagein[MAXPROCESSES][MAXPROCPAGES];
    
    /* Local vars */
    int proctmp;
    int pagetmp;
    
    /* initialize static vars on first run */
    if(!initialized){
	/* Init complex static vars here */
        for(int i = 0; i < MAXPROCESSES; i++) {
            for (int j = 0; j < MAXPROCPAGES; j++) {
                who_to_pagein[i][j] = 0;
            }
        }
	    initialized = 1;
    }
    
    /* TODO: Implement Predictive Paging */

            // Which page is being requested?
            pagetmp = q[proctmp].pc / PAGESIZE; // get requested page

            // Which pages are likely to be needed next?

            // Given the current page, which page in the page table is 
            // ... least likely to be needed next?

            // Pageout and pagein based on the above
}

