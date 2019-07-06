

void parseRecvdPhaseCharges(TXsys *TX){
    
}

void enterControlLoop(TX_OutputControlProgram_t *ocp, uint32_t cmd){
    uint32_t child;
    if ( ocp->current < ocp->end ){
        child = ocp->currentChild;
        ocp->currentAction++;
        ocp->currentChild++;
        ocp = *(ocp->children)[child];
    } else {
        ocp = ocp->parent;
    }
}

void updateFireCmdBuffer(TX_OutputControlProgram_t *ocp, uint32_t cmd){
    if ( DREF32(ocp->interrupt0) ){

    } else {

    }
}

void updateFireAtCmdBuffer(TX_OutputControlProgram_t *ocp, uint32_t cmd){
    if ( DREF32(ocp->interrupt1) ){

    }
}

void actionsHandler_tx(TX_OutputControlProgram_t *ocp){


}

void populateActionLists_tx(TXsys *TX){
    uint32_t *instr;
    instr = (uint32_t *)(*(TX->instructions));
    int i,j;

    uint8_t progNum;
    TX_OutputControlProgram_t **ocpPtr;
    TX_OutputControlProgram_t *ocp;
    TX_OutputControlProgram_t *child;
    TX_OutputControlProgram_t *parent;
    TX_Action_f *actionList;
    
    ocpPtr = *(TX->progs);

    progNum = 0;
    while( ocpPtr[ progNum ]->is.active ){
        if( ( *(ocpPtr[ progNum ]->actionList) ) != NULL ){
            free( *(ocpPtr[ progNum ]->actionList) );
        }
        *(ocpPtr[ progNum ]->actionList) = (TX_Action_f *)malloc((ocpPtr[ progNum ]->nActions)*sizeof(TX_Action_f));
        progNum++;
    }
       
    // the first action will be taken by the topLevel program
    actionList = *(ocpPtr[0]->actionList);

    ocp = ocpPtr[0];
    // moving via pointers to populate subprogram action lists is pretty nifty
    // no overlapping loops allowed :(
    for( i = 0 ; TX->nInstructions ; i+=2 ) {
        
        if ( ( instr[i] & FPGA_LOOP_START ) && !( instr[i] & ARM_FIRELESS_LOOP ) ){
            actionList[ ocp->currentAction ] = &enterControlLoop;
            ocp->currentAction++;
            parent = ocp;
            child = *(ocp->children);
            ocp = child[ parent->currentChild ];
            actionList = *(ocp->actionList);
            i+=3;

        } else if ( instr[i] & FPGA_FIRE_CMD ){
            actionList[ ocp->currentAction ] = &updateFireCmdBuffer;
            ocp->currentAction++;
            
        } else if ( instr[i] & FPGA_FIREAT_CMD ){
            actionList[ ocp->currentAction ] = &updateFireAtCmdBuffer;
            ocp->currentAction++;
        
        } else if ( ( instr[i] & FPGA_LOOP_END_POINT ) && ( ( instr[i+1] & 0xf ) == ocp->progNum.given ) ) {
            actionList[ ocp->currentAction ] = &enterControlLoop;
            ocp->currentAction++;
            ocp = parent;
            ocp->currentChild++;
            actionList = *(ocp->actionList);

        } else if ( instr[i] & ARM_SETUP_ITERATOR ){
            i+=3;
        }

    }

    while( ocpPtr[progNum]->is.active ){
        ocpPtr[progNum]->currentAction = 0;
        ocpPtr[progNum]->currentChild = 0;
    }
}


