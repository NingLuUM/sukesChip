
// struct and typedef prototypes
struct ENETsock_;
struct BOARDsettings_;
struct FPGAvars_;
struct ADCchip_;
struct RCVsys_;
struct TXvars_;
struct loopVars_;
struct loopEnum_;
union loopIncCntr_;
struct TXactions_; 
typedef struct ENETsock_ ENETsock;
typedef struct BOARDsettings_ BOARDsettings;
typedef struct FPGAvars_ FPGAvars;
typedef struct ADCchip_ ADCchip;
typedef struct RCVsys_ RCVsys;
typedef struct TXvars_ TXvars;
typedef struct loopVars_ loopVars_t;
typedef struct loopEnum_ loopEnum_t;
typedef union loopIncCntr_ loopIncCntr_t;
typedef struct TXactions_ TXactions_t;

// function prototypes for ADC
void powerOn_adc(RCVsys *RCV);
void powerOff_adc(RCVsys *RCV);
void sync_adc(RCVsys *RCV);
void initializeSettings_adc(RCVsys *RCV);
void issueDirectSerialCommand_adc(RCVsys *RCV, uint32_t addr, uint32_t cmd);
void setGain_adc(RCVsys *RCV, uint32_t coarseGain, uint32_t fineGain);

// function prototypes for RCV subsystem
void resetVars_rcv(RCVsys *RCV);	
void stateResetFPGA_rcv(RCVsys *RCV);
void setRecLen_rcv(RCVsys *RCV, uint32_t recLen);
void setTrigDelay_rcv(RCVsys *RCV, uint32_t trigDelay);
void adcmemcpy_rcv(char *dest, int32_t volatile *sourceData, size_t nbytes);
void copyDataToMem_rcv(RCVsys *RCV);
void setDataAddrPointers_rcv(RCVsys *RCV, ENETsock **ENET);
void setLocalStorage_rcv(RCVsys *RCV, uint32_t isLocal, uint32_t nPulses);
void allocateLocalStorage_rcv(RCVsys *RCV);
uint32_t getInterruptMsg_rcv(RCVsys *RCV);


// function prototypes for ENET
void setnonblocking_enet(int sock);
void connectInterrupt_intr(ENETsock **INTR, char *gpio_lab, int portnum);
void disconnectInterrupt_intr(ENETsock **INTR);
void addPollSock_enet(ENETsock **ENET, int portNum);
void connectPollSock_enet(ENETsock **ENET, int portNum);
void disconnectSock_enet(ENETsock **ENET, int portNum);
void setPacketSize_enet(ENETsock *ENET, uint32_t packetsize);
void sendAcqdData_enet(ENETsock **ENET, RCVsys *ADC, int portNum);
void setupENETsockFunctionPointers_intr(ENETsock *tmp);
void setupENETsockFunctionPointers_enet(ENETsock *tmp);

typedef struct BOARDsettings_{
	int packetsize;
    int queryTimeout;
    int moduloBoardNum;
	int moduloTimer;
	int packetWait;
	int numPorts;
	int queryMode;
	int boardNum;
} BOARDsettings;

/* TODO: split up 'ENETsock' struct into separate structs
    1) ENET txrx reserved communication channel
    2) Interrupts
    3) ENET send data only
    4) ENET recv tx commands
typedef struct SOCKtype_{
    int sockfd;
    uint8_t sockType;

    void *sockStruct; 
} SOCKtype;

typedef struct INTRsock_{
    int sockfd;
    int is_tx_interrupt; 
    void (*connectInterrupt)(struct ENETsock_ **, char *, int);
    void (*disconnectInterrupt)(struct ENETsock_ **);
    SOCKtype *sockTypeAddr;
	struct INTRsock_ *next;
	struct INTRsock_ *prev;
} INTRsock;

typedef struct ENETcomm_{
    int sockfd;

} ENETcomm;
*/

