
// structure defs and typedefs
struct FPGAvars_;
struct SOCK_;
struct POLLserver_;
struct RCVsys_;
struct ADCvars_;
union FMSG_;
union TXpioreg0_;
union TXpioreg2_;
union TXpioreg3_;
union TXpioreg4_;
union TXpioreg5_;
union TXpioreg6_;
union TXpioreg7_;
union TXtrigtimings_;
union TXpioreg2425_;
struct TXpiocmdlist_;

typedef struct FPGAvars_ FPGAvars_t;
typedef struct SOCK_ SOCK_t;
typedef struct POLLserver_ POLLserver_t;
typedef struct RCVsys_ RCVsys_t;
typedef struct ADCvars_ ADCvars_t;
typedef union FMSG_ FMSG_t;
typedef union TXpioreg0_ TXpioreg0_t;
typedef union TXpioreg2_ TXpioreg2_t;
typedef union TXpioreg3_ TXpioreg3_t;
typedef union TXpioreg4_ TXpioreg4_t;
typedef union TXpioreg5_ TXpioreg5_t;
typedef union TXpioreg6_ TXpioreg6_t;
typedef union TXpioreg7_ TXpioreg7_t;
typedef union TXtrigtimings_ TXtrigtimings_t;
typedef union TXpioreg2425_ TXpioreg2425_t;
typedef struct TXpiocmdlist_ TXpiocmdlist_t;

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
            uint8_t rcv_interrupt : 1;
            uint8_t blnk : 4;
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
            uint32_t blnk : 18;
        };
        uint32_t all;
    } pioSettings_ref;

    char **data;

	SOCK_t interrupt;
    
    ADCvars_t *ADC;

    void (*setRecLen)(RCVsys_t *,uint32_t);   
    void (*setPioVarGain)(RCVsys_t *,uint32_t); 
    void (*setLEDs)(RCVsys_t *,uint32_t);
    void (*setClkDivisor)(RCVsys_t *,uint32_t); 
    void (*setSamplingMode)(RCVsys_t *,uint32_t); 
	void (*setCompressorMode)(RCVsys_t *,uint32_t); 
} RCVsys_t;


typedef struct TXpiocmdlist_{
    int cmdNumber;

    TXpioreg0_t reg0;
    uint32_t    reg1;
    TXpioreg2_t reg2;
    TXpioreg3_t reg3;
    TXpioreg4_t reg4;
    TXpioreg5_t reg5;
    TXpioreg6_t reg6;
    TXpioreg7_t reg7;

    TXtrigtimings_t reg8_23[16];
    TXpioreg2425_t reg24_25;
    
    TXpiocmdlist_t *top;
    TXpiocmdlist_t *prev;
    TXpiocmdlist_t *next;

} TXpiocmdlist_t;


typedef struct TXsys_{
    
    TXpioreg0_t reg0;
    uint32_t    reg1;
    TXpioreg2_t reg2;
    TXpioreg3_t reg3;
    TXpioreg4_t reg4;
    TXpioreg5_t reg5;
    TXpioreg6_t reg6;
    TXpioreg7_t reg7;

    TXtrigtimings_t *reg8_23; // trig/led durations and delays
	
    TXpioreg2425_t reg24_25;
		
	uint32_t volatile **pio_reg;
    
    TXpiocmdlist_t **pio_cmd_list;

    char volatile *instructions;
    char volatile *phaseDelays;

    SOCK_t interrupt;

    void (*setControlState)(TXsys_t *, uint32_t);
    void (*setTrigRestLvls)(TXsys_t *, uint32_t);
    void (*setActiveTransducers)(TXsys_t *, uint32_t);
    
    void (*setTrigs)(TXsys_t *, uint32_t *);
    void (*setChargeTime)(TXsys_t *, uint32_t);
    void (*setPhaseDelay)(TXsys_t *, uint32_t *);
    void (*setRecvTrigDelay)(TXsys_t *, uint32_t);
    void (*setRequestNextInstrTimer)(TXsys_t *, uint64_t);
    void (*issuePioCommand)(TXsys_t *, uint32_t);

    void (*modifyField_pioCmdList)(TXsys_t *, uint32_t);
    void (*addItem_pioCmdList)(TXsys_t *, uint32_t);
    void (*delItem_pioCmdList)(TXsys_t *, uint32_t);

    void (*gotoIdx_pioCmdList)(TXsys_t *, uint32_t);
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
    uint32_t u[10];
    float f[10];
    double d[5];
} FMSG_t;






