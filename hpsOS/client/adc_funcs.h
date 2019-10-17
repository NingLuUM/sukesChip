


void adcSync(ADCvars_t *ADC){
	DREF32(ADC->controlComms) = ADC_SYNC_COMMAND;
	usleep(5);
}

void adcIssueSerialCmd(ADCvars_t *ADC, uint32_t cmd){
	DREF32(ADC->serialCommand) = cmd;
	usleep(5);
	DREF32(ADC->controlComms) = ADC_BUFFER_SERIAL_COMMAND;
	usleep(5);
	DREF32(ADC->controlComms) = ADC_ISSUE_SERIAL_COMMAND;
	usleep(100);
	DREF32(ADC->controlComms) = ADC_IDLE_STATE;
	usleep(5);
}

void adcSetDefaultSettings(ADCvars_t *ADC){
	
	DREF32(ADC->controlComms) = ADC_HARDWARE_RESET;
	usleep(100000);
	DREF32(ADC->controlComms) = ADC_IDLE_STATE;
	usleep(10);
	
	ADC->gpreg0.SOFTWARE_RESET = 1;
	ADC->issueSerialCommand(ADC,ADC->gpreg0.adccmd);
	usleep(100000);
	
	ADC->gpreg0.SOFTWARE_RESET = 0;
	ADC->gpreg0.TGC_REGISTER_WREN = 1;
	ADC->issueSerialCommand(ADC,ADC->gpreg0.adccmd);

	// WITHOUT INTERP_ENABLE=1, THIS ONLY ALLOWS COARSE_GAIN TO BE SET
	ADC->tgcreg0x99.STATIC_PGA = 1;
	ADC->issueSerialCommand(ADC,ADC->tgcreg0x99.adccmd);
	
	// NOT IN MANUAL!!!, NEEDED FOR FINE GAIN CONTROL
	ADC->tgcreg0x97.INTERP_ENABLE = 1;
	ADC->issueSerialCommand(ADC,ADC->tgcreg0x97.adccmd);
	
    // GO TO GENERAL PURPOSE REGISTERS
	ADC->gpreg0.TGC_REGISTER_WREN = 0;
	ADC->issueSerialCommand(ADC,ADC->gpreg0.adccmd);
	
    // SET TO DC COUPLING (VARIABE NAME IS MISLEADING/BACKWARDS)
    ADC->gpreg7.INTERNAL_AC_COUPLING = 1;
	ADC->issueSerialCommand(ADC,ADC->gpreg7.adccmd);

    // SET DATA TYPE TO UNSIGNED
    ADC->gpreg4.DFS = 1;
    ADC->issueSerialCommand(ADC,ADC->gpreg4.adccmd);
    
    // DISABLE THE CLAMP!!!
    ADC->gpreg70.CLAMP_DISABLE = 1;
    ADC->issueSerialCommand(ADC,ADC->gpreg70.adccmd);
}

void adcSetGain(ADCvars_t *ADC, double gainVal){
	
	uint32_t coarseGain, fineGain;
	
    gainVal += 5.0;
    coarseGain = (uint32_t )(gainVal);
    fineGain = (uint32_t )((gainVal-floor(gainVal))*8);

	//~ // set TGC_REG_WREN = 1 to access gain controls
	if( !(ADC->gpreg0.TGC_REGISTER_WREN) ){
		ADC->gpreg0.TGC_REGISTER_WREN = 1;
		ADC->issueSerialCommand(ADC,ADC->gpreg0.adccmd);
	}
	
    ADC->tgcreg0x9A.COARSE_GAIN = coarseGain;
	ADC->issueSerialCommand(ADC,ADC->tgcreg0x9A.adccmd);
    printf("ADC->tgcreg0x9A.COARSE_GAIN:\n\t0x%02x (%u) ->  gain: %g [%g]\n\n",
        ADC->tgcreg0x9A.COARSE_GAIN, ADC->tgcreg0x9A.COARSE_GAIN, gainVal-5.0, gainVal);
	
    ADC->tgcreg0x99.FINE_GAIN = fineGain;
	ADC->issueSerialCommand(ADC,ADC->tgcreg0x99.adccmd);
    printf("ADC->tgcreg0x99.FINE_GAIN:\n\t0x%02x (%u) -> gain: %g [%g]\n\n",
        ADC->tgcreg0x99.FINE_GAIN,ADC->tgcreg0x99.FINE_GAIN, gainVal-5.0,gainVal);
	
	
}

