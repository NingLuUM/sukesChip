
module Output_Control_Module_PIO (

	input 					txCLK,
	
	/*	all PIO registers will be 32bit wide and packed with multiple
		commands unless otherwise specified */
	
	// PIO register for controlling module outputs
	input	[31:0]			itxControlComms, // pio_tx_reg0
	input	[31:0]			itxPioControlSettings, // pio_tx_reg1
	
	// PIO output control registers
	input	[31:0]			itxPioPhaseDelay_ch0_ch1, 	// pio_tx_reg2
	input	[31:0]			itxPioPhaseDelay_ch2_ch3, 	// pio_tx_reg3
	input	[31:0]			itxPioPhaseDelay_ch4_ch5, 	// pio_tx_reg4
	input	[31:0]			itxPioPhaseDelay_ch6_ch7, 	// pio_tx_reg5
	
	input	[31:0]			itxPioChargetime_reg,		// pio_tx_reg6
	
	input	[15:0][31:0]	itxTrigLedDurationAndDelay,	// pio_tx_reg7-23
	
	input	[15:0]			itxPioTriggerLedRestLevels,	// pio_tx_reg24
	input	[7:0]			itxTransducerOutputIsActive,	// pio_tx_reg24
	input	[7:0]			itxBoardIdentifiers,		// pio_tx_reg24
	
	// input triggers and 'fast' comm lines
	input					itxExternalTrigger,
	
	output [7:0]			otxTransducerOutput,
	
	output [15:0]			otxTriggerLedOutput,
	output reg	[1:0]		otxVarRxAtten,
	
	input					itxADCTriggerAck,
	output reg				otxADCTriggerLine,
	
	output reg	[31:0]		otxInterrupt
);

/**********************************************************************/
/*** CASE VARIABLE DEFINITIONS ****************************************/
/**********************************************************************/
// CONTROL STATES
parameter	[1:0]	CASE_IDLE 			= 2'b00;
parameter	[1:0]	CASE_PIO_CONTROL	= 2'b01;
parameter	[1:0]	CASE_RAM_CONTROL	= 2'b10;
parameter	[1:0]	CASE_PIO_CONTROL_RAM_SUBCALL = 2'b11;

// COMMANDS THAT CAN BE ISSUED THROUGH PIOs
parameter	[8:0]	PIO_CMD_IDLE				= 9'b000000000;

parameter	[8:0]	PIO_CMD_SET_TRIG_LEDS		= 9'b000000001;
parameter	[8:0]	PIO_CMD_ISSUE_RCV_TRIG		= 9'b000000010;
parameter	[8:0]	PIO_CMD_FIRE 				= 9'b000000100;


parameter	[8:0]	PIO_CMD_SET_AMP		 		= 9'b000001000;
parameter	[8:0]	PIO_CMD_SET_VAR_ATTEN		= 9'b000010000;

parameter	[8:0]	PIO_CMD_RESET_RCV_TRIG		= 9'b000100000;
parameter	[8:0]	PIO_CMD_RESET_INTERRUPT		= 9'b001000000;


/**********************************************************************/
/*** TX CONTROL REGISTER(S) *******************************************/
/**********************************************************************/
wire [1:0] control_state;
assign control_state = itxControlComms[1:0];

wire [8:0]	pio_output_commands;
wire [15:0]	pio_trigger_led_output_is_active;
wire		pio_var_atten_output;
wire [1:0]	pio_charge_selector;

assign pio_output_commands 				= itxControlComms[10:2];
assign pio_trigger_led_output_is_active	= itxControlComms[26:11];
assign pio_var_atten_output 			= itxControlComms[27];
assign pio_charge_selector				= itxControlComms[29:28];

/**********************************************************************/
/*** PIO REGISTER ASSIGNMENTS *****************************************/
/**********************************************************************/
wire [7:0][15:0] itxPioPhaseDelay;
assign itxPioPhaseDelay[0] = itxPioPhaseDelay_ch0_ch1[15:0];
assign itxPioPhaseDelay[1] = itxPioPhaseDelay_ch0_ch1[31:16];
assign itxPioPhaseDelay[2] = itxPioPhaseDelay_ch2_ch3[15:0];
assign itxPioPhaseDelay[3] = itxPioPhaseDelay_ch2_ch3[31:16];
assign itxPioPhaseDelay[4] = itxPioPhaseDelay_ch4_ch5[15:0];
assign itxPioPhaseDelay[5] = itxPioPhaseDelay_ch4_ch5[31:16];
assign itxPioPhaseDelay[6] = itxPioPhaseDelay_ch6_ch7[15:0];
assign itxPioPhaseDelay[7] = itxPioPhaseDelay_ch6_ch7[31:16];

