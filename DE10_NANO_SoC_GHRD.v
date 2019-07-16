
//=======================================================
//  This code is generated by Terasic System Builder
//=======================================================

module DE10_NANO_SoC_GHRD(

	//////////// CLOCK //////////
	input               FPGA_CLK1_50,
	input               FPGA_CLK2_50,
	input               FPGA_CLK3_50,

	//////////// HPS //////////
	inout               HPS_CONV_USB_N,
	output   [14: 0]    HPS_DDR3_ADDR,
	output   [ 2: 0]    HPS_DDR3_BA,
	output              HPS_DDR3_CAS_N,
	output              HPS_DDR3_CK_N,
	output              HPS_DDR3_CK_P,
	output              HPS_DDR3_CKE,
	output              HPS_DDR3_CS_N,
	output   [ 3: 0]    HPS_DDR3_DM,
	inout    [31: 0]    HPS_DDR3_DQ,
	inout    [ 3: 0]    HPS_DDR3_DQS_N,
	inout    [ 3: 0]    HPS_DDR3_DQS_P,
	output              HPS_DDR3_ODT,
	output              HPS_DDR3_RAS_N,
	output              HPS_DDR3_RESET_N,
	input               HPS_DDR3_RZQ,
	output              HPS_DDR3_WE_N,
	output              HPS_ENET_GTX_CLK,
	inout               HPS_ENET_INT_N,
	output              HPS_ENET_MDC,
	inout               HPS_ENET_MDIO,
	input               HPS_ENET_RX_CLK,
	input    [ 3: 0]    HPS_ENET_RX_DATA,
	input               HPS_ENET_RX_DV,
	output   [ 3: 0]    HPS_ENET_TX_DATA,
	output              HPS_ENET_TX_EN,
	inout               HPS_GSENSOR_INT,
	inout               HPS_I2C0_SCLK,
	inout               HPS_I2C0_SDAT,
	inout               HPS_I2C1_SCLK,
	inout               HPS_I2C1_SDAT,
	inout               HPS_KEY,
	inout               HPS_LED,
	inout               HPS_LTC_GPIO,
	output              HPS_SD_CLK,
	inout               HPS_SD_CMD,
	inout    [ 3: 0]    HPS_SD_DATA,
	output              HPS_SPIM_CLK,
	input               HPS_SPIM_MISO,
	output              HPS_SPIM_MOSI,
	inout               HPS_SPIM_SS,
	input               HPS_UART_RX,
	output              HPS_UART_TX,
	input               HPS_USB_CLKOUT,
	inout    [ 7: 0]    HPS_USB_DATA,
	input               HPS_USB_DIR,
	input               HPS_USB_NXT,
	output              HPS_USB_STP,

	//////////// KEY //////////
	input	[ 1: 0]		KEY,

	//////////// LED //////////
	output	[ 7: 0]		LED,

	//////////// SW //////////
	input	[ 3: 0]		SW,

	
	//*****************************************//
	//************* NON-GHRD CODE *************//
	//*****************************************//
	input				MASTER_SYNC,			// SYNC line from backplane
	input				MASTER_CLK,			// CLK line from backplane (10 MHz)
	
	// SYNCHRONIZER ONLY I/Os //
	input	[ 1: 0]		iTRIG,					// input triggers 1 and 2
	output	[ 9: 0]		RED_LED,				// red RGB led bank
	output	[ 9: 0]		GREEN_LED,			// green RGB led bank
	output	[ 9: 0]		BLUE_LED,				// blue RGB led bank
	output	[ 7: 0]		oTRIG,					// eight output triggers on Synchronizer board
	output				MASTER_CLK_OUT,		// 10 MHz clock from synchronizer board
	output				MASTER_SYNC_OUT,   // SYNC line from synchronizer board
	
	// TRIDENT ONLY I/Os //
	input				EXTERNAL_TRIGGER_INPUT,				// external input trigger via SMA on each 8-channel board
	input	[ 1: 0]		LOSS_OF_SIGNAL,					// loss-of-signal indicators for CLK buffers
	input	[ 7: 0]		ADC_DATA_LINES, 	// Digial data from ADC for all 8 channels
	input				BIT_CLK,				// bit clock from ADC (= frame_clk * 6)
	input				FRAME_CLK,			// frame clock from ADC 
	input				ADC_SDOUT,			// ADC serial data register readout
	
	output	[ 7: 0]		TRANSDUCER_OUTPUT_PINS,			// outputs to gate driver of amplifier channels
	
	output				ADC_RESET,
	output				ADC_CLKINP,
	output				ADC_SCLK,
	output				ADC_SDATA,
	output				ADC_SEN,
	output				ADC_PDN,
	output				ADC_SYNC, 
	
	output				COMLED0,				// automatic communications LED
	output				COMLED1,				// user-programmed
	output				COMLED2				// user-programmed

);


