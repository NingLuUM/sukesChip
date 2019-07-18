

void parseRecvdPhaseCharges(TXsys *TX){
    
}


void compressInstruction_tx(TXsys *TX){
    uint32_t i,j,k;
    int addedInstr;
    uint32_t buffLen = (TX->nInstructionsBuff);
    uint8_t trigs, leds, loopNum;
    TX_inputCommands_t *buff;
    TX_InstructionReg_t *instr;

    int nIterators,nUnused,extraLoopsWanted;
    uint8_t unusedLoop[MAX_TX_LOOPS];
    uint8_t isIteratorLoop[MAX_TX_LOOPS];
    uint32_t loopStartInstrAddr[MAX_TX_LOOPS];
    uint32_t loopCounterRef[MAX_TX_LOOPS];
    uint8_t iterLoopPairs[MAX_TX_LOOPS][2];
    
    nUnused = MAX_TX_LOOPS+1;
    nIterators = 0;
    extraLoopsWanted = 0;

    for( k=0; k<MAX_TX_LOOPS; k++ ){
        iterLoopPairs[k][0] = 0xff;
        iterLoopPairs[k][1] = 0xff;
        unusedLoop[k] = 1;
        isIteratorLoop[k] = 0;
        loopStartInstrAddr[k] = 0;
        loopCounterRef[k] = 0;
    }

    for( i=0; i<buffLen; i+=2){
        if( buff[i].fpga & FPGA_IS_LOOP_START ){
            if( unusedLoop[ buff[i+1].instr & 0xf ] ){
                unusedLoop[ buff[i+1].instr & 0xf ] = 0;
                nUnused--;
            }
            if( buff[i].arm & ARM_IS_ITERATOR ){
                nIterators++;
                if( !buff[i+3].instr ) extraLoopsWanted++;
            }
        }
        i+=4;
    }


    buff = (TX_inputCommands_t *)(*(TX->instructionBuff));
    instr = (TX_InstructionReg_t *)calloc(buffLen,sizeof(TX_InstructionReg_t));

    j=0;
    for( i=0; i<buffLen; i+=2){
        switch( buff[i].fpga ){
            case( FPGA_SET_TRIG ):{
                if( buff[i].arm & ARM_SET_EXPLICIT ){
                    trigs = buff[i+1].instr;
                } else {
                    if( buff[i].arm & ARM_UNSET_VAL ){
                        trigs &= ~buff[i+1].instr;
                    } else {
                        trigs |= buff[i+1].instr;
                    }
                }
                if( buff[i].arm & ARM_IS_MASTER ){
                    instr[j].masterTrigVals = trigs;
                } else {
                    instr[j].trigVals = trigs;
                }
                instr[j].type |= FPGA_SET_TRIG;
                addedInstr = 0;
                break;
            }

            case( FPGA_SET_LEDS ):{
                if( buff[i].arm & ARM_SET_EXPLICIT ){
                    leds = buff[i+1].instr;
                } else {
                    if( buff[i].arm & ARM_UNSET_VAL ){
                        leds &= ~buff[i+1].instr;
                    } else {
                        leds |= buff[i+1].instr;
                    }
                }
                if( buff[i].arm & ARM_IS_MASTER ){
                    instr[j].ledVals = trigs;
                }
                instr[j].type |= FPGA_SET_LEDS;
                addedInstr = 0;
                break;
            }

            case( FPGA_TRIGGER_RECV_SYS ):{
                instr[j].type |= FPGA_TRIGGER_RECV_SYS;
                addedInstr = 0;
                break;
            }

            case( FPGA_IS_LOOP_START ):{
                if ( !addedInstr ){
                    j++;
                }
                loopNum = buff[i+1].instr & 0xf;
                loopCounterTmp = buff[i+2].instr & 0x0fffffff;
                loopPhaseAddrStart = buff[i+3].instr;
                loopPhaseAddrEnd = buff[i+4].instr;
                loopPhaseAddrIncr = buff[i+5].instr;

                if ( ( buff[i].arm & ARM_IS_ITERATOR ) ){
                    isIterator[loopNum] = 1;

                    // check for illegal conditions, auto kill program if they exist
                    if( (loopPhaseAddrStart > loopPhaseAddrEnd) || // start addr cannot be after end addr
                        (loopPhaseAddrIncr & 0xffffff00) || // cannot increment position by > 255
                        ((loopPhaseAddrStart | loopPhaseAddrEnd) & 0xf0000000) // cannot have > 2^28 locations
                      ){
                        instr[j].type = FPGA_END_PROGRAM;
                        j++;
                        break;
                    }

                    // interrupt the arm processor 
                    instr[j].type |= FPGA_GENERATE_TX_INTERRUPT;

                    // let arm know that 'loop start' operation generated the interrupt
                    instr[j].instr = FPGA_IS_LOOP_START;

                    // tell the arm processor which loop is being entered
                    instr[j].phaseAddrRequestLoopNumber = loopNum;

                    // lets arm processor know the first location to be treated in the loop.
                    instr[j].requestedPhaseDelayStartAddr = loopPhaseAddrStart;

                    j++;
                    
                    // wait for signal from arm that phase delay register is populated
                    instr[j].type |= FPGA_WAIT_FOR_INTERRUPT_TO_RESOLVE; 
                    // tx interrupts halt all FPGAs. none can resume until signaled by 'master'  
                    
                    // wait for signal from 'master' before executing loop
                    instr[j].type |= FPGA_WAIT_FOR_EXTERNAL_TRIGGER; 
                    j++;
                    
                    // check if the iteration loop repeats, if so add an external loop to do so
                    if( buff[i+2].instr && ( nUnused >= extraLoopsWanted ) ){
                        for( k=0;k<MAX_TX_LOOPS;k++ ){
                            if( unusedLoop[k] ){
                                iterLoopPairs[loopNum][0] = loopNum;
                                iterLoopPairs[loopNum][1] = k; 
                                
                                loopStartInstrAddr[k] = j+1;
                                loopCounterRef[k] = ( buff[i+2].instr | 0xf0000000 ) ? 0x0fffffff : buff[i+2].instr;
                                unusedLoop[k] = 0;
                                break;
                            }
                        }
                    }

                    instr[j].type = FPGA_IS_LOOP_START;
                    instr[j].loopNumber = loopNum;
                    instr[j].phaseDelayStartAddr = 0;
                    instr[j].phaseDelayAddrIncr = ( buff[i+5].instr ) ? buff[i+5].instr : 1;
                    loopStartInstrAddr[loopNum] = j+1;

                    if( buff[i+5].instr ){
                        loopCounterRef[loopNum] = (buff[i+4].instr-buff[i+3].instr)/(buff[i+5].instr);
                    } else {
                        loopCounterRef[loopNum] = (buff[i+4].instr-buff[i+3].instr);
                    }
                    loopCounterRef[loopNum] = (loopCounterRef[loopNum] | 0xf0000000) ? 0x0fffffff : loopCounterRef[loopNum];

                    j++;
                    addedInstr = 1;
                } else {
                    loopStartInstrAddr[loopNum] = j;
                    loopCounterRef[ loopNum ] = ( buff[i+2].instr | 0xf0000000 ) ? 0x0fffffff : buff[i+2].instr;
                    addedInstr = 0;
                }
                i+=4;
                break;
            }
            
            case( FPGA_IS_LOOP_END ):{
                if( !addedInstr ){
                    j++;
                }
                loopNum = buff[i+1].instr & 0xf;
                instr[j].type |= FPGA_IS_LOOP_END;
                instr[j].loopNumber = loopNum;
                instr[j].loopCounterRef = loopCounterRef[loopNum];
                if( isIterator[loopNum] && !( iterLoopPairs[loopNum][0] & 0xf0 ) ){
                    j++;
                    instr[j].type |= FPGA_IS_LOOP_END;
                    instr[j].loopNumber = iterLoopPairs[loopNum][1];
                    instr[j].loopCounterRef = loopCounterRef[iterLoopPairs[loopNum][1]];
                }
                j++;
                addedInstr = 1;

                break;
            }

            case( FPGA_FIRE_CMD ):{
                if( !(instr[j].type & FPGA_FIREAT_CMD) ){
                    if( !(buff[i].arm & ARM_IS_MASTER) ){
                        instr[j].type |= FPGA_FIRE_CMD;
                    }
                    addedInstr = 0;
                } else {
                    instr[j].type = FPGA_END_PROGRAM;
                }
                break;
            }

            case( FPGA_FIREAT_CMD ):{
                if( !(instr[j].type & FPGA_FIRE_CMD) ){
                    if( !(buff[i].arm & ARM_IS_MASTER) ){
                        instr[j].type |= FPGA_FIREAT_CMD;
                        instr[j].phaseDelayStartAddr = buff[i+1];
                    }
                    addedInstr = 0;
                } else {
                    instr[j].type = FPGA_END_PROGRAM;
                }
                break;
            }

            case( FPGA_SET_CHARGE_TIME ):{
                if( !(buff[i].arm & ARM_IS_MASTER) ){
                    instr[j].type |= FPGA_SET_CHARGE_TIME;
                    instr[j].chargeTime = buff[i+1].instr;
                }
                addedInstr = 0;
                break;
            }
            
            case( FPGA_GENERATE_TX_INTERRUPT ):{
                instr[j].type = FPGA_GENERATE_TX_INTERRUPT;
                addedInstr = 0;
                break;
            }
            
            case( FPGA_WAIT_FOR_EXTERNAL_TRIGGER ):{
                if( !addedInstr && !(instr[j].type & FPGA_WAIT_FOR_INTERRUPT_TO_RESOLVE) ){
                    j++;
                }
                instr[j].type |= FPGA_WAIT_FOR_EXTERNAL_TRIGGER;
                addedInstr = 0;
                break;
            }
            
            case( FPGA_WAIT_FOR_INTERRUPT_TO_RESOLVE ):{
                if( !addedInstr && !(instr[j].type & FPGA_WAIT_FOR_EXTERNAL_TRIGGER) ){
                    j++;
                }
                instr[j].type |= FPGA_WAIT_FOR_INTERRUPT_TO_RESOLVE;
                addedInstr = 0;
                break;
            }
            
            case( FPGA_END_PROGRAM ):{
                j++;
                instr[j].type = FPGA_END_PROGRAM;
                break;
            }

            default:{
                if( buff[i].arm & ARM_WAIT_CMD ){
                    instr[j].t = buff[i+1].t;
                    j++;
                    addedInstr = 1;
                }
                break;
            }
        } // end switch
    }
}