wire [3:0][7:0] itxPioChargetime;
assign itxPioChargetime[0] = itxPioChargetime_reg[7:0];
assign itxPioChargetime[1] = itxPioChargetime_reg[15:8];
assign itxPioChargetime[2] = itxPioChargetime_reg[23:16];
assign itxPioChargetime[3] = itxPioChargetime_reg[31:24];

wire [15:0][15:0] itxTriggerLedDuration;
wire [15:0][15:0] itxTriggerLedDelay;
assign itxTriggerLedDuration[0] 	= itxTrigLedDurationAndDelay[0][15:0];
assign itxTriggerLedDelay[0] 		= itxTrigLedDurationAndDelay[0][31:16];
assign itxTriggerLedDuration[1] 	= itxTrigLedDurationAndDelay[1][15:0];
assign itxTriggerLedDelay[1] 		= itxTrigLedDurationAndDelay[1][31:16];
assign itxTriggerLedDuration[2] 	= itxTrigLedDurationAndDelay[2][15:0];
assign itxTriggerLedDelay[2] 		= itxTrigLedDurationAndDelay[2][31:16];
assign itxTriggerLedDuration[3] 	= itxTrigLedDurationAndDelay[3][15:0];
assign itxTriggerLedDelay[3] 		= itxTrigLedDurationAndDelay[3][31:16];
assign itxTriggerLedDuration[4] 	= itxTrigLedDurationAndDelay[4][15:0];
assign itxTriggerLedDelay[4] 		= itxTrigLedDurationAndDelay[4][31:16];
assign itxTriggerLedDuration[5] 	= itxTrigLedDurationAndDelay[5][15:0];
assign itxTriggerLedDelay[5] 		= itxTrigLedDurationAndDelay[5][31:16];
assign itxTriggerLedDuration[6] 	= itxTrigLedDurationAndDelay[6][15:0];
assign itxTriggerLedDelay[6] 		= itxTrigLedDurationAndDelay[6][31:16];
assign itxTriggerLedDuration[7] 	= itxTrigLedDurationAndDelay[7][15:0];
assign itxTriggerLedDelay[7] 		= itxTrigLedDurationAndDelay[7][31:16];
assign itxTriggerLedDuration[8] 	= itxTrigLedDurationAndDelay[8][15:0];
assign itxTriggerLedDelay[8] 		= itxTrigLedDurationAndDelay[8][31:16];
assign itxTriggerLedDuration[9] 	= itxTrigLedDurationAndDelay[9][15:0];
assign itxTriggerLedDelay[9] 		= itxTrigLedDurationAndDelay[9][31:16];
assign itxTriggerLedDuration[10] 	= itxTrigLedDurationAndDelay[10][15:0];
assign itxTriggerLedDelay[10] 		= itxTrigLedDurationAndDelay[10][31:16];
assign itxTriggerLedDuration[11] 	= itxTrigLedDurationAndDelay[11][15:0];
assign itxTriggerLedDelay[11] 		= itxTrigLedDurationAndDelay[11][31:16];
assign itxTriggerLedDuration[12] 	= itxTrigLedDurationAndDelay[12][15:0];
assign itxTriggerLedDelay[12] 		= itxTrigLedDurationAndDelay[12][31:16];
assign itxTriggerLedDuration[13] 	= itxTrigLedDurationAndDelay[13][15:0];
assign itxTriggerLedDelay[13] 		= itxTrigLedDurationAndDelay[13][31:16];
assign itxTriggerLedDuration[14] 	= itxTrigLedDurationAndDelay[14][15:0];
assign itxTriggerLedDelay[14] 		= itxTrigLedDurationAndDelay[14][31:16];
assign itxTriggerLedDuration[15] 	= itxTrigLedDurationAndDelay[15][15:0];
assign itxTriggerLedDelay[15] 		= itxTrigLedDurationAndDelay[15][31:16];


