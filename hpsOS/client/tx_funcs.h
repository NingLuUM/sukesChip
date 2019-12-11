
void txSetControlState(TXsys_t *TX, uint32_t control_state){
    TXpiocmd_t *cmd_list;
    cmd_list = *(TX->pio_cmd_list);
    while( cmd_list->next != NULL ){
        cmd_list = cmd_list->next;
    }
    if(control_state == 1){
        if(cmd_list->flags.all){
            TX->addCmd(TX);
        }
        *(TX->pio_cmd_list) = cmd_list->top;

    } else {
        while(cmd_list->next != NULL){
            TX->delCmd(TX,0);
        }
    }
    uint32_t **pio_reg;
    pio_reg = TX->pio_reg;

    TX->reg0.control_state = control_state;
    DREF32(pio_reg[0]) = TX->reg0.all;

    if(control_state == 1){
        TX->resetTxInterrupt(TX);
        TX->resetRcvTrig(TX);
        printf("aloha\n");
        cmd_list = cmd_list->top;
        printf("%llu\n",cmd_list->reg25_26.all);
        DREF32(pio_reg[25]) = cmd_list->reg25_26.reg25;
        DREF32(pio_reg[26]) = cmd_list->reg25_26.reg26;
        DREF32(pio_reg[2]) = cmd_list->reg2.all;
        usleep(5);
        DREF32(pio_reg[2]) = 0;
    }

    if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) ){
        printf("change control state cmd issued successfully\n");
    } else {
        printf("change control state cmd not issued successfully\n");
    }
}

void txSetTrigRestLvls(TXsys_t *TX, uint32_t trigRestLvls){
    uint32_t **pio_reg;
    pio_reg = TX->pio_reg;

    TX->reg1.trigRestLvls = trigRestLvls;
    DREF32(pio_reg[1]) = TX->reg1.all;
    if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) ){
        printf("trig rest lvls set successfully\n");
    } else {
        printf("trig rest lvls not set successfully\n");
    }
}

void txSetActiveTransducers(TXsys_t *TX, uint32_t activeTransducers){
    uint32_t **pio_reg;
    pio_reg = TX->pio_reg;

    TX->reg1.activeTransducers = activeTransducers;
    DREF32(pio_reg[1]) = TX->reg1.all;
    if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) ){
        printf("transducers activated successfully\n");
    } else {
        printf("transducers not activated successfully\n");
    }
}

void txSetTrigs(TXsys_t *TX){
    uint32_t **pio_reg;
    pio_reg = TX->pio_reg;

    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);

    TX->reg9_24[0].all = ( cmd->flags.setTrig0 ) ? cmd->reg9_24[0].all : 0;
    TX->reg9_24[1].all = ( cmd->flags.setTrig1 ) ? cmd->reg9_24[1].all : 0;
    TX->reg9_24[2].all = ( cmd->flags.setTrig2 ) ? cmd->reg9_24[2].all : 0;
    TX->reg9_24[3].all = ( cmd->flags.setTrig3 ) ? cmd->reg9_24[3].all : 0;
    TX->reg9_24[4].all = ( cmd->flags.setTrig4 ) ? cmd->reg9_24[4].all : 0;
    TX->reg9_24[5].all = ( cmd->flags.setTrig5 ) ? cmd->reg9_24[5].all : 0;
    TX->reg9_24[6].all = ( cmd->flags.setTrig6 ) ? cmd->reg9_24[6].all : 0;
    TX->reg9_24[7].all = ( cmd->flags.setTrig7 ) ? cmd->reg9_24[7].all : 0;
    TX->reg9_24[8].all = ( cmd->flags.setTrig8 ) ? cmd->reg9_24[8].all : 0;
    TX->reg9_24[9].all = ( cmd->flags.setTrig9 ) ? cmd->reg9_24[9].all : 0;
    TX->reg9_24[10].all = ( cmd->flags.setTrig10 ) ? cmd->reg9_24[10].all : 0;
    TX->reg9_24[11].all = ( cmd->flags.setTrig11 ) ? cmd->reg9_24[11].all : 0;
    TX->reg9_24[12].all = ( cmd->flags.setTrig12 ) ? cmd->reg9_24[12].all : 0;
    TX->reg9_24[13].all = ( cmd->flags.setTrig13 ) ? cmd->reg9_24[13].all : 0;
    TX->reg9_24[14].all = ( cmd->flags.setTrig14 ) ? cmd->reg9_24[14].all : 0;
    TX->reg9_24[15].all = ( cmd->flags.setTrig15 ) ? cmd->reg9_24[15].all : 0;

    for(int i=0;i<16;i++){
        DREF32(pio_reg[i+9]) = TX->reg9_24[i].all;
    }
}

