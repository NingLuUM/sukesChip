
int txProgramExecutionHandler(TXsys_t *TX){

    TXpiocmd_t *cmd;
    TXpiocmd_t *loopHead;
    uint16_t *phaseDelays;
    uint32_t prog_complete = 1;
    phaseDelays = *(TX->phaseDelays);
    cmd = *(TX->pio_cmd_list);

    //printf("tx current:\n");
    //printBinary(cmd->reg2.pio_cmds);
    // reset the interrupt BEFORE updating command
    TX->resetTxInterrupt(TX);
    
    // actions to take if cmd was loop end cmd
    if ( cmd->flags.isLoopEndCmd | cmd->flags.isSteeringEndCmd ) {

        loopHead = cmd->loopHead;
        loopHead->currentIdx += loopHead->stepSize;
        
        if ( ( loopHead->currentIdx ) < ( loopHead->endIdx ) ) {
            // if loop not finished, set cmd pointer to loopHead->prev    
            // next step sets cmd to cmd->next, ie loopHead 
            cmd = loopHead->prev; 

        } else {
            // if loop finished, reset its counter to the start index
            loopHead->currentIdx = loopHead->startIdx;

        }

    } 
    
    //printf("tx next:\n");
    //printBinary(cmd->next->reg2.pio_cmds);
    if ( cmd->next != NULL ) {

        cmd = cmd->next;    
        *(TX->pio_cmd_list) = cmd;
        
        if ( cmd->reg2.set_trig_leds ) {
            TX->setTrigs(TX);
        }

        if ( cmd->reg2.set_var_atten ) {
            TX->setVarAtten(TX);
        }

        if ( cmd->reg2.issue_rcv_trig ) {
            TX->setRecvTrigDelay(TX);
        }

        if ( cmd->reg2.fire ) {
            TX->setFireCmdDelay(TX);
        }

        if ( cmd->reg2.set_async_wait ) {
            TX->setAsyncWait(TX);
        }

        if ( cmd->reg2.set_amp ) {
            TX->setChargeTime(TX);
            TX->setTmpMask(TX);
        }

        if ( cmd->reg2.set_phase ) {
            if ( cmd->flags.isSteeringStartCmd ) {
                cmd->reg3.ch0 = phaseDelays[cmd->currentIdx*8];
                cmd->reg3.ch1 = phaseDelays[cmd->currentIdx*8+1];
                cmd->reg4.ch2 = phaseDelays[cmd->currentIdx*8+2];
                cmd->reg4.ch3 = phaseDelays[cmd->currentIdx*8+3];
                cmd->reg5.ch4 = phaseDelays[cmd->currentIdx*8+4];
                cmd->reg5.ch5 = phaseDelays[cmd->currentIdx*8+5];
                cmd->reg6.ch6 = phaseDelays[cmd->currentIdx*8+6];
                cmd->reg6.ch7 = phaseDelays[cmd->currentIdx*8+7];
            }
            TX->setPhaseDelay(TX);
        }
        
        TX->issuePioCommand(TX);

        return(1);

    } else {

        TX->setControlState(TX,0);

        if( send(TX->comm_sock->fd,&prog_complete,sizeof(uint32_t),MSG_CONFIRM) ){
            printf("exited tx program successfully\n");
        }
    
    }

    return(2);

}