void defineSubPrograms_tx(TXsys *TX){

    // type that 
    TX_instructionTypeReg_t *instr;
    intsr = (TX_instructionTypeReg_t *)(*(TX->instructions));
    
    int i,j;

    uint16_t masterProg, curProg;
    free(*(TX->progDefs));
    *(TX->progDefs) = (TX_OutputControlProgram_t *)malloc((MAX_LOOPS+1)*sizeof(TX_OutputControlProgram_t));
    free(*(*(TX->progs)));
    *(TX->progs) = (TX_OutputControlProgram_t **)malloc((MAX_LOOPS+1)*sizeof(TX_OutputControlProgram_t *));

    // bitField to keep track of whether or not current instr exists within a loop 
    uint32_t progTracker;
    int32_t incr;
    int fireAt_detected;
    int nFireAts = 0;
    int nFireAts_Inner = 0;
    TX->nFireAts = 0;

    // user might define loops out of order: start_loop(3), start_loop(2), start_loop(4)
    // logical order of the given loop nums is:  [logical](given): [0](3), [1](2), [2](4)
    uint8_t logicalProgNum;
    TX_OutputControlProgram_t *ocpTmp;
    TX_OutputControlProgram_t **ocpPtr;
    TX_OutputControlProgram_t *ocpChildren[MAX_LOOPS];
    ocpTmp = *(TX->progDefs);
    ocpPtr = *(TX->progs);

    for( i=0 ; i < (MAX_LOOPS+1) ; i++ ){
        ocpTmp[i].progNum.logical = 0;
        ocpTmp[i].progNum.given = 0;
        ocpTmp[i].ocpFlags = 0;
        ocpTmp[i].start.loc = 0;
        ocpTmp[i].end.loc = 0;
        ocpTmp[i].current.loc = 0;
        ocpTmp[i].increment = 0;
        ocpTmp[i].fireAt.first = 0;
        ocpTmp[i].fireAt.last = 0;
        ocpTmp[i].fireAt.current = &(TX->currentFireAt);
        ocpTmp[i].parent = NULL;
        ocpTmp[i].children = NULL;
        ocpTmp[i].nChildren = 0;
        ocpTmp[i].currentChild = 0;
        ocpTmp[i].nActions = 0;
        ocpTmp[i].currentAction = 0;
        ocpTmp[i].actionList = NULL;
        ocpTmp[i].fireReg = &(TX->fireReg);
        ocpTmp[i].fireAtReg = &(TX->fireAtReg);
        ocpTmp[i].interrupt0 = TX->interrupt0;
        ocpTmp[i].interrupt1 = TX->interrupt1;
    }
    ocpTmp[0].is.topLevel = 1;
    ocpTmp[0].is.active = 1;

    // populate output control program loop definitions
    for( i=0 ; i < (TX->nInstructions) ; i+=2 ) {
        if ( instr[i].instr & ( ARM_SETUP_ITERATOR | FPGA_LOOP_START ) ){
            curProg = instr[i+1].instr & 0xf;
            masterProg = curProg+1;
            if ( !ocpTmp[masterProg].is.iterator ){
                ocpTmp[masterProg].progNum.given = curProg;
                ocpTmp[masterProg].start.loc = instr[i+2].instr;
                ocpTmp[masterProg].end.loc = instr[i+3].instr;
                ocpTmp[masterProg].is.reversed = ( ocpTmp[masterProg].start.loc > ocpTmp[masterProg].end.loc );
                incr = ( abs((int32_t)instr[i+4].instr) ) ? abs((int32_t)instr[i+4].instr) : 1;
                ocpTmp[masterProg].increment = ( ocpTmp[masterProg].is.reversed ) ? -incr : incr ;
                ocpTmp[masterProg].is.iterator = ( instr[i].instr & ARM_SETUP_ITERATOR ) ? 1 : 0 ;
            }
            i+=5;
        }

        if ( instr[i].instr & FPGA_FIREAT_CMD ){
            TX->nFireAts++;
        }
    }
  
    // parse the topLevel program to find actionable instructions
    ocpPtr[0] = &ocpTmp[0];     // ocpXxx[0] = topLevel
    progTracker = 0;            // keeps track of whether currently in loop
    fireAt_detected = 0;
    for( i = 0 ; i < (TX->nInstructions) ; i+=2 ) {
        if ( instr[i].instr & FPGA_LOOP_START ){
            if( !( instr[i].instr & ARM_FIRELESS_LOOP ) ){
                ocpTmp[0].nActions++;
                ocpChildren[ ocpTmp[0].nChildren ] = &ocpTmp[ ( instr[i+1].instr & 0xf ) + 1 ];
                ocpTmp[ ( instr[i+1].instr & 0xf ) + 1 ].parent = &ocpTmp[0];
                ocpTmp[0].nChildren++;
                progTracker |= ( 1 << ( instr[i+1].instr & 0xf ) );
                j=i+5;

                while( ( ( instr[j].fpga ^ FPGA_LOOP_END_POINT ) | progTracker ) && ( j < TX->nInstructions ) ){
                    if ( instr[j].instr & FPGA_LOOP_START ){
                        progTracker |= ( 1 << ( instr[j+1].instr & 0xf ) );
                        j+=5;
                    } else if ( ( instr[j].instr & FPGA_LOOP_END_POINT ) && ( ( 1 << (instr[j+1].instr & 0xf) ) & progTracker ) ) {
                        progTracker &= ~( 1 << ( instr[j+1].instr & 0xf ) );
                    } else if ( instr[j].instr & FPGA_FIREAT_CMD ){
                        nFireAts++;
                        j+=2;
                    } else if ( instr[j].instr & ARM_SETUP_ITERATOR ){
                        j+=5;
                    } else {
                        j+=2;
                    }
                }

                i=j;
            } else {
                i+=3;
            }
        } else if ( instr[i].instr & ( FPGA_FIRE_CMD | FPGA_FIREAT_CMD ) ) {
            ocpTmp[0].nActions++;
            if ( instr[i].instr & FPGA_FIREAT_CMD ){
                if ( !fireAt_detected ) {
                    ocpTmp[0].fireAt.first = nFireAts;
                    fireAt_detected = 1;
                } else {
                    ocpTmp[0].fireAt.last = nFireAts;
                }
                nFireAts++;
                ocpTmp[0].has.fireAtCmd=1;
            } else {
                ocpTmp[0].has.fireCmd=1;
            }
        } else if ( instr[i].instr & ARM_SETUP_ITERATOR ){
            i+=3
        }
    }
    // one extra action is need for the topLevel program as a placeholder for returning to parent subprogram
    ocpTmp[0].nActions++;
    if( ocpTmp[0].nChildren ){
        if( ocpTmp[0].children != NULL ) free( ocpTmp[0].children );
        ocpTmp[0].children = (TX_OutputControlProgram_t **)malloc(ocpTmp[0].nChildren*sizeof( TX_OutputControlProgram_t *) );
        for( i = 0 ; i < ocpTmp[0].nChildren ; i++ ){
            ocpTmp[0].children[i] = ocpChildren[i];
        }
    }

    // parse the loops defined by the user for actionable items
    logicalProgNumber = 1;
    nFireAts = 0;
    for( i = 0 ; i < (TX->nInstructions) ; i+=2 ) {
        if ( instr[i].instr & FPGA_LOOP_START ) {
            if ( !( instr[i].instr & ARM_FIRELESS_LOOP ) ){
                curProg = instr[i+1].instr & 0xf;
                masterProg = curProg + 1;
                ocpPtr[logicalProgNumber] = &ocpTmp[masterProg];
                ocpTmp[masterProg].progNum.logical = logicalProgNumber;
                ocpTmp[masterProg].is.active = 1;
                
                j=i+5;
                progTracker = 0;
                fireAt_detected = 0;
                nFireAts_Inner = nFireAts;
                while( ( ( instr[j].fpga ^ FPGA_LOOP_END_POINT ) | ( ( instr[j+1].instr & 0xf ) ^ curProg ) ) && ( j < TX->nInstructions ) ){
                    
                    if ( instr[j].instr & FPGA_LOOP_START ){
                        if ( !( instr[j].instr & ARM_FIRELESS_LOOP ) ){
                            ocpTmp[masterProg].nActions++;
                            ocpChildren[ ocpTmp[masterProg].nChildren ] = &ocpTmp[ ( instr[i+1].instr & 0xf ) + 1 ];
                            ocpTmp[masterProg].nChildren++;
                            ocpTmp[ ( instr[j+1].instr & 0xf ) + 1 ].parent = &ocpTmp[masterProg];
                            progTracker |= ( 1 << ( instr[j+1].instr & 0xf ) );
                            j+=5;

                            // all actions within sub-loops get executed by the sub-loops, this basically just skips them
                            while( ( ( instr[j].fpga ^ FPGA_LOOP_END_POINT ) | progTracker ) && ( j < TX->nInstructions ) ){
                                if ( instr[j].instr & FPGA_LOOP_START ){
                                    if( !( instr[j].instr & ARM_FIRELESS_LOOP ) ){
                                        progTracker |= ( 1 << ( instr[j+1].instr & 0xf ) );
                                    }
                                    j+=5;
                                } else if ( ( instr[j].instr & FPGA_LOOP_END_POINT ) && ( ( instr[j+1].instr & 0xf ) & curProg ) ){
                                    break;
                                } else if ( ( instr[j].instr & FPGA_LOOP_END_POINT ) && ( ( 1 << (instr[j+1].instr & 0xf) ) & progTracker ) ) {
                                    progTracker &= ~( 1 << (instr[j+1].instr & 0xf) );
                                } else if ( instr[j].instr & FPGA_FIREAT_CMD ){
                                    nFireAts_Inner++;
                                    j+=2;
                                } else if ( instr[j].instr & ARM_SETUP_ITERATOR ){
                                    j+=5;
                                } else {
                                    j+=2;
                                }
                            }
                        } else {
                            j+=5;
                        }
                    } else if ( instr[j].instr & ( FPGA_FIRE_CMD | FPGA_FIREAT_CMD ) ){
                        ocpTmp[masterProg].nActions++;
                        if ( instr[j].instr & FPGA_FIREAT_CMD ){
                            if ( !fireAt_detected ){
                                ocpTmp[masterProg].fireAt.first = nFireAts_Inner;
                            } else {
                                ocpTmp[masterProg].fireAt.last = nFireAts_Inner;
                            }
                            ocpTmp[masterProg].has.fireAtCmd = 1;
                            fireAt_detected++;
                            nFireAts_Inner++;
                        } else {
                            ocpTmp[masterProg].has.fireCmd = 1;
                        }
                        j+=2;
                    } else if ( instr[j].instr & ARM_SETUP_ITERATOR ){
                        j+=5;
                    } else {
                        j+=2;
                    }
                }

                if( fireAt_detected ){
                    ocpTmp[masterProg].at.exit_UpdateFireAt = ( nFireAts_Inner < TX->nFireAts) ? 1 : 0;
                    ocpTmp[masterProg].at.loopEnd_ReloadFireAt = ( fireAt_detected > 1 ) ? 1 : 0;
                }

                if( ocpTmp[masterProg].nChildren ){
                    if( ocpTmp[masterProg].children != NULL ) free( ocpTmp[masterProg].children );
                    ocpTmp[masterProg].children = (TX_OutputControlProgram_t **)malloc(ocpTmp[masterProg].nChildren*sizeof( TX_OutputControlProgram_t *) );
                    for( j = 0 ; j < ocpTmp[masterProg].nChildren ; j++ ){
                        ocpTmp[masterProg].children[j] = ocpChildren[j];
                    }
                }
                
                // the last action of each subprogram needs to be to return to parent
                ocpTmp[masterProg].nActions++;
                logicalProgNumber++;
            }
            i+=3;
        } else if( instr[i].instr & ARM_SETUP_ITERATOR ){
            i+=3;
        } else if ( instr[i].instr & FPGA_FIREAT_CMD ){
            nFireAts+=1;
        }
    }

    /* at this point the loops know:
        how many iterations they run for
        whether they contain any fire/fireAt commands
            how many fireAt commands
            the indices of the first and last fireAt commands used by each subprogram in the list of fireAt commands
        whether they iterate through phaseCharge buffer/locations
            where they start
            where they end
            how much to increment by
        who their parent is
        who their children are
    */
    TX->populateActionLists(TX);
}