void txSetChargeTime(TXsys_t *TX){
    uint32_t **pio_reg;
    pio_reg = TX->pio_reg;

    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);

    
    TX->reg7.chargeTime = cmd->reg7.chargeTime;
    DREF32(pio_reg[7]) = TX->reg7.all;

    // only do this if the amp gets set from outside of sync block
    TXpioreg2_t reg2tmp;
    if( !(cmd->flags.isPioCmd) ){
        reg2tmp.all = 0;
        reg2tmp.set_amp = 1;
        DREF32(pio_reg[2]) = reg2tmp.all;
        usleep(5);
        DREF32(pio_reg[2]) = 0;
    }
}

void txSetFireCmdDelay(TXsys_t *TX){
    uint32_t **pio_reg;
    pio_reg = TX->pio_reg;
    
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);

    TX->reg7.fireDelay = cmd->reg7.fireDelay;
    DREF32(pio_reg[7]) = TX->reg7.all;
}

void txSetPhaseDelay(TXsys_t *TX){
    uint32_t **pio_reg;
    pio_reg = TX->pio_reg;
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);

    TX->reg3.all = cmd->reg3.all;
    TX->reg4.all = cmd->reg4.all;
    TX->reg5.all = cmd->reg5.all;
    TX->reg6.all = cmd->reg6.all;
    DREF32(pio_reg[3]) = TX->reg3.all;
    DREF32(pio_reg[4]) = TX->reg4.all;
    DREF32(pio_reg[5]) = TX->reg5.all;
    DREF32(pio_reg[6]) = TX->reg6.all;

    // only do this if the amp gets set from outside of sync block
    TXpioreg2_t reg2tmp;
    if( !(cmd->flags.isPioCmd) ){
        reg2tmp.all = 0;
        reg2tmp.set_phase = 1;
        DREF32(pio_reg[2]) = reg2tmp.all;
        usleep(5);
        DREF32(pio_reg[2]) = 0;
    }
}

void txSetRecvTrigDelay(TXsys_t *TX){
    uint32_t **pio_reg;
    pio_reg = TX->pio_reg;
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);

    TX->reg8.recvTrigDelay = cmd->reg8.recvTrigDelay;
    DREF32(pio_reg[8]) = TX->reg8.all;
}

void txSetAsyncWait(TXsys_t *TX){
    uint32_t **pio_reg;
    pio_reg = TX->pio_reg;
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);

    TX->reg25_26.all = cmd->reg25_26.all;
    DREF32(pio_reg[25]) = TX->reg25_26.reg25;
    DREF32(pio_reg[26]) = TX->reg25_26.reg26;

    // only do this if the amp gets set from outside of sync block
    TXpioreg2_t reg2tmp;
    if( !(cmd->flags.isPioCmd) & !(cmd->flags.isAsyncWait) ){
        reg2tmp.all = 0;
        reg2tmp.set_async_wait = 1;
        DREF32(pio_reg[2]) = reg2tmp.all;
        usleep(5);
        DREF32(pio_reg[2]) = 0;
    }
}

void txIssuePioCommand(TXsys_t *TX){
    uint32_t **pio_reg;
    pio_reg = TX->pio_reg;
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);

    TX->reg2.pio_cmds = cmd->reg2.pio_cmds;
    DREF32(pio_reg[2]) = TX->reg2.all;
    usleep(5);
    DREF32(pio_reg[2]) = 0;
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
    tmp->flags.all = 0;
    tmp->reg25_26.all = 0;
    
    tmp->reg9_24 = NULL;

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
            if(tmp->reg9_24 != NULL) free(tmp->reg9_24);
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
                if( loopCmd->reg9_24 != NULL ) free(loopCmd->reg9_24);
                free(loopCmd);
            }

            cmd_list = cmd_list->top;
            do {
                cmd_list = cmd_list->next;
                cmdCntr++;
                cmd_list->cmdNumber = cmdCntr;
            } while( cmd_list->next != NULL );
        } 

    } else {
        next = cmd_list->next;
        if(next != NULL){
            cmd_list->next = next->next;
            next->next->prev = cmd_list;
            free(next);
        }
    }

    *(TX->pio_cmd_list) = cmd_list->top;
}

void txMakeLoopStart(TXsys_t *TX, uint32_t startIdx, uint32_t endIdx, uint32_t stepSize){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);
    
    if( cmd->flags.all ){ 
        txAddPioCmd_f(TX);
        cmd = *(TX->pio_cmd_list);
    }

    while(cmd->next != NULL){
        cmd = cmd->next;
    }
    
    cmd->flags.isLoopStartCmd = 1;
    cmd->prev->flags.nextCmdIsLoopStart = 1;

    cmd->startIdx = startIdx;
    cmd->endIdx = endIdx;
    cmd->stepSize = stepSize;
    cmd->currentIdx = startIdx;
    if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) ){
        printf("loop started successfully\n");
    } else {
        printf("loop not started successfully\n");
    }
}

