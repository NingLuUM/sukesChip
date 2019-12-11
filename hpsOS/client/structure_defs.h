
// structure defs and typedefs
struct FPGAvars_;
struct SOCK_;
struct POLLserver_;
struct RCVsys_;
struct ADCvars_;
struct TXsys_;
union FMSG_;
union TXpioreg0_;
union TXpioreg1_;
union TXpioreg2_;
union TXpioreg3_;
union TXpioreg4_;
union TXpioreg5_;
union TXpioreg6_;
union TXpioreg7_;
union TXpioreg8_;
union TXtrigtimings_;
union TXpioreg2526_;
struct TXpiocmd_;

typedef struct FPGAvars_ FPGAvars_t;
typedef struct SOCK_ SOCK_t;
typedef struct POLLserver_ POLLserver_t;
typedef struct RCVsys_ RCVsys_t;
typedef struct ADCvars_ ADCvars_t;
typedef struct TXsys_ TXsys_t;
typedef union FMSG_ FMSG_t;
typedef union TXpioreg0_ TXpioreg0_t;
typedef union TXpioreg1_ TXpioreg1_t;
typedef union TXpioreg2_ TXpioreg2_t;
typedef union TXpioreg3_ TXpioreg3_t;
typedef union TXpioreg4_ TXpioreg4_t;
typedef union TXpioreg5_ TXpioreg5_t;
typedef union TXpioreg6_ TXpioreg6_t;
typedef union TXpioreg7_ TXpioreg7_t;
typedef union TXpioreg8_ TXpioreg8_t;
typedef union TXtrigtimings_ TXtrigtimings_t;
typedef union TXpioreg2526_ TXpioreg2526_t;
typedef struct TXpiocmd_ TXpiocmd_t;

// rcv system functions
void rcvSetRecLen(RCVsys_t *RCV, uint32_t recLen);
void rcvSetPioVarGain(RCVsys_t *RCV, uint32_t val);
void rcvSetClkDivisor(RCVsys_t *RCV, uint32_t val);
void rcvSetSamplingMode(RCVsys_t *RCV, uint32_t val);
void rcvSetCompressorMode(RCVsys_t *RCV, uint32_t val);
void rcvSetLEDs(RCVsys_t *RCV, uint32_t val);

// adc function prototypes
void adcIssueSerialCmd(ADCvars_t *ADC, uint32_t cmd);
void adcSetGain(ADCvars_t *ADC, double gainVal);
void adcSetUnsignedInt(ADCvars_t *ADC, uint32_t val);
void adcSetLowNoiseMode(ADCvars_t *ADC, uint32_t val);
void adcToggleChannelPwr(ADCvars_t *ADC, uint32_t pwrOn);
void adcSetFilterBW(ADCvars_t *ADC, uint32_t filter);
void adcSetInternalAcCoupling(ADCvars_t *ADC, uint32_t accoupling);
void adcIssueDirectCmd(ADCvars_t *ADC, FMSG_t *msg);
void adcSetDefaultSettings(ADCvars_t *ADC);
void adcSync(ADCvars_t *ADC);

// tx system function prototypes
void txSetControlState(TXsys_t *TX, uint32_t control_state);
void txSetTrigRestLvls(TXsys_t *TX, uint32_t trigRestLvls);
void txSetActiveTransducers(TXsys_t *TX, uint32_t activeTransducers);

void txSetTrigs(TXsys_t *TX);
void txSetChargeTime(TXsys_t *TX);
void txSetFireCmdDelay(TXsys_t *TX);
void txSetPhaseDelay(TXsys_t *TX);
void txSetRecvTrigDelay(TXsys_t *TX);
void txSetAsyncWait(TXsys_t *TX);
void txIssuePioCommand(TXsys_t *TX);

void txAddCmd_f(TXsys_t *TX);
void txDelCmd_f(TXsys_t *TX, uint32_t cmdNum);

