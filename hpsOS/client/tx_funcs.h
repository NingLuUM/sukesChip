
void txSetControlState(TXsys_t *TX, uint32_t control_state){
    TXpiocmd_t *cmd_list;
    cmd_list = *(TX->pio_cmd_list);

    if ( control_state == 1 ) {
        cmd_list = cmd_list->top;

        /* the fpga issues commands when the commands change. if consecutive
            commands are equivalent, execution will freeze. this makes sure no
            adjacent commands are equivalent. 
        */
        while ( cmd_list->next != NULL ) {
            cmd_list->reg2.new_cmd_flag = ((cmd_list->cmdNumber)%2);
            //printf("cmd %d:",cmd_list->cmdNumber);
            //printBinary(cmd_list->reg2.pio_cmds);
            cmd_list = cmd_list->next;
        }
        
        /* execution halts when cmd->next == NULL. in order to execute the last
            command issued by user, an empty command needs to be inserted at the
            end of the list. 
        */
        if ( cmd_list->flags.all ) {
            TX->addCmd(TX);
            cmd_list->next->reg2.new_cmd_flag = ((cmd_list->next->cmdNumber)%2);
        }
        
        /* the last user issued command in the program needs to issue an
            interrupt in order for the program to end but not all commands
            generate an interrupt. this makes sure that an interrupt is 
            generated after the last user-issued command.
        */
        if( !(cmd_list->flags.isAsyncWait) ){
            cmd_list->flags.isAsyncWait = 1;
            TX->bufferAsyncWait(TX,0);
        }

        *(TX->pio_cmd_list) = cmd_list->top;
        
        TX->reg0.control_state = control_state;
        DREF32(TX->pio_reg[0]) = TX->reg0.all;

        TX->resetTxInterrupt(TX);
        TX->resetRcvTrig(TX);
        DREF32(TX->pio_reg[2]) = 0;
        usleep(5);
        TX->issuePioCommand(TX);

    } else {
        /* if control state set to idle, deletes all commands so nothing bad
            will happen if the program somehow ran accidentally... cosmic rays
            flipping bits and such. who knows.
        */
        cmd_list = cmd_list->top;
        while(cmd_list->next != NULL){
            TX->delCmd(TX,0);
        }
        TX->reg0.control_state = control_state;
        DREF32(TX->pio_reg[0]) = TX->reg0.all;
    
    }
}

void txSetTrigRestLvls(TXsys_t *TX, uint32_t trigRestLvls){
    TX->reg1.trigRestLvls = trigRestLvls;
    DREF32(TX->pio_reg[1]) = TX->reg1.all;
}

void txSetVarAttenRestLvl(TXsys_t *TX, uint32_t varAttenRestLvl){
    TX->reg1.varAttenRestLvl = varAttenRestLvl;
    DREF32(TX->pio_reg[1]) = TX->reg1.all;
}

void txSetActiveTransducers(TXsys_t *TX, uint32_t activeTransducers){
    TX->reg1.activeTransducers = activeTransducers;
    DREF32(TX->pio_reg[1]) = TX->reg1.all;
}

void txIssuePioCommand(TXsys_t *TX){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);

    TX->reg2.pio_cmds = cmd->reg2.pio_cmds;
    DREF32(TX->pio_reg[2]) = TX->reg2.all;
}

void txSetTrigs(TXsys_t *TX){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);

    TX->reg12_16[0].all = ( cmd->flags.setTrig0 ) ? cmd->reg12_16[0].all : 0;
    TX->reg12_16[1].all = ( cmd->flags.setTrig1 ) ? cmd->reg12_16[1].all : 0;
    TX->reg12_16[2].all = ( cmd->flags.setTrig2 ) ? cmd->reg12_16[2].all : 0;
    TX->reg12_16[3].all = ( cmd->flags.setTrig3 ) ? cmd->reg12_16[3].all : 0;
    TX->reg12_16[4].all = ( cmd->flags.setTrig4 ) ? cmd->reg12_16[4].all : 0;

    for(int i=0;i<5;i++){
        DREF32(TX->pio_reg[i+12]) = TX->reg12_16[i].all;
        DREF32(TX->pio_reg[i+17]) = TX->reg17_21[i].all;
    }
}