void txMakeLoopEnd(TXsys_t *TX){
    TXpiocmd_t *cmd,*tmp;
    cmd = *(TX->pio_cmd_list);
    
    if( cmd->flags.all ){ 
        txAddPioCmd_f(TX);
        cmd = *(TX->pio_cmd_list);
    }
    
    while(cmd->next != NULL){
        cmd = cmd->next;
    }
    tmp = cmd;

    cmd->flags.isLoopEndCmd = 1;
    cmd->prev->flags.nextCmdIsLoopEnd = 1;
    
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
    if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) ){
        printf("loop ended successfully\n");
    } else {
        printf("loop not ended successfully\n");
    }
}

void txMakeSteeringLoopStart(TXsys_t *TX, uint32_t startIdx, uint32_t endIdx, uint32_t stepSize){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);
    if( cmd->flags.all ){ 
        txAddPioCmd_f(TX);
        cmd = *(TX->pio_cmd_list);
    }
    while(cmd->next != NULL){
        cmd = cmd->next;
    }
    
    cmd->flags.isSteeringStartCmd = 1;
    cmd->prev->flags.nextCmdIsSteeringStart = 1;
    
    cmd->reg2.set_phase = 1;
    
    cmd->startIdx = startIdx;
    cmd->endIdx = endIdx;
    cmd->stepSize = stepSize;
    cmd->currentIdx = startIdx;
    if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) ){
        printf("steering loop started successfully\n");
    } else {
        printf("steering loop not started successfully\n");
    }
}

void txMakeSteeringLoopEnd(TXsys_t *TX){
    TXpiocmd_t *cmd,*tmp;
    cmd = *(TX->pio_cmd_list);
    if( cmd->flags.all ){ 
        txAddPioCmd_f(TX);
        cmd = *(TX->pio_cmd_list);
    }
    while(cmd->next != NULL){
        cmd = cmd->next;
    }
    tmp = cmd;

    cmd->flags.isSteeringEndCmd = 1;
    cmd->prev->flags.nextCmdIsSteeringEnd = 1;
    
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
    if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) ){
        printf("steering loop ended successfully\n");
    } else {
        printf("steering loop not ended successfully\n");
    }
}

void txMakePioCmd(TXsys_t *TX){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);
    if( cmd->flags.all ){ 
        txAddPioCmd_f(TX);
        cmd = *(TX->pio_cmd_list);
    }
    while(cmd->next != NULL){
        cmd = cmd->next;
    }
    cmd->flags.isPioCmd = 1;
    cmd->prev->flags.nextCmdIsPio = 1;
    if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) ){
        printf("pio cmd made successfully\n");
    } else {
        printf("pio cmd not made successfully\n");
    }
}

void txBufferTrigTimingCmd(TXsys_t *TX, uint32_t *trigs){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);
    while(cmd->next != NULL){
        cmd = cmd->next;
    }

    if(cmd->reg9_24 == NULL){
        cmd->reg9_24 = (TXtrigtimings_t *)calloc(16,sizeof(TXtrigtimings_t));
    }

    for(int i=0;i<16;i++){
        cmd->reg9_24[i].all = trigs[i];
    }

    cmd->flags.setTrig0 = ( cmd->reg9_24[0].duration ) ? 1 : 0;
    cmd->flags.setTrig1 = ( cmd->reg9_24[1].duration ) ? 1 : 0;
    cmd->flags.setTrig2 = ( cmd->reg9_24[2].duration ) ? 1 : 0;
    cmd->flags.setTrig3 = ( cmd->reg9_24[3].duration ) ? 1 : 0;
    cmd->flags.setTrig4 = ( cmd->reg9_24[4].duration ) ? 1 : 0;
    cmd->flags.setTrig5 = ( cmd->reg9_24[5].duration ) ? 1 : 0;
    cmd->flags.setTrig6 = ( cmd->reg9_24[6].duration ) ? 1 : 0;
    cmd->flags.setTrig7 = ( cmd->reg9_24[7].duration ) ? 1 : 0;
    cmd->flags.setTrig8 = ( cmd->reg9_24[8].duration ) ? 1 : 0;
    cmd->flags.setTrig9 = ( cmd->reg9_24[9].duration ) ? 1 : 0;
    cmd->flags.setTrig10 = ( cmd->reg9_24[10].duration ) ? 1 : 0;
    cmd->flags.setTrig11 = ( cmd->reg9_24[11].duration ) ? 1 : 0;
    cmd->flags.setTrig12 = ( cmd->reg9_24[12].duration ) ? 1 : 0;
    cmd->flags.setTrig13 = ( cmd->reg9_24[13].duration ) ? 1 : 0;
    cmd->flags.setTrig14 = ( cmd->reg9_24[14].duration ) ? 1 : 0;
    cmd->flags.setTrig15 = ( cmd->reg9_24[15].duration ) ? 1 : 0;
    
    cmd->reg2.set_trig_leds = 1;
    if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) ){
        printf("trigs set successfully\n");
    } else {
        printf("trigs not set successfully\n");
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
    if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) ){
        printf("charge time set successfully\n");
    } else {
        printf("charge time not set successfully\n");
    }
}

