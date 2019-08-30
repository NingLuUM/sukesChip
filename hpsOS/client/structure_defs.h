
// structure defs and typedefs
struct FPGAvars_;
struct SOCK_;
struct POLLserver_;
struct ADCvars_;
struct FMSG_;
typedef struct FPGAvars_ FPGAvars_t;
typedef struct SOCK_ SOCK_t;
typedef struct POLLserver_ POLLserver_t;
typedef struct ADCvars_ ADCvars_t;
typedef struct FMSG_ FMSG_t;

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
	// memory mapped variables in FPGA
	uint32_t volatile *stateReset;
	uint32_t volatile *controlComms;
	uint32_t volatile *recLen;
	uint32_t volatile *pioVarGain;
	uint32_t volatile *serialCommand;
	uint32_t volatile *dataReadyFlag;
	uint32_t volatile *leds;
	
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
	
	char volatile *ramBank0;
	char volatile *ramBank1;
    
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

    char **data;

	SOCK_t interrupt;
    
    void (*issueSerialCommand)(ADCvars_t *,uint32_t);
    void (*setDefaultSettings)(ADCvars_t *);
    void (*setGain)(ADCvars_t *,double);
    void (*setUnsignedInt)(ADCvars_t *,uint32_t);
    void (*setLowNoiseMode)(ADCvars_t *,uint32_t);
    void (*toggleChannelPower)(ADCvars_t *,uint32_t);
    void (*setFilterBW)(ADCvars_t *,uint32_t);
    void (*setInternalAcCoupling)(ADCvars_t *,uint32_t);
    void (*issueDirectCommand)(ADCvars_t *,FMSG_t *);
	
} ADCvars_t;


typedef union LED_{
	struct{
		uint32_t hiRest : 3;
		uint32_t loRest : 2;
		uint32_t zero : 27;
	};
	uint32_t vals;
} LED_t;

typedef union RAMBANK0_{
	struct {
		uint64_t ch0 : 12;
		uint64_t ch1 : 12;
		uint64_t ch2 : 12;
		uint64_t ch3 : 12;
		uint64_t ch4 : 12;
		uint64_t ch5lo : 4;
	}u;
	struct{
		int64_t ch0 : 12;
		int64_t ch1 : 12;
		int64_t ch2 : 12;
		int64_t ch3 : 12;
		int64_t ch4 : 12;
		int64_t ch5lo : 4;
	}s;
} RAMBANK0_t;

typedef union RAMBANK1_{
	struct{
		uint32_t ch5hi : 8;
		uint32_t ch6 : 12;
		uint32_t ch7 : 12;
	}u;
	struct{
		int32_t ch5hi : 8;
		int32_t ch6 : 12;
		int32_t ch7 : 12;
	}s;
} RAMBANK1_t;

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

typedef struct FMSG_{
    union{
        uint32_t u[10];
        float f[10];
        double d[5];
    };
} FMSG_t;
