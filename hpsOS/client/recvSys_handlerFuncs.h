

void rcvSysMsgHandler(RCVsys_t *RCV, FMSG_t *msg, int *runner){

    static uint32_t printMsgs = 0;

    switch(msg->u[0]){
        case(CASE_RCV_SET_RECLEN):{
            RCV->setRecLen(RCV,msg->u[1]);
            if( msg->u[2] ){
                printf("hello from here\n");
				setupInternalStorage(RCV);
                printf("hello from here\n");
			}
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("reclen set successfully\n");
            }
            break;
        }
        
        case(CASE_RCV_SET_PIO_VAR_GAIN):{
            RCV->setPioVarGain(RCV,msg->u[1]);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("pio gain set successfully\n");
            }
            break;
        }
        
        case(CASE_SET_LEDS):{
            RCV->setLEDs(RCV,msg->u[1]);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("comm leds set successfully\n");
            }
            break;
        }
        
        case(CASE_RCV_STATE_RESET):{
            RCV->setStateReset(RCV,msg->u[1]);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("state reset set successfully\n");
            }
            break;
        }

        case(CASE_ADC_SET_GAIN):{
            RCV->ADC->setGain(RCV->ADC,msg->d[1]);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("adc gain set successfully\n");
            }
            break;
        }
        
        case(CASE_ADC_SET_UNSIGNED):{
            RCV->ADC->setUnsignedInt(RCV->ADC, msg->u[1]);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("adc unsigned mode set successfully\n");
            }
            break;
        }
        
        case(CASE_ADC_SET_LOW_NOISE_MODE):{
            RCV->ADC->setLowNoiseMode(RCV->ADC, msg->u[1]);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("adc low noise mode cmd set successfully\n");
            }
            break;
        }
        
        case(CASE_ADC_TOGGLE_CHANNEL_POWER):{
            RCV->ADC->toggleChannelPower(RCV->ADC,msg->u[1]);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("adc channel power set successfully\n");
            }
            break;
        }
        
        case(CASE_ADC_SET_FILTER_BW):{
            RCV->ADC->setFilterBW(RCV->ADC,msg->u[1]);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("adc filter bandwidth set successfully\n");
            }
            break;
        }
        
        case(CASE_ADC_SET_INTERNAL_AC_COUPLING):{
            RCV->ADC->setInternalAcCoupling(RCV->ADC,msg->u[1]);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("adc internal ac coupling cmd set successfully\n");
            }
            break;
        }
        
        case(CASE_ADC_ISSUE_DIRECT_CMD):{
            RCV->ADC->issueDirectCommand(RCV->ADC, msg);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("adc issue direct command set successfully\n");
            }
            break;
        }
        
        case(CASE_RCV_CONNECT_INTERRUPT):{
            if(msg->u[1] && ( RCV->interrupt->ps == NULL ) ){
                connectPollInterrupter(RCV->ps,RCV->interrupt,"gpio@0x100000000",RCV_INTERRUPT_ID);
            } else if ( !msg->u[1] && ( RCV->interrupt->ps != NULL ) ){
                disconnectPollSock(RCV->interrupt);
            }
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) ){
                printf("rcv interrupt connected successfully\n");
            }
            break;
        }
        
        case(CASE_RCV_SET_QUERY_MODE):{
            RCV->queryMode.all = 0;
            
            if(msg->u[1]){
                RCV->queryMode.realTime = 1;
            } else {
                RCV->queryMode.realTime = 0;
            }
            
            if(msg->u[2]){
                RCV->queryMode.transferData = 1;
            } else {
                RCV->queryMode.transferData = 0;
            }

            if(msg->u[3]){
                RCV->queryMode.saveDataFile = 1;
            } else {
                RCV->queryMode.saveDataFile = 0;
            }

            if(msg->u[4]){
                RCV->queryMode.is16bit = 1;
            } else {
                RCV->queryMode.is16bit = 0;
            }

            if(msg->u[5]){
                RCV->queryMode.sendOnRequest = 1;
            } else {
                RCV->queryMode.sendOnRequest = 0;
            }
            //printf("queryMode: rt %d, td %d, sdf %d, -- 16bit: %d\n",RCV->queryMode.realTime,RCV->queryMode.transferData,RCV->queryMode.saveDataFile,RCV->queryMode.is16bit);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("rcv query mode set successfully\n");
            }
            break;
        }
        
        case(CASE_RCV_SET_NPULSES):{
            if( msg->u[1] ){
                RCV->npulses = msg->u[1];
            } else {
                RCV->npulses = 1;
            }
            if( msg->u[2] ){
                printf("hello from there\n");
				setupInternalStorage(RCV);
                printf("hello from there\n");
			}
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("rcv npulses set successfully\n");
            }
            break;
        }
        
        case(CASE_RCV_SETUP_LOCAL_STORAGE):{
            
            printf("hello from where\n");
            setupInternalStorage(RCV);
            printf("hello from where\n");
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("rcv internal storage set successfully\n");
            }
            break;
        }
        
        case(CASE_ADC_SET_DEFAULT_SETTINGS):{
            RCV->ADC->setDefaultSettings(RCV->ADC);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("adc defaults set successfully\n");
            }
            break;
        }
        
        case(CASE_UPDATE_AUTO_SHUTDOWN_SETTING):{
            if(msg->u[1]){
                g_auto_shutdown_enabled = 1;
            } else {
                g_auto_shutdown_enabled = 0;
            }
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("system auto shutdown settings updated successfully\n");
            }
            break;
        }
        
        case(CASE_RCV_SET_CLOCK_DIVISOR):{
            RCV->setClkDivisor(RCV,msg->u[1]);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("rcv sys clock divisor set successfully\n");
            }
            break;
        }
        
        case(CASE_RCV_SET_SAMPLING_MODE):{
            RCV->setSamplingMode(RCV,msg->u[1]);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("rcv sys sampling mode set successfully\n");
            }
            break;
        }
        
        case(CASE_RCV_SET_COMPRESSOR_MODE):{
            RCV->setCompressorMode(RCV,msg->u[1]);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("rcv sys compressor mode set successfully\n");
            }
            break;
        }
        
        case(CASE_RCV_INTERRUPT_THYSELF):{
            RCV->refVals.pioSettings.interrupt_thyself = msg->u[1];
            DREF32(RCV->pioSettings) = RCV->refVals.pioSettings.all;
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("self interrupt set successfully\n");
            }
            break;
        }
 
        case(CASE_ADC_SET_POWER_ON_OFF):{
            RCV->ADC->setPowerState(RCV->ADC,msg->u[1]);
            printf("what the hell\n");
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("adc power state set successfully\n");
            }
            break;
        }

        case(CASE_ADC_DISABLE_CLAMP):{
            RCV->ADC->disableClamp(RCV->ADC,msg->u[1]);
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("adc clamp disable state set successfully\n");
            }
            break;
        }

        case(CASE_EXIT_PROGRAM):{
            *runner=0;
            break;
        }
       
		case(CASE_ADC_SET_FCLOCK_DELAY):{
			RCV->refVals.pioSettings.fclk_delay = msg->u[1];
			DREF32(RCV->pioSettings) = RCV->refVals.pioSettings.all;
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("adc fclock delay set successfully\n");
            }
			break;
		}

        case(CASE_RCV_PRINT_FEEDBACK_MSGS):{
            printMsgs = msg->u[1];
            if ( send(RCV->comm_sock->fd,&(RCV->npulses),sizeof(uint32_t),MSG_CONFIRM) && printMsgs ){
                printf("rcv print msgs updated successfully\n");
            }
            break;
        }
		
        default:{
            *runner=0;
            break;
        }
    }
}



