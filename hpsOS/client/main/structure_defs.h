
// struct and typedef prototypes
struct ENETsock_;
struct IPCsock_;
struct BOARDdata_;
struct FPGAvars_;
struct ADCchip_;
struct RCVsys_;
struct TXsys_;
struct TX_PhaseDelayReg_;
struct TX_SteeringLoopDefs_;
typedef struct ENETsock_ ENETsock_t;
typedef struct IPCsock_ IPCsock_t;
typedef struct BOARDdata_ BOARDdata;
typedef struct FPGAvars_ FPGAvars;
typedef struct ADCchip_ ADCchip;
typedef struct RCVsys_ RCVsys;
typedef struct TXsys_ TXsys;
typedef struct TX_PhaseDelayReg_ TX_PhaseDelayReg_t;
typedef struct TX_SteeringLoopDefs_ TX_SteeringLoopDefs_t;

union TX_InputCommands_;
union TX_InstructionReg_;
typedef union TX_InputCommands_ TX_InputCommands_t;
typedef union TX_InstructionReg_ TX_InstructionReg_t;

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
void setDataAddrPointers_rcv(RCVsys *RCV, ENETsock_t **ENET);
void setLocalStorage_rcv(RCVsys *RCV, uint32_t isLocal, uint32_t nPulses);
void allocateLocalStorage_rcv(RCVsys *RCV);
uint32_t getInterruptMsg_rcv(RCVsys *RCV);

// function prototypes for ENET
void setnonblocking_enet(int sock);
void connectInterrupt_intr(ENETsock_t **INTR, char *gpio_lab, int portnum);
void disconnectInterrupt_intr(ENETsock_t **INTR);
void addEnetSock_enet(ENETsock_t **ENET, int portNum, int makeCommSock);
void connectEnetSock_enet(ENETsock_t **ENET, int portNum);
void disconnectSock_enet(ENETsock_t **ENET, int portNum);
void setPacketSize_enet(ENETsock_t *ENET, uint32_t packetsize);
void sendAcqdData_enet(ENETsock_t **ENET, RCVsys *ADC, int portNum);
void setupENETsockFunctionPointers_intr(ENETsock_t *tmp);
void setupENETsockFunctionPointers_enet(ENETsock_t *tmp);


typedef struct BOARDdata_{
	int boardNum;
	int elements_in_array;
	int *subelement_list;
	float *element_coords;
} BOARDdata;


typedef struct SOCKgeneric_{
	int fd;
	
	union{
		
		struct{
			uint16_t enetCommSock : 1;
			uint16_t enetRecvLargeBuffer : 1;
			uint16_t interruptTx : 1;
			uint16_t interruptRcv : 1;
			uint16_t ipcServer : 1;
			uint16_t ipcGeneric : 1;
			uint16_t ipcTx : 1;
			uint16_t ipcRx : 1;
			uint16_t active : 1;
		} is;
		
		uint16_t isFlags;
	};
	
	void *parent;
	
} SOCKgeneric_t;


typedef struct ENETsettings_{
	uint16_t packetsize;
	uint16_t queryMode;
	
}ENETsettings_t;


typedef struct ENETsock_{
	
	SOCKgeneric_t sock;
	
	int *epfd;
	struct epoll_event *ev;
	struct epoll_event *events;
	
	ENETsettings_t *settings;

	int portNum;
	char *dataAddr;
	int bytesInPacket;
    
	ENETsock_t *next;
	ENETsock_t *prev;
    ENETsock_t *commsock;

	void (*connectInterrupt)(ENETsock_t **, char *, int);
    void (*disconnectInterrupt)(ENETsock_t **);
    void (*addEnetSock)(ENETsock_t **, int, int);
    void (*connectEnetSock)(ENETsock_t **, int);
    void (*disconnectSock)(ENETsock_t **, int);
    void (*setPacketSize)(ENETsock_t *, uint32_t);
    //~ void (*sendAcqdData)(ENETsock_t **, struct RCVsys_ *, char *, int);
} ENETsock_t;