//=======================================================
//  REG/WIRE declarations
//=======================================================
wire			hps_fpga_reset_n;
wire	[1: 0]	fpga_debounced_buttons;
wire	[7: 0]	fpga_led_internal;
wire	[2: 0]	hps_reset_req;
wire			hps_cold_reset;
wire			hps_warm_reset;
wire			hps_debug_reset;
wire	[27: 0]	stm_hw_events;
wire			fpga_clk_50;

//assign LED[7: 1] = fpga_led_internal;
assign fpga_clk_50 = FPGA_CLK1_50;
assign stm_hw_events = {{15{1'b0}}, SW, fpga_led_internal, fpga_debounced_buttons};



//*****************************************//
//************* NON-GHRD CODE *************//
//*****************************************//


wire [7:0] tx_output_pins;
assign TRANSDUCER_OUTPUT_PINS[7:0] = tx_output_pins[7:0];

wire [7:0] tx_trigger_pins;
assign oTRIG[7:0] = tx_trigger_pins;

wire [7:0] tx_led_pins;
assign RED_LED[7:0] = tx_led_pins;

wire CLK2, CLK25, CLK100, CLK200;
assign ADC_SCLK = CLK2;
assign ADC_CLKINP = CLK25;

// wires and assignments for adc outputs
wire adc_reset, adc_sen, adc_sdata, adc_pdn, adc_sync;

assign ADC_RESET	= adc_reset; 
assign ADC_SEN		= adc_sen;
assign ADC_SDATA	= adc_sdata; 
assign ADC_PDN		= adc_pdn;
assign ADC_SYNC		= adc_sync;


//=======================================================
//  Structural coding
//=======================================================

wire	[7:0]			adc_to_arm_interrupt;
wire	[1:0][31:0]		tx_to_arm_interrupt;


wire	[23:0]			adc_serial_command;
wire	[7:0]			adc_control_comms;

wire	[12:0]			adc_write_addr;
wire					adc_state_reset;

wire	[1:0]			adc_state;
wire	[31:0]			adc_trig_delay;
wire	[12:0]			adc_record_length;

wire  	[2:0]       	adc_wren_bank;
wire  	[2:0][31:0] 	adc_writedata_bank;

wire	[1:0]			tx_adc_trig_trigack; // [0] = trigger sig: tx to adc, [1] = ack sig: adc to tx

wire	[7:0]			tx_transducer_output_error_msg;
	
wire	[31:0]			arm_to_tx_interrupt_response;
wire	[3:0]			tx_interrupt_error_msg;
	
wire	[7:0]			tx_led_reg;
wire	[7:0]			tx_trig_reg;
wire	[7:0]			tx_trig_rest_level_reg;


wire	[3:0]			tx_state;
wire	[7:0]			tx_control_comms;
wire	[7:0]			tx_user_mask;


// for reading tx instructions from FPGA RAM
wire	[63:0]			tx_instruction_reg;
wire	[12:0]			tx_set_instruction_read_addr;
wire	[12:0]			tx_instr_read_addr; // 8191 instructions (13bit)


// for reading 'fire' cmd phase delays from FPGA RAM
wire	[127:0]			tx_phase_delay_reg;
wire	[13:0]			tx_phase_delay_read_addr; // 16384 locations (14bit)


// for reading 'fireAt' cmd phase delays from FPGA RAM
wire	[127:0]			tx_fire_at_phase_delay_reg;
wire	[11:0]			tx_fire_at_phase_delay_read_addr; // 4096 locations (12bit)


wire					FRAMECLK_SHIFT;
wire 					BITCLK_SHIFT;

assign rst = 0;


ADCclock u4 (
	.refclk   			(FPGA_CLK1_50),
	.rst      			(rst),
	.outclk_0 			(CLK2),		// 2 MHz
	.outclk_1 			(CLK25),		// 25 MHz
	.outclk_2			(CLK100),	// 100 MHz
	.outclk_3			(CLK200),		// 200 MHz
	.outclk_4			(BITCLK_SHIFT),
	.outclk_5			(FRAMECLK_SHIFT)
);

		
ADC_Control_Module u2(

	.ref_frame_clk			(CLK25),
	.frame_clk				(FRAMECLK_SHIFT),
	.bit_clk				(BITCLK_SHIFT),
	
	.adc_control_comm		(adc_control_comms),
	.adc_serial_cmd			(adc_serial_command),
	
	.ADC_RESET				(adc_reset),
	.ADC_SYNC				(adc_sync),
	.ADC_SDATA				(adc_sdata),
	.ADC_SEN				(adc_sen),
	.ADC_PDN				(adc_pdn),
	
	.ADC_SCLK				(ADC_SCLK),
	.ADC_SDOUT				(ADC_SDOUT),
	.ADC_INPUT_DATA_LINES	(ADC_DATA_LINES),
	
	.iSystemTrig			(EXTERNAL_TRIGGER_INPUT),
	.iTxTrigger				(tx_adc_trig_trigack[0]),
	
	.iTrigDelay				(adc_trig_delay),
	.iRecLength				(adc_record_length),
	.iStateReset			(adc_state_reset),
	
	.oTriggerAck			(tx_adc_trig_trigack[1]),
	.oADCData				(adc_writedata_bank),
	.oWREN					(adc_wren_bank),
	.oWAddr					(adc_write_addr),

	.oArmInterrupt			(adc_to_arm_interrupt)	
);


Output_Control_Module u3(

	.txCLK							(CLK100),
	
	.iSystemTrig					(EXTERNAL_TRIGGER_INPUT),
	
	.iExternalTrig					(EXTERNAL_TRIGGER_INPUT),
	.itxControlComms				(tx_control_comms),
	
	// procedural controls for instructions
	.itxInstruction					(tx_instruction_reg),
	.itxSetInstructionReadAddr		(tx_set_instruction_read_addr),
	.oInstructionReadAddr			(tx_instr_read_addr),
	
	// for 'fire' cmd
	.itxPhaseDelays					(tx_phase_delay_reg),
	.oPhaseDelayReadAddr			(tx_phase_delay_read_addr),
	
	// for 'fireAt' cmd
	.itxFireAtPhaseDelays			(tx_fire_at_phase_delay_reg),
	.oFireAtPhaseDelayReadAddr		(tx_fire_at_phase_delay_read_addr),
	
	// controls for physical outputs (transducers)
	.itxTransducerChannelMask		(tx_user_mask),
	.otxTransducerOutput			(tx_output_pins),
	.otxTransducerOutputError		(tx_transducer_output_error_msg),
	
	// controls for physical outputs (trigger & led pins)
	.otxTriggerOutput				(tx_trigger_pins),
	.otxLedOutput					(tx_led_pins),
	
	// recv system
	.itxADCTriggerAck				(tx_adc_trig_trigack[1]),
	.otxADCTriggerLine				(tx_adc_trig_trigack[0]),
	
	// arm interrupts
	.oArmInterrupt					(tx_to_arm_interrupt),
	.interruptResponse				(arm_to_tx_interrupt_response),
	.interruptError					(tx_interrupt_error_msg),
	
	// allow trigs and leds to be set without running program
	.ledReg							(tx_led_reg),
	.trigReg						(tx_trig_reg),
	.trigRestLevelReg				(tx_trig_rest_level_reg)
);

	
//=======================================================
//  Structural coding
//=======================================================
soc_system u0(
		.pio_adc_interrupt_generator_export			(adc_to_arm_interrupt),
		.pio_tx_interrupt_generator_1_export		(tx_to_arm_interrupt[0]),
		.pio_tx_interrupt_generator_0_export		(tx_to_arm_interrupt[1]),
		.pio_tx_control_comms_export				(tx_control_comms),
		.pio_set_tx_channel_mask_export				(tx_user_mask),
		
		.pio_led_external_connection_export			(tx_led_reg),
		.pio_trig_val_export						(tx_trig_reg),
		.pio_trig_rest_levels_export				(tx_trig_rest_level_reg),

		
		.pio_adc_serial_command_export				(adc_serial_command),
		.pio_adc_control_comms_export				(adc_control_comms),

		.pio_adc_fpga_state_reset_export			(adc_state_reset),

		.pio_set_adc_record_length_export			(adc_record_length),
		.pio_set_adc_trig_delay_export				(adc_trig_delay),
		
		.tx_output_error_msg_export					(tx_transducer_output_error_msg),
		.tx_interrupt_error_msg_export				(tx_interrupt_error_msg),
		.tx_error_comms_export						(arm_to_tx_interrupt_response),
		
		.tx_set_instruction_read_addr_export		(tx_set_instruction_read_addr),

		.adc_ram_bank0_address						(adc_write_addr),
		.adc_ram_bank0_write						(adc_wren_bank[0]),
		.adc_ram_bank0_writedata					(adc_writedata_bank[0]),
		
		.adc_ram_bank1_address						(adc_write_addr),
		.adc_ram_bank1_write						(adc_wren_bank[1]),
		.adc_ram_bank1_writedata					(adc_writedata_bank[1]),
		
		.adc_ram_bank2_address						(adc_write_addr),
		.adc_ram_bank2_write						(adc_wren_bank[2]),
		.adc_ram_bank2_writedata					(adc_writedata_bank[2]),
		

		.tx_instruction_register_address			(tx_read_addr),	
		.tx_instruction_register_readdata			(tx_instruction_reg),
		
		.tx_phase_delay_register_address					(tx_phase_delay_read_addr),
		.tx_phase_delay_register_readdata				(tx_phase_delay_reg),
	
		.tx_fire_at_phase_delay_register_address					(tx_fire_at_phase_delay_read_addr),
		.tx_fire_at_phase_delay_register_readdata				(tx_fire_at_phase_delay_reg),

		.ram_clock_bridge_clk						(CLK200), 
		.adc_clock_bridge_clk						(CLK25), 
		
		.adc_ram_bank0_chipselect					(1'b1),
		.adc_ram_bank0_byteenable					(4'b1111),
		.adc_ram_bank0_clken						(1'b1),
		.adc_ram_bank0_readdata						(32'b0),
		
		.adc_ram_bank1_chipselect					(1'b1),
		.adc_ram_bank1_byteenable					(4'b1111),
		.adc_ram_bank1_clken						(1'b1),
		.adc_ram_bank1_readdata						(32'b0),
		
		.adc_ram_bank2_chipselect					(1'b1),
		.adc_ram_bank2_byteenable					(4'b1111),
		.adc_ram_bank2_clken						(1'b1),
		.adc_ram_bank2_readdata						(32'b0),
		
		.tx_instruction_register_chipselect			(1'b1),
		.tx_instruction_register_clken				(1'b1),
		.tx_instruction_register_write				(1'b0),
		.tx_instruction_register_writedata			(64'b0),
		.tx_instruction_register_byteenable			(8'b11111111),

		.tx_phase_delay_register_chipselect				(1'b1),
		.tx_phase_delay_register_clken					(1'b1),
		.tx_phase_delay_register_write					(1'b0),
		.tx_phase_delay_register_writedata				(128'b0),
		.tx_phase_delay_register_byteenable				(16'b1111111111111111),

		.tx_fire_at_phase_delay_register_chipselect				(1'b1),
		.tx_fire_at_phase_delay_register_clken					(1'b1),
		.tx_fire_at_phase_delay_register_write					(1'b0),
		.tx_fire_at_phase_delay_register_writedata				(128'b0),
		.tx_fire_at_phase_delay_register_byteenable				(16'b1111111111111111),	

		
		//Clock&Reset
		.clk_clk(FPGA_CLK1_50),													//                          clk.clk (hps_fpga_reset_n),
		.reset_reset_n(1'b1),													//                          reset.reset_n
		//HPS ddr3
		.memory_mem_a(HPS_DDR3_ADDR),                                //                         memory.mem_a
		.memory_mem_ba(HPS_DDR3_BA),                                 //                               .mem_ba
		.memory_mem_ck(HPS_DDR3_CK_P),                               //                               .mem_ck
		.memory_mem_ck_n(HPS_DDR3_CK_N),                             //                               .mem_ck_n
		.memory_mem_cke(HPS_DDR3_CKE),                               //                               .mem_cke
		.memory_mem_cs_n(HPS_DDR3_CS_N),                             //                               .mem_cs_n
		.memory_mem_ras_n(HPS_DDR3_RAS_N),                           //                               .mem_ras_n
		.memory_mem_cas_n(HPS_DDR3_CAS_N),                           //                               .mem_cas_n
		.memory_mem_we_n(HPS_DDR3_WE_N),                             //                               .mem_we_n
		.memory_mem_reset_n(HPS_DDR3_RESET_N),                       //                               .mem_reset_n
		.memory_mem_dq(HPS_DDR3_DQ),                                 //                               .mem_dq
		.memory_mem_dqs(HPS_DDR3_DQS_P),                             //                               .mem_dqs
		.memory_mem_dqs_n(HPS_DDR3_DQS_N),                           //                               .mem_dqs_n
		.memory_mem_odt(HPS_DDR3_ODT),                               //                               .mem_odt
		.memory_mem_dm(HPS_DDR3_DM),                                 //                               .mem_dm
		.memory_oct_rzqin(HPS_DDR3_RZQ),                             //                               .oct_rzqin
		//HPS ethernet
		.hps_0_hps_io_hps_io_emac1_inst_TX_CLK(HPS_ENET_GTX_CLK),    //                   hps_0_hps_io.hps_io_emac1_inst_TX_CLK
		.hps_0_hps_io_hps_io_emac1_inst_TXD0(HPS_ENET_TX_DATA[0]),   //                               .hps_io_emac1_inst_TXD0
		.hps_0_hps_io_hps_io_emac1_inst_TXD1(HPS_ENET_TX_DATA[1]),   //                               .hps_io_emac1_inst_TXD1
		.hps_0_hps_io_hps_io_emac1_inst_TXD2(HPS_ENET_TX_DATA[2]),   //                               .hps_io_emac1_inst_TXD2
		.hps_0_hps_io_hps_io_emac1_inst_TXD3(HPS_ENET_TX_DATA[3]),   //                               .hps_io_emac1_inst_TXD3
		.hps_0_hps_io_hps_io_emac1_inst_RXD0(HPS_ENET_RX_DATA[0]),   //                               .hps_io_emac1_inst_RXD0
		.hps_0_hps_io_hps_io_emac1_inst_MDIO(HPS_ENET_MDIO),         //                               .hps_io_emac1_inst_MDIO
		.hps_0_hps_io_hps_io_emac1_inst_MDC(HPS_ENET_MDC),           //                               .hps_io_emac1_inst_MDC
		.hps_0_hps_io_hps_io_emac1_inst_RX_CTL(HPS_ENET_RX_DV),      //                               .hps_io_emac1_inst_RX_CTL
		.hps_0_hps_io_hps_io_emac1_inst_TX_CTL(HPS_ENET_TX_EN),      //                               .hps_io_emac1_inst_TX_CTL
		.hps_0_hps_io_hps_io_emac1_inst_RX_CLK(HPS_ENET_RX_CLK),     //                               .hps_io_emac1_inst_RX_CLK
		.hps_0_hps_io_hps_io_emac1_inst_RXD1(HPS_ENET_RX_DATA[1]),   //                               .hps_io_emac1_inst_RXD1
		.hps_0_hps_io_hps_io_emac1_inst_RXD2(HPS_ENET_RX_DATA[2]),   //                               .hps_io_emac1_inst_RXD2
		.hps_0_hps_io_hps_io_emac1_inst_RXD3(HPS_ENET_RX_DATA[3]),   //                               .hps_io_emac1_inst_RXD3
		//HPS SD card
		.hps_0_hps_io_hps_io_sdio_inst_CMD(HPS_SD_CMD),              //                               .hps_io_sdio_inst_CMD
		.hps_0_hps_io_hps_io_sdio_inst_D0(HPS_SD_DATA[0]),           //                               .hps_io_sdio_inst_D0
		.hps_0_hps_io_hps_io_sdio_inst_D1(HPS_SD_DATA[1]),           //                               .hps_io_sdio_inst_D1
		.hps_0_hps_io_hps_io_sdio_inst_CLK(HPS_SD_CLK),              //                               .hps_io_sdio_inst_CLK
		.hps_0_hps_io_hps_io_sdio_inst_D2(HPS_SD_DATA[2]),           //                               .hps_io_sdio_inst_D2
		.hps_0_hps_io_hps_io_sdio_inst_D3(HPS_SD_DATA[3]),           //                               .hps_io_sdio_inst_D3
		//HPS USB
		.hps_0_hps_io_hps_io_usb1_inst_D0(HPS_USB_DATA[0]),          //                               .hps_io_usb1_inst_D0
		.hps_0_hps_io_hps_io_usb1_inst_D1(HPS_USB_DATA[1]),          //                               .hps_io_usb1_inst_D1
		.hps_0_hps_io_hps_io_usb1_inst_D2(HPS_USB_DATA[2]),          //                               .hps_io_usb1_inst_D2
		.hps_0_hps_io_hps_io_usb1_inst_D3(HPS_USB_DATA[3]),          //                               .hps_io_usb1_inst_D3
		.hps_0_hps_io_hps_io_usb1_inst_D4(HPS_USB_DATA[4]),          //                               .hps_io_usb1_inst_D4
		.hps_0_hps_io_hps_io_usb1_inst_D5(HPS_USB_DATA[5]),          //                               .hps_io_usb1_inst_D5
		.hps_0_hps_io_hps_io_usb1_inst_D6(HPS_USB_DATA[6]),          //                               .hps_io_usb1_inst_D6
		.hps_0_hps_io_hps_io_usb1_inst_D7(HPS_USB_DATA[7]),          //                               .hps_io_usb1_inst_D7
		.hps_0_hps_io_hps_io_usb1_inst_CLK(HPS_USB_CLKOUT),          //                               .hps_io_usb1_inst_CLK
		.hps_0_hps_io_hps_io_usb1_inst_STP(HPS_USB_STP),             //                               .hps_io_usb1_inst_STP
		.hps_0_hps_io_hps_io_usb1_inst_DIR(HPS_USB_DIR),             //                               .hps_io_usb1_inst_DIR
		.hps_0_hps_io_hps_io_usb1_inst_NXT(HPS_USB_NXT),             //                               .hps_io_usb1_inst_NXT
		//HPS SPI
		.hps_0_hps_io_hps_io_spim1_inst_CLK(HPS_SPIM_CLK),           //                               .hps_io_spim1_inst_CLK
		.hps_0_hps_io_hps_io_spim1_inst_MOSI(HPS_SPIM_MOSI),         //                               .hps_io_spim1_inst_MOSI
		.hps_0_hps_io_hps_io_spim1_inst_MISO(HPS_SPIM_MISO),         //                               .hps_io_spim1_inst_MISO
		.hps_0_hps_io_hps_io_spim1_inst_SS0(HPS_SPIM_SS),            //                               .hps_io_spim1_inst_SS0
		//HPS UART
		.hps_0_hps_io_hps_io_uart0_inst_RX(HPS_UART_RX),             //                               .hps_io_uart0_inst_RX
		.hps_0_hps_io_hps_io_uart0_inst_TX(HPS_UART_TX),             //                               .hps_io_uart0_inst_TX
		//HPS I2C1
		.hps_0_hps_io_hps_io_i2c0_inst_SDA(HPS_I2C0_SDAT),           //                               .hps_io_i2c0_inst_SDA
		.hps_0_hps_io_hps_io_i2c0_inst_SCL(HPS_I2C0_SCLK),           //                               .hps_io_i2c0_inst_SCL
		//HPS I2C2
		.hps_0_hps_io_hps_io_i2c1_inst_SDA(HPS_I2C1_SDAT),           //                               .hps_io_i2c1_inst_SDA
		.hps_0_hps_io_hps_io_i2c1_inst_SCL(HPS_I2C1_SCLK),           //                               .hps_io_i2c1_inst_SCL
		//GPIO
		.hps_0_hps_io_hps_io_gpio_inst_GPIO09(HPS_CONV_USB_N),       //                               .hps_io_gpio_inst_GPIO09
		.hps_0_hps_io_hps_io_gpio_inst_GPIO35(HPS_ENET_INT_N),       //                               .hps_io_gpio_inst_GPIO35
		.hps_0_hps_io_hps_io_gpio_inst_GPIO40(HPS_LTC_GPIO),         //                               .hps_io_gpio_inst_GPIO40
		.hps_0_hps_io_hps_io_gpio_inst_GPIO53(HPS_LED),              //                               .hps_io_gpio_inst_GPIO53
		.hps_0_hps_io_hps_io_gpio_inst_GPIO54(HPS_KEY),              //                               .hps_io_gpio_inst_GPIO54
		.hps_0_hps_io_hps_io_gpio_inst_GPIO61(HPS_GSENSOR_INT),      //                               .hps_io_gpio_inst_GPIO61
		//FPGA Partion
		//.led_pio_external_connection_export(fpga_led_internal),      //    led_pio_external_connection.export
		//.dipsw_pio_external_connection_export(SW),                   //  dipsw_pio_external_connection.export
		//.button_pio_external_connection_export(fpga_debounced_buttons),
																						 // button_pio_external_connection.export
		//               .hps_0_h2f_reset_reset_n(hps_fpga_reset_n),                  //                hps_0_h2f_reset.reset_n
		.hps_0_f2h_cold_reset_req_reset_n(~hps_cold_reset),          //       hps_0_f2h_cold_reset_req.reset_n
		.hps_0_f2h_debug_reset_req_reset_n(~hps_debug_reset),        //      hps_0_f2h_debug_reset_req.reset_n
		.hps_0_f2h_stm_hw_events_stm_hwevents(stm_hw_events),        //        hps_0_f2h_stm_hw_events.stm_hwevents
		//               .hps_0_f2h_warm_reset_req_reset_n(~hps_warm_reset),          //       hps_0_f2h_warm_reset_req.reset_n

);

// Debounce logic to clean out glitches within 1ms
debounce debounce_inst(
		.clk(fpga_clk_50),
		.reset_n(hps_fpga_reset_n),
		.data_in(KEY),
		.data_out(fpga_debounced_buttons)
);

defparam debounce_inst.WIDTH = 2;
defparam debounce_inst.POLARITY = "LOW";
defparam debounce_inst.TIMEOUT = 50000;               // at 50Mhz this is a debounce time of 1ms
defparam debounce_inst.TIMEOUT_WIDTH = 16;            // ceil(log2(TIMEOUT))

// Source/Probe megawizard instance
hps_reset hps_reset_inst(
		.source_clk(fpga_clk_50),
		.source(hps_reset_req)
);

altera_edge_detector pulse_cold_reset(
		.clk(fpga_clk_50),
		.rst_n(hps_fpga_reset_n),
		.signal_in(hps_reset_req[0]),
		.pulse_out(hps_cold_reset)
);
defparam pulse_cold_reset.PULSE_EXT = 6;
defparam pulse_cold_reset.EDGE_TYPE = 1;
defparam pulse_cold_reset.IGNORE_RST_WHILE_BUSY = 1;

altera_edge_detector pulse_warm_reset(
		.clk(fpga_clk_50),
		.rst_n(hps_fpga_reset_n),
		.signal_in(hps_reset_req[1]),
		.pulse_out(hps_warm_reset)
);
defparam pulse_warm_reset.PULSE_EXT = 2;
defparam pulse_warm_reset.EDGE_TYPE = 1;
defparam pulse_warm_reset.IGNORE_RST_WHILE_BUSY = 1;

altera_edge_detector pulse_debug_reset(
		.clk(fpga_clk_50),
		.rst_n(hps_fpga_reset_n),
		.signal_in(hps_reset_req[2]),
		.pulse_out(hps_debug_reset)
);
defparam pulse_debug_reset.PULSE_EXT = 32;
defparam pulse_debug_reset.EDGE_TYPE = 1;
defparam pulse_debug_reset.IGNORE_RST_WHILE_BUSY = 1;


endmodule