void txBufferFireDelayCmd(TXsys_t *TX, uint32_t fireDelay){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);
    while(cmd->next != NULL){
        cmd = cmd->next;
    }
    cmd->reg7.fireDelay = fireDelay;
    cmd->reg2.fire = 1;
    if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) ){
        printf("fire cmd set successfully\n");
    } else {
        printf("fire cmd not set successfully\n");
    }
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
    if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) ){
        printf("phase delays set successfully\n");
    } else {
        printf("phase delays not set successfully\n");
    }
}

void txBufferRecvTrigDelayCmd(TXsys_t *TX, uint32_t recvTrigDelay){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);
    while(cmd->next != NULL){
        cmd = cmd->next;
    }
    cmd->reg8.recvTrigDelay = recvTrigDelay;
    cmd->reg2.issue_rcv_trig = 1;
    if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) ){
        printf("recv trig set successfully\n");
    } else {
        printf("recv trig not set successfully\n");
    }
}

void txBufferAsyncWaitCmd(TXsys_t *TX, uint64_t timerVal){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);
    if( (cmd->flags.isAsyncWait) | (cmd->flags.isCmd0) | (cmd->flags.isLoopEndCmd) | (cmd->flags.isSteeringEndCmd) ){ 
        txAddPioCmd_f(TX);
        cmd = *(TX->pio_cmd_list);
    }
    cmd->flags.isAsyncWait = 1;
    cmd->prev->flags.nextCmdIsAsyncWait = 1;
    while(cmd->next != NULL){
        cmd = cmd->next;
    }
    cmd->reg25_26.all = timerVal;
    cmd->reg2.set_async_wait = 1;
    if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) ){
        printf("async wait set successfully\n");
    } else {
        printf("async wait not set successfully\n");
    }
}

void txResetTxInterrupt(TXsys_t *TX){
    TXpioreg2_t intr_reset;
    uint32_t **pio_reg;
    pio_reg = TX->pio_reg;

    intr_reset.all = 0;
    intr_reset.reset_interrupt = 1;
    DREF32(pio_reg[2]) = intr_reset.all;
    usleep(5);
    DREF32(pio_reg[2]) = 0;
}

void txResetRcvTrig(TXsys_t *TX){
    TXpioreg2_t rcv_trig_reset;
    uint32_t **pio_reg;
    pio_reg = TX->pio_reg;

    rcv_trig_reset.all = 0;
    rcv_trig_reset.reset_rcv_trig = 1;
    DREF32(pio_reg[2]) = rcv_trig_reset.all;
    usleep(5);
    DREF32(pio_reg[2]) = 0;
}

void txSetNumSteeringLocs(TXsys_t *TX, uint32_t nSteeringLocs){
    uint16_t *tmp;
    uint32_t ns;
    
    ns = ( nSteeringLocs ) ? nSteeringLocs : 1;

    if( ns != TX->nSteeringLocs ){
        tmp = (uint16_t *)calloc(8*ns,sizeof(uint16_t));
        free(*(TX->phaseDelays));
        *(TX->phaseDelays) = tmp;
        TX->nSteeringLocs = ns;
    }
    TX->nPhaseDelaysWritten = 0;
    if ( send(TX->comm_sock->fd,&ns,sizeof(uint32_t),MSG_CONFIRM) ){
        printf("num steering locations set successfully\n");
    } else {
        printf("num steering locations not set successfully\n");
    }
}

void txStorePhaseDelays(TXsys_t *TX, int nrecv, FMSG_t *msg){
    printf("tx store phase delays\n");
    char *pdin;
    char *pdtx;
    uint16_t *pd16;
    int i;

    pd16 = *(TX->phaseDelays);

    pdin = (char *)msg;
    pdtx = (char *)pd16;
    i=0;
    while(i<nrecv){
        pdtx[TX->nPhaseDelaysWritten+i] = pdin[i];
        TX->nPhaseDelaysWritten++;
        i++;
    }
    if ( TX->nPhaseDelaysWritten == TX->nSteeringLocs*8*sizeof(uint16_t) ){
        if ( send(TX->pd_data_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) ){
            printf("phase delays stored successfully\n");
        } else {
            printf("phase delays not stored successfully\n");
        }
        TX->nPhaseDelaysWritten = 0;
    }
}