void txMakeLoopStart(TXsys_t *TX, uint32_t startIdx, uint32_t endIdx, uint32_t stepSize);
void txMakeLoopEnd(TXsys_t *TX);
void txMakeSteeringLoopStart(TXsys_t *TX, uint32_t startIdx, uint32_t endIdx, uint32_t stepSize);
void txMakeSteeringLoopEnd(TXsys_t *TX);
void txMakePioCmd(TXsys_t *TX);
void txBufferTrigTimingCmd(TXsys_t *TX, uint32_t *trigs);
void txBufferChargeTimeCmd(TXsys_t *TX, uint32_t chargeTime);
void txBufferFireDelayCmd(TXsys_t *TX, uint32_t fireDelay);
void txBufferPhaseDelayCmd(TXsys_t *TX, uint16_t *phaseDelays);
void txBufferRecvTrigDelayCmd(TXsys_t *TX, uint32_t recvTrigDelay);
void txBufferAsyncWaitCmd(TXsys_t *TX, uint64_t timerVal);

void txResetTxInterrupt(TXsys_t *TX);
void txResetRcvTrig(TXsys_t *TX);

void txSetNumSteeringLocs(TXsys_t *TX, uint32_t nSteeringLocs);
void txStorePhaseDelays(TXsys_t *TX, int nrecv, FMSG_t *msg);

typedef struct POLLserver_{
	int epfd;
	struct epoll_event ev;
	struct epoll_event events[MAX_POLL_EVENTS];
} POLLserver_t;


typedef struct SOCK_{
	int fd;
    int portNum;

    union{
        struct {
            uint8_t listener : 1;
            uint8_t commsock : 1;
            uint8_t adc_control : 1;
            uint8_t tx_control : 1;
            uint8_t tx_incoming_data : 1;
            uint8_t rcv_interrupt : 1;
            uint8_t tx_interrupt : 1;
            uint8_t alive : 1;
        };
        uint8_t flags;
    } is;

    SOCK_t *partner;
    POLLserver_t *ps;
} SOCK_t;


typedef struct FPGAvars_{ // structure to hold variables that are mapped to the FPGA hardware registers
	void *virtual_base;
	void *axi_virtual_base;
	int fd_pio;
	int fd_ram;	
	
} FPGAvars_t;


typedef struct ADCvars_{
    // shared with RCVsys	
	uint32_t volatile *controlComms;
	uint32_t volatile *serialCommand;
	
    GPREG0_t gpreg0;
	GPREG1_t gpreg1;
	GPREG2_t gpreg2;
	GPREG3_t gpreg3;
	GPREG4_t gpreg4;
	GPREG5_t gpreg5;
	GPREG7_t gpreg7;
	GPREG13_t gpreg13;
	GPREG15_t gpreg15;
	GPREG17_t gpreg17;
	GPREG19_t gpreg19;
	GPREG21_t gpreg21;
	GPREG25_t gpreg25;
	GPREG27_t gpreg27;
	GPREG29_t gpreg29;
	GPREG31_t gpreg31;
	GPREG33_t gpreg33;
	GPREG70_t gpreg70; // 18
	
	TGCREG_0x01_t tgcreg0x01[148]; //166
	
	TGCREG_0x95_t tgcreg0x95; // 167
	TGCREG_0x96_t tgcreg0x96; // 168
	TGCREG_0x97_t tgcreg0x97; // 169
	TGCREG_0x98_t tgcreg0x98; // 170
	TGCREG_0x99_t tgcreg0x99; // 171
	TGCREG_0x9A_t tgcreg0x9A; // 172
	TGCREG_0x9B_t tgcreg0x9B; // 173
	
	ADCREG_t **reg;
    uint32_t **reg_dict;
	
    void (*issueSerialCommand)(ADCvars_t *,uint32_t);
    void (*setDefaultSettings)(ADCvars_t *);
    void (*setGain)(ADCvars_t *,double);
    void (*setUnsignedInt)(ADCvars_t *,uint32_t);
    void (*setLowNoiseMode)(ADCvars_t *,uint32_t);
    void (*toggleChannelPower)(ADCvars_t *,uint32_t);
    void (*setFilterBW)(ADCvars_t *,uint32_t);
    void (*setInternalAcCoupling)(ADCvars_t *,uint32_t);
    void (*issueDirectCommand)(ADCvars_t *,FMSG_t *);
    void (*sync)(ADCvars_t *);	
} ADCvars_t;