typedef struct ENETsock_{
	int sockfd;
	int is_rcv_interrupt;
	int is_tx_interrupt;
	int is_commsock;
    int is_tx_recvsock;
	int portNum;
	char *dataAddr;
	int bytesInPacket;
    int is_active;
	struct ENETsock_ *next;
	struct ENETsock_ *prev;
    struct ENETsock_ *commsock;
    
	BOARDsettings *board;

    void (*connectInterrupt)(struct ENETsock_ **, char *, int);
    void (*disconnectInterrupt)(struct ENETsock_ **);
    void (*addPollSock)(struct ENETsock_ **, int);
    void (*connectPollSock)(struct ENETsock **, int);
    void (*disconnectSock)(struct ENETsock_ **, int);
    void (*setPacketSize)(struct ENETsock_ *, uint32_t);
    void (*sendAcqdData)(struct ENETsock_ **, struct RCVsys_ *, char *, int)
} ENETsock;

typedef struct FPGAvars_{ // structure to hold variables that are mapped to the FPGA hardware registers
	void *virtual_base;
	void *axi_virtual_base;
	int fd_pio;
	int fd_ram;	
} FPGAvars;

typedef struct ADCchip_{
	
    // memory mapped variables in FPGA
	uint32_t volatile *serialCommand;
		
	// functions that act on ADC vars
	void (*powerOn)(RCVsys *);
	void (*powerOff)(RCVsys *);
	void (*sync)(RCVsys *);
	void (*initializeSettings)(RCVsys *)
	void (*setGain)(RCVsys *, uint32_t, uint32_t);
	void (*issueDirectSerialCommand)(RCVsys *, uint32_t, uint32_t);

} ADCchip;

typedef struct RCVsys_{
	
	uint32_t volatile *stateReset;
	uint32_t volatile *controlComms;
	uint32_t volatile *recLen;
	uint32_t volatile *trigDelay;
    uint32_t volatile *interrupt0;
	
	int32_t volatile *ramBank0;
	int32_t volatile *ramBank1;
	int32_t volatile *ramBank2;
	
	// local variables on arm proc only
	uint32_t recLen_ref;
	uint32_t trigDelay_ref;
    

	// variables to setup local data acq
	size_t sizeof_bytesPerTimePoint;
	uint32_t isLocal;
	uint32_t nPulses;
	uint32_t currentPulse;
	
	char **data;
	
    ADCchip *ADC;    
	BOARDsettings *board;

	void (*resetVars)(RCVsys *);
	void (*stateResetFPGA)(RCVsys *);
	void (*setRecLen)(RCVsys *, uint32_t);
	void (*setTrigDelay)(RCVsys *, uint32_t);
	void (*copyDataToMem)(RCVsys *, char *);
	void (*setDataAddrPointers)(RCVsys *, ENETsock **, char *);
	void (*setLocalStorage)(RCVsys *, uint32_t, uint32_t);
    void (*allocateLocalStorage)(RCVsys *);
	uint32_t (*getInterruptMsg)(RCVsys *);
} RCVsys;

typedef struct loopEnum_{
    uint16_t logical : 8;
    uint16_t given : 8; 
} loopEnum_t;

typedef struct loopFlags_{
    uint16_t topLevel : 1;
    uint16_t iterator : 1;
    uint16_t fireCmd : 1;
    uint16_t reversed : 1;
} loopFlags_t;

typedef union loopIncCntr_{
    uint32_t loc;
    uint32_t pulse; 
} loopIncCntr_t;

typedef struct TXactions_{

    void (*enterControlLoop)(TXvars *TX, uint32_t);
    void (*updateFireCmdBuffer)(TXvars *TX, uint32_t);
    void (*updateFireAtCmdBuffer)(TXvars *TX, uint32_t);
} TXactions_t;

typedef struct loopVars_{
    loopEnum_t loopNum;
    loopFlags_t is;
    loopIncCntr_t start;
    loopIncCntr_t end;
    int32_t increment;
    loopIncCntr_t current;

    TXactions_t **actionList;
    uint32_t currentAction;
    uint32_t nActions;
     
} loopVars_t;
 