void txSetVarAtten(TXsys_t *TX){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);

    TX->reg10.all = cmd->reg10.all;
    TX->reg11.all = cmd->reg11.all;
    DREF32(TX->pio_reg[10]) = TX->reg10.all;
    DREF32(TX->pio_reg[11]) = TX->reg11.all;
}

void txSetChargeTime(TXsys_t *TX){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);

    TX->reg7.chargeTime = cmd->reg7.chargeTime;
    DREF32(TX->pio_reg[7]) = TX->reg7.all;
}

void txSetTmpMask(TXsys_t *TX){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);

    TX->reg7.tmpMask = cmd->reg7.tmpMask;
    DREF32(TX->pio_reg[7]) = TX->reg7.all;
}

void txSetFireCmdDelay(TXsys_t *TX){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);

    TX->reg8.fireDelay = cmd->reg8.fireDelay;
    DREF32(TX->pio_reg[8]) = TX->reg8.all;
}

void txSetPhaseDelay(TXsys_t *TX){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);

    TX->reg3.all = cmd->reg3.all;
    TX->reg4.all = cmd->reg4.all;
    TX->reg5.all = cmd->reg5.all;
    TX->reg6.all = cmd->reg6.all;
    DREF32(TX->pio_reg[3]) = TX->reg3.all;
    DREF32(TX->pio_reg[4]) = TX->reg4.all;
    DREF32(TX->pio_reg[5]) = TX->reg5.all;
    DREF32(TX->pio_reg[6]) = TX->reg6.all;
}

void txSetRecvTrigDelay(TXsys_t *TX){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);

    TX->reg9.recvTrigDelay = cmd->reg9.recvTrigDelay;
    DREF32(TX->pio_reg[9]) = TX->reg9.all;
}

void txSetAsyncWait(TXsys_t *TX){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);

    TX->reg25_26.all = cmd->reg25_26.all;
    DREF32(TX->pio_reg[25]) = TX->reg25_26.reg25;
    DREF32(TX->pio_reg[26]) = TX->reg25_26.reg26;
}

void txAddPioCmd_f(TXsys_t *TX){

    TXpiocmd_t *cmd_list;
    TXpiocmd_t *tmp;
    tmp = (TXpiocmd_t *)malloc(sizeof(TXpiocmd_t));

    cmd_list = *(TX->pio_cmd_list);
    while( cmd_list->next != NULL ){
        cmd_list = cmd_list->next;
    }
    cmd_list->next = tmp;

    tmp->top = cmd_list->top;
    tmp->prev = cmd_list;
    tmp->next = NULL;
    tmp->loopHead = NULL;
    tmp->loopTail = NULL;

    tmp->flags.all = 0;
    
    tmp->cmdNumber = cmd_list->cmdNumber+1;
    tmp->reg2.all = 0;
    tmp->reg3.all = 0;
    tmp->reg4.all = 0;
    tmp->reg5.all = 0;
    tmp->reg6.all = 0;
    tmp->reg7.all = 0;
    tmp->reg8.all = 0;
    tmp->reg9.all = 0;
    tmp->reg10.all = 0;
    tmp->reg11.all = 0;
    tmp->flags.all = 0;
    tmp->reg25_26.all = 0;
    
    tmp->reg12_16 = NULL;
    tmp->reg17_21 = NULL;

    *(TX->pio_cmd_list) = tmp;
}

