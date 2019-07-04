

void parseRecvdPhaseCharges(TXsys *TX){
    
}

void populateActionLists_tx(TXsys *TX){
    uint32_t *instr;
    instr = (uint32_t *)TX->instructionBuff;
    int i,j;

    uint16_t masterProg, curProg, nestLoop;

    uint8_t progNum;
    uint32_t progTracker;
    TX_OutputControlProgram_t **ocpPtr;
    ocpPtr = *(TX->progs);
    
    TX_Action_f *actionList;

    progNum = 0;
    while( ocpPtr[progNum]->is.active ){
        if( ( *(ocpPtr[progNum]->actionList) ) != NULL ){
            free( *(ocpPtr[progNum]->actionList) );
        }
        *(ocpPtr[progNum]->actionList) = (TX_Action_f *)malloc((ocpPtr[progNum]->nActions)*sizeof(TX_Action_f));
    }
    
    progNum = 0;
    progTracker = 0;
    while( ( ocpPtr[progNum]->is.active ) && ( ocpPtr[progNum]->currentAction < ocpPtr[progNum]->nActions ) ){
        actionList = *(ocpPtr[progNum]->actionList);
        
        for( i = 0 ; i < (TX->nInstructions) ; i+=2 ) {
            
            if ( instr[i] & FPGA_LOOP_START ){
                actionList[ ocpPtr[progNum]->currentAction ] = &enterControlLoop;
                
                ocpPtr[progNum]->currentAction++;
                progTracker |= ( 1 << ( instr[i+1] & 0xf ) );
                j=i+4;
                i+=2;

                while( ( ( instr[j] ^ FPGA_LOOP_END_POINT ) | progTracker ) && ( j < TX->nInstructions ) ){
                    if ( instr[j] & FPGA_LOOP_START ){
                        progTracker |= ( 1 << ( instr[j+1] & 0xf ) );
                        j+=4;
                    } else if ( ( instr[j] & FPGA_LOOP_END_POINT ) && ( ( 1 << (instr[j+1] & 0xf) ) & progTracker ) ) {
                        progTracker &= ~( 1 << (instr[j+1] & 0xf) );
                    } else {
                        j+=2;
                    }
                }

            } else if ( instr[i] & FPGA_FIRE_CMD ){
                actionList[ ocpPtr[progNum]->currentAction ] = &updateFireCmdBuffer;
                ocpPtr[progNum]->currentAction++;

            } else if ( instr[i] & FPGA_FIREAT_CMD ){
                actionList[ loopPts[progNum]->currentAction ] = &updateFireAtCmdBuffer;
                ocpPtr[progNum]->currentAction++;

            } else if ( instr[i] & FPGA_SETLOC_CMD ){
            
            } else {
                i+=2;
            }
        
        }
    }
}