typedef struct TXvars_{
	uint32_t volatile *controlComms;
	uint32_t volatile *channelMask;
	uint32_t volatile *setInstructionReadAddr;
	uint32_t volatile *errorReply;
	uint32_t volatile *errorMsgFromInterrupt;
	uint32_t volatile *errorMsgFromTransducer;
	uint32_t volatile *setTrigRestLevel;
	uint32_t volatile *setTrig;
	uint32_t volatile *setLed;
	uint32_t volatile *interrupt0;
	uint32_t volatile *interrupt1;
	
	uint32_t volatile **fireCmdPhaseCharge;
	uint32_t volatile *fireCmdPhaseCharge0;
	uint32_t volatile *fireCmdPhaseCharge1;
	uint32_t volatile *fireCmdPhaseCharge2;
	uint32_t volatile *fireCmdPhaseCharge3;
	uint32_t volatile *fireCmdPhaseCharge4;
	uint32_t volatile *fireCmdPhaseCharge5;
	uint32_t volatile *fireCmdPhaseCharge6;
	uint32_t volatile *fireCmdPhaseCharge7;
	
	uint32_t volatile **fireAtCmdPhaseCharge;
	uint32_t volatile *fireAtCmdPhaseCharge0;
	uint32_t volatile *fireAtCmdPhaseCharge1;
	uint32_t volatile *fireAtCmdPhaseCharge2;
	uint32_t volatile *fireAtCmdPhaseCharge3;
	uint32_t volatile *fireAtCmdPhaseCharge4;
	uint32_t volatile *fireAtCmdPhaseCharge5;
	uint32_t volatile *fireAtCmdPhaseCharge6;
	uint32_t volatile *fireAtCmdPhaseCharge7;
	
	uint16_t volatile *instructionTypeReg;
	uint32_t volatile *instructionReg;
	uint32_t volatile *timingReg;
	uint32_t volatile *loopAddressReg;
	uint32_t volatile *loopCounterReg;

	
	uint16_t **instructionTypeReg_local;
	uint32_t **instructionReg_local;
	uint32_t **timingReg_local;
	uint32_t **loopAddressReg_local;
	uint32_t **loopCounterReg_local;

    uint32_t recvType;
    uint32_t nLocs;
    uint32_t nInstructions;
    
    char **phaseChargeBuff;
    char **instructionBuff;

    loopVars_t **loopDefs;
    loopVars_t ***loops;
	
	void (*resetTX_vars)(struct TXvars *);
    void (*setChannelMask)(struct TXvars *, uint32_t);
    void (*setupLoops)(struct TXvars *);
} TXvars;


typedef struct UserProgram_{
	int instructionsCount;
	int fireCmdPhaseChargesCount;
	int fireAtCmdPhaseChargesCount;
	uint32_t *instructionList;
	uint16_t *instructionTypeList;
	uint32_t *instructionTiming;
	uint32_t *fireCmdPhaseCharge;
	uint32_t *fireAtCmdPhaseCharge;
} UserProgram;


void FPGAclose(FPGAvars *FPGA){ // closes the memory mapped file with the FPGA hardware registers
	
	if( munmap( FPGA->virtual_base, HW_REGS_SPAN ) != 0 ) {
		printf( "ERROR: munmap() failed...\n" );
		close( FPGA->fd_pio );
	}
	if( munmap( FPGA->axi_virtual_base, HW_FPGA_AXI_SPAN ) != 0 ) {
		printf( "ERROR: munmap() failed...\n" );
		close( FPGA->fd_ram );
	}

	close( FPGA->fd_pio );
	close( FPGA->fd_ram );
}