// needs to know if its master so it doesn't issue 'otxWaitForMe'
wire isMaster;
wire isSolo;
assign isMaster = itxBoardIdentifiers[0];
assign isSolo = itxBoardIdentifiers[1];

reg	internalTrigger;
wor triggerSignal;
assign triggerSignal = itxExternalTrigger;
assign triggerSignal = internalTrigger;

// PIO commands only get issued when the PIO state changes, need to store previous state
reg [8:0]			pio_cmd_previous;


reg [8:0]			transducer_chargetime;
reg [7:0]			transducer_is_active;
reg [15:0]			trig_led_output_buffer;
reg					var_atten_buffer;
reg [1:0]			adcTrigFlag;
reg					fireFlag;
reg					fireReset;
wire [7:0]			fireDanger;
wire [7:0]			fireComplete;

reg					trigLedFlag;
reg					trigLedReset;
wire [15:0]			trigLedComplete;

reg					txResetFlag;
reg					trigLedStop;


initial
begin
	transducer_is_active = 8'b0;
	otxADCTriggerLine = 1'b0;
	
	transducer_chargetime = 9'b0;

	fireReset = 1'b1;
	trigLedReset = 1'b1;
	txResetFlag = 1'b1;
	trigLedStop = 1'b0;
	internalTrigger = 1'b0;
end