void minimizeRedundantInterrupts_tx(TXsys *TX){
    /* fire/fireAt will generate iterrupts every time they're called
        if these commands appear in a loop, they'll generate interrupts every time
        if the fire/fireAt location doesn't change within the loop though, the
        interrupts get generated even though they don't need to do anything.
        this would tax the hell out of the 'poll' function  especially if iterrupts 
        are being generated at rates > 1kHz, which might be the case for deletion
        pulses, or if trying some (probably) over zealous treatment rates.

        this function goes through the list of instructions and takes this:
        
            start_loop(1,100)
            fireAt(x) // triggers interrupt
            wait()
            end_loop(1)

        and makes it this:

            start_loop(1,99)
            fireSingle(x) // doesn't trigger interrupt
            wait()
            end_loop(1)
            fireAt(x)
            wait()

        this should be operationally equivalent, but won't trigger unecessary
        interrupts and so won't tax the hell out of the 'poll' function
    */
    
    int i,j,k;
    uint32_t curProg;
    uint32_t progTracker;
    TX_instructionTypeReg_t *instr;
    uint32_t nLines;
    uint32_t isUnrollable;
    uint32_t nFireAts;
    uint32_t nFires;
    uint32_t nFiresRef;
    uint32_t fireAtLoc;
    uint32_t gapFlag;
    uint32_t isReversed;
    uint32_t isIterator;
    instr = (TX_instructionTypeReg_t *)(*(TX->instructionBuff));

    typedef struct segment_{
        uint32_t start;
        uint32_t end;
        uint32_t newLines;
        uint32_t *instr;
        struct segment_ *next;
        struct segment_ *prev;
    } segment_t;

    typedef struct iterator_{
        uint32_t loopNum;
        uint32_t start;
        uint32_t end;
        int32_t incr;
        int32_t isReversed;
        struct iterator_ *next;
        struct iterator_ *prev;
    } iterator_t;
    
    // go through all fireAt's in program, convert repeat fireAts to fireAtSingle
    for( i = 0; i < (TX->nInstructionsBuff); i+=2 ){
        if( instr[i].instr & ( ARM_SETUP_ITERATOR | FPGA_LOOP_START ) ){
            i+=3;
        } else if ( instr[i].instr & FPGA_FIREAT_CMD ){
            fireAtLoc = instr[i+1].instr;
            for ( j = i+2; j<(TX->nInstructions); j+=2 ){
                if( instr[j].instr & ( ARM_SETUP_ITERATOR | FPGA_LOOP_START ) ){
                    j+=3;
                } else if ( instr[j].instr & FPGA_FIREAT_CMD ){
                    if( instr[j+1].instr == fireAtLoc ){
                        instr[i].instr = ( FPGA_FIREAT_CMD_SINGLE | ARM_FIRE_UNROLLED );
                    } else {
                        i=j;
                        break;
                    }
                } 
            }
        }
    }


    segment_t *seg;
    seg = (segment_t *)malloc(sizeof(segment_t));
    seg->start=0;
    seg->end=0;
    seg->newLines=0;
    seg->prev = NULL;
    seg->next = NULL;

    iterator_t *iter;
    iter = NULL;

    for( i=0 ; i < (TX->nInstructionsBuff) ; i+=2 ) {
        if ( instr[i].instr & ARM_SETUP_ITERATOR ){
            if(iter == NULL){
                iter = (iterator_t *)malloc(sizeof(iterator_t));
                iter->prev=NULL;
            }
            iter->loopNum = instr[i+1].instr & 0xf;
            iter->start = instr[i+2].instr;
            iter->end = instr[i+3].instr;
            iter->isReversed = ( (iter->start) > (iter->end) ) ? 1 : 0;
            iter->incr = (uint32_t)abs((int32_t )instr[i+4].instr);
            iter->next = (iterator_t *)malloc(sizeof(iterator_t));
            iter->next->prev = iter;
            iter = iter->next;
            iter->next = NULL;
            i+=3;
        } else if ( instr[i].instr & FPGA_LOOP_START ) {
            i+=3;
        }
    }

    for( i=0 ; i < (TX->nInstructionsBuff) ; i+=2 ) {
        progTracker = 0;
        nLines = 0;
        nFireAts = 0;
        fireAtLoc = 0;
        isUnrollable = 1;
        isReversed = 0;
        isIterator = 0;
        if ( instr[i].instr & FPGA_LOOP_START ){
            curProg = instr[i+1].instr & 0xf;
          
            // check if loop was defined as an iterator
            if( iter != NULL ){
                while( iter->prev != NULL ) iter = iter->prev;
                
                while( iter->next != NULL ){
                    if ( iter->loopNum == curProg ) {
                        break;
                    }
                    iter = iter->next;
                }
            }

            // check if loop will execute more than once
            if( ( iter != NULL ) && ( iter->loopNum == curProg ) ){
                isIterator = 1;
                isReversed = iter->isReversed;
                if ( !isReversed && ( ( (( iter->end ) - ( iter->start )) / ( iter->incr ) ) < 2 ) ){
                    isUnrollable = 0;
                } else if ( isReversed && ( ( (( iter->start ) - ( iter->end )) / ( iter->incr ) ) < 2 ) ){
                    isUnrollable = 0;
                }
            } else {
                isReversed = ( instr[i+2].instr > instr[i+3].instr ) ? 1 : 0;
                if ( !isReversed && ( ( (( instr[i+3].instr - instr[i+2].instr )) / (uint32_t)abs( (int32_t)instr[i+4].instr ) ) < 2 ) ){
                    isUnrollable = 0;
                } else if ( isReversed && ( ( (( instr[i+2].instr - instr[i+3].instr )) / (uint32_t)abs( (int32_t)instr[i+4].instr ) ) < 2 ) ) {
                    isUnrollable = 0;
                }
            }
            
            progTracker |= ( 1 << ( instr[i+1].instr & 0xf ) );
            j=i+5;
            i+=3;

            /* check if loop fails other unrolling criteria
                - can't unroll a loop if there's a nested loop within it
                - can't unroll a loop if it contains multiple fireAt(x)'s and (x) changes 
            */
            while( ( ( instr[j].fpga ^ FPGA_LOOP_END_POINT ) | progTracker ) && ( j < TX->nInstructionsBuff ) && isUnrollable ){
                if ( instr[j].instr & FPGA_LOOP_START ){
                    isUnrollable = 0;
                    break;
                } else if ( instr[j].fpga & FPGA_LOOP_END_POINT ) {
                    if ( ( ( 1 << (instr[j+1].instr & 0xf) ) ^ progTracker ) || ( !nFireAts && !nFires ) ) {
                        isUnrollable = 0;
                    }
                    j+=2;
                    break;
                } else if ( instr[j].instr & FPGA_FIREAT_CMD ){
                    if( !nFireAts ){
                        fireAtLoc = instr[j+1].instr;
                    } else {
                        if( instr[j+1].instr != fireAtLoc ){
                            isUnrollable = 0;
                            break;
                        }
                    }
                    nFireAts++;
                } else if ( instr[j].instr & FPGA_FIRE_CMD ) {
                    nFires++;
                }
                
                if ( !( instr[j].instr & ARM_SETUP_ITERATOR ) ){
                    j+=2;
                    nLines+=2;
                } else {
                    j+=5;
                }
            }
        } else {
            isUnrollable = 0;
        }
        
        gapFlag = 0;
        if ( isUnrollable ){
            if ( ( seg->prev == NULL ) && ( i > 0 ) ) {
                seg->start = 0;
                seg->end = i;
                seg->newLines = 0;
                gapFlag = 1;
            } else if ( ( seg->prev != NULL ) && ( i - seg->prev->end ) ){
                seg->start = seg->prev->end;
                seg->end = i;
                seg->newLines = 0;
                gapFlag = 1;
            }

            if( gapFlag ){
                seg->instr = (uint32_t *)calloc( (seg->end - seg->start), sizeof(uint32_t) );
                memcpy(seg->instr, &instr[ seg->start ], (seg->end - seg->start)*sizeof(uint32_t) );
                seg->next = (segment_t *)malloc( sizeof(segment_t));
                seg->next->prev = seg;
                seg = seg->next;
                seg->next = NULL;
            }

            nFiresRef = nFires;
            seg->start = i;
            seg->end = j;
            seg->newLines = nLines;
            seg->instr = (uint32_t *)calloc( ( seg->end - seg->start + seg->newLines ) , sizeof(uint32_t));
            
            // copy original instructions into the segment's instruction array
            memcpy(seg->instr, &instr[ seg->start ], (seg->end - seg->start)*sizeof(uint32_t) );
            
            // go through the instruction array and modify the fire/fireAt commands as appropriate
            for( k = seg->start; k < seg->end; k+=2){
                if ( instr[k].instr & ( FPGA_LOOP_START | ARM_SETUP_ITERATOR ) ){
                    k+=3;
                } else if ( instr[k].instr & FPGA_FIREAT_CMD ){
                    seg->instr[k] = ( FPGA_FIREAT_CMD_SINGLE | ARM_FIRE_UNROLLED );
                } else if ( ( instr[k].instr & FPGA_FIRE_CMD ) ){
                    if ( !isIterator || ( isIterator && ( nFires > 1 ) ) ){
                        seg->instr[k] =  ( FPGA_FIRE_CMD_SINGLE | ARM_FIRE_UNROLLED );
                    } else {
                        seg->instr[k] = instr[k].instr;
                    }
                    nFires--;
                }
            }

            // copy the innards of the unrolled loop (sans iterator definitions) into the segment's instruction array 
            for( k = 0; k < ( seg->newLines ); k+=2){
                if ( instr[ ( seg->start + 5 ) + k ].instr & ARM_SETUP_ITERATOR ){
                    k+=3;
                } else {
                    if ( ( nFiresRef > 1 ) && ( instr[ ( seg->start + 5 ) + k ].instr & FPGA_FIRE_CMD ) ){
                        seg->instr[ k + seg->end ] = ( FPGA_FIRE_CMD_SINGLE | ARM_FIRE_UNROLLED );
                        nFiresRef--;
                    } else {
                        seg->instr[ k + seg->end ] = instr[ ( seg->start + 5 ) + k ].instr | ARM_FIRE_UNROLLED; 
                    }
                    seg->instr[ k + seg->end + 1 ] = instr[ ( seg->start + 5 ) + k + 1 ].instr; 
                }
            }

            // allocate memory for the next segment and set 'seg' to point to its address
            seg->next = (segment_t *)malloc( sizeof(segment_t) );
            seg->next->prev = seg;
            seg = seg->next;
            seg->next = NULL;
            
            // 'i' will increment by 2 on the next iteration. decrement it accordingly
            i = j-2;
        }
    }

    // free up all the memory allocated to hold the iterator definitions
    iterator_t *tmp;
    if( iter != NULL ){
        while(iter->prev != NULL) iter = iter->prev;

        while(iter != NULL){
            tmp = iter;
            iter = iter->next;
            free(tmp);
        }
    }

    // go through all segments of the program, determine how many instructions the new program has, and allocate it
    TX->nInstructions = 0;
    segment_t *tmps;
    while( seg->prev != NULL ) seg = seg->prev;
    tmps = seg;
    do { // while( tmps != NULL )
        TX->nInstructions += ( tmps->end - tmps->start + tmps->newLines );
        tmps = tmps->next;
    } while( tmps != NULL );
    *(TX->instructions) = (uint32_t *)calloc(TX->nInstructions,sizeof(uint32_t));
    instr = (TX_instructionTypeReg_t *)(*(TX->instructions));

    // copy the program segments into the newly allocated instruction memory, delete the segments
    nLines = 0;
    do { // while( seg != NULL )
        memcpy(&instr[nLines],seg->instr,(seg->end - seg->start + seg->newLines)*sizeof(uint32_t));
        nLines += (seg->end - seg->start + seg->newLines);
        tmps = seg;
        seg = seg->next;
        free(tmps->instr);
        free(tmps);
    } while( seg != NULL );

    // go through all loops and check if they DON'T contain fire/fireAt, flag them accordingly
    // ( arm doesn't need to know about subprograms it won't actively interract with )
    progTracker = 0;
    for( i = 0; i < (TX->nInstructions); i+=2 ){
        if( instr[i].instr & FPGA_LOOP_START ){
            curProg = instr[i+1].instr & 0xf;
            progTracker = ( 1 << curProg );
            nFires = 0;
            j=i+5;
            while( ( ( instr[j].fpga ^ FPGA_LOOP_END_POINT ) | progTracker ) && ( j < TX->nInstructionsBuff ) ){
                if( instr[j].instr & FPGA_LOOP_START ){
                    progTracker |= ( 1 << ( instr[j+1].instr & 0xf ) );
                    j+=5;
                } else if ( instr[j].instr & ARM_SETUP_ITERATOR ){
                    j+=5;
                } else if ( instr[j].instr & FPGA_LOOP_END_POINT ){
                    progTracker &= ~( 1 << ( instr[j+1].instr & 0xf ) );
                    if ( progTracker ){
                        j+=2;
                    } else {
                        break;
                    }
                } else if ( ( instr[j].instr & ( FPGA_FIRE_CMD | FPGA_FIREAT_CMD ) ) && ( progTracker == ( 1 << curProg ) ) ){
                    nFires = 1;
                    j+=2;
                }
            }
            if ( !nFires ){
                instr[i].instr |= ARM_FIRELESS_LOOP;
                instr[j].instr |= ARM_FIRELESS_LOOP;
            }
            i+=3;
        } else if ( instr[i].instr & ARM_SETUP_ITERATOR ){
            i+=3;
        }
    }
}