void txDelPioCmd_f(TXsys_t *TX, uint32_t cmdNum){
    TXpiocmd_t *cmd_list;
    TXpiocmd_t *tmp, *prev, *next;
    TXpiocmd_t *loopCmd = NULL;
    uint32_t loopCmdNum = 0;
    uint32_t cmdCntr = 0;

    cmd_list = *(TX->pio_cmd_list);
    cmd_list = cmd_list->top;

    if ( cmdNum ){
        while( ( cmd_list->cmdNumber != cmdNum ) && ( cmd_list->next != NULL ) ){
            cmd_list = cmd_list->next;
        }
        if( ( cmd_list->cmdNumber == cmdNum ) ){
            tmp = cmd_list;
            
            if(tmp->flags.isLoopStartCmd | tmp->flags.isSteeringStartCmd ){
                loopCmd = tmp->loopTail;
            } else if ( tmp->flags.isLoopEndCmd | tmp->flags.isSteeringEndCmd ){
                loopCmd = tmp->loopHead;
            }
            if(loopCmd != NULL){
                loopCmdNum = loopCmd->cmdNumber;
            }

            cmd_list = cmd_list->top;
            prev = tmp->prev;
            next = tmp->next;
            prev->next = next;
            prev->flags.nextFlags = 0;
            if( next != NULL ){
                next->prev = prev;
                prev->flags.nextFlags = next->flags.isFlags;
            }
            if(tmp->reg12_16 != NULL) free(tmp->reg12_16);
            if(tmp->reg17_21 != NULL) free(tmp->reg17_21);
            free(tmp);

            if(loopCmdNum){
                prev = loopCmd->prev;
                next = loopCmd->next;
                prev->next = next;
                prev->flags.nextFlags = 0;
                if( next != NULL ){
                    next->prev = prev;
                    prev->flags.nextFlags = next->flags.isFlags;
                }
                if( loopCmd->reg12_16 != NULL ) free(loopCmd->reg12_16);
                if( loopCmd->reg17_21 != NULL ) free(loopCmd->reg17_21);
                free(loopCmd);
            }

            cmd_list = cmd_list->top;
            do {
                cmd_list = cmd_list->next;
                cmd_list->reg2.new_cmd_flag = !(cmd_list->prev->reg2.new_cmd_flag);
                cmdCntr++;
                cmd_list->cmdNumber = cmdCntr;
            } while( cmd_list->next != NULL );
        } 

    } else {
        next = cmd_list->next;
        loopCmdNum = next->cmdNumber; // using loopCmdNum as dummy variable
        if(next != NULL){
            cmd_list->next = next->next;
            if(next->next != NULL)  next->next->prev = cmd_list;
            if(next->reg12_16 != NULL) free(next->reg12_16);
            if(next->reg17_21 != NULL) free(next->reg17_21);
            free(next); 
        }
    }

    *(TX->pio_cmd_list) = cmd_list->top;
}

void txMakeLoopStart(TXsys_t *TX, uint32_t startIdx, uint32_t endIdx, uint32_t stepSize){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);
    
    if( cmd->flags.all ){ 
        TX->addCmd(TX);
        cmd = *(TX->pio_cmd_list);
    }

    while(cmd->next != NULL){
        cmd = cmd->next;
    }
    
    cmd->flags.isLoopStartCmd = 1;
    cmd->prev->flags.nextFlags = cmd->flags.isFlags;

    if( !(cmd->flags.isAsyncWait) ){
        TX->bufferAsyncWait(TX,0);
    }

    cmd->startIdx = startIdx;
    cmd->endIdx = endIdx;
    cmd->stepSize = stepSize;
    cmd->currentIdx = startIdx;
}

void txMakeLoopEnd(TXsys_t *TX){
    TXpiocmd_t *cmd,*tmp;
    cmd = *(TX->pio_cmd_list);
    
    if( ( cmd->flags.all && cmd->flags.hasNonWaitCmds ) || ( cmd->flags.isLoopEndCmd | cmd->flags.isSteeringEndCmd ) ){ 
        TX->addCmd(TX);
        cmd = *(TX->pio_cmd_list);
    }
    
    while(cmd->next != NULL){
        cmd = cmd->next;
    }
    tmp = cmd;

    cmd->flags.isLoopEndCmd = 1;
    cmd->prev->flags.nextFlags = cmd->flags.isFlags;
    
    if( !(cmd->flags.isAsyncWait) ){
        TX->bufferAsyncWait(TX,0);
    }
    
    int loopTracker = 0;
    while(tmp->prev != NULL){
        tmp = tmp->prev;
        if( tmp->flags.isLoopEndCmd && ( tmp->loopHead != NULL ) ){
            loopTracker++;
        } else if ( tmp->flags.isLoopStartCmd ){
            if( loopTracker ){
                loopTracker--;
            } else {
                cmd->loopHead = tmp;
                tmp->loopTail = cmd;
                break;
            }
        }
    }
}

