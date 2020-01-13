
void txSysMsgHandler(TXsys_t *TX, FMSG_t *msg, int nrecvd, int *runner){

    static uint32_t currentTimer;

    static TXtrigduration_t trig_duration[5];
    static TXtrigdelay_t trig_delay[5];
    static int cmdCntr;
    TXpioreg2526_t timerVal;
    timerVal.all = 0;
    uint64_t timeOutVal = 0;
    int stepSize=0;
    uint16_t *phase_addr;
    
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);

    switch(msg->u32[0]){
        case(CASE_TX_SET_CONTROL_STATE):{ // CASE: 0
            TX->setControlState(TX,msg->u32[1]);
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && TX->printMsgs ){
                printf("change control state cmd issued successfully\n");
            }
            break;
        }

        case(CASE_TX_SET_TRIG_REST_LVLS):{ // CASE: 1
            TX->setTrigRestLvls(TX,msg->u32[1]);
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && TX->printMsgs ){
                printf("trig rest lvls set successfully\n");
            }
            break;
        }

        case(CASE_TX_SET_ACTIVE_TRANSDUCERS):{ // CASE: 2
            TX->setActiveTransducers(TX,msg->u32[1]);
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && TX->printMsgs ){
                printf("transducers activated successfully\n");
            }
            break;
        }

        case(CASE_TX_MAKE_PIO_CMD):{ // CASE: 3
            currentTimer = 0;
            for(int i=0;i<5;i++){
                trig_duration[i].all = 0;
                trig_delay[i].all = 0;
            }
            if( cmd->flags.all && cmd->flags.isFlags ){ 
                cmdCntr++;
            }
            TX->makePioCmd(TX);
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && TX->printMsgs ){
                printf("pio cmd made successfully\n");
            }
            break;
        }
        
        case(CASE_TX_END_PIO_CMD):{ // CASE: 4
            timeOutVal = currentTimer;
            TX->bufferAsyncWait(TX,timeOutVal);
            TX->addCmd(TX);
            for(int i=0;i<5;i++){
                trig_duration[i].all = 0;
                trig_delay[i].all = 0;
            }
            cmdCntr++;
            currentTimer = 0;
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && TX->printMsgs ){
                printf("pio cmd ended successfully\n");
            }
            break;
        }

        case(CASE_TX_MAKE_COUNTER_START):{ // CASE: 5
            currentTimer = 0;
            for(int i=0;i<5;i++){
                trig_duration[i].all = 0;
                trig_delay[i].all = 0;
            }
            if( cmd->flags.all ){ 
                cmdCntr++;
            }
            TX->makeCounterStart(TX,msg->u32[1],msg->u64[1],msg->u64[2],msg->u64[3]);
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && TX->printMsgs ){
                printf("loop started successfully\n");
            }
            break;
        }

        case(CASE_TX_MAKE_COUNTER_END):{ // CASE: 6
            currentTimer = 0;
            for(int i=0;i<5;i++){
                trig_duration[i].all = 0;
                trig_delay[i].all = 0;
            }
            if( ( cmd->flags.all && cmd->flags.hasNonWaitCmds ) || ( cmd->flags.isCounterEndCmd | cmd->flags.isLoopEndCmd ) ){ 
                cmdCntr++;
            }
            TX->makeCounterEnd(TX);
            TX->addCmd(TX);
            cmdCntr++;
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && TX->printMsgs ){
                printf("loop ended successfully\n");
            }
            break;
        }

        case(CASE_TX_MAKE_LOOP_START):{ // CASE: 7
            currentTimer = 0;
            for(int i=0;i<5;i++){
                trig_duration[i].all = 0;
                trig_delay[i].all = 0;
            }
            TX->makeLoopStart(TX,msg->u32[1],msg->d[1],msg->d[2],msg->d[3],msg->d[4]);
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && TX->printMsgs ){
                printf("steering loop started successfully\n");
            }
            break;
        }

        case(CASE_TX_MAKE_LOOP_END):{ // CASE: 8
            currentTimer = 0;
            for(int i=0;i<5;i++){
                trig_duration[i].all = 0;
                trig_delay[i].all = 0;
            }
            if( ( cmd->flags.all && cmd->flags.hasNonWaitCmds ) || ( cmd->flags.isCounterEndCmd | cmd->flags.isLoopEndCmd ) ){ 
                cmdCntr++;
            }
            TX->makeLoopEnd(TX);
            TX->addCmd(TX);
            cmdCntr++;
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && TX->printMsgs ){
                printf("steering loop ended successfully\n");
            }
            break;
        }

        case(CASE_TX_BUFFER_TRIG_TIMINGS):{ // CASE: 10
            if( msg->u32[1] && ( msg->u32[1]<6 ) ){
                trig_duration[msg->u32[1]-1].duration = msg->u32[2];
                trig_delay[msg->u32[1]-1].delay = currentTimer;
                if ( !(cmd->flags.all) ){
                    TX->makePioCmd(TX);
                    stepSize = 1; // as dummy variable
                }
                TX->bufferTrigTimings(TX,&(trig_duration[0].all),&(trig_delay[0].all));
                if( stepSize ){ // as dummy variable
                    TX->addCmd(TX);
                    for(int i=0;i<5;i++){
                        trig_duration[i].all = 0;
                        trig_delay[i].all = 0;
                    }
                    cmdCntr++;
                    currentTimer = 0;
                }
            }
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && TX->printMsgs ){
                printf("trigs set successfully\n");
            }
            break;
        }

        case(CASE_TX_BUFFER_CHARGE_TIME):{ // CASE: 11
            TX->bufferChargeTime(TX,msg->u32[1]);
            if ( !(cmd->flags.all) ){
                TX->makePioCmd(TX);
                TX->addCmd(TX);
                for(int i=0;i<5;i++){
                    trig_duration[i].all = 0;
                    trig_delay[i].all = 0;
                }
                cmdCntr++;
                currentTimer = 0;
            }
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && TX->printMsgs ){
                printf("charge time set successfully\n");
            }
            break;
        }

        case(CASE_TX_BUFFER_PHASE_DELAYS):{ // CASE: 12
            phase_addr = &(msg->u16[2]);
            TX->bufferPhaseDelays(TX,phase_addr);
            if ( !(cmd->flags.all) ){
                TX->makePioCmd(TX);
                TX->addCmd(TX);
                for(int i=0;i<5;i++){
                    trig_duration[i].all = 0;
                    trig_delay[i].all = 0;
                }
                cmdCntr++;
                currentTimer = 0;
            }
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && TX->printMsgs ){
                printf("phase delays set successfully\n");
            }
            break;
        }

        case(CASE_TX_BUFFER_FIRE_CMD):{ // CASE: 13
            TX->bufferFireCmd(TX,currentTimer);
            if ( !(cmd->flags.all) ){
                TX->makePioCmd(TX);
                TX->addCmd(TX);
                for(int i=0;i<5;i++){
                    trig_duration[i].all = 0;
                    trig_delay[i].all = 0;
                }
                cmdCntr++;
                currentTimer = 0;
            }
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && TX->printMsgs ){
                printf("fire cmd set successfully\n");
            }
            break;
        }

        case(CASE_TX_BUFFER_RECV_TRIG):{ // CASE: 14
            TX->bufferRecvTrig(TX,currentTimer);
            if ( !(cmd->flags.all) ){
                TX->makePioCmd(TX);
                TX->addCmd(TX);
                for(int i=0;i<5;i++){
                    trig_duration[i].all = 0;
                    trig_delay[i].all = 0;
                }
                cmdCntr++;
                currentTimer = 0;
            }
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && TX->printMsgs ){
                printf("recv trig set successfully\n");
            }
            break;
        }

        case(CASE_TX_BUFFER_ASYNC_WAIT):{ // CASE: 15
            timerVal.all = msg->u64[1];
            timeOutVal = timerVal.all+currentTimer;
            if( cmd->flags.isCmd0 ){ 
                cmdCntr++;
                for(int i=0;i<5;i++){
                    trig_duration[i].all = 0;
                    trig_delay[i].all = 0;
                }
            }
            TX->bufferAsyncWait(TX,timeOutVal);
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && TX->printMsgs ){
                printf("async wait set successfully\n");
            }
            break;
        }

        case(CASE_TX_WAIT_CMD):{ // CASE: 16
            currentTimer += msg->u32[1];
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && TX->printMsgs ){
                printf("wait cmd issued successfully\n");
            }
            break;
        }

        case(CASE_TX_SET_SYNC_CMD_TIME_VAL):{ // CASE: 17
            currentTimer = msg->u32[1];
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && TX->printMsgs ){
                printf("wait cmd issued successfully\n");
            }
            break;
        }

        case(CASE_TX_SET_NSTEERING_LOCS):{ // CASE: 18
            TX->setNumSteeringLocs(TX,msg->u32[1]);
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && TX->printMsgs ){
                printf("num steering locations set successfully\n");
            }
            break;
        }
        
        case(CASE_TX_CONNECT_INTERRUPT):{ // CASE: 19
            if(msg->u32[1] && ( TX->interrupt->ps == NULL ) ){
                connectPollInterrupter(TX->ps,TX->interrupt,"gpio@0x100000010",TX_INTERRUPT_ID);
            } else if ( !msg->u32[1] && ( TX->interrupt->ps != NULL ) ){
                disconnectPollSock(TX->interrupt);
            }
            break;
        }

        case(CASE_TX_SET_EXTERNAL_TRIGGER_MODE):{ // CASE: 20
            TX->bufferRecvTrig(TX,currentTimer);
            if ( !(cmd->flags.all) ){
                TX->makePioCmd(TX);
                TX->addCmd(TX);
                for(int i=0;i<5;i++){
                    trig_duration[i].all = 0;
                    trig_delay[i].all = 0;
                }
                cmdCntr++;
                currentTimer = 0;
            }
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && TX->printMsgs ){
                printf("recv trig set successfully\n");
            }
            break;
        }
        
        case(CASE_TX_BUFFER_VAR_ATTEN_TIMINGS):{ // CASE: 21
            TX->bufferVarAttenTiming(TX,msg->u32[1],currentTimer);
            if ( !(cmd->flags.all) ){
                TX->makePioCmd(TX);
                TX->addCmd(TX);
                for(int i=0;i<5;i++){
                    trig_duration[i].all = 0;
                    trig_delay[i].all = 0;
                }
                cmdCntr++;
                currentTimer = 0;
            }
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && TX->printMsgs ){
                printf("var atten set successfully\n");
            }
            break;
        }

        case(CASE_TX_SET_VAR_ATTEN_REST_LVL):{ // CASE: 22
            TX->setVarAttenRestLvl(TX,msg->u32[1]);
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && TX->printMsgs ){
                printf("var atten rest lvl set successfully\n");
            }
            break;
        }
        
        case(CASE_TX_BUFFER_TMP_MASK_CMD):{ // CASE: 23
            TX->bufferTmpMask(TX,msg->u32[1]);
            if ( !(cmd->flags.all) ){
                TX->makePioCmd(TX);
                TX->addCmd(TX);
                for(int i=0;i<5;i++){
                    trig_duration[i].all = 0;
                    trig_delay[i].all = 0;
                }
                cmdCntr++;
                currentTimer = 0;
            }
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && TX->printMsgs ){
                printf("charge time set successfully\n");
            }
            break;
        }

        case(CASE_TX_SET_SOUND_SPEED):{ // CASE: 24
            TX->setSoundSpeed(TX,msg->d[1]);
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && TX->printMsgs ){
                printf("tx sound speed updated\n");
            }
            break;
        }

        case(CASE_TX_SET_PHASE_FROM_LOOP_IDX_AS_MEM_IDX):{ // CASE: 25
            TX->setPhaseFromLoopIdxAsMemIdx(TX,msg->u32[1]);
            if ( !(cmd->flags.all) ){
                TX->makePioCmd(TX);
                TX->addCmd(TX);
                for(int i=0;i<5;i++){
                    trig_duration[i].all = 0;
                    trig_delay[i].all = 0;
                }
                cmdCntr++;
                currentTimer = 0;
            }
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && TX->printMsgs ){
                printf("fire at loc cmd set successfully\n");
            }
            break;
        }

        case(CASE_TX_CALC_AND_SET_PHASE_AT_SPECIFIED_COORD_VALS):{ // CASE: 26
            TX->calcAndSetPhaseAtSpecifiedCoordVals(TX,msg->d[1],msg->d[2],msg->d[3]);
            if ( !(cmd->flags.all) ){
                TX->makePioCmd(TX);
                TX->addCmd(TX);
                for(int i=0;i<5;i++){
                    trig_duration[i].all = 0;
                    trig_delay[i].all = 0;
                }
                cmdCntr++;
                currentTimer = 0;
            }
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && TX->printMsgs ){
                printf("fire at loc cmd set successfully\n");
            }
            break;
        }
        
        case(CASE_TX_CALC_AND_SET_PHASE_FROM_LOOP_IDXS_AS_COORD_VALS):{ // CASE: 27
            TX->calcAndSetPhaseFromLoopIdxsAsCoordVals(TX,msg->u32[1],msg->u32[2],msg->u32[3]);
            if ( !(cmd->flags.all) ){
                TX->makePioCmd(TX);
                TX->addCmd(TX);
                for(int i=0;i<5;i++){
                    trig_duration[i].all = 0;
                    trig_delay[i].all = 0;
                }
                cmdCntr++;
                currentTimer = 0;
            }
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && TX->printMsgs ){
                printf("fire at loc cmd set successfully\n");
            }
            break;
        }

        case(CASE_TX_PRINT_FEEDBACK_MSGS):{ // CASE: 50
            TX->printMsgs = msg->u32[1];
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && TX->printMsgs ){
                printf("tx print msgs setting updated successfully\n");
            }
            break;
        }

        case(CASE_EXIT_PROGRAM):{ // CASE: 100
            printf("case exit program\n");
            *runner=0;
            break;
        }

        default:{
            printf("error in tx case statement. default ending program\n");
            *runner=0;
            break;
        }
    }

}