void parseRecvdInstructions_tx(TXsys *TX){
    
    uint32_t nLocs;
    uint32_t nCommands = 2;
    uint32_t nLoops = 0;
    uint32_t *instr;

    // these three are done i think
    minimizeRedundantInterrupts_tx(TX);
    defineSubPrograms_tx(TX);
    populateActionLists_tx(TX);

	free(*(TX->instructionTypeReg_local));
	*(TX->instructionTypeReg_local) = (uint16_t *)malloc(nCommands*sizeof(uint16_t));
    memset(*(TX->instructionTypeReg_local),0,nCommands*sizeof(uint16_t));

	free(*(TX->instructionReg_local));
	*(TX->instructionReg_local) = (uint32_t *)malloc(nCommands*sizeof(uint32_t));
    memset(*(TX->instructionReg_local),0,nCommands*sizeof(uint32_t));

	free(*(TX->timingReg_local));
	*(TX->timingReg_local) = (uint32_t *)malloc(nCommands*sizeof(uint32_t));
    memset(*(TX->timingReg_local),0,nCommands*sizeof(uint32_t));

    free(*(TX->loopAddressReg_local));
    *(TX->loopAddressReg_local) = (uint32_t *)malloc(MAX_LOOPS*sizeof(uint32_t));
    memset(*(TX->loopAddressReg_local),0,MAX_LOOPS*sizeof(uint32_t));

    free(*(TX->loopCounterReg_local));
    *(TX->loopCounterReg_local) = (uint32_t *)malloc(MAX_LOOPS*sizeof(uint32_t));
    memset(*(TX->loopCounterReg_local),0,MAX_LOOPS*sizeof(uint32_t));

    TX->setupLoops(TX);

    uint32_t cmdN = 0;
    uint32_t lastCmd = 0;
    uint32_t cmd;
    uint16_t cmd16;

    
    for( int i=0 ; i < (TX->nInstructions) ; i+=2 ) { 
        
        // no instructionReg/instructionTypeReg value to update
        if ( instr[i] & FPGA_WAIT ) {        
            TX->timingReg_local[cmdN] = instr[i+1];
            lastCmd = FPGA_WAIT;
            cmdN++;

        } else if ( instr[i] & FPGA_WAIT_FOR_EXTERNAL ) {
            // if ( ( lastCmd ^ FPGA_WAIT ) | ( lastCmd ^ FPGA_LOOP_END_POINT ) ) cmdN++;
            if ( lastCmd & FPGA_WAIT ) cmdN--;
            TX->instructionTypeReg_local[cmdN] |= ( instr[i] & 0xffff );
            lastCmd = FPGA_WAIT_FOR_EXTERNAL;
            cmdN++;

        } else if ( instr[i] & FPGA_LOOP_END_POINT ) {
            if ( lastCmd & ( FPGA_WAIT | FPGA_WAIT_FOR_EXTERNAL ) ) cmdN--;
            TX->instructionTypeReg_local[cmdN] |= ( instr[i] & 0xffff );
            TX->loopAddressReg_local[ instr[i+1] ] |= ( cmdN<<16 );
            lastCmd = FPGA_LOOP_END_POINT;
            cmdN++;
            
        } else if ( instr[i] & FPGA_LOOP_START ) { 
            /* TODO: 
                1) Check if phaseCharge has been loaded yet, allocate pulse mem if not
                2) Check if loop is location iterator, set up pulse linkages

            */
            if ( lastCmd & ~( FPGA_WAIT | FPGA_WAIT_FOR_EXTERNAL | FPGA_LOOP_END_POINT ) ) cmdN++;
            TX->instructionTypeReg_local[ cmdN ] |= ( instr[i] & 0xffff );
            TX->instructionReg_local[ cmdN ] |= ( ( instr[i+1] & 0xf ) << 16 );
            TX->loopAddressReg_local[ ( instr[i+1] & 0xf ) ] |= ( cmdN & 0xffff );
            TX->loopCounterReg_local[ ( instr[i+1] & 0xf ) ] |= ( ( instr[i+2] & 0xffff ) | ( ( instr[i+2] & 0xffff ) << 16 ) );
            lastCmd = FPGA_LOOP_START;
            i+=2;
            
        }

    }
    


   uint16_t *tmp;
    uint32_t *tmp32;

    uint16_t instrLoc;
    uint16_t nLines;
    uint16_t startLine;
    
    tmp = (uint16_t *)TX->recvBuff;
    tmp32 = (uint32_t *)TX->recvBuff;

    instrLoc = tmp[0];
    nLines = tmp[1];
    startLine = tmp[2];

    if( instrLoc == INSTRUCTION_TYPE_REG ){
        // copy the data into the instruction type reg
    } else if ( instrLoc == INSTRUCTION_REG ){
        // copy the data into the instruction reg
    } else if ( instrLoc == TIMING_REG ){
        // copy the data into the timing reg
    }
}




