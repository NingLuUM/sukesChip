
typedef union TXpioreg0_{ // reg0: controlComms
    struct{
        uint32_t control_state : 2;
        
        // pio command set list
        uint32_t set_trig_leds : 1;
        uint32_t issue_rcv_trig : 1;
        uint32_t fire : 1;
        uint32_t set_amp : 1;
        uint32_t set_phase : 1;
        uint32_t set_instr_request_timer : 1;
        uint32_t reset_rcv_trig : 1;
        uint32_t reset_interrupt : 1;
        uint32_t pio_cmd_blnk : 1;
        
        // values of trig/led outputs
        uint32_t trigLedVals : 16;
        
        uint32_t blnk : 5;
    };

    struct{
        uint32_t idleblnk : 2;
        uint32_t pio_cmd_all : 9;
        uint32_t idleblnk2 : 21;
    };
    uint32_t all;		
} TXpioreg0_t;

typedef union TXpioreg2_{ // reg2: trig/led rest levels and transducer output mask
    struct{
        uint32_t isSolo : 1;
        uint32_t isMaster : 1;
        uint32_t blnk : 6;
        uint32_t activeTransducers : 8;
        uint32_t trigRestLvls : 16;
    };
    uint32_t all;
} TXpioreg2_t;

typedef union TXpioreg3_{ // reg3: phase delays ch0 & ch1
    struct{
        uint32_t ch0 : 16;
        uint32_t ch1 : 16;
    };
    uint32_t all;
} TXpioreg3_t;

typedef union TXpioreg4_{ // reg4: phase delays ch2 & ch3
    struct{
        uint32_t ch2 : 16;
        uint32_t ch3 : 16;
    };
    uint32_t all;
} TXpioreg4_t;

typedef union TXpioreg5_{ // reg5: phase delays ch4 & ch5
    struct{
        uint32_t ch4 : 16;
        uint32_t ch5 : 16;
    };
    uint32_t all;
} TXpioreg5_t;

typedef union TXpioreg6_{ // reg6: phase delays ch6 & ch7
    struct{
        uint32_t ch6 : 16;
        uint32_t ch7 : 16;
    };
    uint32_t all;
} TXpioreg6_t;

typedef union TXpioreg7_{ // reg7: tranducer chargetime & recv trig delay
    struct{
        uint32_t chargeTime : 9;
        uint32_t recvTrigDelay : 23;
    }
    uint32_t all;
} TXpioreg7_t;

typedef union TXtrigtimings_{
    struct{
        uint32_t duration : 15;
        uint32_t isInf : 1;
        uint32_t delay : 16;
    };
    struct{
        uint32_t blnk0 : 14;
        uint32_t infLvl : 1;
        uint32_t blnk32 : 17;
    };
    uint32_t all;
} TXtrigtimings_t;

typedef union TXpioreg2425_{ // reg24_25: instruction request timer
    struct{
        uint32_t reg24;
        uint32_t reg25;
    };
    uint64_t all;
} TXpioreg2425_t;
