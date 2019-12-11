
void txSys_enetMsgHandler(TXsys_t *TX, POLLserver_t *PS, FMSG_t *msg, int nrecvd, int *runner){

    static uint32_t currentTimer;
    static TXtrigtimings_t trigs[16];
    static int cmdCntr;
    TXpioreg2526_t timerVal;
    timerVal.all = 0;
    uint64_t timeOutVal = 0;
    int stepSize;
    int ncmds;
    uint16_t *phase_addr;
    ncmds = nrecvd/sizeof(uint32_t)-1;
    
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);

    switch(msg->u[0]){
        case(CASE_TX_SET_CONTROL_STATE):{
            cmd = cmd->top;
            *(TX->pio_cmd_list) = cmd;
            TX->setControlState(TX,msg->u[1]);
            break;
        }

        case(CASE_TX_SET_TRIG_REST_LVLS):{
            TX->setTrigRestLvls(TX,msg->u[1]);
            break;
        }

        case(CASE_TX_SET_ACTIVE_TRANSDUCERS):{
            printf("CASE_TX_SET_ACTIVE_TRANSDUCERS\n");
            TX->setActiveTransducers(TX,msg->u[1]);
            break;
        }

        case(CASE_TX_MAKE_PIO_CMD):{
            currentTimer = 0;
            for(int i=0;i<16;i++){
                trigs[i].all = 0;
            }
            if(!(cmd->flags.all)){ 
                cmdCntr++;
            }
            TX->makePioCmd(TX);
            break;
        }
        
        case(CASE_TX_END_PIO_CMD):{
            timeOutVal = currentTimer;
            TX->bufferAsyncWait(TX,timeOutVal);
            TX->addCmd(TX);
            cmdCntr++;
            currentTimer = 0;
            break;
        }

        case(CASE_TX_MAKE_LOOP_START):{
            currentTimer = 0;
            for(int i=0;i<16;i++){
                trigs[i].all = 0;
            }
            if(!(cmd->flags.all)){ 
                cmdCntr++;
            }
            TX->makeLoopStart(TX,msg->u[1],msg->u[2],msg->u[3]);
            break;
        }

        case(CASE_TX_MAKE_LOOP_END):{
            currentTimer = 0;
            for(int i=0;i<16;i++){
                trigs[i].all = 0;
            }
            if(!(cmd->flags.all)){ 
                cmdCntr++;
            }
            TX->makeLoopEnd(TX);
            TX->addCmd(TX);
            cmdCntr++;
            break;
        }

        case(CASE_TX_MAKE_STEERING_LOOP_START):{
            currentTimer = 0;
            for(int i=0;i<16;i++){
                trigs[i].all = 0;
            }
            stepSize = (msg->u[3]) ? msg->u[3] : 1;
            if(!(cmd->flags.all)){ 
                cmdCntr++;
            }
            TX->makeSteeringLoopStart(TX,msg->u[1],msg->u[2],stepSize);
            break;
        }

        case(CASE_TX_MAKE_STEERING_LOOP_END):{
            currentTimer = 0;
            for(int i=0;i<16;i++){
                trigs[i].all = 0;
            }
            if(!(cmd->flags.all)){ 
                cmdCntr++;
            }
            TX->makeSteeringLoopEnd(TX);
            TX->addCmd(TX);
            cmdCntr++;
            break;
        }

        case(CASE_TX_BUFFER_TRIG_TIMINGS):{
            for(int i=1; i<ncmds; i+=2){
                if( ( msg->u[i] ) && ( (msg->u[i])<16 ) ){
                    trigs[msg->u[i]].duration = msg->u[i+1];
                    trigs[msg->u[i]].delay = currentTimer;
                }
            }
            TX->bufferTrigTimings(TX,&(trigs[0].all));
            break;
        }

        case(CASE_TX_BUFFER_CHARGE_TIME):{
            TX->bufferChargeTime(TX,msg->u[1]);
            break;
        }

        case(CASE_TX_BUFFER_PHASE_DELAYS):{
            phase_addr = (uint16_t *)(&(msg->u[1]));
            TX->bufferPhaseDelays(TX,phase_addr);
            break;
        }

        case(CASE_TX_BUFFER_FIRE_CMD):{
            TX->bufferFireCmd(TX,currentTimer);
            break;
        }

        case(CASE_TX_BUFFER_RECV_TRIG):{
            TX->bufferRecvTrig(TX,currentTimer);
            break;
        }

        case(CASE_TX_BUFFER_ASYNC_WAIT):{
            timerVal.all = msg->u64[1];
            timeOutVal = timerVal.all+currentTimer;
            if(!(cmd->flags.all)){ 
                cmdCntr++;
            }
            TX->bufferAsyncWait(TX,timeOutVal);
            break;
        }

        case(CASE_TX_WAIT_CMD):{
            currentTimer += msg->u[1];
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) ){
                printf("wait cmd issued successfully\n");
            } else {
                printf("wait cmd issued successfully\n");
            }
            break;
        }

        case(CASE_TX_SET_NSTEERING_LOCS):{
            printf("CASE_TX_SET_NSTEERING_LOCS\n");
            TX->setNumSteeringLocs(TX,msg->u[1]);
            break;
        }
        
        case(CASE_TX_CONNECT_INTERRUPT):{
            if(msg->u[1] && ( TX->interrupt.ps == NULL ) ){
                connectPollInterrupter(PS,&(TX->interrupt),"gpio@0x100000010",TX_INTERRUPT_ID);
            } else if ( !msg->u[1] && ( TX->interrupt.ps != NULL ) ){
                disconnectPollSock(&(TX->interrupt));
            }
            break;
        }
        
        case(CASE_EXIT_PROGRAM):{
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



