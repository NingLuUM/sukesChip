#define ADC_REGISTER_DEFS

enum ADC_ADDRS{
	software_reset_addr=0,
	register_readout_enable_addr=0,
	tgc_register_wren_addr=0,
	
	global_pdn_addr=1,
	output_disable_addr=1,
	pdn_channel1_addr=1,
	pdn_channel2_addr=1,
	pdn_channel3_addr=1,
	pdn_channel4_addr=1,
	pdn_channel5_addr=1,
	pdn_channel6_addr=1,
	pdn_channel7_addr=1,
	pdn_channel8_addr=1,
	pdn_channel_addr=1,
	stdby_addr=1,
	low_frequency_noise_supression_addr=1,
	external_reference_addr=1,
	output_rate_2x_addr=1,
	
	pdn_lvds_addr=2,
	averaging_enable_addr=2,
	pattern_modes_addr=2,
	
	register_offset_subtraction_enable_addr=3,
	digital_gain_enable_addr=3,
	serialize_data_rate_addr=3,
	
	dfs_addr=4,
	
	custom_pattern_addr=5,
	
	internal_ac_coupling_addr=6,
	filter_bw_addr=6,
	self_test_addr=6,
	vca_low_noise_mode_addr=6,
	
	offset_ch1_addr=7,
	dig_gain1_addr=7,
	
	offset_ch2_addr=8,
	dig_gain2_addr=8,
	
	offset_ch3_addr=9,
	dig_gain3_addr=9,
	
	offset_ch4_addr=10,
	dig_gain4_addr=10,
	
	offset_ch5_addr=11,
	dig_gain5_addr=11,
	
	offset_ch6_addr=12,
	dig_gain6_addr=12,
	
	offset_ch7_addr=13,
	dig_gain7_addr=13,
	
	offset_ch8_addr=14,
	dig_gain8_addr=14,
	
	digital_high_pass_filter_enable_for_channels_1_4_addr = 15,
	digital_high_pass_filter_corner_freq_for_channels_1_4_addr = 15,
	
	digital_high_pass_filter_enable_for_channels_5_8_addr = 16,
	digital_high_pass_filter_corner_freq_for_channels_5_8_addr = 16,
	
	clamp_disable_addr = 17
};

typedef union GPREG0_{
	struct{
		union{
			struct {
				uint16_t SOFTWARE_RESET : 1;
				uint16_t REGISTER_READOUT_ENABLE : 1;
				uint16_t TGC_REGISTER_WREN : 1;
				uint16_t b3_b16 : 13;
			};
			uint16_t cmd;
		};
		
		uint8_t addr;
		uint8_t blnk;
	};
	
	uint32_t adccmd;
} GPREG0_t;

typedef union GPREG1_{
	struct{
		union{
			struct {
				uint16_t GLOBAL_PDN 					: 1; // b0
				uint16_t OUTPUT_DISABLE 				: 1; // b1
				uint16_t PDN_CHANNEL1 					: 1; // b2
				uint16_t PDN_CHANNEL2 					: 1; // b3
				uint16_t PDN_CHANNEL3 					: 1; // b4
				uint16_t PDN_CHANNEL4 					: 1; // b5
				uint16_t PDN_CHANNEL5 					: 1; // b6
				uint16_t PDN_CHANNEL6 					: 1; // b7
				uint16_t PDN_CHANNEL7 					: 1; // b8
				uint16_t PDN_CHANNEL8 					: 1; // b9
				uint16_t STDBY 							: 1; // b10
				uint16_t LOW_FREQUENCY_NOISE_SUPRESSION : 1; // b11
				uint16_t b12 							: 1; // b12
				uint16_t EXTERNAL_REFERENCE 			: 1; // b13
				uint16_t OUTPUT_RATE_2X 				: 1; // b14
				uint16_t b15 							: 1; // b15
			};
			struct {
				uint16_t b0 : 1;
				uint16_t b1 : 1;
				uint16_t PDN_CHANNEL : 8;
				uint16_t b10_b15 : 6;
			};
			uint16_t cmd;
		};
		
		uint8_t addr;
		uint8_t blnk;
	};
	uint32_t adccmd;
} GPREG1_t;

