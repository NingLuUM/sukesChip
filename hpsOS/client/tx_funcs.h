
void txSetControlState(TXsys_t *TX, uint32_t control_state){
    uint32_t *pio_reg;
    pio_reg = *(TX->pio_reg);

    TX->reg0.control_state = control_state;
    DREF32(pio_reg[0]) = TX->reg0.all;
}

void txSetTrigRestLvls(TXsys_t *TX, uint32_t trigRestLvls){
    uint32_t *pio_reg;
    pio_reg = *(TX->pio_reg);

    TX->reg1.trigRestLvls = trigRestLvls;
    DREF32(pio_reg[1]) = TX->reg1.all;
}

void txSetActiveTransducers(TXsys_t *TX, uint32_t activeTransducers){
    uint32_t *pio_reg;
    pio_reg = *(TX->pio_reg);

    TX->reg1.activeTransducers = activeTransducers;
    DREF32(pio_reg[1]) = TX->reg1.all;
}

void txSetTrigs(TXsys_t *TX){
    uint32_t *pio_reg;
    pio_reg = *(TX->pio_reg);

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
    uint32_t *pio_reg;
    pio_reg = *(TX->pio_reg);

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
    uint32_t *pio_reg;
    pio_reg = *(TX->pio_reg);
    
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);

    TX->reg7.fireDelay = cmd->reg7.fireDelay;
    DREF32(pio_reg[7]) = TX->reg7.all;
}

void txSetPhaseDelay(TXsys_t *TX){
    uint32_t *pio_reg;
    pio_reg = *(TX->pio_reg);
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
    uint32_t *pio_reg;
    pio_reg = *(TX->pio_reg);
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);

    TX->reg8.recvTrigDelay = cmd->reg8.recvTrigDelay;
    DREF32(pio_reg[8]) = TX->reg8.all;
}

void txSetRequestNextInstrTimer(TXsys_t *TX){
    uint32_t *pio_reg;
    pio_reg = *(TX->pio_reg);
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);

    TX->reg25_26.all = cmd->reg25_26.all;
    DREF32(pio_reg[25]) = TX->reg25_26.reg25;
    DREF32(pio_reg[26]) = TX->reg25_26.reg26;

    // only do this if the amp gets set from outside of sync block
    TXpioreg2_t reg2tmp;
    if( !(cmd->flags.isPioCmd) ){
        reg2tmp.all = 0;
        reg2tmp.set_instr_request_timer = 1;
        DREF32(pio_reg[2]) = reg2tmp.all;
        usleep(5);
        DREF32(pio_reg[2]) = 0;
    }
}

void txIssuePioCommand(TXsys_t *TX){
    uint32_t *pio_reg;
    pio_reg = *(TX->pio_reg);
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
    while( ( cmd_list->cmdNumber != cmdNum ) && ( cmd_list->next != NULL ) ){
        cmd_list = cmd_list->next;
    }

    if( ( cmd_list->cmdNumber == cmdNum ) && ( cmdNum != 0 ) ){
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
            if( next->flags.isPioCmd ){
                prev->flags.nextCmdIsPio = 1;
            } else if ( next->flags.isLoopStartCmd ){
                prev->flags.nextCmdIsLoopStart = 1;
            } else if ( next->flags.isLoopEndCmd ){
                prev->flags.nextCmdIsLoopEnd = 1;
            } else if ( next->flags.isSteeringStartCmd ){
                prev->flags.nextCmdIsSteeringStart = 1;
            } else if ( next->flags.isSteeringEndCmd ){
                prev->flags.nextCmdIsSteeringEnd = 1;
            }
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
                if( next->flags.isPioCmd ){
                    prev->flags.nextCmdIsPio = 1;
                } else if ( next->flags.isLoopStartCmd ){
                    prev->flags.nextCmdIsLoopStart = 1;
                } else if ( next->flags.isLoopEndCmd ){
                    prev->flags.nextCmdIsLoopEnd = 1;
                } else if ( next->flags.isSteeringStartCmd ){
                    prev->flags.nextCmdIsSteeringStart = 1;
                } else if ( next->flags.isSteeringEndCmd ){
                    prev->flags.nextCmdIsSteeringEnd = 1;
                }
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

    cmd_list = cmd_list->top;
    *(TX->pio_cmd_list) = cmd_list;
}

void txMakeLoopStart(TXsys_t *TX, uint32_t startIdx, uint32_t endIdx, uint32_t stepSize){
    txAddPioCmd_f(TX);
    
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);
    while(cmd->next != NULL){
        cmd = cmd->next;
    }
    
    cmd->flags.isLoopStartCmd = 1;
    cmd->prev->flags.nextCmdIsLoopStart = 1;

    cmd->startIdx = startIdx;
    cmd->endIdx = endIdx;
    cmd->stepSize = stepSize;
    cmd->currentIdx = startIdx;
}