void setChannelMask_tx(TXsys *TX, uint32_t chMask){
    DREF32(TX->channelMask) = chMask;
}

void setNumberOfLocations_tx(TXsys *TX, uint32_t nLocs){
    TX->nLocs = nLocs;
	free(*(TX->phaseChargeBuff));
	*(TX->phaseChargeBuff) = (char *)malloc(8*(TX->nLocs)*sizeof(uint32_t));
}

void setNumberOfInstructions_tx(TXsys *TX, uint32_t nInstructions){
    TX->nInstructions = nInstructions;
	free(*(TX->instructionBuff));
	*(TX->instructionBuff) = (char *)malloc(2*(TX->nInstructions)*sizeof(uint32_t));
}

void TX_Settings(TXsys *TX, ENETsock **ENET, ENETsock *INTR, uint32_t *msg){	
	
	switch(msg[0]){

	    case(CASE_TX_SET_CHANNEL_MASK):{
            TX->setChannelMask(TX,msg[1]);
            break;
        }
        
        case(CASE_TX_UPLOAD_PHASE_CHARGE_ALL):{
            TX->setNumberOfLocations(TX,msg[1]);
            break; 
        }

        case(CASE_TX_UPLOAD_PHASE_CHARGE_SINGLE):{
            TX->recvSinglePhaseCharge(TX,&msg[1]);
            break;
        }

        case(CASE_TX_UPLOAD_PROGRAM):{
            TX->setNumberOfInstructions(TX,msg[1]);
            break;
        }
		
	}
}













