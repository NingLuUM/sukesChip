

void parseRecvdPhaseCharges(TXsys *TX){
    
}

void populateActionLists_tx(TXsys *TX){
    uint32_t *instr;
    instr = (uint32_t *)TX->instructionBuff;
    int i,j;

    uint16_t masterLoop, curLoop, nestLoop;

    uint8_t loopNum;
    uint32_t loopTracker;
    TX_LoopVars_t **loopPtr;
    loopPtr = *(TX->loops);

    loopNum=0;
    while( loopPtr[loopNum]->is.active ){
        if( ( *(loopPtr[loopNum]->actionList) ) != NULL ){
            free( *(loopPtr[loopNum]->actionList) );
        }
        *(loopPtr[loopNum]->actionList) = malloc((loopPtr[loopNum]->nActions)*sizeof(void *));
    }
    
    loopNum = 0;
    loopTracker = 0;
    while( ( loopPtr[loopNum]->is.active ) && ( loopPtr[loopNum]->currentAction < loopPtr[loopNum]->nActions ) ){
        for( i = 0 ; i < (TX->nInstructions) ; i+=2 ) {
            
            if ( instr[i] & FPGA_LOOP_START ){
                *(loopPtr[loopNum]->actionList)[ loopPtr[loopNum].currentAction ] = 
                loopPtr[loopNum].currentAction++;
                loopTracker |= ( 1 << ( instr[i+1] & 0xf ) );
                j=i+4;
                i+=2;

                while( ( ( instr[j] ^ FPGA_LOOP_END_POINT ) | loopTracker ) && ( j < TX->nInstructions ) ){
                    if ( instr[j] & FPGA_LOOP_START ){
                        loopTracker |= ( 1 << ( instr[j+1] & 0xf ) );
                        j+=4;
                    } else if ( ( instr[j] & FPGA_LOOP_END_POINT ) && ( ( 1 << (instr[j+1] & 0xf) ) & loopTracker ) ) {
                        loopTracker &= ~( 1 << (instr[j+1] & 0xf) );
                    } else {
                        j+=2;
                    }
                }
            } else if ( instr[i] & ( FPGA_FIRE_CMD | FPGA_FIREAT_CMD | FPGA_SETLOC_CMD ) ){
                loopTmp[0].nActions++;
                i+=2;
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

    uint16_t masterLoop, curLoop, nestLoop;
    free(*(TX->loopDefs));
    *(TX->loopDefs) = (TX_LoopVars_t *)malloc((MAX_LOOPS+1)*sizeof(TX_LoopVars_t));
    free(*(*(TX->loops)));
    *(TX->loops) = (TX_LoopVars_t **)malloc((MAX_LOOPS+1)*sizeof(TX_LoopVars_t *));

    // bitField to keep track of whether or not current instr exists within a loop 
    uint32_t loopTracker;

    // user might define loops out of order: start_loop(3), start_loop(2), start_loop(4)
    // logical order of the given loop nums is:  [logical](given): [0](3), [1](2), [2](4)
    uint8_t logicalLoopNum;
    TX_LoopVars_t *loopTmp;
    loopTmp = *(TX->loopDefs);
    TX_LoopVars_t **loopPtr;
    loopPtr = *(TX->loops);

    for( i=0 ; i < (MAX_LOOPS+1) ; i++ ){
        loopTmp[i].loopNum.logical = 0;
        loopTmp[i].loopNum.given = 0;
        loopTmp[i].is.topLevel = 0;
        loopTmp[i].is.iterator = 0;
        loopTmp[i].is.fireCmd = 0;
        loopTmp[i].is.reversed = 0;
        loopTmp[i].is.active = 0;
        loopTmp[i].start.loc = 0;
        loopTmp[i].end.loc = 0;
        loopTmp[i].increment = 0;
        loopTmp[i].current.loc = 0;
        loopTmp[i].nActions = 0;
        loopTmp[i].currentAction = 0;
    }
    loopTmp[0].is.topLevel = 1;
    loopTmp[0].is.active = 1;

    for( i=0 ; i < (TX->nInstructions) ; i+=2 ) {
        if ( instr[i] & ARM_SETUP_ITERATOR ){
            curLoop = instr[i+1] & 0xf;
            masterLoop = curLoop+1;
            loopTmp[masterLoop].is.iterator = 1;
            loopTmp[masterLoop].loopNum.given = curLoop;
            loopTmp[masterLoop].start.loc = instr[i+2] & 0xffff;
            loopTmp[masterLoop].end.loc = ( instr[i+2]>>16 ) & 0xffff;
            loopTmp[masterLoop].increment = instr[i+3] & 0xffff;
            i+=2;
        }
    }
   
    // 'loop[0]' is resevered for the program as a whole 
    loopPtr[0] = &loopTmp[0];
    loopTmp[0].loopNum.logical = 0;
    loopTracker = 0;
    for( i = 0 ; i < (TX->nInstructions) ; i+=2 ) {
        if ( instr[i] & FPGA_LOOP_START ){
            loopTmp[0].nActions++;
            loopTracker |= ( 1 << ( instr[i+1] & 0xf ) );
            j=i+4;
            i+=2;

            while( ( ( instr[j] ^ FPGA_LOOP_END_POINT ) | loopTracker ) && ( j < TX->nInstructions ) ){
                if ( instr[j] & FPGA_LOOP_START ){
                    loopTracker |= ( 1 << ( instr[j+1] & 0xf ) );
                    j+=4;
                } else if ( ( instr[j] & FPGA_LOOP_END_POINT ) && ( ( 1 << (instr[j+1] & 0xf) ) & loopTracker ) ) {
                    loopTracker &= ~( 1 << (instr[j+1] & 0xf) );
                } else {
                    j+=2;
                }
            }
        } else if ( instr[i] & ( FPGA_FIRE_CMD | FPGA_FIREAT_CMD | FPGA_SETLOC_CMD ) ){
            loopTmp[0].nActions++;
            i+=2;
        } else {
            i+=2;
        }
    }

    logicalLoopNumber = 1;
    for( i = 0 ; i < (TX->nInstructions) ; i+=2 ) {
        if ( instr[i] & FPGA_LOOP_START ){
            curLoop = instr[i+1] & 0xf;
            masterLoop = curLoop+1;
            loopPtr[logicalLoopNumber] = &loopTmp[masterLoop];
            loopTmp[masterLoop].loopNum.logical = logicalLoopNumber;
            loopTmp[masterLoop].is.active = 1;

            if  ( !loopTmp[masterLoop].is.iterator ){
                loopTmp[masterLoop].start.pulse = instr[i+2] & 0xffff;
                loopTmp[masterLoop].end.pulse = ( instr[i+2]>>16 ) & 0xffff;

                if ( instr[i+3] ){
                    if ( (int32_t)instr[i+3] < 0 ) {
                        if( loopTmp[masterLoop].start.pulse > loopTmp[masterLoop].end.pulse ){
                            loopTmp[masterLoop] = instr[i+3];
                        } else {
                            loopTmp[masterLoop] = ( instr[i+3] & ~(1<<31) );
                        }
                    } else {
                        if( loopTmp[masterLoop].start.pulse > loopTmp[masterLoop].end.pulse ){
                            loopTmp[masterLoop] = ( instr[i+3] | (1<<31) );
                        } else {
                            loopTmp[masterLoop] = instr[i+3];
                        }
                    }
                    loopTmp[masterLoop].is.reversed = ( loopTmp[masterLoop].start.pulse > loopTmp[masterLoop].end.pulse ) ? 1 : 0;
                } else {
                    if ( loopTmp[masterLoop].start.pulse > loopTmp[masterLoop].end.pulse ){
                        loopTmp[masterLoop].increment = -1;
                        loopTmp[masterLoop].is.reversed = 1;
                    } else {
                        loopTmp[masterLoop].increment = 1;
                        loopTmp[masterLoop].is.reversed = 0;
                    }
                }
  
            }
            
            j=i+4;
            loopTracker = 0;
            while( ( ( instr[j] ^ FPGA_LOOP_END_POINT ) | ( ( instr[j+1] & 0xf ) ^ curLoop ) ) && ( j < TX->nInstructions ) ){
                if ( instr[j] & FPGA_LOOP_START ){
                    loopTmp[masterLoop].nActions++;
                    loopTracker |= ( 1 << ( instr[j+1] & 0xf ) );
                    j+=4;

                    while( ( ( instr[j] ^ FPGA_LOOP_END_POINT ) | loopTracker ) && ( j < TX->nInstructions ) ){
                        if ( instr[j] & FPGA_LOOP_START ){
                            loopTracker |= ( 1 << ( instr[j+1] & 0xf ) );
                            j+=4;
                        } else if ( ( instr[j] & FPGA_LOOP_END_POINT ) && ( ( instr[j+1] & 0xf ) & curLoop ) ){
                            break;
                        } else if ( ( instr[j] & FPGA_LOOP_END_POINT ) && ( ( 1 << (instr[j+1] & 0xf) ) & loopTracker ) ) {
                            loopTracker &= ~( 1 << (instr[j+1] & 0xf) );
                        } else {
                            j+=2;
                        }
                    }
                } else if ( instr[j] & ( FPGA_FIRE_CMD | FPGA_FIREAT_CMD | FPGA_SETLOC_CMD ) ){
                    loopTmp[masterLoop].nActions++;
                    j+=2;
                } else {
                    j+=2;
                }
            }
            loopTmp[masterLoop].nActions++;
            logicalLoopNumber++;
            i+=2;
        }
    }
        
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