void txMakeLoopEnd(TXsys_t *TX){
    txAddPioCmd_f(TX);
    
    TXpiocmd_t *cmd, *tmp;
    cmd = *(TX->pio_cmd_list);
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
}

void txMakeSteeringLoopStart(TXsys_t *TX, uint32_t startIdx, uint32_t endIdx, uint32_t stepSize){
    txAddPioCmd_f(TX);
    
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);
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
}

void txMakeSteeringLoopEnd(TXsys_t *TX){
    txAddPioCmd_f(TX);
    
    TXpiocmd_t *cmd, *tmp;
    cmd = *(TX->pio_cmd_list);
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
}

void txMakePioCmd(TXsys_t *TX){
    txAddPioCmd_f(TX);
    
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);
    while(cmd->next != NULL){
        cmd = cmd->next;
    }
    cmd->flags.isPioCmd = 1;
    cmd->prev->flags.nextCmdIsPio = 1;
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

void txBufferFireDelayCmd(TXsys_t *TX, uint32_t fireDelay){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);
    while(cmd->next != NULL){
        cmd = cmd->next;
    }
    cmd->reg7.fireDelay = fireDelay;
    cmd->reg2.fire = 1;
}

void txBufferPhaseDelayCmd(TXsys_t *TX, uint32_t *phaseDelays){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);
    while(cmd->next != NULL){
        cmd = cmd->next;
    }
    cmd->reg3.all = phaseDelays[0];
    cmd->reg4.all = phaseDelays[1];
    cmd->reg5.all = phaseDelays[2];
    cmd->reg6.all = phaseDelays[3];
    cmd->reg2.set_phase = 1;
}

void txBufferRecvTrigDelayCmd(TXsys_t *TX, uint32_t recvTrigDelay){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);
    while(cmd->next != NULL){
        cmd = cmd->next;
    }
    cmd->reg8.recvTrigDelay = recvTrigDelay;
    cmd->reg2.issue_rcv_trig = 1;
}

void txBufferSetRequestNextInstrTimerCmd(TXsys_t *TX, uint64_t timerVal){
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);
    while(cmd->next != NULL){
        cmd = cmd->next;
    }
    cmd->reg25_26.all = timerVal;
    cmd->reg2.set_instr_request_timer = 1;
}

void txResetTxInterrupt(TXsys_t *TX){
    TXpioreg2_t intr_reset;
    uint32_t *pio_reg;
    pio_reg = *(TX->pio_reg);

    intr_reset.all = 0;
    intr_reset.reset_interrupt = 1;
    DREF32(pio_reg[2]) = intr_reset.all;
    usleep(5);
    DREF32(pio_reg[2]) = 0;
}

void txResetRcvTrig(TXsys_t *TX){
    TXpioreg2_t rcv_trig_reset;
    uint32_t *pio_reg;
    pio_reg = *(TX->pio_reg);

    rcv_trig_reset.all = 0;
    rcv_trig_reset.reset_rcv_trig = 1;
    DREF32(pio_reg[2]) = rcv_trig_reset.all;
    usleep(5);
    DREF32(pio_reg[2]) = 0;
}

void txSetNumSteeringLocs(TXsys_t *TX, uint32_t nSteeringLocs){
    
    if(nSteeringLocs != TX->nSteeringLocs){
        free(*(TX->phaseDelays));
        *(TX->phaseDelays) = (uint32_t *)calloc(nSteeringLocs*4,sizeof(uint32_t));
        TX->nSteeringLocs = nSteeringLocs;
    } else if ( !nSteeringLocs ){
        free(*(TX->phaseDelays));
        *(TX->phaseDelays) = (uint32_t *)calloc(4,sizeof(uint32_t));
        TX->nSteeringLocs = 1;
    }

}

void txStorePhaseDelays(TXsys_t *TX, int nrecv, FMSG_t *msg){
    static int nwritten = 0;
    char *pdin;
    char *pdtx;
    uint32_t *pd32;
    int i;

    pd32 = *(TX->phaseDelays);

    pdin = (char *)msg;
    pdtx = (char *)pd32;
    i=0;
    while(i<nrecv){
        pdtx[nwritten+i] = pdin[i];
        nwritten++;
        i++;
    }

    if( nwritten >= ( 4*TX->nSteeringLocs*sizeof(uint32_t) ) ){
        nwritten=0;
    }

}
