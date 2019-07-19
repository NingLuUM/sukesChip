

void parseRecvdPhaseCharges(TXsys *TX){
    
}


void compressInstruction_tx(TXsys *TX){
    /*  Instruction Handling Notes:

        All instructions except START_LOOP are received as pairs uint32_t.
            uint32_t[0] = INSTRUCTION TYPE 
            uint32_t[1] = INSTRUCTION
        
        'INSTRUCTION TYPE' is split, bitwise, into two components
            bits[15: 0] = FPGA instruction
            bits[31:16] = ARM processor directive/instruction

        'INSTRUCTION' is a single number, whose meaning changes depending on
        the 'INSTRUCTION TYPE'
            INSTRUCTION TYPE
                DESCRIPTION of INSTRUCTION
            
            SET_LED 
                turns on/off corresponding LED outputs (8bits)
            
            SET_TRIG    
                turns on/off corresponding trigger outputs (7 or 8bit)
                -7bit if daughter FPGA (one trig is reserved to signal MASTER)
                -8bit if MASTER FPGA
            
            WAIT        
                32bits, number of clock cycles to wait until next instruction
            
            FIRE
                none
            
            FIREAT
                the memory location in phaseDelay mem corresponding to the 
                desired steering location (28bit). value at corresponding mem
                location gets preloaded into the fireAtPhaseDelay register.
                only 4096 locations can be preloaded for use with FIREAT.

            SET_CHARGE_TIME
                the charge time of the pulse to be fired (9bit)
                
            END_LOOP 
                the assigned number of the ending loop (4bit)
                *address in instruction memory where the loop begins (12bit).
                    *auto assigned by ARM

            GEN_TX_INTERRUPT
                variable, nominally none. usually set by ARM
            
            WAIT_FOR_EXTERNAL_TRIG
                none

            WAIT_FOR_INTERRUPT_TO_RESOLVE
                none

            END_PROGRAM
                none


        START_LOOP is received as 6 uint32_t. ARM directives are used to
        indicate if loop is used for steering the focus, or just to repeat 
        the code block. The START_LOOP command is only loaded into the FPGA
        instruction memory for steering loops, where it is needed to tell the
        ARM processor the assigned number of the loop, and the first steering
        location to be targeted in the loop.

            uint32_t[0] = INSTRUCTION TYPE
            uint32_t[1] = assigned loop number
            uint32_t[2] = number of iterations

            [2] is set explicitly for non-steering loops, and through ARM
            directives for steering loops. Effectively, ARM auto-generates a
            new loop, internal or external to the steering loop, which allows
            the steering loop to be repeated. This can also be accomlished by
            explicitly programming non-steering loops in/outside the steering
            loop. Because steering loops generate interrupts before they start,
            it is recommended to generate non-steering loops around steering
            loops through the ARM directives, particularly for external non-
            steering loops, otherwise the ARM interrupt will be repeated every
            iteration of the outer loop and slow things down. (This outcome can
            be avoided, but I don't want to spend time writing a more advanced
            instruction parser to catch this scenario)
            
            [3]-[5] are only set for steering loops, ignored otherwise
            
            uint32_t[3] = mem location in phaseDelay mem where loop starts
            uint32_t[4] = mem location in phaseDelay mem where loop ends
            uint32_t[5] = mem location increment in phaseDelay mem
                (allows treating every n'th location, 1 <= n <= 255)



        all commands except 'END_LOOP' and 'START_LOOP' (of the steering type) 
        are combined into single commands unless explicity directed not to be 
        through ARM directives. As such a set of commands like:
            SET_TRIG(15)
            SET_CHARGE_TIME(100)
            FIRE()
        would all execute concurrently. A WAIT(X) command following this would
        signal the FPGA to WAIT X clock cycles before executing the next block
        of commands, but wouldn't occupy its own line of an instruction.
        
        END_LOOP needs to occupy its own line in the instructions because the 
        address in instruction memory where the loop begins cannot fit in the
        INSTRUCTION alongside the INSTRUCTIONS from other commands. As such, 
        an extra clock cycle is needed for END_LOOP. 
        
        START_LOOP does not need to be issued as an FPGA instruction for non-
        steering loops because the END_LOOP command automatically returns the 
        FPGA to the instruction address where the loop started. It does,
        however, need to be issued during programming so that ARM can store the
        loop number and number of iterations to insert them into the END_LOOP
        command later. For steering loops, START_LOOP needs to take up its own 
        instruction in the FPGA instruction list because the steering location 
        at which the loop starts can be larger than number of steering locations
        that can be stored directly to the FPGA memory, and so a call to the ARM
        processor is required to load the correct phase delays into the FPGA 
        memory before the loop can begin.

        FIREAT cannot be combined with FIRE, and subsequent FIRE(AT)s cannot be 
        issued before the previous FIRE(AT) ends. Trying to issue them too 
        close together will result in the second one not being issued and cause
        the FPGA to generate an interrupt to let the ARM processor know that 
        something went wrong. This is one of the few cases where a programming
        error won't automatically stop the FPGA from running because the phase
        delay register is independent of the instruction register and I'm not
        enforcing an order between uploading the instructions and phase delays.
        Since I can't check the phase delays to make sure they don't overrun
        the instruction timing without doing that (unless I wanted to reparse
        the instructions everytime new phase delays were uploaded to check,
        which I do not want to), it basically just skips FIRE(AT) commands that
        are out of compliance. I might make the FPGA terminate the program 
        internally instead of just issuing an interrupt though.

        Any errors detected in the programming outside of the FIRE/FIREAT
        issue note, eg trying to run loops >2^28 times, will result in the FPGA
        terminating the program. I don't want to spend time writing in checks
        for all the different ways things could go wrong and then figuring out
        how to handle them all without interrupting program execution on the 
        FPGA. Instead, I just terminate the program. If you screw up and this
        happens it's your own fault. 
        
        To avoid this, write your programs correctly.
    
    */

    uint32_t i,j,k;
    int addedInstr;
    uint32_t buffLen = (TX->nInstructionsBuff);
    uint8_t trigs, leds, loopNum;
    TX_inputCommands_t *buff;
    TX_InstructionReg_t *instr;
    uint32_t loopCounterTmp;
    uint32_t loopPhaseAddrStart;
    uint32_t loopPhaseAddrEnd;
    uint32_t loopPhaseAddrIncr;

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
                if( buff[i+2].instr ) extraLoopsWanted++;
            }
            i+=4;
        }
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

            // TODO: generate list of steering loops and their parameters to store locally on ARM
            case( FPGA_IS_LOOP_START ):{
                if ( !addedInstr ){
                    j++;
                }
                loopNum = buff[i+1].instr & 0xf;
                loopCounterTmp = buff[i+2].instr;
                loopPhaseAddrStart = buff[i+3].instr;
                loopPhaseAddrEnd = buff[i+4].instr;
                loopPhaseAddrIncr = buff[i+5].instr;

                if ( ( buff[i].arm & ARM_IS_ITERATOR ) ){
                    isIterator[loopNum] = 1;

                    // check for illegal conditions, have FPGA terminate program if they exist
                    if( // start addr cannot be after end addr
                        (loopPhaseAddrStart > loopPhaseAddrEnd) ||

                        // cannot increment position by > 255
                        (loopPhaseAddrIncr & 0xffffff00) || 

                        // cannot have > 2^28 locations or iterations
                        ((loopPhaseAddrStart | loopPhaseAddrEnd | loopCounterTmp) & 0xf0000000) 
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
                    if( loopCounterTmp && ( nUnused >= extraLoopsWanted ) ){
                        for( k=0;k<MAX_TX_LOOPS;k++ ){
                            if( unusedLoop[k] ){
                                iterLoopPairs[loopNum][0] = loopNum;
                                iterLoopPairs[loopNum][1] = k; 
                                
                                loopStartInstrAddr[k] = j+1;
                                loopCounterRef[k] = loopCounterTmp;
                                unusedLoop[k] = 0;
                                break;
                            }
                        }
                    }

                    instr[j].type = FPGA_IS_LOOP_START;
                    instr[j].loopNumber = loopNum;
                    instr[j].phaseDelayStartAddr = 0;
                    instr[j].phaseDelayAddrIncr = ( loopPhaseAddrIncr ) ? loopPhaseAddrIncr : 1;

                    // the loop doesn't need to return to the instruction location where START is issued
                    // START is just a directive to the FPGA to let it know that it needs to increment
                    // the address from which the phase delay is read once it reaches the end of the loop
                    // the code block executed within the loop starts 1 instruction after START
                    loopStartInstrAddr[loopNum] = j+1;

                    if( loopPhaseAddrIncr ){
                        loopCounterRef[loopNum] = (loopPhaseAddrEnd-loopPhaseAddrStart)/loopPhaseAddrIncr;
                    } else {
                        loopCounterRef[loopNum] = (loopPhaseAddrEnd-loopPhaseAddrStart);
                    }

                    j++;
                    addedInstr = 1;
                } else {
                    loopStartInstrAddr[loopNum] = j;
                    if ( !(loopCounterTmp | 0xf0000000) ){ // can't have >2^28 iterations
                        loopCounterRef[ loopNum ] = loopCounterTmp;
                    } else {
                        instr[j].type = FPGA_END_PROGRAM;
                        j++;
                        break;
                    }
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

            // TODO: write these into the fireAt_phaseDelayReg
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
                    instr[j].chargeTime = (buff[i+1].instr & 0x1ff);
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