typedef union GPREG2_{
	struct{
		union{
			struct {
				uint16_t b0_b2 				: 3; // b0-b2
				uint16_t PDN_LVDS 			: 8; // b3-b10
				uint16_t AVERAGING_ENABLE 	: 1; // b11
				uint16_t b12 				: 1; // b12
				uint16_t PATTERN_MODES 		: 3; // b13-b15
			};
			uint16_t cmd;
		};
		
		uint8_t addr;
		uint8_t blnk;
	};
	uint32_t adccmd;
} GPREG2_t;

typedef union GPREG3_{
	struct{
		union{
			struct {
				uint16_t b0_b7 								: 8; // b0-b7
				uint16_t REGISTER_OFFSET_SUBTRACTION_ENABLE : 1; // b8
				uint16_t b9_b11								: 3; // b9-b11
				uint16_t DIGITAL_GAIN_ENABLE				: 1; // b12
				uint16_t SERIALIZED_DATA_RATE				: 2; // b13-b14
				uint16_t b15								: 1; // b15
			};
			uint16_t cmd;
		};
		
		uint8_t addr;
		uint8_t blnk;
	};
	uint32_t adccmd;
} GPREG3_t;

typedef union GPREG4_{
	struct{
		union{
			struct {
				uint16_t b0_b2 	: 3; // b0-b2
				uint16_t DFS 	: 1; // b3
				uint16_t b4_b15	:12; // b15
			};
			uint16_t cmd;
		};
		
		uint8_t addr;
		uint8_t blnk;
	};
	uint32_t adccmd;
} GPREG4_t;

typedef union GPREG5_{
	struct{
		union{
			struct {
				uint16_t CUSTOM_PATTERN :14; // b0-b13
				uint16_t b14_b15		: 2; // b14-b15
			};
			uint16_t cmd;
		};
		
		uint8_t addr;
		uint8_t blnk;
	};
	uint32_t adccmd;
} GPREG5_t;

typedef union GPREG7_{
	struct{
		union{
			struct {
				uint16_t b0 					: 1; // b0
				uint16_t INTERNAL_AC_COUPLING 	: 1; // b1
				uint16_t FILTER_BW 				: 2; // b2-b3
				uint16_t b4_b6 					: 3; // b4-b6
				uint16_t SELF_TEST 				: 2; // b7-b8
				uint16_t b9 					: 1; // b9
				uint16_t VCA_LOW_NOISE_MODE 	: 1; // b10
				uint16_t b11_b15 				: 5; // b11-b15
			};
			uint16_t cmd;
		};
		
		uint8_t addr;
		uint8_t blnk;
	};
	uint32_t adccmd;
} GPREG7_t;

typedef union GPREG13_{ // CH1
	struct{
		union{
			struct {
				uint16_t b0_b1 			: 2; // b0-b1
				uint16_t OFFSET_CH1 	: 8; // b2-b9
				uint16_t b10 			: 1; // b10
				uint16_t DIG_GAIN1 		: 5; // b11-b15
			};
			uint16_t cmd;
		};
		
		uint8_t addr;
		uint8_t blnk;
	};
	uint32_t adccmd;
} GPREG13_t;

typedef union GPREG15_{ // CH2
	struct{
		union{
			struct {
				uint16_t b0_b1 			: 2; // b0-b1
				uint16_t OFFSET_CH2 	: 8; // b2-b9
				uint16_t b10 			: 1; // b10
				uint16_t DIG_GAIN2 		: 5; // b11-b15
			};
			uint16_t cmd;
		};
		
		uint8_t addr;
		uint8_t blnk;
	};
	
	uint32_t adccmd;
} GPREG15_t;

