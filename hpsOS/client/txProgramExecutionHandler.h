
int txProgramExecutionHandler(TXsys_t *TX){
    TXpiocmd_t *cmd;
    TXpiocmd_t *loopHead;
    uint32_t *phaseDelays;
    phaseDelays = *(TX->phaseDelays);
    cmd = *(TX->pio_cmd_list);

    // reset the interrupt and rcv trig BEFORE updating command
    TX->resetTxInterrupt(TX);
    TX->resetRcvTrig(TX);

    // increment to next command
    if( cmd->flags.isLoopEndCmd | cmd->flags.isSteeringEndCmd ){
        loopHead = cmd->loopHead;
        if( ( loopHead->currentIdx ) < ( loopHead->endIdx ) ){
            
            *(TX->pio_cmd_list) = loopHead;
            TX->setRequestNextInstrTimer(TX);
            return(1);

        } else if( cmd->next == NULL ){
            
            cmd = cmd->top;
            TX->setControlState(TX,0);
            return(2);

        } else {

            cmd = cmd->next;
            *(TX->pio_cmd_list) = cmd;
            TX->setRequestNextInstrTimer(TX);
            return(1);
        
        }

    } else if( cmd->next == NULL ){
   
        cmd = cmd->top;
        TX->setControlState(TX,0);
        return(2);
    
    } else {
    
        cmd = cmd->next;
        *(TX->pio_cmd_list) = cmd;
    
    }
    
    
    if ( cmd->flags.isLoopStartCmd | cmd->flags.isSteeringStartCmd ) {
        if(cmd->flags.isSteeringStartCmd){
            cmd->reg3.all = phaseDelays[cmd->currentIdx*4];
            cmd->reg4.all = phaseDelays[cmd->currentIdx*4+1];
            cmd->reg5.all = phaseDelays[cmd->currentIdx*4+2];
            cmd->reg6.all = phaseDelays[cmd->currentIdx*4+3];
        }
        cmd->currentIdx += cmd->stepSize;
        
        if( TX->reg2.set_amp ){
            TX->setChargeTime(TX);
        }

        if( TX->reg2.set_phase ){
            TX->setPhaseDelay(TX);
        }

        TX->setRequestNextInstrTimer(TX);
        return(1);
    }

    if ( cmd->flags.isPioCmd ){
        if( cmd->reg2.set_trig_leds ){
            TX->setTrigs(TX);
        }

        if( cmd->reg2.issue_rcv_trig ){
            TX->setRecvTrigDelay(TX);
        }

        if( cmd->reg2.fire ){
            TX->setFireCmdDelay(TX);
        }

        if( cmd->reg2.set_instr_request_timer ){
            TX->setRequestNextInstrTimer(TX);
        }

        if( cmd->reg2.set_amp ){
            TX->setChargeTime(TX);
        }

        if( cmd->reg2.set_phase ){
            TX->setPhaseDelay(TX);
        }
        
        TX->issuePioCommand(TX);
    }
    return(1);
}




