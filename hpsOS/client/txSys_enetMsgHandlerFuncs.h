
void txSys_enetMsgHandler(POLLserver_t *PS, TXsys_t *TX, FMSG_t *msg, int nrecvd, int *runner){

    static uint32_t currentTimer = 0;
    static TXtrigtimings_t trigs[16];
    static int cmdCntr;
    static uint32_t lastCmd = 0;
    TXpioreg2526_t timerVal;
    timerVal.all = 0;
    uint64_t timeOutVal = 0;
    TXpiocmd_t *cmd;
    int stepSize;
    int ncmds;
    ncmds = nrecvd/sizeof(uint32_t)-1;

    switch(msg->u[0]){
        case(CASE_TX_SET_CONTROL_STATE):{
            cmd = *(TX->pio_cmd_list);
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
            TX->setActiveTransducers(TX,msg->u[1]);
            break;
        }

        case(CASE_TX_MAKE_PIO_CMD):{
            currentTimer = 0;
            for(int i=0;i<16;i++){
                trigs[i].all = 0;
            }
            TX->makePioCmd(TX);
            cmdCntr++;
            break;
        }
        
        case(CASE_TX_END_PIO_CMD):{ // more or less equivalent to buffer instruction timer
            if( lastCmd != CASE_TX_WAIT_CMD ){
                timerVal.reg25 = 1;
                timeOutVal = currentTimer + 10;
            }
            if( lastCmd != CASE_TX_BUFFER_INSTRUCTION_TIMER ){
                timerVal.reg26 = 1;
                if( !timerVal.reg25 ){
                    timerVal.reg25 = 1;
                    timeOutVal = currentTimer + 100000000;
                }
            }
            TX->bufferInstructionTimeout(TX,timeOutVal);
            break;
        }

        case(CASE_TX_MAKE_LOOP_START):{
            currentTimer = 0;
            for(int i=0;i<16;i++){
                trigs[i].all = 0;
            }
            TX->makeLoopStart(TX,msg->u[1],msg->u[2],msg->u[3]);
            cmdCntr++;
            break;
        }

        case(CASE_TX_MAKE_LOOP_END):{
            currentTimer = 0;
            for(int i=0;i<16;i++){
                trigs[i].all = 0;
            }
            TX->makeLoopEnd(TX);
            cmdCntr++;
            break;
        }

        case(CASE_TX_MAKE_STEERING_LOOP_START):{
            currentTimer = 0;
            for(int i=0;i<16;i++){
                trigs[i].all = 0;
            }
            stepSize = (msg->u[3]) ? msg->u[3] : 1;
            TX->makeSteeringLoopStart(TX,msg->u[1],msg->u[2],stepSize);
            cmdCntr++;
            break;
        }

        case(CASE_TX_MAKE_STEERING_LOOP_END):{
            currentTimer = 0;
            for(int i=0;i<16;i++){
                trigs[i].all = 0;
            }
            TX->makeSteeringLoopEnd(TX);
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
            TX->bufferPhaseDelays(TX,&(msg->u[1]));
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

        case(CASE_TX_BUFFER_INSTRUCTION_TIMER):{
            timerVal.reg25 = msg->u[1];
            timerVal.reg26 = msg->u[2];
            timeOutVal = timerVal.all+currentTimer;
            TX->bufferInstructionTimeout(TX,timeOutVal);
            break;
        }

        case(CASE_TX_WAIT_CMD):{
            currentTimer += msg->u[1];
            break;
        }

        case(CASE_TX_SET_NSTEERING_LOCS):{
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
            *runner=0;
            break;
        }

        default:{
            *runner=0;
            break;
        }
    }

    lastCmd = msg->u[0];
}