void txMakeSteeringLoopStart(TXsys_t *TX, uint32_t startIdx, uint32_t endIdx, uint32_t stepSize){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);
    if( cmd->flags.all ){ 
        TX->addCmd(TX);
        cmd = *(TX->pio_cmd_list);
    }
    while(cmd->next != NULL){
        cmd = cmd->next;
    }
    
    cmd->flags.isSteeringStartCmd = 1;
    cmd->prev->flags.nextFlags = cmd->flags.isFlags;
    cmd->reg2.set_phase = 1;

    TX->bufferAsyncWait(TX,0);
    
    cmd->startIdx = startIdx;
    cmd->endIdx = endIdx;
    cmd->stepSize = stepSize;
    cmd->currentIdx = startIdx;
}

void txMakeSteeringLoopEnd(TXsys_t *TX){
    TXpiocmd_t *cmd,*tmp;
    cmd = *(TX->pio_cmd_list);
    if( ( cmd->flags.all && cmd->flags.hasNonWaitCmds ) || ( cmd->flags.isLoopEndCmd | cmd->flags.isSteeringEndCmd ) ){ 
        TX->addCmd(TX);
        cmd = *(TX->pio_cmd_list);
    }
    while(cmd->next != NULL){
        cmd = cmd->next;
    }
    tmp = cmd;

    cmd->flags.isSteeringEndCmd = 1;
    cmd->prev->flags.nextFlags = cmd->flags.isFlags;
    
    if( !(cmd->flags.isAsyncWait) ){
        TX->bufferAsyncWait(TX,0);
    }
    
    int loopTracker = 0;
    while(tmp->prev != NULL){
        tmp = tmp->prev;
        if( tmp->flags.isSteeringEndCmd && ( tmp->loopHead != NULL ) ){
            loopTracker++;
        } else if ( tmp->flags.isSteeringStartCmd ){
            if( loopTracker ){
                loopTracker--;
            } else {
                cmd->loopHead = tmp;
                tmp->loopTail = cmd;
                break;
            }
        }
    }
}

void txMakePioCmd(TXsys_t *TX){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);
    if( cmd->flags.all && cmd->flags.isFlags ){ 
        TX->addCmd(TX);
        cmd = *(TX->pio_cmd_list);
    }
    while(cmd->next != NULL){
        cmd = cmd->next;
    }
    cmd->flags.isPioCmd = 1;
    cmd->prev->flags.nextFlags = cmd->flags.isFlags;
    TX->bufferAsyncWait(TX,0);

}

void txBufferTrigTimingCmd(TXsys_t *TX, uint32_t *duration, uint32_t *delay){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);
    while(cmd->next != NULL){
        cmd = cmd->next;
    }

    if(cmd->reg12_16 == NULL){
        cmd->reg12_16 = (TXtrigduration_t *)calloc(16,sizeof(TXtrigduration_t));
    }
    if(cmd->reg17_21 == NULL){
        cmd->reg17_21 = (TXtrigdelay_t *)calloc(16,sizeof(TXtrigduration_t));
    }

    for(int i=0;i<5;i++){
        cmd->reg12_16[i].all = duration[i];
        cmd->reg17_21[i].all = delay[i];
    }

    cmd->flags.setTrig0 = ( cmd->reg12_16[0].duration ) ? 1 : 0;
    cmd->flags.setTrig1 = ( cmd->reg12_16[1].duration ) ? 1 : 0;
    cmd->flags.setTrig2 = ( cmd->reg12_16[2].duration ) ? 1 : 0;
    cmd->flags.setTrig3 = ( cmd->reg12_16[3].duration ) ? 1 : 0;
    cmd->flags.setTrig4 = ( cmd->reg12_16[4].duration ) ? 1 : 0;
    
    cmd->reg2.set_trig_leds = 1;
}

void txBufferVarAttenTimingCmd(TXsys_t *TX, uint32_t duration, uint32_t delay){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);
    while(cmd->next != NULL){
        cmd = cmd->next;
    }
    if(duration){
        cmd->reg10.duration = duration;
        cmd->reg11.delay = delay;
        cmd->reg2.set_var_atten = 1;
    }
}

