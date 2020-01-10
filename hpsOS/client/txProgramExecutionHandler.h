
int txProgramExecutionHandler(TXsys_t *TX){

    TXpiocmd_t *cmd;
    TXpiocmd_t *loopHead;
    uint16_t *phaseDelays;
    uint16_t pd_tmp[8];
    uint32_t prog_complete = 1;
    uint32_t tmp_idx = 0;
    float xloc,yloc,zloc;
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
        
        if ( cmd->flags.isLoopEndCmd && ( ( loopHead->currentIdx ) < ( loopHead->endIdx ) ) ) {
            // if loop not finished, set cmd pointer to loopHead->prev    
            // next step sets cmd to cmd->next, ie loopHead 
            cmd = loopHead->prev; 

        } else if ( cmd->flags.isSteeringEndCmd && ( ( loopHead->currentIdx ) <= ( loopHead->endIdx ) ) ) {

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
            /*
            if ( cmd->flags.isSteeringStartCmd ) {
                
    printf("tx next2:\n");
                cmd->reg3.ch0 = phaseDelays[cmd->currentIdx*8];
                cmd->reg3.ch1 = phaseDelays[cmd->currentIdx*8+1];
                cmd->reg4.ch2 = phaseDelays[cmd->currentIdx*8+2];
                cmd->reg4.ch3 = phaseDelays[cmd->currentIdx*8+3];
                cmd->reg5.ch4 = phaseDelays[cmd->currentIdx*8+4];
                cmd->reg5.ch5 = phaseDelays[cmd->currentIdx*8+5];
                cmd->reg6.ch6 = phaseDelays[cmd->currentIdx*8+6];
                cmd->reg6.ch7 = phaseDelays[cmd->currentIdx*8+7];
            
            }
            */
            if ( cmd->flags.isLoadPhaseFromMemIdx ){
                
    printf("tx next2:\n");
                tmp_idx = *(cmd->cur_mem_idx);
                cmd->reg3.ch0 = phaseDelays[tmp_idx*8];
                cmd->reg3.ch1 = phaseDelays[tmp_idx*8+1];
                cmd->reg4.ch2 = phaseDelays[tmp_idx*8+2];
                cmd->reg4.ch3 = phaseDelays[tmp_idx*8+3];
                cmd->reg5.ch4 = phaseDelays[tmp_idx*8+4];
                cmd->reg5.ch5 = phaseDelays[tmp_idx*8+5];
                cmd->reg6.ch6 = phaseDelays[tmp_idx*8+6];
                cmd->reg6.ch7 = phaseDelays[tmp_idx*8+7];

            } else if ( cmd->flags.isCalcPhaseFromLoopIdxs ){
                
    printf("tx next3: %p, %p, %p\n", cmd->cur_xloc, cmd->cur_yloc, cmd->cur_zloc);
                xloc = *(cmd->cur_xloc);
                yloc = *(cmd->cur_yloc);
                zloc = *(cmd->cur_zloc);
    printf("tx next3 %f,%f,%f:\n",xloc,yloc,zloc);
                TX->calcPhaseDelaysSingle(TX,pd_tmp,xloc,yloc,zloc);

                cmd->reg3.ch0 = pd_tmp[0];
                cmd->reg3.ch1 = pd_tmp[1];
                cmd->reg4.ch2 = pd_tmp[2];
                cmd->reg4.ch3 = pd_tmp[3];
                cmd->reg5.ch4 = pd_tmp[4];
                cmd->reg5.ch5 = pd_tmp[5];
                cmd->reg6.ch6 = pd_tmp[6];
                cmd->reg6.ch7 = pd_tmp[7];

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




