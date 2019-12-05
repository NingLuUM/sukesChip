
void txSys_enetMsgHandler(POLLserver_t *PS, TXsys_t *TX, FMSG_t *msg, int *runner){

    static int instrCnt = 0;
    static uint32_t currentTimer = 0;
    static trigLedTimings_t trigs[16];
    static int nTrigsRecvd = 0;
    static int cmdCntr;
    static uint32_t lastCmd = 0;
    TXpioreg2526_t timerVal;
    timerVal.all = 0;
    uint64_t timeOutVal = 0;
    TXpiocmd_t *cmd;
    int stepSize;

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
            if(nTrigsRecvd){
                TX->bufferTrigTimings(TX,&(trigs[0].all));
            }
            instrCnt = 0;
            currentTimer = 0;
            nTrigsRecvd = 0;
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
            if(nTrigsRecvd){
                TX->bufferTrigTimings(TX,&(trigs[0].all));
            }
            instrCnt = 0;
            currentTimer = 0;
            nTrigsRecvd = 0;
            for(int i=0;i<16;i++){
                trigs[i].all = 0;
            }
            TX->makeLoopStart(TX,msg->u[1],msg->u[2],msg->u[4]);
            cmdCntr++;
            break;
        }

        case(CASE_TX_MAKE_LOOP_END):{
            if(nTrigsRecvd){
                TX->bufferTrigTimings(TX,&(trigs[0].all));
            }
            instrCnt = 0;
            currentTimer = 0;
            nTrigsRecvd = 0;
            for(int i=0;i<16;i++){
                trigs[i].all = 0;
            }
            TX->makeLoopEnd(TX);
            cmdCntr++;
            break;
        }

        case(CASE_TX_MAKE_STEERING_LOOP_START):{
            if(nTrigsRecvd){
                TX->bufferTrigTimings(TX,&(trigs[0].all));
            }
            instrCnt = 0;
            currentTimer = 0;
            nTrigsRecvd = 0;
            for(int i=0;i<16;i++){
                trigs[i].all = 0;
            }
            stepSize = (msg->u[4]) ? msg->u[4] : 1;
            TX->makeSteeringLoopStart(TX,msg->u[1],msg->u[2],stepSize);
            cmdCntr++;
            break;
        }

        case(CASE_TX_MAKE_STEERING_LOOP_END):{
            if(nTrigsRecvd){
                TX->bufferTrigTimings(TX,&(trigs[0].all));
            }
            instrCnt = 0;
            currentTimer = 0;
            nTrigsRecvd = 0;
            for(int i=0;i<16;i++){
                trigs[i].all = 0;
            }
            TX->makeSteeringLoopEnd(TX);
            cmdCntr++;
            break;
        }

        case(CASE_TX_BUFFER_TRIG_TIMINGS):{
            if((msg->u[1])<16){
                trigs[msg->u[1]].duration = msg->u[2];
                trigs[msg->u[1]].delay = currentTimer;
            }
            if((msg->u[3])<16){
                trigs[msg->u[3]].duration = msg->u[4];
                trigs[msg->u[3]].delay = currentTimer;
            }
            if((msg->u[5])<16){
                trigs[msg->u[5]].duration = msg->u[6];
                trigs[msg->u[5]].delay = currentTimer;
            }
            if((msg->u[7])<16){
                trigs[msg->u[7]].duration = msg->u[8];
                trigs[msg->u[7]].delay = currentTimer;
            }
            nTrigsRecvd++;
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

        default:{
            *runner=0;
            break;
        }
    }

    lastCmd = msg->u[0];
}


void controlMsgHandler(POLLserver_t *PS, FMSG_t *msg, int *runner){

    switch(msg->u[0]){
        case(CASE_ADD_DATA_SOCKET):{
            break;
        }
        
        default:{
            break;
        }
    }
}