void txBufferChargeTimeCmd(TXsys_t *TX, uint32_t chargeTime){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);
    while(cmd->next != NULL){
        cmd = cmd->next;
    }
    cmd->reg7.chargeTime = chargeTime;
    cmd->reg2.set_amp = 1;
}

void txBufferTmpMaskCmd(TXsys_t *TX, uint32_t tmpMask){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);
    while(cmd->next != NULL){
        cmd = cmd->next;
    }
    cmd->reg7.tmpMask = tmpMask;
    cmd->reg2.set_amp = 1;
}

void txBufferFireDelayCmd(TXsys_t *TX, uint32_t fireDelay){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);
    while(cmd->next != NULL){
        cmd = cmd->next;
    }
    cmd->reg8.fireDelay = fireDelay;
    cmd->reg2.fire = 1;
}

void txBufferPhaseDelayCmd(TXsys_t *TX, uint16_t *phaseDelays){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);
    while(cmd->next != NULL){
        cmd = cmd->next;
    }
    cmd->reg3.ch0 = phaseDelays[0];
    cmd->reg3.ch1 = phaseDelays[1];
    cmd->reg4.ch2 = phaseDelays[2];
    cmd->reg4.ch3 = phaseDelays[3];
    cmd->reg5.ch4 = phaseDelays[4];
    cmd->reg5.ch5 = phaseDelays[5];
    cmd->reg6.ch6 = phaseDelays[6];
    cmd->reg6.ch7 = phaseDelays[7];
    cmd->reg2.set_phase = 1;
}

void txBufferRecvTrigDelayCmd(TXsys_t *TX, uint32_t recvTrigDelay){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);
    while(cmd->next != NULL){
        cmd = cmd->next;
    }
    cmd->reg9.recvTrigDelay = recvTrigDelay;
    cmd->reg2.issue_rcv_trig = 1;
}

void txBufferAsyncWaitCmd(TXsys_t *TX, uint64_t timerVal){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);
    if( cmd->flags.isCmd0 ){ 
        TX->addCmd(TX);
        cmd = *(TX->pio_cmd_list);
    }
    cmd->flags.isAsyncWait = 1;
    cmd->prev->flags.nextFlags = cmd->flags.isFlags;
    while(cmd->next != NULL){
        cmd = cmd->next;
    }
    cmd->reg25_26.all = timerVal;
    cmd->reg2.set_async_wait = 1;
}

void txResetTxInterrupt(TXsys_t *TX){
    TXpioreg2_t intr_reset;

    intr_reset.all = 0;
    intr_reset.reset_interrupt = 1;
    DREF32(TX->pio_reg[2]) = intr_reset.all;
}

void txResetRcvTrig(TXsys_t *TX){
    TXpioreg2_t rcv_trig_reset;

    rcv_trig_reset.all = 0;
    rcv_trig_reset.reset_rcv_trig = 1;
    DREF32(TX->pio_reg[2]) = rcv_trig_reset.all;
}

void txSetNumSteeringLocs(TXsys_t *TX, uint32_t nSteeringLocs){
    TX->nSteeringLocs = ( nSteeringLocs ) ? nSteeringLocs : 1;
    if( TX->phaseDelays[0] ) free( TX->phaseDelays[0] );
    TX->phaseDelays[0] = (uint16_t *)calloc(8*(TX->nSteeringLocs+2),sizeof(uint16_t));
    TX->nPhaseDelaysWritten = 0;
}

void txStorePhaseDelays(TXsys_t *TX, int nrecv, FMSG_t *msg){
    int i;
    i=0;
    while( ( TX->nPhaseDelaysWritten < (TX->nSteeringLocs*8) ) ){
        TX->phaseDelays[0][TX->nPhaseDelaysWritten] = msg->u16[i];
        TX->nPhaseDelaysWritten++;
        i++;
    }
    //printf("pdwritten %d/%d\n",TX->nPhaseDelaysWritten/8,TX->nSteeringLocs);
    if ( TX->nPhaseDelaysWritten*sizeof(uint16_t) == TX->nSteeringLocs*8*sizeof(uint16_t) ){
        if ( send(TX->pd_data_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) ){
            //printf("phase delays stored successfully\n");
            i = 0;
        }
    }
}