void setupLoops_tx(TXsys *TX){

    uint32_t *instr;
    instr = (uint32_t *)TX->instructionBuff;
    int i,j;

    uint16_t masterProg, curProg, nestLoop;
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
    ocpTmp = *(TX->progDefs);
    TX_OutputControlProgram_t **ocpPtr;
    ocpPtr = *(TX->progs);
    TX_OutputControlProgram_t *ocpChildren[MAX_LOOPS];

    for( i=0 ; i < (MAX_LOOPS+1) ; i++ ){
        ocpTmp[i].progNum.logical = 0;
        ocpTmp[i].progNum.given = 0;
        ocpTmp[i].is.topLevel = 0;
        ocpTmp[i].is.iterator = 0;
        ocpTmp[i].has.fireCmd = 0;
        ocpTmp[i].is.reversed = 0;
        ocpTmp[i].is.active = 0;
        ocpTmp[i].atExit.updateFire = 0;
        ocpTmp[i].atExit.updateFireAt = 0;
        ocpTmp[i].atLoopEnd.reloadFireAt = 0;
        ocpTmp[i].start.loc = 0;
        ocpTmp[i].end.loc = 0;
        ocpTmp[i].current.loc = 0;
        ocpTmp[i].increment = 0;
        ocpTmp[i].fireAt.first = 0;
        ocpTmp[i].fireAt.last = 0;
        ocpTmp[i].nChildren = 0;
        ocpTmp[i].nActions = 0;
        ocpTmp[i].currentAction = 0;
    }
    ocpTmp[0].is.topLevel = 1;
    ocpTmp[0].is.active = 1;

    // populate output control program loop definitions
    for( i=0 ; i < (TX->nInstructions) ; i+=2 ) {
        if ( instr[i] & ( ARM_SETUP_ITERATOR | FPGA_LOOP_START ) ){
            curProg = instr[i+1] & 0xf;
            masterProg = curProg+1;
            if ( !ocpTmp[masterProg].is.iterator ){
                ocpTmp[masterProg].progNum.given = curProg;
                ocpTmp[masterProg].start.loc = instr[i+2];
                ocpTmp[masterProg].end.loc = instr[i+3];
                ocpTmp[masterProg].is.reversed = ( ocpTmp[masterProg].start.loc > ocpTmp[masterProg].end.loc );
                incr = (int32_t)instr[i+4];
                if ( incr<0 ) {
                    ocpTmp[masterProg].increment = ( ocpTmp[masterProg].is.reversed ) ? incr : -incr ;
                } else if ( incr>0 ) {
                    ocpTmp[masterProg].increment = ( ocpTmp[masterProg].is.reversed ) ? -incr : incr ;
                } else {
                    ocpTmp[masterProg].increment =  ( ocpTmp[masterProg].is.reversed ) ? -1 : 1 ;
                }
                ocpTmp[masterProg].is.iterator = ( instr[i] & ARM_SETUP_ITERATOR ) ? 1 : 0 ;
            }
            i+=5;
        }

        if ( instr[i] & ( FPGA_FIREAT_CMD | FPGA_SETLOC_CMD ) ){
            TX->nFireAts++;
        }
    }
  
    // parse the topLevel program to find actionable instructions
    ocpPtr[0] = &ocpTmp[0];     // ocpXxx[0] = topLevel
    progTracker = 0;            // keeps track of whether currently in loop
    fireAt_detected = 0;
    for( i = 0 ; i < (TX->nInstructions) ; i+=2 ) {
        if ( instr[i] & FPGA_LOOP_START ){
            ocpTmp[0].nActions++;
            ocpChildren[ ocpTmp[0].nChildren ] = &ocpTmp[ ( instr[i+1] & 0xf ) + 1 ];
            ocpTmp[ ( instr[i+1] & 0xf ) + 1 ].parent = &ocpTmp[0];
            ocpTmp[0].nChildren++;
            progTracker |= ( 1 << ( instr[i+1] & 0xf ) );
            j=i+5;

            while( ( ( instr[j] ^ FPGA_LOOP_END_POINT ) | progTracker ) && ( j < TX->nInstructions ) ){
                if ( instr[j] & FPGA_LOOP_START ){
                    progTracker |= ( 1 << ( instr[j+1] & 0xf ) );
                    j+=5;
                } else if ( ( instr[j] & FPGA_LOOP_END_POINT ) && ( ( 1 << (instr[j+1] & 0xf) ) & progTracker ) ) {
                    progTracker &= ~( 1 << (instr[j+1] & 0xf) );
                } else if ( instr[i] & ( FPGA_FIREAT_CMD | FPGA_SETLOC_CMD ) ){
                    nFireAts++1;
                    j+=2;
                } else {
                    j+=2;
                }
            }

            i=j;
        } else if ( instr[i] & ( FPGA_FIRE_CMD | FPGA_FIREAT_CMD | FPGA_SETLOC_CMD ) ){
            ocpTmp[0].nActions++;
            ocpTmp[0].has.fireCmd=1;
            if ( instr[i] & ( FPGA_FIREAT_CMD | FPGA_SETLOC_CMD ) ){
                if ( !fireAt_detected ){
                    ocpTmp[0].fireAt.first = nFireAts;
                    *(ocpTmp[0].current) = nFireAts;
                    fireAt_detected = 1;
                } else {
                    ocpTmp[0].fireAt.last = nFireAts;
                }
                nFireAts++;
            }
            i+=2;
        } else {
            i+=2;
        }
    }
    if( ocpTmp[0].nChildren ){
        if( ocpTmp[0].children != NULL ) free( ocpTmp[0].children );
        ocpTmp[0].children = (TX_OutputControlProgram_t **)malloc(ocpTmp[0].nChildren*sizeof( TX_OutputControlProgram_t *) );
        for( i = 0 ; i < ocpTmp[0].nChildren ; i++ ){
            ocpTmp[0].children[i] = ocpChildren[i];
        }
    }

    // parse the loops defined by the user
    logicalProgNumber = 1;
    nFireAts = 0;
    for( i = 0 ; i < (TX->nInstructions) ; i+=2 ) {
        if ( instr[i] & FPGA_LOOP_START ) {
            fireAt_detected = 0;
            curProg = instr[i+1] & 0xf;
            masterProg = curProg + 1;
            ocpPtr[logicalProgNumber] = &ocpTmp[masterProg];
            ocpTmp[masterProg].progNum.logical = logicalProgNumber;
            ocpTmp[masterProg].is.active = 1;
            
            j=i+5;
            progTracker = 0;
            nFireAts_Inner = nFireAts;
            while( ( ( instr[j] ^ FPGA_LOOP_END_POINT ) | ( ( instr[j+1] & 0xf ) ^ curProg ) ) && ( j < TX->nInstructions ) ){
                
                if ( instr[j] & FPGA_LOOP_START ){
                    ocpTmp[masterProg].nActions++;
                    ocpChildren[ ocpTmp[masterProg].nChildren ] = &ocpTmp[ ( instr[i+1] & 0xf ) + 1 ];
                    ocpTmp[masterProg].nChildren++;
                    ocpTmp[ ( instr[j+1] & 0xf ) + 1 ].parent = &ocpTmp[masterProg];
                    progTracker |= ( 1 << ( instr[j+1] & 0xf ) );
                    j+=5;

                    // all actions within sub-loops get executed by the sub-loops, this basically just skips them
                    while( ( ( instr[j] ^ FPGA_LOOP_END_POINT ) | progTracker ) && ( j < TX->nInstructions ) ){
                        if ( instr[j] & FPGA_LOOP_START ){
                            progTracker |= ( 1 << ( instr[j+1] & 0xf ) );
                            j+=5;
                        } else if ( ( instr[j] & FPGA_LOOP_END_POINT ) && ( ( instr[j+1] & 0xf ) & curProg ) ){
                            break;
                        } else if ( ( instr[j] & FPGA_LOOP_END_POINT ) && ( ( 1 << (instr[j+1] & 0xf) ) & progTracker ) ) {
                            progTracker &= ~( 1 << (instr[j+1] & 0xf) );
                        } else if ( instr[j] & ( FPGA_FIREAT_CMD | FPGA_SETLOC_CMD ) ){
                            nFireAts_Inner++;
                            j+=2;
                        } else {
                            j+=2;
                        }
                    }
                } else if ( instr[j] & ( FPGA_FIRE_CMD | FPGA_FIREAT_CMD | FPGA_SETLOC_CMD ) ){
                    ocpTmp[masterProg].nActions++;
                    ocpTmp[masterProg].has.fireCmd = 1;
                    if ( instr[j] & ( FPGA_FIREAT_CMD | FPGA_SETLOC_CMD ) ){
                        if ( !fireAt_detected ){
                            ocpTmp[masterProg].fireAt.first = nFireAts_Inner;
                        } else {
                            ocpTmp[masterProg].fireAt.last = nFireAts_Inner;
                        }
                        fireAt_detected++;
                        nFireAts_Inner++;
                    }
                    j+=2;
                } else {
                    j+=2;
                }
            }

            if( nFireAts_Inner < TX->nFireAts){
                ocpTmp[masterProg].atExit.updateFireAt = 1;
            }

            if( fireAt_detected > 1 ){
                ocpTmp[masterProg].atLoopEnd.reloadFireAt = 1;
            }

            if( ocpTmp[masterProg].nChildren ){
                if( ocpTmp[masterProg].children != NULL ) free( ocpTmp[masterProg].children );
                ocpTmp[masterProg].children = (TX_OutputControlProgram_t **)malloc(ocpTmp[masterProg].nChildren*sizeof( TX_OutputControlProgram_t *) );
                for( j = 0 ; j < ocpTmp[masterProg].nChildren ; j++ ){
                    ocpTmp[masterProg].children[j] = ocpChildren[j];
                }
            }

            ocpTmp[masterProg].nActions++;
            logicalProgNumber++;
            i+=3;
        } else if ( instr[i] & ( FPGA_FIREAT_CMD | FPGA_SETLOC_CMD ) ){
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

void parseRecvdInstructions(TXsys *TX){
    
    uint32_t nLocs;
    uint32_t nCommands = 2;
    uint32_t nLoops = 0;
    uint32_t *instr;
    instr = (uint32_t *)TX->instructionBuff;
 
    for( int i=0 ; i < (TX->nInstructions) ; i+=2 ){
        if ( ( instr[i] & FPGA_WAIT ) | ( instr[i] & FPGA_LOOP_END_POINT ) | ( instr[i] & FPGA_LOOP_START ) | ( instr[i] & FPGA_WAIT_FOR_EXTERNAL ) ){
            nCommands++;
            if ( instr[i] & FPGA_LOOP_START ){
                i+=2;
            }
        }
    } 

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