typedef union GPREG17_{ // CH3
	struct{
		union{
			struct {
				uint16_t b0_b1 			: 2; // b0-b1
				uint16_t OFFSET_CH3 	: 8; // b2-b9
				uint16_t b10 			: 1; // b10
				uint16_t DIG_GAIN3 		: 5; // b11-b15
			};
			uint16_t cmd;
		};
		
		uint8_t addr;
		uint8_t blnk;
	};
	uint32_t adccmd;
} GPREG17_t;

typedef union GPREG19_{ // CH4
	struct{
		union{
			struct {
				uint16_t b0_b1 			: 2; // b0-b1
				uint16_t OFFSET_CH4 	: 8; // b2-b9
				uint16_t b10 			: 1; // b10
				uint16_t DIG_GAIN4 		: 5; // b11-b15
			};
			uint16_t cmd;
		};
		
		uint8_t addr;
		uint8_t blnk;
	};
	uint32_t adccmd;
} GPREG19_t;

typedef union GPREG31_{ // CH5
	struct{
		union{
			struct {
				uint16_t b0_b1 			: 2; // b0-b1
				uint16_t OFFSET_CH5 	: 8; // b2-b9
				uint16_t b10 			: 1; // b10
				uint16_t DIG_GAIN5 		: 5; // b11-b15
			};
			uint16_t cmd;
		};
		
		uint8_t addr;
		uint8_t blnk;
	};
	uint32_t adccmd;
} GPREG31_t;

typedef union GPREG29_{ // CH6
	struct{
		union{
			struct {
				uint16_t b0_b1 			: 2; // b0-b1
				uint16_t OFFSET_CH6 	: 8; // b2-b9
				uint16_t b10 			: 1; // b10
				uint16_t DIG_GAIN6 		: 5; // b11-b15
			};
			uint16_t cmd;
		};
		
		uint8_t addr;
		uint8_t blnk;
	};
	uint32_t adccmd;
} GPREG29_t;

typedef union GPREG27_{ // CH7
	struct{
		union{
			struct {
				uint16_t b0_b1 			: 2; // b0-b1
				uint16_t OFFSET_CH7 	: 8; // b2-b9
				uint16_t b10 			: 1; // b10
				uint16_t DIG_GAIN7 		: 5; // b11-b15
			};
			uint16_t cmd;
		};
		
		uint8_t addr;
		uint8_t blnk;
	};
	uint32_t adccmd;
} GPREG27_t;

typedef union GPREG25_{ // CH8
	struct{
		union{
			struct {
				uint16_t b0_b1 			: 2; // b0-b1
				uint16_t OFFSET_CH8 	: 8; // b2-b9
				uint16_t b10 			: 1; // b10
				uint16_t DIG_GAIN8 		: 5; // b11-b15
			};
			uint16_t cmd;
		};
		
		uint8_t addr;
		uint8_t blnk;
	};
	uint32_t adccmd;
} GPREG25_t;

typedef union GPREG21_{
	struct{
		union{
			struct {
				uint16_t DIGITAL_HIGH_PASS_FILTER_ENABLE_FOR_CHANNELS_1_4 : 1; // b0
				uint16_t DIGITAL_HIGH_PASS_FILTER_CORNER_FREQ_FOR_CHANNELS_1_4 : 4; // b1-b4
				uint16_t b5_b15 			: 11; // b5-b15
			};
			uint16_t cmd;
		};
		
		uint8_t addr;
		uint8_t blnk;
	};
	uint32_t adccmd;
} GPREG21_t;

typedef union GPREG33_{
	struct{
		union{
			struct {
				uint16_t DIGITAL_HIGH_PASS_FILTER_ENABLE_FOR_CHANNELS_5_8 : 1; // b0
				uint16_t DIGITAL_HIGH_PASS_FILTER_CORNER_FREQ_FOR_CHANNELS_5_8 : 4; // b1-b4
				uint16_t b5_b15 			: 11; // b5-b15
			};
			uint16_t cmd;
		};
		
		uint8_t addr;
		uint8_t blnk;
	};
	uint32_t adccmd;
} GPREG33_t;

