
module Output_Control_Module(

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
	
	input	[7:0]			itxPioTriggerRestLevels,	// pio_tx_reg7
	input	[6:0]			itxPioLedRestLevels,		// pio_tx_reg7
	input	[7:0]			itxTransducerOutputMask,	// pio_tx_reg7
	input	[9:0]			itxBoardIdentifiers,		// pio_tx_reg7
	
	// input triggers and 'fast' comm lines
	input					itxExternalTrigger,
	
	input					itxWaitSignal,
	output reg				otxWaitForMe,
	
	// in case error is detected, will force system shutdown
	input					itxEmergency,
	output reg				otxEmergency,
	
	// RAM
	input	[127:0]			itxInstructionMem,
	input	[127:0]			itxPhaseChargeMem,
	
	output reg 	[12:0]		otxInstructionReadAddr,
	output reg 	[12:0]		otxPhaseChargeReadAddr,
	
	output reg	[7:0]		otxTransducerOutput,
	
	output reg	[7:0]		otxTriggerOutputs,
	output reg	[6:0]		otxLedOutputs,
	output reg				otxVarRxAtten,
	
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
parameter	[8:0]	PIO_CMD_DISABLE				= 9'b000000000;
parameter	[8:0]	PIO_CMD_IDLE				= 9'b000000001;
parameter	[8:0]	PIO_CMD_SET_AMP_PHASE 		= 9'b000000010;
parameter	[8:0]	PIO_CMD_SET_TRIG_LEDS		= 9'b000000100;
parameter	[8:0]	PIO_CMD_ISSUE_RCV_TRIG		= 9'b000001000;
parameter	[8:0]	PIO_CMD_ASYNC_RCV_TRIG		= 9'b000010000;

parameter	[8:0]	PIO_CMD_FIRE 				= 9'b000100000;
parameter	[8:0]	PIO_CMD_RESET_RCV_TRIG		= 9'b001000000;
parameter	[8:0]	PIO_CMD_RESET_INTERRUPT		= 9'b010000000;

parameter	[8:0]	PIO_CMD_RESET_INTERRUPT		= 9'b100000000;

// COMMANDS THAT CAN BE ISSUED THROUGH RAM
parameter	[15:0]	RAM_CMD_IDLE				= 16'b0000000000000000;
parameter	[15:0]	RAM_CMD_SET_PHASE			= 16'b0000000000000001;
parameter	[15:0]	RAM_CMD_FIRE 				= 16'b0000000000000010;
parameter	[15:0]	RAM_CMD_FIRE_FROM_PIO		= 16'b0000000000000100;
parameter	[15:0]	RAM_CMD_SET_AMP 			= 16'b0000000000001000;
parameter	[15:0]	RAM_CMD_SET_AMP_FROM_PIO	= 16'b0000000000010000;
parameter	[15:0]	RAM_CMD_SET_TRIGS			= 16'b0000000000100000;
parameter	[15:0]	RAM_CMD_SET_LEDS			= 16'b0000000001000000;
parameter	[15:0]	RAM_CMD_ISSUE_RCV_TRIG		= 16'b0000000010000000;
parameter	[15:0]	RAM_CMD_START_LOOP			= 16'b0000000100000000;
parameter	[15:0]	RAM_CMD_END_LOOP			= 16'b0000001000000000;


parameter	[15:0]	RAM_CMD_START_SYNCHRONOUS	= 16'b0000010000000000;
parameter	[15:0]	RAM_CMD_SYNCHRONOUS_WAIT	= 16'b0000100000000000;

parameter	[15:0]	RAM_CMD_SET_VAR_ATTEN		= 16'b0001000000000000;

parameter	[15:0]	RAM_CMD_UPDATE_RAM_PROGRAM	= 16'b0010000000000000;
parameter	[15:0]	RAM_CMD_CEDE_CONTROL_TO_PIO = 16'b0100000000000000;
parameter	[15:0]	RAM_CMD_HALT				= 16'b1000000000000000;


/**********************************************************************/
/*** TX CONTROL REGISTER(S) *******************************************/
/**********************************************************************/
wire [1:0] control_state;
assign control_state = itxControlComms[1:0];

wire [7:0] pio_output_commands;
wire [8:0] pio_trigger_outputs;
wire [6:0] pio_led_output;
wire 	   pio_var_atten_output;
wire [1:0] pio_charge_selector;
wire pio_adc_trigger;
wire pio_no_interrupt;
wire pio_ram_control_toggle_indicator;

assign pio_output_commands 	= itxControlComms[10:2];
assign pio_trigger_outputs 	= itxControlComms[18:11];
assign pio_led_outputs 		= itxControlComms[25:19];
assign pio_var_atten_output = itxControlComms[26];
assign pio_charge_selector	= itxControlComms[28:27];
assign pio_adc_trigger 		= itxControlComms[29];
assign pio_no_interrupt		= itxControlComms[30];
assign pio_ram_control_toggle_indicator = itxControlComms[31];


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




/**********************************************************************/
/*** RAM REGISTER ASSIGNMENTS *****************************************/
/**********************************************************************/
reg	[1:0][15:0]	itxInstructionType;
reg	[1:0][79:0]	itxInstruction;
reg	[1:0][31:0]	itxTimeUntilNext;
reg	[7:0][29:0] loopCounters;
reg [7:0][12:0] loopStartAddr;

wire [12:0]		ramPhaseAddr;
wire [8:0]		ramChargeTime;
wire [7:0]		ramTrigVals;
wire [6:0]		ramLedVals;
wire			ramVarRxAtten;
wire [1:0]		ramChargeAddr;
wire [2:0]		ramLoopNumber;
wire [29:0]		ramLoopCounter;
wire			ramPioControlToggle;

assign ramLookupAddr = itxInstruction[1][12:0];

assign ramChargeTime = itxInstruction[0][21:13];
assign ramTrigVals = itxInstruction[0][29:22];
assign ramLedVals = itxInstruction[0][36:30];
assign ramVarRxAtten = itxInstruction[0][37];
assign ramChargeAddr = itxInstruction[0][39:38];

assign ramLoopNumber = itxInstruction[1][42:40];
assign ramLoopCounter = itxInstruction[1][72:43];

assign ramPioControlToggle = itxInstruction[0][73];

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


reg [7:0][15:0] 	transducer_phase;
reg [8:0]			transducer_chargetime;
reg [7:0]			transducer_mask;
reg [7:0]			trig_output_buffer;
reg [6:0]			led_output_buffer;
reg					var_atten_buffer;
reg [1:0]			adcTrigFlag;
reg					fireFlag;
reg					fireReset;
wire [7:0]			fireDanger;
wire [7:0]			fireComplete;




reg [31:0] sync_timer;
reg [63:0] cmd_timer;

initial
begin
	otxInstructionReadAddr = 13'b0;
	otxPhaseChargeReadAddr = 13'b0;
	otxTransducerOutput = 8'b0;
	otxADCTriggerLine = 1'b0;
	
	transducer_chargetime = 9'b0;
	transducer_phase[0] = 16'b0;
	transducer_phase[1] = 16'b0;
	transducer_phase[2] = 16'b0;
	transducer_phase[3] = 16'b0;
	transducer_phase[4] = 16'b0;
	transducer_phase[5] = 16'b0;
	transducer_phase[6] = 16'b0;
	transducer_phase[7] = 16'b0;
	
	loopCounters[0] = 30'b0;
	loopCounters[1] = 30'b0;
	loopCounters[2] = 30'b0;
	loopCounters[3] = 30'b0;
	loopCounters[4] = 30'b0;
	loopCounters[5] = 30'b0;
	loopCounters[6] = 30'b0;
	loopCounters[7] = 30'b0;
	sync_timer = 32'b0;
	cmd_timer = 64'b0;
	internalTrigger = 1'b0;
end


always @(posedge txCLK)
begin

	case ( control_state )
		CASE_IDLE:
			begin
				transducer_chargetime <= 9'b0;
				fireFlag <= 1'b0;
				transducer_mask <= 8'b11111111;
				otxTriggerOutputs <= itxPioTriggerRestLevels;
				otxLedOutputs <= itxPioLedRestLevels;
				internalTrigger <= 1'b0;
			end
		
		CASE_PIO_CONTROL:
			begin
				if ( pio_output_commands &&  !otxEmergency && !fireDanger )
				begin
							
					if( ( transducer_mask ^ itxTransducerOutputMask ) )
					begin
						transducer_mask <= itxTransducerOutputMask;
					end
										
					if ( pio_output_commands ^ pio_cmd_previous ) 
					begin
						pio_cmd_previous <= pio_output_commands;
						
						// all commands need to be synced
						if( !( pio_output_commands & PIO_CMD_RESET_INTERRUPT ) )
						begin
							if( !otxWaitForMe ) otxWaitForMe <= 1'b1;
							if( !isSolo ) otxInterrupt[0] <= 1'b1;
						end
						
						/******************************************************/
						/*** INTERNAL TRIGGER FOR SOLO BOARDS *****************/
						/******************************************************/
						if( ( pio_output_commands & 9'b000111111 ) ) 
						begin
							if ( isSolo & !internalTrigger ) internalTrigger <= 1'b1;
						end
							
						/******************************************************/
						/*** FIRE FROM PIO COMMAND ****************************/
						/******************************************************/
						if( ( pio_output_commands & PIO_CMD_FIRE ) && !fireFlag )
						begin
							if ( !fireFlag )
							begin
								fireFlag <= 1'b1;
								if ( fireReset ) fireReset <= 1'b0;
							end
							else
							begin
								otxInterrupt[31] <= 1'b1;
								otxEmergency <= 1'b1;
							end
						end
						
						/******************************************************/
						/*** SET CHARGETIME/AMP & PHASE DELAYS OF PULSES ******/
						/******************************************************/
						if( ( pio_output_commands & PIO_CMD_SET_AMP_PHASE ) && !fireFlag )
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
							
							transducer_phase[0] <= itxPioPhaseDelay[0];
							transducer_phase[1] <= itxPioPhaseDelay[1];
							transducer_phase[2] <= itxPioPhaseDelay[2];
							transducer_phase[3] <= itxPioPhaseDelay[3];
							transducer_phase[4] <= itxPioPhaseDelay[4];
							transducer_phase[5] <= itxPioPhaseDelay[5];
							transducer_phase[6] <= itxPioPhaseDelay[6];
							transducer_phase[7] <= itxPioPhaseDelay[7];
						end
						
						/******************************************************/
						/*** SET TRIGS AND FROM PIO COMMAND *******************/
						/******************************************************/
						if( pio_output_commands & PIO_CMD_SET_TRIG_LEDS )
						begin
							trig_output_buffer <= pio_trigger_outputs ^ itxPioTriggerRestLevels;
							led_output_buffer <= pio_led_outputs ^ itxPioLedRestLevels;
						end
						
						/******************************************************/
						/*** ISSUE RCV SYS TRIG FROM PIO COMMAND **************/
						/******************************************************/
						if( ( pio_output_commands & PIO_CMD_ISSUE_RCV_TRIG ) && !adcTrigFlag )
						begin
							adcTrigFlag[0] <= 1'b1;
							if( !( pio_output_commands & PIO_CMD_ASYNC_RCV_TRIG ) ) otxInterrupt[3] <= 1'b1;
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
							otxVarRxAtten <= pio_var_atten_output;
						end
					end
					
					/******************************************************/
					/*** RESET RCV SYS TRIG FROM PIO COMMAND **************/
					/******************************************************/
					if( pio_output_commands & PIO_CMD_RESET_RCV_TRIG )
					begin
						if( adcTrigFlag[0] & adcTrigFlag[1] )
						begin
							adcTrigFlag <= 2'b00;
							if( pio_output_commands & ~PIO_CMD_RESET_INTERRUPT ) otxInterrupt[1] <= 1'b1;
						end
					end
					
					/******************************************************/
					/*** RESET INTERRUPT FROM PIO COMMAND *****************/
					/******************************************************/
					if( ( pio_output_commands & PIO_CMD_RESET_INTERRUPT	) && otxInterrupt )
					begin
						otxInterrupt <= 32'b0;
					end
						
					/******************************************************/
					/*** ACKNOWLEDGE RCV SYS's RESPONSE TO TRIGGER ********/
					/******************************************************/
					if ( adcTrigFlag[0] & !adcTrigFlag[1] & itxADCTriggerAck )
					begin
						otxADCTriggerLine <= 1'b0;
						adcTrigFlag[1] <= 1'b1;
					end
					
					if( ( otxWaitForMe | isSolo ) & triggerSignal )
					begin
						if( otxTriggerOutputs ^ trig_output_buffer )
						begin
							otxTriggerOutputs <= trig_output_buffer;
						end
						
						if( otxLedOutputs ^ led_output_buffer )
						begin
							otxLedOutputs <= led_output_buffer;
						end
						
						if( adcTrigFlag[0] )
						begin
							otxADCTriggerLine <= 1'b1;
						end
						
						otxWaitForMe <= 1'b0;
						if ( isSolo & internalTrigger ) internalTrigger <= 1'b0;
						
					end

					/******************************************************/
					/*** NO NEW FIRE CMDS UNTIL PREVIOUS FIRE COMPLETE ****/
					/******************************************************/
					if ( fireComplete == 8'b11111111 )
					begin
						if ( fireFlag ) fireFlag <= 1'b0;
						if ( !fireReset ) fireReset <= 1'b1;
					end
					
				end
				else if ( ( !pio_ram_control_toggle_indicator | otxEmergency ) || fireDanger ) 
				begin
					transducer_chargetime <= 9'b0;
					fireFlag <= 1'b0;
					fireReset <= 1'b1;
					transducer_mask <= 8'b0;
					otxTriggerOutputs <= itxPioTriggerRestLevels;
					otxLedOutputs <= itxPioLedRestLevels;
					pio_cmd_previous <= pio_output_commands
					otxInterrupt[15] <= 1'b1;
				end
				else if ( pio_ram_control_toggle_indicator )
				begin
					sync_timer = 32'b0;
					cmd_timer = 64'b0;
					otxInstructionReadAddr = itxPioControlSettings[12:0];
					otxPhaseChargeReadAddr = itxPioControlSettings[25:13];
				end
				
			end
		
		CASE_RAM_CONTROL:
			begin
				/******************************************************/
				/*** PLACE HOLDER UNTIL THIS IS IMPLEMENTED ***********/
				/******************************************************/
				transducer_chargetime <= 9'b0;
				fireFlag <= 2'b0;
				transducer_mask <= 8'b0;
				internalTrigger <= 1'b0;
			end
		
		CASE_PIO_CONTROL_RAM_SUBCALL:
			begin
				/******************************************************/
				/*** PLACE HOLDER UNTIL THIS IS IMPLEMENTED ***********/
				/******************************************************/
				transducer_chargetime <= 9'b0;
				fireFlag <= 2'b0;
				transducer_mask <= 8'b0;
				internalTrigger <= 1'b0;
			end
			
		default:
			begin
				transducer_chargetime <= 9'b0;
				fireFlag <= 2'b0;
				transducer_mask <= 8'b0;
				internalTrigger <= 1'b0;
			
			end

	endcase
	
end



























transducerOutput_Module c0(
	.clk(txCLK),
	.rst(fireReset),
	.mask(transducer_mask[0]),
	.onYourMark(fireFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	.chargeTime(transducer_chargetime),
	.phaseDelay(transducer_phase[0]),
	.transducerOutput(otxTransducerOutput[0]),
	.fireComplete(fireComplete[0]),
	.warning(fireDanger[0])
);

transducerOutput_Module c1(
	.clk(txCLK),
	.rst(fireReset),
	.mask(transducer_mask[1]),
	.onYourMark(fireFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	.chargeTime(transducer_chargetime),
	.phaseDelay(transducer_phase[1]),
	.transducerOutput(otxTransducerOutput[1]),
	.fireComplete(fireComplete[1]),
	.warning(fireDanger[1])
);

transducerOutput_Module c2(
	.clk(txCLK),
	.rst(fireReset),
	.mask(transducer_mask[2]),
	.onYourMark(fireFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	.chargeTime(transducer_chargetime),
	.phaseDelay(transducer_phase[2]),
	.transducerOutput(otxTransducerOutput[2]),
	.fireComplete(fireComplete[2]),
	.warning(fireDanger[2])
);

transducerOutput_Module c3(
	.clk(txCLK),
	.rst(fireReset),
	.mask(transducer_mask[3]),
	.onYourMark(fireFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	.chargeTime(transducer_chargetime),
	.phaseDelay(transducer_phase[3]),
	.transducerOutput(otxTransducerOutput[3]),
	.fireComplete(fireComplete[3]),
	.warning(fireDanger[3])
);

transducerOutput_Module c4(
	.clk(txCLK),
	.rst(fireReset),
	.mask(transducer_mask[4]),
	.onYourMark(fireFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	.chargeTime(transducer_chargetime),
	.phaseDelay(transducer_phase[4]),
	.transducerOutput(otxTransducerOutput[4]),
	.fireComplete(fireComplete[4]),
	.warning(fireDanger[4])
);

transducerOutput_Module c5(
	.clk(txCLK),
	.rst(fireReset),
	.mask(transducer_mask[5]),
	.onYourMark(fireFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	.chargeTime(transducer_chargetime),
	.phaseDelay(transducer_phase[5]),
	.transducerOutput(otxTransducerOutput[5]),
	.fireComplete(fireComplete[5]),
	.warning(fireDanger[5])
);

transducerOutput_Module c6(
	.clk(txCLK),
	.rst(fireReset),
	.mask(transducer_mask[6]),
	.onYourMark(fireFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	.chargeTime(transducer_chargetime),
	.phaseDelay(transducer_phase[6]),
	.transducerOutput(otxTransducerOutput[6]),
	.fireComplete(fireComplete[6]),
	.warning(fireDanger[6])
);

transducerOutput_Module c7(
	.clk(txCLK),
	.rst(fireReset),
	.mask(transducer_mask[7]),
	.onYourMark(fireFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	.chargeTime(transducer_chargetime),
	.phaseDelay(transducer_phase[7]),
	.transducerOutput(otxTransducerOutput[7]),
	.fireComplete(fireComplete[7]),
	.warning(fireDanger[7])
);

endmodule


