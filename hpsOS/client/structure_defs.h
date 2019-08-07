
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
typedef struct BOARDdata_ BOARDdata_t;
typedef struct FPGAvars_ FPGAvars_t;
typedef struct ADCchip_ ADCchip_t;
typedef struct RCVsys_ RCVsys_t;
typedef struct TXsys_ TXsys;
typedef struct TX_PhaseDelayReg_ TX_PhaseDelayReg_t;
typedef struct TX_SteeringLoopDefs_ TX_SteeringLoopDefs_t;

union TX_InputCommands_;
union TX_InstructionReg_;
typedef union TX_InputCommands_ TX_InputCommands_t;
typedef union TX_InstructionReg_ TX_InstructionReg_t;

// function prototypes for ADC
void powerOn_adc(RCVsys_t *RCV);
void powerOff_adc(RCVsys_t *RCV);
void sync_adc(RCVsys_t *RCV);
void initializeSettings_adc(RCVsys_t *RCV);
void issueDirectSerialCommand_adc(RCVsys_t *RCV, uint32_t addr, uint32_t cmd);
void setGain_adc(RCVsys_t *RCV, uint32_t coarseGain, uint32_t fineGain);

// function prototypes for RCV subsystem
void resetVars_rcv(RCVsys_t *RCV);	
void stateResetFPGA_rcv(RCVsys_t *RCV);
int setRecLen_rcv(RCVsys_t *RCV, uint32_t recLen);
int setTrigDelay_rcv(RCVsys_t *RCV, uint32_t trigDelay);
void adcmemcpy_rcv(char *dest, int32_t volatile *sourceData, size_t nbytes);
void copyDataToMem_rcv(RCVsys_t *RCV);
void setDataAddrPointers_rcv(RCVsys_t *RCV);
void setLocalStorage_rcv(RCVsys_t *RCV, uint32_t isLocal, uint32_t nPulses);
void allocateLocalStorage_rcv(RCVsys_t *RCV);
void setDataTransferPacketSize_rcv(RCVsys_t *RCV, uint32_t packetsize);
void spawnDataTransferSocks_rcv(RCVsys_t *RCV);
uint32_t getInterruptMsg_rcv(RCVsys_t *RCV);

// function prototypes for ENET
void setnonblocking_enet(int sock);
void connectInterrupt_intr(ENETsock_t **INTR, char *gpio_lab, int portnum);
void disconnectInterrupt_intr(ENETsock_t **INTR);
void addEnetSock_enet(ENETsock_t **ENET, int portNum, int makeCommSock);
void connectEnetSock_enet(ENETsock_t **ENET, int portNum);
void disconnectSock_enet(ENETsock_t **ENET, int portNum);
void setPacketSize_enet(ENETsock_t *ENET, uint32_t packetsize);
void sendAcqdData_enet(ENETsock_t **ENET, RCVsys_t *ADC, int portNum);
void setupENETsockFunctionPointers_intr(ENETsock_t *tmp);
void setupENETsockFunctionPointers_enet(ENETsock_t *tmp);


typedef struct BOARDdata_{
	int boardNum;
	int elements_in_array;
	int *subelement_list;
	float *element_coords;
} BOARDdata_t;


typedef struct SOCKgeneric_{
	int fd;
	
	union{
		
		struct{
			uint16_t active : 1;
			uint16_t enetCommSock : 1;
			uint16_t rcvCommSock : 1;
			uint16_t txCommSock : 1;
			uint16_t enetRecvLargeBuffer : 1;
			uint16_t interruptTx : 1;
			uint16_t interruptRcv : 1;
			uint16_t ipcTx : 1;
			uint16_t ipcRx : 1;
			
		} is;
		
		uint16_t isFlags;
	};
	
	void *parent;
	
} SOCKgeneric_t;


typedef struct ENETsettings_{
	uint32_t packetsize;
	uint32_t numPorts;
	uint32_t queryMode;
	
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
	

	
} IPCsock_t;


typedef struct FPGAvars_{ // structure to hold variables that are mapped to the FPGA hardware registers
	void *virtual_base;
	void *axi_virtual_base;
	int fd_pio;
	int fd_ram;	
	int elbows;
} FPGAvars_t;


typedef struct ADCchip_{
	
    // memory mapped variables in FPGA
	uint32_t volatile *serialCommand;
		
	// functions that act on ADC vars
	void (*powerOn)(RCVsys_t *);
	void (*powerOff)(RCVsys_t *);
	void (*sync)(RCVsys_t *);
	void (*initializeSettings)(RCVsys_t *);
	void (*setGain)(RCVsys_t *, uint32_t, uint32_t);
	void (*issueDirectSerialCommand)(RCVsys_t *, uint32_t, uint32_t);

} ADCchip_t;


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
    
    ENETsock_t **ENET;
    ADCchip_t *ADC;

	void (*resetVars)(RCVsys_t *);
	void (*stateResetFPGA)(RCVsys_t *);
	int (*setRecLen)(RCVsys_t *, uint32_t);
	int (*setTrigDelay)(RCVsys_t *, uint32_t);
	void (*copyDataToMem)(RCVsys_t *);
	void (*setDataAddrPointers)(RCVsys_t *);
	void (*setLocalStorage)(RCVsys_t *, uint32_t, uint32_t);
    void (*allocateLocalStorage)(RCVsys_t *);
    void (*setDataTransferPacketSize)(RCVsys_t *);
    void (*spawnDataTransferSocks)(RCVsys_t *);
	uint32_t (*getInterruptMsg)(RCVsys_t *);
} RCVsys_t;


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