typedef union GPREG70_{
	struct{
		union{
			struct {
				uint16_t b0_b13 		:14; // b0-b13
				uint16_t CLAMP_DISABLE 	: 1; // b14
				uint16_t b15 			: 1; // b15
			};
			uint16_t cmd;
		};
		
		uint8_t addr;
		uint8_t blnk;
	};
	uint32_t adccmd;
} GPREG70_t;

typedef union TGCREG_0x01_{
	struct{
		union{
			struct {
				uint16_t REG_VALUES		: 9; // b0-b13
				uint16_t b9_b15			: 7; 
			};
			uint16_t cmd;
		};
		
		uint8_t addr;
		uint8_t blnk;
	};
	uint32_t adccmd;
} TGCREG_0x01_t;

typedef union TGCREG_0x95_{
	struct{
		union{
			struct {
				uint16_t START_INDEX	: 8; // b0-b13
				uint16_t b8_b15			: 8; 
			};
			uint16_t cmd;
		};
		
		uint8_t addr;
		uint8_t blnk;
	};
	uint32_t adccmd;
} TGCREG_0x95_t;

typedef union TGCREG_0x96_{
	struct{
		union{
			struct {
				uint16_t STOP_INDEX	: 8; // b0-b13
				uint16_t b8_b15		: 8; 
			};
			uint16_t cmd;
		};
		
		uint8_t addr;
		uint8_t blnk;
	};
	uint32_t adccmd;
} TGCREG_0x96_t;

typedef union TGCREG_0x97_{
	struct{
		union{
			struct {
				uint16_t START_GAIN		: 6; // b0-b13
				uint16_t b6				: 1; 
				uint16_t INTERP_ENABLE	: 1;
				uint16_t b8_b15			: 8; 
			};
			uint16_t cmd;
		};
		
		uint8_t addr;
		uint8_t blnk;
	};
	uint32_t adccmd;
} TGCREG_0x97_t;

typedef union TGCREG_0x98_{
	struct{
		union{
			struct {
				uint16_t HOLD_GAIN_TIME	: 8; // b0-b13
				uint16_t b8_b15			: 8; 
			};
			uint16_t cmd;
		};
		
		uint8_t addr;
		uint8_t blnk;
	};
	uint32_t adccmd;
} TGCREG_0x98_t;

typedef union TGCREG_0x99_{
	struct{
		union{
			struct {
				uint16_t FINE_GAIN			: 3; // b0-b13
				uint16_t STATIC_PGA			: 1; 
				uint16_t UNIFORM_GAIN_MODE	: 1; // b0-b13
				uint16_t SOFT_SYNC			: 1; 
				uint16_t b6_b15				: 10; 
			};
			uint16_t cmd;
		};
		
		uint8_t addr;
		uint8_t blnk;
	};
	uint32_t adccmd;
} TGCREG_0x99_t;

typedef union TGCREG_0x9A_{
	struct{
		union{
			struct {
				uint16_t COARSE_GAIN		: 6; // b0-b13
				uint16_t b6_b15				: 10; 
			};
			uint16_t cmd;
		};
		
		uint8_t addr;
		uint8_t blnk;
	};
	uint32_t adccmd;
} TGCREG_0x9A_t;

typedef union TGCREG_0x9B_{
	struct{
		union{
			struct {
				uint16_t UNIFORM_GAIN_SLOPE	: 6; // b0-b13
				uint16_t b8_b15				: 10; 
			};
			uint16_t cmd;
		};
		
		uint8_t addr;
		uint8_t blnk;
	};
	uint32_t adccmd;
} TGCREG_0x9B_t;

typedef union ADCREG_{
	struct{
		uint16_t cmd;
		uint8_t addr;
		uint8_t blnk;
	};
	uint32_t adccmd;
} ADCREG_t;


