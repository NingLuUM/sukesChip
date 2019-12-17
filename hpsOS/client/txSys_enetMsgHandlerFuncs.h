
void txSysMsgHandler(TXsys_t *TX, FMSG_t *msg, int nrecvd, int *runner){

    static uint32_t printMsgs = 0;
    static uint32_t currentTimer;
    static uint32_t innerTimer;

    static TXtrigtimings_t trigs[5];
    static int cmdCntr;
    TXpioreg2526_t timerVal;
    timerVal.all = 0;
    uint64_t timeOutVal = 0;
    int stepSize=0;
    int ncmds=0;
    uint16_t *phase_addr;
    ncmds = nrecvd/sizeof(uint32_t)-1;
    
    TXpiocmd_t *cmd;
    cmd = *(TX->pio_cmd_list);

    switch(msg->u[0]){
        case(CASE_TX_SET_CONTROL_STATE):{
            TX->setControlState(TX,msg->u[1]);
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("change control state cmd issued successfully\n");
            }
            break;
        }

        case(CASE_TX_SET_TRIG_REST_LVLS):{
            TX->setTrigRestLvls(TX,msg->u[1]);
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("trig rest lvls set successfully\n");
            }
            break;
        }

        case(CASE_TX_SET_VAR_ATTEN_REST_LVL):{
            TX->setVarAttenRestLvl(TX,msg->u[1]);
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("var atten rest lvl set successfully\n");
            }
            break;
        }

        case(CASE_TX_SET_ACTIVE_TRANSDUCERS):{
            TX->setActiveTransducers(TX,msg->u[1]);
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("transducers activated successfully\n");
            }
            break;
        }

        case(CASE_TX_MAKE_PIO_CMD):{
            currentTimer = 0;
            for(int i=0;i<5;i++){
                trigs[i].all = 0;
            }
            if( cmd->flags.all && cmd->flags.isFlags ){ 
                cmdCntr++;
            }
            TX->makePioCmd(TX);
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("pio cmd made successfully\n");
            }
            break;
        }
        
        case(CASE_TX_END_PIO_CMD):{
            timeOutVal = currentTimer;
            TX->bufferAsyncWait(TX,timeOutVal);
            TX->addCmd(TX);
            for(int i=0;i<5;i++){
                trigs[i].all = 0;
            }
            cmdCntr++;
            currentTimer = 0;
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("pio cmd ended successfully\n");
            }
            break;
        }

        case(CASE_TX_MAKE_LOOP_START):{
            currentTimer = 0;
            for(int i=0;i<5;i++){
                trigs[i].all = 0;
            }
            if( cmd->flags.all ){ 
                cmdCntr++;
            }
            TX->makeLoopStart(TX,msg->u[1],msg->u[2],msg->u[3]);
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("loop started successfully\n");
            }
            break;
        }

        case(CASE_TX_MAKE_LOOP_END):{
            currentTimer = 0;
            for(int i=0;i<5;i++){
                trigs[i].all = 0;
            }
            if( ( cmd->flags.all && cmd->flags.hasNonWaitCmds ) || ( cmd->flags.isLoopEndCmd | cmd->flags.isSteeringEndCmd ) ){ 
                cmdCntr++;
            }
            TX->makeLoopEnd(TX);
            TX->addCmd(TX);
            cmdCntr++;
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("loop ended successfully\n");
            }
            break;
        }

        case(CASE_TX_MAKE_STEERING_LOOP_START):{
            currentTimer = 0;
            for(int i=0;i<5;i++){
                trigs[i].all = 0;
            }
            stepSize = (msg->u[3]) ? msg->u[3] : 1;
            if( cmd->flags.all ){ 
                cmdCntr++;
            }
            TX->makeSteeringLoopStart(TX,msg->u[1],msg->u[2],stepSize);
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("steering loop started successfully\n");
            }
            break;
        }

        case(CASE_TX_MAKE_STEERING_LOOP_END):{
            currentTimer = 0;
            for(int i=0;i<5;i++){
                trigs[i].all = 0;
            }
            if( ( cmd->flags.all && cmd->flags.hasNonWaitCmds ) || ( cmd->flags.isLoopEndCmd | cmd->flags.isSteeringEndCmd ) ){ 
                cmdCntr++;
            }
            TX->makeSteeringLoopEnd(TX);
            TX->addCmd(TX);
            cmdCntr++;
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("steering loop ended successfully\n");
            }
            break;
        }

        case(CASE_TX_BUFFER_TRIG_TIMINGS):{
            for(int i=1; i<ncmds; i+=2){
                if( ( msg->u[i] ) && ( (msg->u[i])<6 ) ){
                    trigs[msg->u[i]-1].duration = msg->u[i+1];
                    trigs[msg->u[i]-1].delay = currentTimer;
                }
            }
            if ( !(cmd->flags.all) ){
                TX->makePioCmd(TX);
                stepSize = 1; // as dummy variable
            }
            TX->bufferTrigTimings(TX,&(trigs[0].all));
            if( stepSize ){ // as dummy variable
                TX->addCmd(TX);
                for(int i=0;i<5;i++){
                    trigs[i].all = 0;
                }
                cmdCntr++;
                currentTimer = 0;
            }
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("trigs set successfully\n");
            }
            break;
        }

        case(CASE_TX_BUFFER_VAR_ATTEN_TIMINGS):{
            TX->bufferVarAttenTiming(TX,msg->u[1],currentTimer);
            if ( !(cmd->flags.all) ){
                TX->makePioCmd(TX);
                TX->addCmd(TX);
                for(int i=0;i<5;i++){
                    trigs[i].all = 0;
                }
                cmdCntr++;
                currentTimer = 0;
            }
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("var atten set successfully\n");
            }
            break;
        }

        case(CASE_TX_BUFFER_CHARGE_TIME):{
            TX->bufferChargeTime(TX,msg->u[1]);
            if ( !(cmd->flags.all) ){
                TX->makePioCmd(TX);
                TX->addCmd(TX);
                for(int i=0;i<5;i++){
                    trigs[i].all = 0;
                }
                cmdCntr++;
                currentTimer = 0;
            }
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("charge time set successfully\n");
            }
            break;
        }

        case(CASE_TX_BUFFER_PHASE_DELAYS):{
            phase_addr = &(msg->u16[2]);
            TX->bufferPhaseDelays(TX,phase_addr);
            if ( !(cmd->flags.all) ){
                TX->makePioCmd(TX);
                TX->addCmd(TX);
                for(int i=0;i<5;i++){
                    trigs[i].all = 0;
                }
                cmdCntr++;
                currentTimer = 0;
            }
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("phase delays set successfully\n");
            }
            break;
        }

        case(CASE_TX_BUFFER_FIRE_CMD):{
            TX->bufferFireCmd(TX,currentTimer);
            if ( !(cmd->flags.all) ){
                TX->makePioCmd(TX);
                TX->addCmd(TX);
                for(int i=0;i<5;i++){
                    trigs[i].all = 0;
                }
                cmdCntr++;
                currentTimer = 0;
            }
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("fire cmd set successfully\n");
            }
            break;
        }

        case(CASE_TX_BUFFER_RECV_TRIG):{
            TX->bufferRecvTrig(TX,currentTimer);
            if ( !(cmd->flags.all) ){
                TX->makePioCmd(TX);
                TX->addCmd(TX);
                for(int i=0;i<5;i++){
                    trigs[i].all = 0;
                }
                cmdCntr++;
                currentTimer = 0;
            }
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("recv trig set successfully\n");
            }
            break;
        }

        case(CASE_TX_BUFFER_ASYNC_WAIT):{
            timerVal.all = msg->u64[1];
            timeOutVal = timerVal.all+currentTimer;
            if( cmd->flags.isCmd0 ){ 
                cmdCntr++;
                for(int i=0;i<5;i++){
                    trigs[i].all = 0;
                }
            }
            TX->bufferAsyncWait(TX,timeOutVal);
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("async wait set successfully\n");
            }
            break;
        }

        case(CASE_TX_WAIT_CMD):{
            currentTimer += msg->u[1];
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("wait cmd issued successfully\n");
            }
            break;
        }

        case(CASE_TX_SET_NSTEERING_LOCS):{
            TX->setNumSteeringLocs(TX,msg->u[1]);
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("num steering locations set successfully\n");
            }
            break;
        }
        
        case(CASE_TX_CONNECT_INTERRUPT):{
            if(msg->u[1] && ( TX->interrupt->ps == NULL ) ){
                connectPollInterrupter(TX->ps,TX->interrupt,"gpio@0x100000010",TX_INTERRUPT_ID);
            } else if ( !msg->u[1] && ( TX->interrupt->ps != NULL ) ){
                disconnectPollSock(TX->interrupt);
            }
            break;
        }

        case(CASE_TX_SET_EXTERNAL_TRIGGER_MODE):{
            TX->bufferRecvTrig(TX,currentTimer);
            if ( !(cmd->flags.all) ){
                TX->makePioCmd(TX);
                TX->addCmd(TX);
                for(int i=0;i<5;i++){
                    trigs[i].all = 0;
                }
                cmdCntr++;
                currentTimer = 0;
            }
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("recv trig set successfully\n");
            }
            break;
        }
        
        case(CASE_TX_PRINT_FEEDBACK_MSGS):{
            printMsgs = msg->u[1];
            if ( send(TX->comm_sock->fd,&(TX->nSteeringLocs),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("tx print msgs setting updated successfully\n");
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



