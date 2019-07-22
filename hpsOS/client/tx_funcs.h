


void parseRecvdInstructions_tx(TXsys *TX){
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

    uint32_t i,j,k,l;
    int addedInstr;
    uint32_t buffLen = (TX->nInstructionsBuff);
    uint8_t trigs, leds, loopNum;
    TX_InputCommands_t *buff;
    TX_InstructionReg_t *instr;
    uint32_t loopCounterTmp;
    uint32_t loopPhaseAddrStart;
    uint32_t loopPhaseAddrEnd;
    uint32_t loopPhaseAddrIncr;
    uint32_t maxPhaseAddr;

    int illegalInputDetected;
    int nIterators,nUnused,extraLoopsWanted;
    uint8_t unusedLoop[MAX_TX_LOOPS];
    uint8_t isIteratorLoop[MAX_TX_LOOPS];
    uint32_t loopStartInstrAddr[MAX_TX_LOOPS];
    uint32_t loopCounterRef[MAX_TX_LOOPS];
    uint8_t iterLoopPairs[MAX_TX_LOOPS][2];
    uint32_t **fireAtList = *(TX->fireAtList);
    uint32_t nFireAts;
    uint32_t uniqueFireAts;
    
    nUnused = MAX_TX_LOOPS+1;
    nIterators = 0;
    extraLoopsWanted = 0;
    illegalInputDetected = 0;
    nFireAts = 0;
    maxPhaseAddr = 0;

    for( k=0; k<MAX_TX_LOOPS; k++ ){
        iterLoopPairs[k][0] = 0xff;
        iterLoopPairs[k][1] = 0xff;
        unusedLoop[k] = 1;
        isIteratorLoop[k] = 0;
        loopStartInstrAddr[k] = 0;
        loopCounterRef[k] = 0;
    }

    /* count the number of user defined loops. if it's a steering loop, check
        if it repeats and if so increment 'extraLoopsWanted'. this is used to
        determine if extra loops can be auto-allocated to the FPGA program to
        repeat the steering loops */
    for( i=0; i<buffLen; i+=2 ){
        if( buff[i].fpga & FPGA_IS_LOOP_START ){
            // only 16 numbered loops allowed, cant repeat loop >2^28 times
            if( ( buff[i+1].instr & 0xfffffff0 ) || ( buff[i+2].instr & 0xf0000000 ) ){
                illegalInputDetected = 1;
                break;
            }
            if( unusedLoop[ buff[i+1].instr ] ){
                unusedLoop[ buff[i+1].instr ] = 0;
                nUnused--;
            }
            if( buff[i].arm & ARM_IS_ITERATOR ){
                nIterators++;
                maxPhaseAddr = (maxPhaseAddr>buff[i+4].instr) ? maxPhaseAddr : buff[i+4].instr;
                if( buff[i+2].instr ) extraLoopsWanted++;
            }
            i+=4;
        }
        if( buff[i].fpga & FPGA_FIREAT_CMD ) nFireAts++;
    }

    // can't repeat loop if enough extra loops aren't available
    if(nUnused < extraLoopsWanted){
        illegalInputDetected = 1;
    }

    // generate list of fireAts and their respective steering locations
    if( nFireAts && !illegalInputDetected ){
        uniqueFireAts = 0;
        fireAtList = (uint32_t **)calloc(2,sizeof(uint32_t *));
        fireAtList[0] = (uint32_t *)calloc(nFireAts,sizeof(uint32_t));
        fireAtList[1] = (uint32_t *)calloc(nFireAts,sizeof(uint32_t));
        k=0;
        for( i=0; i<buffLen; i+=2){
            if( buff[i].fpga & FPGA_IS_LOOP_START ) i+=4;
            
            if( buff[i].fpga & FPGA_FIREAT_CMD ){
                // cant have >2^28 locations
                if( buff[i+1].instr & 0xf0000000 ){ 
                    illegalInputDetected=1;
                    break;
                }
                
                if(!k){
                    fireAtList[0][k] = k;
                    fireAtList[1][k] = buff[i+1].instr;
                    uniqueFireAts++;
                    k++;
                } else {
                    j=0;
                    for(l=0;l<k;l++){
                        if(buff[i+1].instr == fireAtList[1][l]){
                            fireAtList[0][k] = fireAtList[0][l];
                            j=1;
                            break;
                        }
                    }
                    if(!j){
                        fireAtList[0][k] = k;
                        uniqueFireAts++;
                    }
                    fireAtList[1][k] = buff[i+1].instr;
                    k++;
                }
            }
        }
        // can't have more than 12bits worth of fireAt locations
        if( uniqueFireAts>4095 ){
            illegalInputDetected = 1;
        }
    }

    buff = (TX_InputCommands_t *)(*(TX->instructionBuff));
    instr = (TX_InstructionReg_t *)calloc(buffLen,sizeof(TX_InstructionReg_t));

    j=0;
    for( i=0; i<buffLen; i+=2){

        // auto terminate any program where illegal inputs are detected
        if(illegalInputDetected){ 
            instr[0].type = FPGA_END_PROGRAM;
            break;
        }
        
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
                loopNum = buff[i+1].instr; // checked for validity above
                loopCounterTmp = buff[i+2].instr; // checked for validity above
                loopPhaseAddrStart = buff[i+3].instr;
                loopPhaseAddrEnd = buff[i+4].instr;
                loopPhaseAddrIncr = buff[i+5].instr;

                // check for illegal conditions, have FPGA terminate program if they exist
                if( // start addr cannot be after end addr
                    (loopPhaseAddrStart > loopPhaseAddrEnd) ||

                    // cannot increment position by > 255
                    (loopPhaseAddrIncr & 0xffffff00) || 

                    // cannot have > 2^28 steering locations
                    ((loopPhaseAddrStart | loopPhaseAddrEnd) & 0xf0000000) 
                  ){
                    illegalInputDetected = 1;
                    break;
                } else {
                    TX->steeringLoopDefs[loopNum].loopNumber = loopNum;
                    TX->steeringLoopDefs[loopNum].loopCounter = loopCounterTmp;
                    TX->steeringLoopDefs[loopNum].phaseAddrStart = loopPhaseAddrStart;
                    TX->steeringLoopDefs[loopNum].phaseAddrEnd = loopPhaseAddrEnd;
                    TX->steeringLoopDefs[loopNum].phaseAddrIncr = loopPhaseAddrIncr;
                    TX->steeringLoopDefs[loopNum].currentCallback = 0;
                }

                if ( ( buff[i].arm & ARM_IS_ITERATOR ) ){
                    isIterator[loopNum] = 1;

                    if(maxPhaseAddr & 0xffff4000){
                        // interrupt the arm processor 
                        instr[j].type |= FPGA_GENERATE_TX_INTERRUPT;

                        // let arm know that 'loop start' operation generated the interrupt
                        instr[j].instr = FPGA_IS_LOOP_START;

                        // tell the arm processor which loop generated the interrupt
                        instr[j].phaseAddrRequestLoopNumber = loopNum;

                        // lets arm processor know the first address requested by the loop
                        instr[j].requestedPhaseDelayStartAddr = loopPhaseAddrStart;
                        j++;
                        
                        // wait for signal from arm that phase delay register is populated
                        // tx interrupts halt all FPGAs. none can resume until signaled by 'master'  
                        instr[j].type |= FPGA_WAIT_FOR_INTERRUPT_TO_RESOLVE; 
                        
                        // wait for signal from 'master' before executing loop
                        instr[j].type |= FPGA_WAIT_FOR_EXTERNAL_TRIGGER; 
                        j++;
                    }
                    
                    // check if the iteration loop repeats, if so add an external loop to do so
                    if( loopCounterTmp ){
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

                    if(maxPhaseAddr & 0xffff4000){
                        instr[j].phaseDelayStartAddr = 0;
                    } else {
                        instr[j].phaseDelayStartAddr = loopPhaseAddrStart;
                    }

                    instr[j].phaseDelayAddrIncr = ( loopPhaseAddrIncr ) ? loopPhaseAddrIncr : 1;

                    /* loop doesn't need to return to the instruction location where START is issued
                       START is just a directive to the FPGA to let it know that it needs to increment
                       the address from which the phase delay is read once it reaches the end of the
                       loop the code block executed within the loop starts 1 instruction after START */
                    loopStartInstrAddr[loopNum] = j+1;

                    // if location increments by more than one, the number of loop iterations decreases
                    if( loopPhaseAddrIncr ){
                        loopCounterRef[loopNum] = (loopPhaseAddrEnd-loopPhaseAddrStart)/loopPhaseAddrIncr;
                    } else {
                        loopCounterRef[loopNum] = (loopPhaseAddrEnd-loopPhaseAddrStart);
                    }

                    j++;
                    addedInstr = 1;
                } else {
                    loopStartInstrAddr[loopNum] = j;
                    loopCounterRef[ loopNum ] = loopCounterTmp;
                    addedInstr = 0;
                }
                i+=4;
                break;
            }
           
            // TODO: update loop handling. if >16k locations, but loop iterates through less than 16k locations
            // don't need to go back to the big interrupt, just back to 'loop start' cmd
            case( FPGA_IS_LOOP_END ):{
                if( !addedInstr ){
                    j++;
                }
                loopNum = buff[i+1].instr;
                
                // only 16 numbered loops allowed
                if(loopNum & 0xfffffff0){
                    illegalInputDetected = 1;
                    break;
                }
                instr[j].type |= FPGA_IS_LOOP_END;
                instr[j].loopNumber = loopNum;
                instr[j].loopCounterRef = loopCounterRef[loopNum];
                instr[j].loopStartAddr = loopStartInstrAddr[loopNum];

                if( isIterator[loopNum] ){
                    // if there are more than 16k locations, add interrupt to the command 
                    // to make ARM update the phaseDelayReg
                    if( maxPhaseAddr & 0xffff4000 ){
                        instr[j].type |= FPGA_GENERATE_TX_INTERRUPT;
                    }
                    
                    // if the loop was an iterator and set to repeat fill in the outer loop
                    if( !( iterLoopPairs[loopNum][0] & 0xf0 ) ){
                        j++;
                        instr[j].type |= FPGA_IS_LOOP_END;
                        instr[j].loopNumber = iterLoopPairs[loopNum][1];
                        instr[j].loopCounterRef = loopCounterRef[iterLoopPairs[loopNum][1]];
                        if( maxPhaseAddr & 0xffff4000 ){
                            // generate the big interrupt if there's more than 16k locations
                            instr[j].loopStartAddr = loopStartInstrAddr[loopNum]-3; 
                        } else {
                            // go back to the 'loop_start' cmd to read from the first phaseDelay address
                            instr[j].loopStartAddr = loopStartInstrAddr[loopNum]-1;
                        }
                    }
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
                    illegalInputDetected = 1;
                }
                break;
            }

            case( FPGA_FIREAT_CMD ):{
                if( !(instr[j].type & FPGA_FIRE_CMD) ){
                    if( !(buff[i].arm & ARM_IS_MASTER) ){
                        instr[j].type |= FPGA_FIREAT_CMD;
                        for(k=0;k<nFireAts;k++){
                            if(buff[i+1].instr == fireAtList[1][k]){
                                instr[j].fireAt_phaseDelayAddr = fireAtList[0][k];
                                break;
                            }
                        }
                    }
                    addedInstr = 0;
                } else {
                    illegalInputDetected = 1;
                }
                break;
            }

            case( FPGA_SET_CHARGE_TIME ):{
                if( !(buff[i].arm & ARM_IS_MASTER) ){
                    
                    if( ( buff[i].arm & ARM_MAKE_SINGLE_INSTRUCTION ) && !addedInstr ){
                        j++;
                    }
                    
                    instr[j].type |= FPGA_SET_CHARGE_TIME;

                    // charge time must be 9bits or less
                    if( buff[i+1].instr & 0xfffff200 ){ 
                        illegalInputDetected = 1;
                    } else {
                        instr[j].chargeTime = (buff[i+1].instr & 0x1ff);
                    }

                }

                if( buff[i].arm & ARM_MAKE_SINGLE_INSTRUCTION ){
                    j++;
                    addedInstr = 1;
                } else {
                    addedInstr = 0;
                }
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
    // TODO: write function to load phase delays to (fireAt_)phaseDelayReg
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