typedef struct INTRsock_{
	
	SOCKgeneric_t sock;
	
    void (*connectInterrupt)(ENETsock_t **, char *, int);
    void (*disconnectInterrupt)(ENETsock_t **);
    void (*disconnectSock)(ENETsock_t **, int);

} INTRsock_t;


typedef struct IPCsock_{

	SOCKgeneric_t sock;
	int *epfd;
	struct epoll_event *ev;
	struct epoll_event *events;

	pid_t ipc_pid;
	
	IPCsock_t *next;
	IPCsock_t *prev;
	IPCsock_t *server;
	
} IPCsock_t;


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
	void (*initializeSettings)(RCVsys *);
	void (*setGain)(RCVsys *, uint32_t, uint32_t);
	void (*issueDirectSerialCommand)(RCVsys *, uint32_t, uint32_t);

} ADCchip;


typedef struct RCVsys_{
	
	uint32_t volatile *stateReset;
	uint32_t volatile *controlComms;
	uint32_t volatile *recLen;
	uint32_t volatile *trigDelay;
    uint32_t volatile *interrupt0;
    uint32_t volatile *recvIsLocal;
	
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

	void (*resetVars)(RCVsys *);
	void (*stateResetFPGA)(RCVsys *);
	void (*setRecLen)(RCVsys *, uint32_t);
	void (*setTrigDelay)(RCVsys *, uint32_t);
	void (*copyDataToMem)(RCVsys *, char *);
	void (*setDataAddrPointers)(RCVsys *, ENETsock_t **, char *);
	void (*setLocalStorage)(RCVsys *, uint32_t, uint32_t);
    void (*allocateLocalStorage)(RCVsys *);
	uint32_t (*getInterruptMsg)(RCVsys *);
} RCVsys;


//~ typedef union TX_InputCommands_{ 
    //~ struct {
        //~ uint16_t fpga; // [15:0]
        //~ uint16_t arm; // [31:16]
    //~ };
    //~ struct {
        //~ uint32_t loopCounter : 28;
        //~ uint32_t loopNumber : 4;
    //~ };
    //~ uint32_t t;
    //~ uint32_t instr;
//~ } TX_InputCommands_t;


//~ typedef union TX_InstructionReg_{ 
    //~ struct {
        //~ /* [31:0] nominally the timing register
            //~ if ( instruction != loop start/end )
                //~ contains time until next instruction
            //~ else
                //~ contains variables responsible for loop flow control
        //~ */
        //~ union { 
            //~ // time until next instruction
            //~ uint32_t t;

            //~ struct {
                //~ uint32_t requestedPhaseDelayStartAddr : 28;
                //~ uint32_t phaseAddrRequestLoopNumber : 4;
            //~ };
            
            //~ // variables for loops
            //~ struct {
                //~ uint32_t loopCounterRef : 28; // member of loop end
                //~ uint32_t loopNumber : 4; // member of loop start AND loop end
            //~ };

            //~ /* address of the phaseDelays in the PHASE DELAY register corresponding 
                //~ to the first steering location of the loop (member of loop start) */
            
            //~ struct {
                //~ uint16_t phaseDelayStartAddr : 14;
                //~ uint16_t blank2t : 2;
            //~ };

            //~ struct {
                //~ uint16_t fireAt_phaseDelayAddr : 12;
                //~ uint16_t blank4t : 4;
            //~ };
        //~ };
        
        //~ /* [47:32] the instruction register
            //~ varies depending on the instruction type
            //~ split up into bitfielded structs accordingly
        //~ */
        //~ union {
            //~ // the whole instruction
            //~ uint16_t instr; 
            
            //~ // only used on master fpga
            //~ struct {
                //~ uint16_t ledVals : 8; 
                //~ uint16_t masterTrigVals : 8;
            //~ };

            //~ // only used on the daughter fpgas
            //~ struct {
                //~ uint16_t chargeTime : 9;
                //~ uint16_t trigVals : 7;
            //~ };

            //~ // address in the INSTRUCTION register where the loop starts (member of loop end)
            //~ struct {
                //~ uint16_t loopStartAddr : 13;
                //~ uint16_t blank3 : 3;
            //~ };

            //~ /* sets how much to increment the address in the PHASE DELAY register after 
                //~ each pulse, allows one to treat every N-th location (member of start loop) */
            //~ struct {
                //~ uint16_t phaseDelayAddrIncr : 14;
                //~ uint16_t blank2i : 2;
            //~ };
            
        //~ };

        //~ // instruction type is always unique so no union
        //~ uint16_t type;
    //~ };
    //~ uint64_t full;