void adcSetUnsignedInt(ADCvars_t *ADC, uint32_t val){
	// set TGC_REG_WREN = 0 to return to gen purpose register map
	if( ADC->gpreg0.TGC_REGISTER_WREN ){
		ADC->gpreg0.TGC_REGISTER_WREN = 0;
		ADC->issueSerialCommand(ADC,ADC->gpreg0.adccmd);
	}
	
	// set dtype as signed (val=0), unsigned (val=1) int
	if(val){
		ADC->gpreg4.DFS = 1;
	} else {
		ADC->gpreg4.DFS = 0;
	}	
    printf("ADC->gpreg4.DFS: (SIGNED/UNSIGNED)\n\t0x%02x\n\n",ADC->gpreg4.DFS);
	ADC->issueSerialCommand(ADC,ADC->gpreg4.adccmd);
}

void adcSetLowNoiseMode(ADCvars_t *ADC, uint32_t val){
	
	if( ADC->gpreg0.TGC_REGISTER_WREN ){
		ADC->gpreg0.TGC_REGISTER_WREN = 0;
		ADC->issueSerialCommand(ADC,ADC->gpreg0.adccmd);
	}
	
	if(val){
		ADC->gpreg7.VCA_LOW_NOISE_MODE = 1;
	} else {
		ADC->gpreg7.VCA_LOW_NOISE_MODE = 0;
	}
	
    printf("ADC->gpreg7.VCA_LOW_NOISE_MODE:\n\t0x%02x\n\n",ADC->gpreg7.VCA_LOW_NOISE_MODE);
	ADC->issueSerialCommand(ADC,ADC->gpreg7.adccmd);
}

void adcToggleChannelPwr(ADCvars_t *ADC, uint32_t pwrOn){
	
	if( ADC->gpreg0.TGC_REGISTER_WREN ){
		ADC->gpreg0.TGC_REGISTER_WREN = 0;
		ADC->issueSerialCommand(ADC,ADC->gpreg0.adccmd);
	}
	
	ADC->gpreg1.PDN_CHANNEL = pwrOn;	
    printf("ADC->gpreg1.PDN_CHANNEL:\n\t0x%02x\n",ADC->gpreg1.PDN_CHANNEL);
    printf(
        "\n\tPDN_CHANNEL<1,...,8> = <%u, %u, %u, %u, %u, %u, %u, %u>\n", 
        ADC->gpreg1.PDN_CHANNEL1, ADC->gpreg1.PDN_CHANNEL2, 
        ADC->gpreg1.PDN_CHANNEL3, ADC->gpreg1.PDN_CHANNEL4, 
        ADC->gpreg1.PDN_CHANNEL5, ADC->gpreg1.PDN_CHANNEL6, 
        ADC->gpreg1.PDN_CHANNEL7, ADC->gpreg1.PDN_CHANNEL8);
	ADC->issueSerialCommand(ADC,ADC->gpreg1.adccmd);
}

void adcSetFilterBW(ADCvars_t *ADC, uint32_t filter){	
	if( ADC->gpreg0.TGC_REGISTER_WREN ){
		ADC->gpreg0.TGC_REGISTER_WREN = 0;
		ADC->issueSerialCommand(ADC,ADC->gpreg0.adccmd);
	}
	
	ADC->gpreg7.FILTER_BW = filter;
    printf("ADC->gpreg7.FILTER_BW:\n\t0x%02x\n\n",ADC->gpreg7.FILTER_BW);
	ADC->issueSerialCommand(ADC,ADC->gpreg7.adccmd);
}

void adcSetInternalAcCoupling(ADCvars_t *ADC, uint32_t accoupling){	
	if( ADC->gpreg0.TGC_REGISTER_WREN ){
		ADC->gpreg0.TGC_REGISTER_WREN = 0;
		ADC->issueSerialCommand(ADC,ADC->gpreg0.adccmd);
	}
	
	ADC->gpreg7.INTERNAL_AC_COUPLING = accoupling;
    printf("ADC->gpreg7.INTERNAL_AC_COUPLING:\n\t0x%02x\n\n",ADC->gpreg7.INTERNAL_AC_COUPLING);
	ADC->issueSerialCommand(ADC,ADC->gpreg7.adccmd);
}

void adcIssueDirectCmd(ADCvars_t *ADC, FMSG_t *msg){
    uint32_t regaddr;
    if( msg->u[1] ){
        ADC->gpreg0.TGC_REGISTER_WREN = 1;
        ADC->issueSerialCommand(ADC,ADC->gpreg0.adccmd);

        regaddr = ADC->reg_dict[1][msg->u[2]];
        printf("regaddr = %u\n",regaddr);
        ADC->reg[regaddr]->cmd = msg->u[3];
        ADC->issueSerialCommand(ADC,ADC->reg[regaddr]->adccmd);
    } else {
        ADC->gpreg0.TGC_REGISTER_WREN = 0;
        ADC->issueSerialCommand(ADC,ADC->gpreg0.adccmd);
    
        regaddr = ADC->reg_dict[0][msg->u[2]];
        ADC->reg[regaddr]->cmd = msg->u[3];
        ADC->issueSerialCommand(ADC,ADC->reg[regaddr]->adccmd);
    }
}

