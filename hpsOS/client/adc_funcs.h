
	
void powerOn_adc(RCVsys *RCV){
	DREF32(RCV->controlComms) = ADC_POWER_ON;
	usleep(10);
}

void powerOff_adc(RCVsys *RCV){
	DREF32(RCV->controlComms) = ADC_POWER_OFF;
	usleep(10);
}

void sync_adc(RCVsys *RCV){
	DREF32(RCV->controlComms) = ADC_SYNC_COMMAND;
	usleep(10);
	DREF32(RCV->controlComms) = ADC_IDLE_STATE;
	usleep(10);
}

void initializeSettings_adc(RCVsys *RCV){
	//~ // turn on ADC
	DREF32(RCV->controlComms) = ADC_POWER_ON;
	usleep(100000);
	
	// software reset, just to be safe
	DREF32(RCV->ADC->serialCommand) = ( ( ADC_SOFTWARE_RESET_ADDR << 16 ) | ADC_SOFTWARE_RESET_CMD );
	usleep(10);
	DREF32(RCV->controlComms) = ADC_BUFFER_SERIAL_COMMAND;
	usleep(10);
	DREF32(RCV->controlComms) = ADC_ISSUE_SERIAL_COMMAND;
	usleep(100);
	DREF32(RCV->controlComms) = ADC_IDLE_STATE;
	usleep(10);
	
	// set tgc, allows setting of gain
	DREF32(RCV->ADC->serialCommand) = ( ( ADC_SET_TGC_ADDR << 16 ) | ADC_SET_TGC_CMD );
	usleep(10);
	DREF32(RCV->controlComms) = ADC_BUFFER_SERIAL_COMMAND;
	usleep(10);
	DREF32(RCV->controlComms) = ADC_ISSUE_SERIAL_COMMAND;
	usleep(100);
	DREF32(RCV->controlComms) = ADC_IDLE_STATE;
	usleep(10);

	// declare the gain to be constant with time
	DREF32(RCV->ADC->serialCommand) = ( ( ADC_SET_FIXED_GAIN_ADDR << 16 ) |  ADC_SET_FIXED_GAIN_CMD );
	usleep(10);
	DREF32(RCV->controlComms) = ADC_BUFFER_SERIAL_COMMAND;
	usleep(10);
	DREF32(RCV->controlComms) = ADC_ISSUE_SERIAL_COMMAND;
	usleep(100);
	DREF32(RCV->controlComms) = ADC_IDLE_STATE;
	usleep(10);
}

void setGain_adc(RCVsys *RCV, uint32_t coarseGain, uint32_t fineGain){
	
	DREF32(RCV->ADC->serialCommand) = ( ( ADC_SET_COARSE_GAIN_ADDR << 16 ) | ADC_SET_COARSE_GAIN_CMD( coarseGain ) );
	usleep(10);
	DREF32(RCV->controlComms) = ADC_BUFFER_SERIAL_COMMAND;
	usleep(10);
	DREF32(RCV->controlComms) = ADC_ISSUE_SERIAL_COMMAND;
	usleep(100);
	DREF32(RCV->controlComms) = ADC_IDLE_STATE;
	usleep(100);
	
	DREF32(RCV->ADC->serialCommand) = ( ( ADC_SET_FINE_GAIN_ADDR << 16 ) | ADC_SET_FINE_GAIN_CMD( fineGain ) );
	usleep(10);
	DREF32(RCV->controlComms) = ADC_BUFFER_SERIAL_COMMAND;
	usleep(10);
	DREF32(RCV->controlComms) = ADC_ISSUE_SERIAL_COMMAND;
	usleep(100);
	DREF32(RCV->controlComms) = ADC_IDLE_STATE;
	usleep(10);
}

void issueDirectSerialCommand_adc(RCVsys *RCV, uint32_t addr, uint32_t cmd){

	
	DREF32(RCV->ADC->serialCommand) = ( ( addr << 16 ) | cmd );
	usleep(10);
	DREF32(RCV->controlComms) = ADC_BUFFER_SERIAL_COMMAND;
	usleep(10);
	DREF32(RCV->controlComms) = ADC_ISSUE_SERIAL_COMMAND;
	usleep(100);
	DREF32(RCV->controlComms) = ADC_IDLE_STATE;
	usleep(10);
	
}

void ADC_Settings(RCVsys *RCV, ENETsock_t **ENET, ENETsock_t *INTR, uint32_t *msg){
	
	switch(msg[0]){
		
		case(CASE_ADC_POWER):{
			if(msg[1] == 1){
				INTR->connectInterrupt(&INTR,"gpio@0x100000020",2); // adc interrupt
				RCV->ADC->powerOn(RCV);
				printf("lvds power = on");
			} else {
				RCV->ADC->powerOff(RCV);
				INTR->disconnectSock(&INTR,2); // adc interrupt
				printf("lvds power = off (%u)\n",msg[1]);
			}
			break;
		}
		
		case(CASE_ADC_SYNC):{
			RCV->ADC->sync(RCV);
			break;
		}
		
		case(CASE_ADC_INITIALIZE):{		
			RCV->ADC->initializeSettings(RCV);	
			break;
		}
		
		case(CASE_ADC_GAIN):{
			RCV->ADC->setGain(RCV,msg[1],msg[2]);
			printf("gain -- enetmsg[1]: %d, enetmsg[2]: %d\n",msg[1], msg[2]);
			break;
		}
			
		case(CASE_ADC_DIRECT_SERIAL_COMMAND):{
			RCV->ADC->issueDirectSerialCommand(RCV,msg[1],msg[2]);
			printf("direct serial command msg[1]=%d, msg[2]=%d\n", msg[1], msg[2]);
			break;
		}
		
		default:{
			printf("default case ADC_settings, doing nothing\n");
            break;
		}
		
	}
}