//~ } TX_InstructionReg_t;


//~ typedef struct TX_SteeringLoopDefs_{
    //~ uint32_t loopNumber;
    //~ uint32_t loopCounter;
    //~ uint32_t phaseAddrStart;
    //~ uint32_t phaseAddrEnd;
    //~ uint32_t phaseAddrIncr;

    //~ uint32_t currentCallback;
//~ } TX_SteeringLoopDefs_t;


//~ typedef struct TX_PhaseDelayReg_{
	//~ uint16_t ch0;
	//~ uint16_t ch1;
	//~ uint16_t ch2;
	//~ uint16_t ch3;
	//~ uint16_t ch4;
	//~ uint16_t ch5;
	//~ uint16_t ch6;
	//~ uint16_t ch7;
//~ } TX_PhaseDelayReg_t;


//~ typedef struct TXsys_{
    //~ // memory mapped FPGA pio's
	//~ uint32_t volatile *controlComms;
	//~ uint32_t volatile *channelMask;
	//~ uint32_t volatile *setInstructionReadAddr;
	//~ uint32_t volatile *errorReply;
	//~ uint32_t volatile *errorMsgFromInterrupt;
	//~ uint32_t volatile *errorMsgFromTransducer;
	//~ uint32_t volatile *setTrigRestLevel;
	//~ uint32_t volatile *setTrig;
	//~ uint32_t volatile *setLed;
	//~ uint32_t volatile *interrupt0;
	//~ uint32_t volatile *interrupt1;
    //~ uint32_t volatile *currentlyInLoop;
    //~ uint32_t volatile *loopCounter;
  
    //~ // memory mapped FPGA ram blocks
	//~ TX_InstructionReg_t *instructionReg; // 64bit, 8192 elements
    //~ TX_PhaseDelayReg_t *phaseDelayReg; // 128bit, 16384 elements
    //~ TX_PhaseDelayReg_t *fireAt_phaseDelayReg; // 128bit, 4096 elements

    //~ // local variables to store program parameters
    //~ uint32_t ***fireAtList;
    //~ TX_SteeringLoopDefs_t *steeringLoopDefs;
    
    //~ // variables used to setup ENET to recv phaseCharge data and user defined program
    //~ uint32_t recvType; // instructions vs phase delays
    //~ uint32_t nLocs;
    //~ uint32_t nInstructionsBuff;
    //~ char **phaseDelayBuff;
    //~ char **instructionBuff;

    //~ uint32_t nInstructions;
    //~ uint32_t **instructions;

	//~ void (*resetTX_vars)(struct TXsys *);
    //~ void (*setChannelMask)(struct TXsys *, uint32_t);
//~ } TXsys;


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


//~ int ADC_setup(FGPAvars *FPGA, ADCchip *ADC){

	//~ ADC->serialCommand = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_SERIAL_COMMAND_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
    //~ ADC->powerOn = &powerOn_adc;
    //~ ADC->powerOff = &powerOff_adc;
    //~ ADC->sync = &sync_adc;
    //~ ADC->initializeSettings = &initializeSettings_adc;
    //~ ADC->setGain = &setGain_adc;
    //~ ADC->issueDirectSerialCommand = &issueDirectSerialCommand_adc;

    //~ return(1);
//~ }