typedef struct RCVsys_{
    uint32_t volatile *interrupt_reg;

	// memory mapped variables in FPGA
	uint32_t volatile *stateReset;
	uint32_t volatile *controlComms;
	uint32_t volatile *serialCommand;
    uint32_t volatile *recLen;
	uint32_t volatile *pioSettings;
	uint32_t volatile *dataReadyFlag;
	uint32_t volatile *leds;
    char volatile *ramBank;
    
    // reference values for setting up data storage
    uint32_t recLen_ref;
    uint32_t npulses;

    union{
        struct{
            uint8_t realTime : 1;
            uint8_t transferData : 1;
            uint8_t saveDataFile : 1;
            uint8_t is16bit : 1;
            uint8_t blnk : 4;
        };
        uint8_t all;
    } queryMode;
    
    union{
        struct{
            uint32_t varGain : 2;
            uint32_t clkDiv : 4;
            uint32_t fclk_delay : 3;
            uint32_t sampling_mode : 3;
			uint32_t compressor_mode : 2;
            uint32_t interrupt_thyself : 1;
            uint32_t blnk : 17;
        };
        uint32_t all;
    } pioSettings_ref;

    char **data;

    SOCK_t *comm_sock;
    SOCK_t *data_sock;
	SOCK_t interrupt;
    
    ADCvars_t *ADC;

    void (*setRecLen)(RCVsys_t *,uint32_t);   
    void (*setPioVarGain)(RCVsys_t *,uint32_t); 
    void (*setLEDs)(RCVsys_t *,uint32_t);
    void (*setClkDivisor)(RCVsys_t *,uint32_t); 
    void (*setSamplingMode)(RCVsys_t *,uint32_t); 
	void (*setCompressorMode)(RCVsys_t *,uint32_t); 
} RCVsys_t;


typedef struct TXpiocmd_{
    int cmdNumber;

    TXpioreg2_t reg2;
    TXpioreg3_t reg3;
    TXpioreg4_t reg4;
    TXpioreg5_t reg5;
    TXpioreg6_t reg6;
    TXpioreg7_t reg7;
    TXpioreg8_t reg8;
    
    uint32_t startIdx;
    uint32_t endIdx;
    uint32_t currentIdx;
    uint32_t stepSize;
    
    union{
        struct{
            uint32_t isCmd0 : 1;
            uint32_t isPioCmd : 1;
            uint32_t isLoopStartCmd : 1;
            uint32_t isLoopEndCmd : 1;
            uint32_t isSteeringStartCmd : 1;
            uint32_t isSteeringEndCmd : 1;
            uint32_t isAsyncWait : 1;

            uint32_t nextCmdIsCmd0 : 1;
            uint32_t nextCmdIsPio : 1;
            uint32_t nextCmdIsLoopStart : 1;
            uint32_t nextCmdIsLoopEnd : 1;
            uint32_t nextCmdIsSteeringStart : 1;
            uint32_t nextCmdIsSteeringEnd : 1;
            uint32_t nextCmdIsAsyncWait : 1;

            uint32_t setTrig0 : 1;
            uint32_t setTrig1 : 1;
            uint32_t setTrig2 : 1;
            uint32_t setTrig3 : 1;
            uint32_t setTrig4 : 1;
            uint32_t setTrig5 : 1;
            uint32_t setTrig6 : 1;
            uint32_t setTrig7 : 1;
            uint32_t setTrig8 : 1;
            uint32_t setTrig9 : 1;
            uint32_t setTrig10 : 1;
            uint32_t setTrig11 : 1;
            uint32_t setTrig12 : 1;
            uint32_t setTrig13 : 1;
            uint32_t setTrig14 : 1;
            uint32_t setTrig15 : 1;

            uint32_t blnkFlags : 2;
        };
        struct{
            uint32_t isFlags : 7;
            uint32_t nextFlags : 7;
            uint32_t trigFlags : 16;
            uint32_t blnkFlags2 : 2;
        };
        struct{
            uint32_t hasNonWaitCmds : 6;
            uint32_t blnk24 : 24;
        };
        uint32_t all;
    } flags;
    
    TXpioreg2526_t reg25_26;
    TXtrigtimings_t *reg9_24;
    
    TXpiocmd_t *top;
    TXpiocmd_t *prev;
    TXpiocmd_t *next;
    TXpiocmd_t *loopHead;
    TXpiocmd_t *loopTail;

} TXpiocmd_t;