always @(posedge txCLK)
begin
	if( ( transducer_is_active ^ itxTransducerOutputIsActive ) )
	begin
		transducer_is_active <= itxTransducerOutputIsActive;
	end
	
	case ( control_state )
		CASE_IDLE:
			begin
				transducer_chargetime <= 9'b0;
				fireFlag <= 1'b0;
				fireReset <= 1'b1;
				trigLedReset <= 1'b1;
				transducer_is_active <= 8'b0;
				internalTrigger <= 1'b0;
				trigLedStop <= 1'b1;
			end
		
		CASE_PIO_CONTROL:
			begin
				
				
				if ( pio_output_commands && !fireDanger )
				begin
					if ( trigLedStop ) trigLedStop <= 1'b0;				
					if ( pio_output_commands ^ pio_cmd_previous ) 
					begin
						pio_cmd_previous <= pio_output_commands;

						/******************************************************/
						/*** INTERNAL TRIGGER FOR SOLO BOARDS *****************/
						/******************************************************/
						if( ( pio_output_commands & 9'b000000111 ) ) 
						begin
							txResetFlag <= 1'b1;
							if ( isSolo & !internalTrigger ) internalTrigger <= 1'b1;
						end
							
						/******************************************************/
						/*** FIRE FROM PIO COMMAND ****************************/
						/******************************************************/
						if( ( pio_output_commands & PIO_CMD_FIRE ) && !fireFlag )
						begin
							fireFlag <= 1'b1;
							if ( fireReset ) fireReset <= 1'b0;
						end

						/******************************************************/
						/*** SET TRIGS AND FROM PIO COMMAND *******************/
						/******************************************************/
						if( ( pio_output_commands & PIO_CMD_SET_TRIG_LEDS ) & !trigLedFlag )
						begin
							trigLedFlag <= 1'b1;
							if ( trigLedReset ) trigLedReset <= 1'b0;
						end
						
						/******************************************************/
						/*** ISSUE RCV SYS TRIG FROM PIO COMMAND **************/
						/******************************************************/
						if( ( pio_output_commands & PIO_CMD_ISSUE_RCV_TRIG ) && !adcTrigFlag )
						begin
							adcTrigFlag[0] <= 1'b1;
						end
						
						/******************************************************/
						/*** SET CHARGETIME/AMP & PHASE DELAYS OF PULSES ******/
						/******************************************************/
						if( ( pio_output_commands & PIO_CMD_SET_AMP ) && !fireFlag )
						begin
							if( !pio_charge_selector )
							begin
								transducer_chargetime <= {itxPioChargetime[0],1'b1};
							end
							else if( pio_charge_selector == 2'b01 )
							begin
								transducer_chargetime <= {itxPioChargetime[1],1'b1};
							end
							else if( pio_charge_selector == 2'b10 )
							begin
								transducer_chargetime <= {itxPioChargetime[2],1'b1};
							end
							else
							begin
								transducer_chargetime <= {itxPioChargetime[3],1'b1};
							end
						end
							
						/******************************************************/
						/*** SET VAR ATTEN FROM PIO COMMAND *******************/
						/******************************************************/
						if( pio_output_commands & PIO_CMD_SET_VAR_ATTEN )
						begin
							if( var_atten_buffer ^ pio_var_atten_output )
							begin
								var_atten_buffer <= pio_var_atten_output;
								otxVarRxAtten[0] <= pio_var_atten_output;
							end
						end
						
						/******************************************************/
						/*** RESET RCV SYS TRIG FROM PIO COMMAND **************/
						/******************************************************/
						if( ( pio_output_commands & PIO_CMD_RESET_RCV_TRIG ) && adcTrigFlag )
						begin
							adcTrigFlag <= 2'b00;
						end
						
						/******************************************************/
						/*** RESET INTERRUPT FROM PIO COMMAND *****************/
						/******************************************************/
						if( pio_output_commands & PIO_CMD_RESET_INTERRUPT )
						begin
							internalTrigger <= 1'b0;
							if ( txResetFlag ) txResetFlag <= 1'b0;
							if ( otxInterrupt ) otxInterrupt <= 32'b0;
						end
					end
					
					/******************************************************/
					/*** ACKNOWLEDGE RCV SYS's RESPONSE TO TRIGGER ********/
					/******************************************************/
					if ( adcTrigFlag[0] & !adcTrigFlag[1] & itxADCTriggerAck )
					begin
						otxADCTriggerLine <= 1'b0;
						adcTrigFlag[1] <= 1'b1;
					end
						
					/******************************************************/
					/*** NO NEW FIRE CMDS UNTIL PREVIOUS FIRE COMPLETE ****/
					/******************************************************/
					if ( fireComplete == 8'b11111111 )
					begin
						if ( fireFlag ) fireFlag <= 1'b0;
						if ( !fireReset ) fireReset <= 1'b1;
					end
					
					/******************************************************/
					/*** NO NEW FIRE CMDS UNTIL PREVIOUS FIRE COMPLETE ****/
					/******************************************************/
					if ( trigLedComplete == 16'b1111111111111111 )
					begin
						if ( trigLedFlag ) trigLedFlag <= 1'b0;
						if ( !trigLedReset ) trigLedReset <= 1'b1;
					end
					
					/******************************************************/
					/*** SIGNAL ARM THAT SYNCHRONOUS COMMANDS FINISHED ****/
					/******************************************************/
					if ( trigLedReset & fireReset & txResetFlag )
					begin
						if( !otxInterrupt[0] ) otxInterrupt[0] <= 1'b1;
					end
					
				end
				else if ( fireDanger ) 
				begin
					transducer_chargetime <= 9'b0;
					fireFlag <= 1'b0;
					fireReset <= 1'b1;
					trigLedReset <= 1'b1;
					transducer_is_active <= 8'b0;
					internalTrigger <= 1'b0;
					trigLedStop <= 1'b1;
					pio_cmd_previous <= pio_output_commands;
					otxInterrupt[15] <= 1'b1;
				end
				
			end
		
		CASE_RAM_CONTROL:
			begin
				/******************************************************/
				/*** PLACE HOLDER UNTIL THIS IS IMPLEMENTED ***********/
				/******************************************************/
				transducer_chargetime <= 9'b0;
				fireFlag <= 1'b0;
				fireReset <= 1'b1;
				trigLedReset <= 1'b1;
				transducer_is_active <= 8'b0;
				internalTrigger <= 1'b0;
				trigLedStop <= 1'b1;
			end
		
		CASE_PIO_CONTROL_RAM_SUBCALL:
			begin
				/******************************************************/
				/*** PLACE HOLDER UNTIL THIS IS IMPLEMENTED ***********/
				/******************************************************/
				transducer_chargetime <= 9'b0;
				fireFlag <= 1'b0;
				fireReset <= 1'b1;
				trigLedReset <= 1'b1;
				transducer_is_active <= 8'b0;
				internalTrigger <= 1'b0;
				trigLedStop <= 1'b1;
			end
			
		default:
			begin
				transducer_chargetime <= 9'b0;
				fireFlag <= 1'b0;
				fireReset <= 1'b1;
				trigLedReset <= 1'b1;
				transducer_is_active <= 8'b0;
				internalTrigger <= 1'b0;
				trigLedStop <= 1'b1;
			end

	endcase
	
end


transducerOutput_Module c0(
	.clk(txCLK),
	.rst(fireReset),
	.isActive(transducer_is_active[0]),
	.onYourMark(fireFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	.chargeTime(transducer_chargetime),
	.phaseDelay(itxPioPhaseDelay[0]),
	.transducerOutput(otxTransducerOutput[0]),
	.fireComplete(fireComplete[0]),
	.warning(fireDanger[0])
);

transducerOutput_Module c1(
	.clk(txCLK),
	.rst(fireReset),
	.isActive(transducer_is_active[1]),
	.onYourMark(fireFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	.chargeTime(transducer_chargetime),
	.phaseDelay(itxPioPhaseDelay[1]),
	.transducerOutput(otxTransducerOutput[1]),
	.fireComplete(fireComplete[1]),
	.warning(fireDanger[1])
);

transducerOutput_Module c2(
	.clk(txCLK),
	.rst(fireReset),
	.isActive(transducer_is_active[2]),
	.onYourMark(fireFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	.chargeTime(transducer_chargetime),
	.phaseDelay(itxPioPhaseDelay[2]),
	.transducerOutput(otxTransducerOutput[2]),
	.fireComplete(fireComplete[2]),
	.warning(fireDanger[2])
);

transducerOutput_Module c3(
	.clk(txCLK),
	.rst(fireReset),
	.isActive(transducer_is_active[3]),
	.onYourMark(fireFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	.chargeTime(transducer_chargetime),
	.phaseDelay(itxPioPhaseDelay[3]),
	.transducerOutput(otxTransducerOutput[3]),
	.fireComplete(fireComplete[3]),
	.warning(fireDanger[3])
);

transducerOutput_Module c4(
	.clk(txCLK),
	.rst(fireReset),
	.isActive(transducer_is_active[4]),
	.onYourMark(fireFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	.chargeTime(transducer_chargetime),
	.phaseDelay(itxPioPhaseDelay[4]),
	.transducerOutput(otxTransducerOutput[4]),
	.fireComplete(fireComplete[4]),
	.warning(fireDanger[4])
);

transducerOutput_Module c5(
	.clk(txCLK),
	.rst(fireReset),
	.isActive(transducer_is_active[5]),
	.onYourMark(fireFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	.chargeTime(transducer_chargetime),
	.phaseDelay(itxPioPhaseDelay[5]),
	.transducerOutput(otxTransducerOutput[5]),
	.fireComplete(fireComplete[5]),
	.warning(fireDanger[5])
);

transducerOutput_Module c6(
	.clk(txCLK),
	.rst(fireReset),
	.isActive(transducer_is_active[6]),
	.onYourMark(fireFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	.chargeTime(transducer_chargetime),
	.phaseDelay(itxPioPhaseDelay[6]),
	.transducerOutput(otxTransducerOutput[6]),
	.fireComplete(fireComplete[6]),
	.warning(fireDanger[6])
);

transducerOutput_Module c7(
	.clk(txCLK),
	.rst(fireReset),
	.isActive(transducer_is_active[7]),
	.onYourMark(fireFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	.chargeTime(transducer_chargetime),
	.phaseDelay(itxPioPhaseDelay[7]),
	.transducerOutput(otxTransducerOutput[7]),
	.fireComplete(fireComplete[7]),
	.warning(fireDanger[7])
);


triggerLedOutput_Module tl0(
	.clk(txCLK),
	.rst(trigLedReset),
	
	.restLevel(itxPioTriggerLedRestLevels[0]),
	.isActive(pio_trigger_led_output_is_active[0]),
	
	.onYourMark(trigLedFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	
	.duration(itxTriggerLedDuration[0]),
	.delay(itxTriggerLedDelay[0]),
	
	.triggerLedOutput(otxTriggerLedOutput[0]),
	.trigLedComplete(trigLedComplete[0]),
	
	.hardStop(trigLedStop)
);

triggerLedOutput_Module tl1(
	.clk(txCLK),
	.rst(trigLedReset),
	
	.restLevel(itxPioTriggerLedRestLevels[1]),
	.isActive(pio_trigger_led_output_is_active[1]),
	
	.onYourMark(trigLedFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	
	.duration(itxTriggerLedDuration[1]),
	.delay(itxTriggerLedDelay[1]),
	
	.triggerLedOutput(otxTriggerLedOutput[1]),
	.trigLedComplete(trigLedComplete[1]),
	
	.hardStop(trigLedStop)
);

triggerLedOutput_Module tl2(
	.clk(txCLK),
	.rst(trigLedReset),
	
	.restLevel(itxPioTriggerLedRestLevels[2]),
	.isActive(pio_trigger_led_output_is_active[2]),
	
	.onYourMark(trigLedFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	
	.duration(itxTriggerLedDuration[2]),
	.delay(itxTriggerLedDelay[2]),
	
	.triggerLedOutput(otxTriggerLedOutput[2]),
	.trigLedComplete(trigLedComplete[2]),
	
	.hardStop(trigLedStop)
);

triggerLedOutput_Module tl3(
	.clk(txCLK),
	.rst(trigLedReset),
	
	.restLevel(itxPioTriggerLedRestLevels[3]),
	.isActive(pio_trigger_led_output_is_active[3]),
	
	.onYourMark(trigLedFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	
	.duration(itxTriggerLedDuration[3]),
	.delay(itxTriggerLedDelay[3]),
	
	.triggerLedOutput(otxTriggerLedOutput[3]),
	.trigLedComplete(trigLedComplete[3]),
	
	.hardStop(trigLedStop)
);

triggerLedOutput_Module tl4(
	.clk(txCLK),
	.rst(trigLedReset),
	
	.restLevel(itxPioTriggerLedRestLevels[4]),
	.isActive(pio_trigger_led_output_is_active[4]),
	
	.onYourMark(trigLedFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	
	.duration(itxTriggerLedDuration[4]),
	.delay(itxTriggerLedDelay[4]),
	
	.triggerLedOutput(otxTriggerLedOutput[4]),
	.trigLedComplete(trigLedComplete[4]),
	
	.hardStop(trigLedStop)
);

triggerLedOutput_Module tl5(
	.clk(txCLK),
	.rst(trigLedReset),
	
	.restLevel(itxPioTriggerLedRestLevels[5]),
	.isActive(pio_trigger_led_output_is_active[5]),
	
	.onYourMark(trigLedFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	
	.duration(itxTriggerLedDuration[5]),
	.delay(itxTriggerLedDelay[5]),
	
	.triggerLedOutput(otxTriggerLedOutput[5]),
	.trigLedComplete(trigLedComplete[5]),
	
	.hardStop(trigLedStop)
);

triggerLedOutput_Module tl6(
	.clk(txCLK),
	.rst(trigLedReset),
	
	.restLevel(itxPioTriggerLedRestLevels[6]),
	.isActive(pio_trigger_led_output_is_active[6]),
	
	.onYourMark(trigLedFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	
	.duration(itxTriggerLedDuration[6]),
	.delay(itxTriggerLedDelay[6]),
	
	.triggerLedOutput(otxTriggerLedOutput[6]),
	.trigLedComplete(trigLedComplete[6]),
	
	.hardStop(trigLedStop)
);

triggerLedOutput_Module tl7(
	.clk(txCLK),
	.rst(trigLedReset),
	
	.restLevel(itxPioTriggerLedRestLevels[7]),
	.isActive(pio_trigger_led_output_is_active[7]),
	
	.onYourMark(trigLedFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	
	.duration(itxTriggerLedDuration[7]),
	.delay(itxTriggerLedDelay[7]),
	
	.triggerLedOutput(otxTriggerLedOutput[7]),
	.trigLedComplete(trigLedComplete[7]),
	
	.hardStop(trigLedStop)
);

triggerLedOutput_Module tl8(
	.clk(txCLK),
	.rst(trigLedReset),
	
	.restLevel(itxPioTriggerLedRestLevels[8]),
	.isActive(pio_trigger_led_output_is_active[8]),
	
	.onYourMark(trigLedFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	
	.duration(itxTriggerLedDuration[8]),
	.delay(itxTriggerLedDelay[8]),
	
	.triggerLedOutput(otxTriggerLedOutput[8]),
	.trigLedComplete(trigLedComplete[8]),
	
	.hardStop(trigLedStop)
);

triggerLedOutput_Module tl9(
	.clk(txCLK),
	.rst(trigLedReset),
	
	.restLevel(itxPioTriggerLedRestLevels[9]),
	.isActive(pio_trigger_led_output_is_active[9]),
	
	.onYourMark(trigLedFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	
	.duration(itxTriggerLedDuration[9]),
	.delay(itxTriggerLedDelay[9]),
	
	.triggerLedOutput(otxTriggerLedOutput[9]),
	.trigLedComplete(trigLedComplete[9]),
	
	.hardStop(trigLedStop)
);

triggerLedOutput_Module tl10(
	.clk(txCLK),
	.rst(trigLedReset),
	
	.restLevel(itxPioTriggerLedRestLevels[10]),
	.isActive(pio_trigger_led_output_is_active[10]),
	
	.onYourMark(trigLedFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	
	.duration(itxTriggerLedDuration[10]),
	.delay(itxTriggerLedDelay[10]),
	
	.triggerLedOutput(otxTriggerLedOutput[10]),
	.trigLedComplete(trigLedComplete[10]),
	
	.hardStop(trigLedStop)
);

triggerLedOutput_Module tl11(
	.clk(txCLK),
	.rst(trigLedReset),
	
	.restLevel(itxPioTriggerLedRestLevels[11]),
	.isActive(pio_trigger_led_output_is_active[11]),
	
	.onYourMark(trigLedFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	
	.duration(itxTriggerLedDuration[11]),
	.delay(itxTriggerLedDelay[11]),
	
	.triggerLedOutput(otxTriggerLedOutput[11]),
	.trigLedComplete(trigLedComplete[11]),
	
	.hardStop(trigLedStop)
);

triggerLedOutput_Module tl12(
	.clk(txCLK),
	.rst(trigLedReset),
	
	.restLevel(itxPioTriggerLedRestLevels[12]),
	.isActive(pio_trigger_led_output_is_active[12]),
	
	.onYourMark(trigLedFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	
	.duration(itxTriggerLedDuration[12]),
	.delay(itxTriggerLedDelay[12]),
	
	.triggerLedOutput(otxTriggerLedOutput[12]),
	.trigLedComplete(trigLedComplete[12]),
	
	.hardStop(trigLedStop)
);

triggerLedOutput_Module tl13(
	.clk(txCLK),
	.rst(trigLedReset),
	
	.restLevel(itxPioTriggerLedRestLevels[13]),
	.isActive(pio_trigger_led_output_is_active[13]),
	
	.onYourMark(trigLedFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	
	.duration(itxTriggerLedDuration[13]),
	.delay(itxTriggerLedDelay[13]),
	
	.triggerLedOutput(otxTriggerLedOutput[13]),
	.trigLedComplete(trigLedComplete[13]),
	
	.hardStop(trigLedStop)
);

triggerLedOutput_Module tl14(
	.clk(txCLK),
	.rst(trigLedReset),
	
	.restLevel(itxPioTriggerLedRestLevels[14]),
	.isActive(pio_trigger_led_output_is_active[14]),
	
	.onYourMark(trigLedFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	
	.duration(itxTriggerLedDuration[14]),
	.delay(itxTriggerLedDelay[14]),
	
	.triggerLedOutput(otxTriggerLedOutput[14]),
	.trigLedComplete(trigLedComplete[14]),
	
	.hardStop(trigLedStop)
);

triggerLedOutput_Module tl15(
	.clk(txCLK),
	.rst(trigLedReset),
	
	.restLevel(itxPioTriggerLedRestLevels[15]),
	.isActive(pio_trigger_led_output_is_active[15]),
	
	.onYourMark(trigLedFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	
	.duration(itxTriggerLedDuration[15]),
	.delay(itxTriggerLedDelay[15]),
	
	.triggerLedOutput(otxTriggerLedOutput[15]),
	.trigLedComplete(trigLedComplete[15]),
	
	.hardStop(trigLedStop)
);


endmodule