//~ int RCV_setup(FPGAvars *FPGA, RCVsys *RCV, ADCchip *ADC, BOARDdata *board){
	
    //~ RCV->ADC = ADC;
    //~ RCV->board = board;

	//~ RCV->stateReset = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_FPGA_STATE_RESET_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	//~ RCV->controlComms = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_CONTROL_COMMS_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	//~ RCV->recLen = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_SET_ADC_RECORD_LENGTH_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	//~ RCV->trigDelay = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_SET_ADC_TRIG_DELAY_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	//~ RCV->interrupt0 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_ADC_INTERRUPT_GENERATOR_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	//~ RCV->ramBank0 = FPGA->axi_virtual_base + ( ( uint32_t  )( ADC_RAMBANK0_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );
	//~ RCV->ramBank1 = FPGA->axi_virtual_base + ( ( uint32_t  )( ADC_RAMBANK1_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );
	//~ RCV->ramBank2 = FPGA->axi_virtual_base + ( ( uint32_t  )( ADC_RAMBANK2_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );

    //~ RCV->data = (char **)malloc(sizeof(char *));
    //~ RCV->resetVars = &resetVars_rcv;
    //~ RCV->stateResetFPGA = &stateResetFPGA_rcv;
    //~ RCV->setRecLen = &setRecLen_rcv;
    //~ RCV->setTrigDelay = &setTrigDelay_rcv;
    //~ RCV->copyDataToMem = &copyDataToMem_rcv;
    //~ RCV->setDataAddrPointers = &setDataAddrPointers_rcv;
    //~ RCV->setLocalStorage = &setLocalStorage_rcv;
    //~ RCV->allocateLocalStorage = &allocateLocalStorage_rcv;
    //~ RCV->getInterruptMsg = &getInterruptMsg_rcv;
	
	//~ return(1);
//~ }


//~ int TX_init(FPGAvars *FPGA, TXsys *TX){
	
	//~ TX->interrupt0 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_TX_INTERRUPT_GENERATOR_0_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	//~ TX->interrupt1 = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_TX_INTERRUPT_GENERATOR_1_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	
	//~ TX->controlComms = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_TXCONTROLCOMMS_BASE ) & ( uint32_t )( HW_REGS_MASK ) );	
	//~ TX->channelMask = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_SET_TXCHANNELMASK_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	
	//~ TX->setLed = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_LED_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	//~ TX->setTrig = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_TRIG_VAL_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	//~ TX->setTrigRestLevel = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + PIO_TRIG_REST_LEVELS_BASE ) & ( uint32_t )( HW_REGS_MASK ) );


	//~ TX->errorMsgFromTransducer = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_OUTPUT_ERROR_MSG_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	//~ TX->errorMsgFromInterrupt = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_INTERRUPT_ERROR_MSG_BASE ) & ( uint32_t )( HW_REGS_MASK ) );
	//~ TX->errorReply = FPGA->virtual_base + ( ( uint32_t )( ALT_LWFPGASLVS_OFST + TX_ERROR_COMMS_BASE ) & ( uint32_t )( HW_REGS_MASK ) );

	//~ TX->instructionReg = FPGA->axi_virtual_base + ( ( uint32_t  )( TX_INSTRUCTIONREGISTER_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );
	//~ TX->phaseDelayReg = FPGA->axi_virtual_base + ( ( uint32_t  )( TX_PHASEDELAYREGISTER_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );
	//~ TX->fireAt_phaseDelayReg = FPGA->axi_virtual_base + ( ( uint32_t  )( TX_FIREAT_PHASEDELAYREGISTER_BASE ) & ( uint32_t)( HW_FPGA_AXI_MASK ) );
	

    //~ TX->instructionBuff = (char **)malloc(sizeof(char *));
    //~ TX->phaseDelayBuff = (char **)malloc(sizeof(char *));
    //~ TX->instructions = (uint32_t **)malloc(sizeof(uint32_t *));
    //~ TX->fireAtList = (uint32_t ***)malloc(sizeof(uint32_t **));
    //~ TX->steeringLoopsDef = (TX_SteeringLoopDefs_t *)calloc(MAX_LOOPS,sizeof(TX_SteeringLoopDefs_t));
	
	//~ DREF32(TX->channelMask) = 0xff;
	//~ DREF32(TX->controlComms) = 0;
	//~ return(1);
//~ }


void loadBoardData(BOARDdata *BOARD){ // load the boards specific data from files stored on SoC		
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
    BOARD->boardNum = boardData[0];
}












