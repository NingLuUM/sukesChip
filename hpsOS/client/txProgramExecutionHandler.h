
int txProgramExecutionHandler(TXsys_t *TX){
    TXpiocmd_t *cmd;
    TXpiocmd_t *loopHead;
    uint16_t *phaseDelays;
    uint32_t prog_complete = 1;
    phaseDelays = *(TX->phaseDelays);
    cmd = *(TX->pio_cmd_list);
    //printf("prev cmd->reg2:\n");
    //printBinary(cmd->reg2.pio_cmds); 
    //printf("prev cmd->flags:\n");
    //printBinary(cmd->flags.isFlags); 
    //printf("\n");
    // reset the interrupt and rcv trig BEFORE updating command
    TX->resetTxInterrupt(TX);
    //TX->resetRcvTrig(TX);
    
    /*
    if(cmd->flags.isLoopEndCmd)         printf("loop end, ");
    if(cmd->flags.isSteeringEndCmd)     printf("steering loop end, ");
    if(cmd->flags.isLoopStartCmd)       printf("loop start, ");
    if(cmd->flags.isSteeringStartCmd)   printf("steering loop start, ");
    if(cmd->flags.isPioCmd)             printf("pio cmd, ");
    if(cmd->flags.isAsyncWait)          printf("async wait, ");
    */
    
    // increment to next command
    if( cmd->flags.isLoopEndCmd | cmd->flags.isSteeringEndCmd ){
        loopHead = cmd->loopHead;
        loopHead->currentIdx += loopHead->stepSize;
        //printf("loopHead->flags:\n");
        //printBinary(loopHead->flags.isFlags); 
        //printf("loopHead->currentIdx/endIdx = %d / %d\n",loopHead->currentIdx, loopHead->endIdx);
        if( ( loopHead->currentIdx ) < ( loopHead->endIdx ) ){
            
            *(TX->pio_cmd_list) = loopHead;
            TX->issuePioCommand(TX);
            return(1);

        } else if( cmd->next == NULL ){
            
            cmd = cmd->top;
            TX->setControlState(TX,0);
            if( send(TX->comm_sock->fd,&prog_complete,sizeof(uint32_t),MSG_CONFIRM) ){
                printf("exited successfully\n");
            } else {
                printf("failed to exit successfully\n");
            }
            return(2);

        } else {
            loopHead->currentIdx = 0;
            cmd = cmd->next;
            *(TX->pio_cmd_list) = cmd;
            TX->issuePioCommand(TX);
            return(1);
        
        }

    } else if( cmd->next == NULL ){
   
        cmd = cmd->top;
        TX->setControlState(TX,0);
        if( send(TX->comm_sock->fd,&prog_complete,sizeof(uint32_t),MSG_CONFIRM) ){
            printf("exited successfully\n");
        } else {
            printf("failed to exit successfully\n");
        }
        return(2);
    
    } else {
    
        cmd = cmd->next;
        *(TX->pio_cmd_list) = cmd;
    
    }
    
    printf("next cmd->reg2:\n");
    printBinary(cmd->reg2.pio_cmds); 
    printf("next cmd->flags:\n");
    printBinary(cmd->flags.isFlags); 
    //printf("cmd->flags = %d. ",cmd->flags.isFlags);
    //printBinary(cmd->flags.all); 
    printf("\n");
    //printf("async_wait = %llu\n",cmd->reg25_26.all);
    
    if ( cmd->flags.isLoopStartCmd | cmd->flags.isSteeringStartCmd ) {
        if(cmd->flags.isSteeringStartCmd){
            cmd->reg3.ch0 = phaseDelays[cmd->currentIdx*8];
            cmd->reg3.ch1 = phaseDelays[cmd->currentIdx*8+1];
            cmd->reg4.ch2 = phaseDelays[cmd->currentIdx*8+2];
            cmd->reg4.ch3 = phaseDelays[cmd->currentIdx*8+3];
            cmd->reg5.ch4 = phaseDelays[cmd->currentIdx*8+4];
            cmd->reg5.ch5 = phaseDelays[cmd->currentIdx*8+5];
            cmd->reg6.ch6 = phaseDelays[cmd->currentIdx*8+6];
            cmd->reg6.ch7 = phaseDelays[cmd->currentIdx*8+7];
        }
    } 
    
    
    if( cmd->reg2.set_trig_leds ){
        TX->setTrigs(TX);
    }

    if( cmd->reg2.issue_rcv_trig ){
        TX->setRecvTrigDelay(TX);
    }

    if( cmd->reg2.fire ){
        TX->setFireCmdDelay(TX);
    }

    if( cmd->reg2.set_async_wait ){
        TX->setAsyncWait(TX);
    }

    if( cmd->reg2.set_amp ){
        TX->setChargeTime(TX);
    }

    if( cmd->reg2.set_phase ){
        TX->setPhaseDelay(TX);
    }
    
    //usleep(100);
    TX->issuePioCommand(TX);

    return(1);
}