void parseRecvdInstructions_tx(TXsys *TX){
    
    uint32_t nLocs;
    uint32_t nCommands = 2;
    uint32_t nLoops = 0;
    uint32_t *instr;

    identifyAndPruneLoops_tx
    minimizeRedundantInterrupts_tx(TX);
    defineSubPrograms_tx(TX);
    populateActionLists_tx(TX);

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
            // if ( ( lastCmd ^ FPGA_WAIT ) | ( lastCmd ^ FPGA_LOOP_END ) ) cmdN++;
            if ( lastCmd & FPGA_WAIT ) cmdN--;
            TX->instructionTypeReg_local[cmdN] |= ( instr[i] & 0xffff );
            lastCmd = FPGA_WAIT_FOR_EXTERNAL;
            cmdN++;

        } else if ( instr[i] & FPGA_LOOP_END ) {
            if ( lastCmd & ( FPGA_WAIT | FPGA_WAIT_FOR_EXTERNAL ) ) cmdN--;
            TX->instructionTypeReg_local[cmdN] |= ( instr[i] & 0xffff );
            TX->loopAddressReg_local[ instr[i+1] ] |= ( cmdN<<16 );
            lastCmd = FPGA_LOOP_END;
            cmdN++;
            
        } else if ( instr[i] & FPGA_LOOP_START ) { 
            /* TODO: 
                1) Check if phaseCharge has been loaded yet, allocate pulse mem if not
                2) Check if loop is location iterator, set up pulse linkages

            */
            if ( lastCmd & ~( FPGA_WAIT | FPGA_WAIT_FOR_EXTERNAL | FPGA_LOOP_END ) ) cmdN++;
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