int FPGA_init(FPGAvars *FPGA){ // maps the FPGA hardware registers to the variables in the FPGAvars struct
	
	if( ( FPGA->fd_pio = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) {
		printf( "ERROR: could not open \"/dev/mem\"...\n" );
		return( 0 );
	}
	
	FPGA->virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, FPGA->fd_pio, HW_REGS_BASE );

	
	if( FPGA->virtual_base == MAP_FAILED ) {
		printf( "ERROR: mmap() failed...\n" );
		close( FPGA->fd_pio );
		return( 0 );
	}
	
	if( ( FPGA->fd_ram = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) {
		printf( "ERROR: could not open \"/dev/mem\"...\n" );
		return( 0 );
	}
	
	FPGA->axi_virtual_base = mmap( NULL, HW_FPGA_AXI_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, FPGA->fd_ram,ALT_AXI_FPGASLVS_OFST );

	if( FPGA->axi_virtual_base == MAP_FAILED ) {
		printf( "ERROR: axi mmap() failed...\n" );
		close( FPGA->fd_ram );
		return( 0 );
	}

	return(1);
}


int ADC_setup(FGPAvars *FPGA, ADCchip *ADC){

	ADC->serialCommand = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_SERIAL_COMMAND_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    ADC->powerOn = &powerOn_adc;
    ADC->powerOff = &powerOff_adc;
    ADC->sync = &sync_adc;
    ADC->initializeSettings = &initializeSettings_adc;
    ADC->setGain = &setGain_adc;
    ADC->issueDirectSerialCommand = &issueDirectSerialCommand_adc;

    return(1);
}


int RCV_setup(FPGAvars *FPGA, RCVsys *RCV, ADCchip *ADC, BOARDsettings *board){
	
    RCV->ADC = ADC;
    RCV->board = board;

	RCV->stateReset = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_FPGA_STATE_RESET_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	RCV->controlComms = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_CONTROL_COMMS_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	RCV->recLen = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_SET_ADC_RECORD_LENGTH_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	RCV->trigDelay = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_SET_ADC_TRIG_DELAY_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	RCV->interrupt0 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_INTERRUPT_GENERATOR_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	RCV->ramBank0 = FPGA->axi_virtual_base + ( ( uint32_t  )( ADC_RAMBANK0_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );
	RCV->ramBank1 = FPGA->axi_virtual_base + ( ( uint32_t  )( ADC_RAMBANK1_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );
	RCV->ramBank2 = FPGA->axi_virtual_base + ( ( uint32_t  )( ADC_RAMBANK2_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );

    RCV->data = (char **)malloc(sizeof(char *));
    RCV->resetVars = &resetVars_rcv;
    RCV->stateResetFPGA = &stateResetFPGA_rcv;
    RCV->setRecLen = &setRecLen_rcv;
    RCV->setTrigDelay = &setTrigDelay_rcv;
    RCV->copyDataToMem = &copyDataToMem_rcv;
    RCV->setDataAddrPointers = &setDataAddrPointers_rcv;
    RCV->setLocalStorage = &setLocalStorage_rcv;
    RCV->allocateLocalStorage = &allocateLocalStorage_rcv;
    RCV->getInterruptMsg = &getInterruptMsg_rcv;
	
	return(1);
}


int TX_init(FPGAvars *FPGA, TXvars *TX){
	
	TX->interrupt0 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_TX_INTERRUPT_GENERATOR_0_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->interrupt1 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_TX_INTERRUPT_GENERATOR_1_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	
	TX->controlComms = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_TXCONTROLCOMMS_BASE ) & ( uint32_t )( HW_REGS_MASK ) );	
	TX->channelMask = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_SET_TXCHANNELMASK_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	
	TX->setLed = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_LED_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->setTrig = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_TRIG_VAL_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->setTrigRestLevel = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_TRIG_REST_LEVELS_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	TX->fireCmdPhaseCharge = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIRE_PHASECHARGE_CH0_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireCmdPhaseCharge0 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIRE_PHASECHARGE_CH0_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireCmdPhaseCharge1 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIRE_PHASECHARGE_CH1_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireCmdPhaseCharge2 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIRE_PHASECHARGE_CH2_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireCmdPhaseCharge3 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIRE_PHASECHARGE_CH3_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireCmdPhaseCharge4 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIRE_PHASECHARGE_CH4_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireCmdPhaseCharge5 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIRE_PHASECHARGE_CH5_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireCmdPhaseCharge6 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIRE_PHASECHARGE_CH6_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireCmdPhaseCharge7 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIRE_PHASECHARGE_CH7_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	TX->fireAtCmdPhaseCharge = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIREAT_PHASECHARGE_CH0_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireAtCmdPhaseCharge0 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIREAT_PHASECHARGE_CH0_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireAtCmdPhaseCharge1 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIREAT_PHASECHARGE_CH1_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireAtCmdPhaseCharge2 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIREAT_PHASECHARGE_CH2_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireAtCmdPhaseCharge3 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIREAT_PHASECHARGE_CH3_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireAtCmdPhaseCharge4 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIREAT_PHASECHARGE_CH4_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireAtCmdPhaseCharge5 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIREAT_PHASECHARGE_CH5_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireAtCmdPhaseCharge6 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIREAT_PHASECHARGE_CH6_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->fireAtCmdPhaseCharge7 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + FIREAT_PHASECHARGE_CH7_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	TX->errorMsgFromTransducer = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_OUTPUT_ERROR_MSG_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->errorMsgFromInterrupt = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_INTERRUPT_ERROR_MSG_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->errorReply = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_ERROR_COMMS_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	TX->setInstructionReadAddr = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_SET_INSTRUCTION_READ_ADDR_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	TX->instructionTypeReg = FPGA->axi_virtual_base + ( ( uint32_t  )( TX_INSTRUCTIONTYPEREGISTER_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );
	TX->instructionReg = FPGA->axi_virtual_base + ( ( uint32_t  )( TX_INSTRUCTIONREGISTER_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );
	TX->timingReg = FPGA->axi_virtual_base + ( ( uint32_t  )( TX_TIMINGREGISTER_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );
	TX->loopAddressReg = FPGA->axi_virtual_base + ( ( uint32_t  )( TX_LOOPADDRESSREGISTER_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );
	TX->loopCounterReg = FPGA->axi_virtual_base + ( ( uint32_t  )( TX_LOOPCOUNTERREGISTER_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );
	
	TX->instructionReg_local 		= (uint32_t *)malloc( TX_INSTRUCTIONREGISTER_SPAN );
	TX->instructionTypeReg_local 	= (uint16_t *)malloc( TX_INSTRUCTIONTYPEREGISTER_SPAN );
	TX->timingReg_local 			= (uint32_t *)malloc( TX_TIMINGREGISTER_SPAN );
	TX->loopAddressReg_local		= (uint32_t *)malloc( TX_LOOPADDRESSREGISTER_SPAN );
	TX->loopCounterReg_local		= (uint32_t *)malloc( TX_LOOPCOUNTERREGISTER_SPAN );

    TX->recvBuff = (char **)malloc(sizeof(char *));
    *(TX->recvBuff) = (char *)malloc(MAX_ENET_TRANSMIT_SIZE*sizeof(char));

	memset(TX->instructionReg_local,0,TX_INSTRUCTIONREGISTER_SPAN);
	memset(TX->instructionTypeReg_local,0,TX_INSTRUCTIONTYPEREGISTER_SPAN);
	memset(TX->timingReg_local,0,TX_TIMINGREGISTER_SPAN);
	memset(TX->loopAddressReg_local,0,TX_LOOPADDRESSREGISTER_SPAN);
	memset(TX->loopCounterReg_local,0,TX_LOOPCOUNTERREGISTER_SPAN);
	
	DREF32(TX->channelMask) = 0xff;
	DREF32(TX->controlComms) = 0;
	return(1);
}


void loadBoardData(RCVsys *RCV, TXvars *TX){ // load the boards specific data from files stored on SoC		
	char const* const fileName = "boardData";
    FILE* file = fopen(fileName, "r");
    char line[256];
	int n=0;
	uint32_t boardData[10];

    while( fgets(line, sizeof(line), file) && n<4 ){
        boardData[n] = atoi(line);
        n++;    
    }  
    fclose(file);
    RCV->board->boardNum = boardData[0];
}