typedef struct TXsys_{
    uint32_t volatile *interrupt_reg;

    TXpioreg0_t reg0;
    TXpioreg1_t reg1;
    TXpioreg2_t reg2;
    TXpioreg3_t reg3;
    TXpioreg4_t reg4;
    TXpioreg5_t reg5;
    TXpioreg6_t reg6;
    TXpioreg7_t reg7;
    TXpioreg8_t reg8;

    TXtrigtimings_t *reg9_24; // trig/led durations and delays
	
    TXpioreg2526_t reg25_26;
		
	uint32_t **pio_reg;
    
    TXpiocmd_t **pio_cmd_list;

    uint32_t nSteeringLocs;
    uint32_t nPhaseDelaysWritten;
    uint16_t **phaseDelays;

    SOCK_t *comm_sock;
    SOCK_t *pd_data_sock;
    SOCK_t interrupt;

    void (*setControlState)(TXsys_t *, uint32_t);
    void (*setTrigRestLvls)(TXsys_t *, uint32_t);
    void (*setActiveTransducers)(TXsys_t *, uint32_t);
    
    void (*setTrigs)(TXsys_t *);
    void (*setChargeTime)(TXsys_t *);
    void (*setFireCmdDelay)(TXsys_t *);
    void (*setPhaseDelay)(TXsys_t *);
    void (*setRecvTrigDelay)(TXsys_t *);
    void (*setAsyncWait)(TXsys_t *);
    void (*issuePioCommand)(TXsys_t *);

    void (*addCmd)(TXsys_t *);
    void (*delCmd)(TXsys_t *, uint32_t);

    void (*makeLoopStart)(TXsys_t *, uint32_t, uint32_t, uint32_t);
    void (*makeLoopEnd)(TXsys_t *);
    void (*makeSteeringLoopStart)(TXsys_t *, uint32_t, uint32_t, uint32_t);
    void (*makeSteeringLoopEnd)(TXsys_t *);
    void (*makePioCmd)(TXsys_t *);

    void (*bufferTrigTimings)(TXsys_t *, uint32_t *);
    void (*bufferChargeTime)(TXsys_t *, uint32_t);
    void (*bufferFireCmd)(TXsys_t *, uint32_t);
    void (*bufferPhaseDelays)(TXsys_t *, uint16_t *);
    void (*bufferRecvTrig)(TXsys_t *, uint32_t);
    void (*bufferAsyncWait)(TXsys_t *, uint64_t);

    void (*resetTxInterrupt)(TXsys_t *);
    void (*resetRcvTrig)(TXsys_t *);

    void (*setNumSteeringLocs)(TXsys_t *, uint32_t);
    void (*storePhaseDelays)(TXsys_t *, int, FMSG_t *);


} TXsys_t;


typedef union LED_{
	struct{
		uint32_t hiRest : 3;
		uint32_t loRest : 2;
		uint32_t zero : 27;
	};
	uint32_t vals;
} LED_t;


typedef union RAMBANK16_{
    struct{
        uint16_t ch0;
        uint16_t ch1;
        uint16_t ch2;
        uint16_t ch3;
        uint16_t ch4;
        uint16_t ch5;
        uint16_t ch6;
        uint16_t ch7;
    }u;
    struct{
        int16_t ch0;
        int16_t ch1;
        int16_t ch2;
        int16_t ch3;
        int16_t ch4;
        int16_t ch5;
        int16_t ch6;
        int16_t ch7;
    }s;
} RAMBANK16_t;


typedef struct RAMBANK12_{
	
	uint32_t c7 : 12;
	uint32_t c6 : 12;
	uint32_t c5l : 8;
	uint32_t c5u : 4;
	uint32_t c4 : 12;
	uint32_t c3 : 12;
	uint32_t c2l : 4;
	uint32_t c2u : 8; 
	uint32_t c1 : 12;
	uint32_t c0 : 12;
	
} RAMBANK12_t;


typedef union FMSG_{
    uint8_t u8[FMSG_SIZE];
    uint16_t u16[FMSG_SIZE/2];
    uint32_t u[FMSG_SIZE/4];
    uint64_t u64[FMSG_SIZE/8];
    float f[FMSG_SIZE/4];
    double d[FMSG_SIZE/8];
} FMSG_t;






