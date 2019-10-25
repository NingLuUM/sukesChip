
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
	
	input	[31:0]			itxPioChargetime_reg,			// pio_tx_reg6
	
	input	[7:0]			itxPioTriggers,				// pio_tx_reg7
	input	[7:0]			itxPioTriggerRestLevels,	// pio_tx_reg7
	input	[7:0]			itxPioLeds,					// pio_tx_reg7
	input	[7:0]			itxPioLedRestLevels,		// pio_tx_reg7
	
	// PIO reg universally controlling transducer output. gets its own reg
	input	[7:0]			itxTransducerOutputMask,	// pio_tx_reg8
	
	
	
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
	
	output reg 	[11:0]		otxInstructionReadAddr,
	output reg 	[11:0]		otxPhaseChargeReadAddr,
	
	output reg	[7:0]		otxTransducerOutput,
	
	output reg	[7:0]		otxTriggerOutputs,
	output reg	[7:0]		otxLedOutputs,
	
	input					itxADCTriggerAck,
	output reg				otxADCTriggerLine,
	
	output reg	[31:0]		otxInterrupt
);

reg	[31:0]	itxInstructionType;
reg	[63:0]	itxInstruction;
reg	[31:0]	itxTimeUntilNext;

reg	[7:0][8:0]	transducerSafetyTimeout;
reg	[7:0][29:0] loopCounters;

wire [1:0] master_state;
assign master_state = itxControlComms[1:0];

wire [8:0] itxPioChargetime;
wire [8:0] itxPioChargetime2;
wire [8:0] itxPioChargetime3;
wire [1:0] itxPioChargetime_selector;

wire [7:0] pio_output_commands;
wire [7:0] pio_trigger_outputs;
wire [7:0] pio_led_output;
wire pio_adc_trigger;
wire pio_no_interrupt;
wire ram_pio_control_toggle_indicator;



assign pio_output_commands 	= itxControlComms[9:2];
assign pio_trigger_outputs 	= itxControlComms[17:10];
assign pio_led_outputs 		= itxControlComms[25:18];
assign pio_adc_trigger 		= itxControlComms[26];
assign pio_no_interrupt = itxControlComms[30];
assign ram_pio_control_toggle_indicator = itxControlComms[31];



initial
begin
	otxTransducerOutput = 2'b0;
	otxADCTriggerLine = 1'b0;
	ctCounter1 = 9'b0;
	ctCounter2 = 9'b0;
	delay_cntr = 16'b0;
end


// CONTROL STATES
parameter	[1:0]	CASE_IDLE 			= 2'b00;
parameter	[1:0]	CASE_PIO_CONTROL	= 2'b01;
parameter	[1:0]	CASE_RAM_CONTROL	= 2'b10;

// COMMANDS THAT CAN BE ISSUED THROUGH PIOs
parameter	[7:0]	PIO_CMD_DISABLE				= 8'b00000000;
parameter	[7:0]	PIO_CMD_IDLE				= 8'b00000001;
parameter	[7:0]	PIO_CMD_FIRE 				= 8'b00000010;
parameter	[7:0]	PIO_CMD_SET_TRIGS 			= 8'b00000100;
parameter	[7:0]	PIO_CMD_SET_LEDS			= 8'b00001000;
parameter	[7:0]	PIO_CMD_ISSUE_RCV_TRIG		= 8'b00010000;
parameter	[7:0]	PIO_CMD_ASYNC_RCV_TRIG		= 8'b00100000;
parameter	[7:0]	PIO_CMD_RESET_RCV_TRIG		= 8'b01000000;
parameter	[7:0]	PIO_CMD_RESET_INTERRUPT		= 8'b10000000;

// PIO commands only get issued when the PIO state changes, need to store previous state
reg [7:0]			pio_cmd_previous;


reg [7:0][15:0] 	transducer_phase;
reg [8:0]			transducer_chargetime;
reg [7:0]			transducer_mask;
reg [7:0]			trig_output_buffer;
reg [7:0]			led_output_buffer;
reg [1:0]			adcTrigFlag;
reg	[1:0]			fireFlag;
wire [7:0]			fireComplete;



always @(posedge txCLK)
begin

	case ( master_state )
		CASE_IDLE:
			begin
				transducer_chargetime <= 9'b0;
				fireFlag <= 2'b0;
				transducer_mask <= 8'b0;
				otxTriggerOutputs <= itxPioTriggerRestLevels;
				otxLedOutputs <= itxPioLedRestLevels;
			end
		
		CASE_PIO_CONTROL:
			begin
				if ( pio_output_commands && !otxEmergency )
				begin			
					if( ( transducer_mask ^ itxTransducerOutputMask ) ) transducer_mask <= itxTransducerOutputMask;
					
					if ( pio_output_commands ^ pio_cmd_previous ) 
					begin
						pio_cmd_previous <= pio_output_commands;
						
						// all commands need to be synced
						if( !( pio_output_commands & PIO_CMD_RESET_INTERRUPT ) )
						begin
							if( !otxWaitForMe ) otxWaitForMe <= 1'b1;
							otxInterrupt[0] <= 1'b1;
						end
						
						if( ( pio_output_commands & 8'b00111111 ) ) 
						begin
							
							/******************************************************/
							/*** FIRE FROM PIO COMMAND ****************************/
							/******************************************************/
							if( ( pio_output_commands & PIO_CMD_FIRE ) && !fireFlag )
							begin
								transducer_phase[0] <= itxPioPhaseDelay_ch0_ch1[15:0];
								transducer_phase[1] <= itxPioPhaseDelay_ch0_ch1[31:16];
								transducer_phase[2] <= itxPioPhaseDelay_ch2_ch3[15:0];
								transducer_phase[3] <= itxPioPhaseDelay_ch2_ch3[31:16];
								transducer_phase[4] <= itxPioPhaseDelay_ch4_ch5[15:0];
								transducer_phase[5] <= itxPioPhaseDelay_ch4_ch5[31:16];
								transducer_phase[6] <= itxPioPhaseDelay_ch6_ch7[15:0];
								transducer_phase[7] <= itxPioPhaseDelay_ch6_ch7[31:16];
								transducer_chargetime <= itxPioChargeTime;
								fireFlag <= 2'b11;
							end
							else
							begin
								otxInterrupt[31] <= 1'b1;
								otxEmergency <= 1'b1;
							end
							
							/******************************************************/
							/*** SET TRIGS FROM PIO COMMAND ***********************/
							/******************************************************/
							if( pio_output_commands & PIO_CMD_SET_TRIGS )
							begin
								trig_output_buffer <= pio_trigger_outputs;
							end
							
							/******************************************************/
							/*** SET LEDS FROM PIO COMMAND ************************/
							/******************************************************/
							if( pio_output_commands & PIO_CMD_SET_LEDS )
							begin
								led_output_buffer <= pio_led_outputs;
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
					
					/******************************************************/
					/*** NO NEW FIRE CMDS UNTIL PREVIOUS FIRE COMPLETE ****/
					/******************************************************/
					if( ( pio_output_commands ^ PIO_CMD_DISABLE ) && fireFlag[0] ) 
					begin
						fireFlag[0] <= 1'b0;
					end
					
					if( otxWaitForMe & itxExternalTrigger )
					begin
						otxTriggerOutputs <= trig_output_buffer;
						otxLedOutputs <= led_output_buffer;
						if( adcTrigFlag[0] ) otxADCTriggerLine <= 1'b1;
						otxWaitForMe <= 1'b0;
					end
					
					/******************************************************/
					/*** NO NEW FIRE CMDS UNTIL PREVIOUS FIRE COMPLETE ****/
					/******************************************************/
					if ( fireComplete == 8'b11111111 )
					begin
						if ( fireFlag[1] ) fireFlag[1] <= 1'b0;
					end
					
				end
				else if ( !ram_pio_control_toggle_indicator | otxEmergency ) 
				begin
					transducer_chargetime <= 9'b0;
					fireFlag <= 2'b0;
					transducer_mask <= 8'b0;
					otxTriggerOutputs <= itxPioTriggerRestLevels;
					otxLedOutputs <= itxPioLedRestLevels;
					pio_cmd_previous <= pio_output_commands
					otxInterrupt[15] <= 1'b1;
				end
				
				
			end
		
		CASE_RAM_CONTROL:
			begin
				if(transducer_mask ^ itxTransducerOutputMask) transducer_mask <= itxTransducerOutputMask;
			
			end
		
		default:
			begin
				transducer_chargetime <= 9'b0;
				fireFlag <= 2'b0;
				transducer_mask <= 8'b0;
			
			end

	endcase
	
end

transducerOutput_Module c0(
	.clk(txCLK),
	.mask(transducer_mask[0]),
	.onYourMark(fireFlag[1]),
	.getSet(otxWaitForMe),
	.GOGOGO_EXCLAMATION(itxExternalTrigger),
	.chargeTime(transducer_chargetime),
	.phaseDelay(transducer_phase[0]),
	.transducerOutput(otxTransducerOutput[0]),
	.fireComplete(fireComplete[0])
);

transducerOutput_Module c1(
	.clk(txCLK),
	.mask(transducer_mask[1]),
	.onYourMark(fireFlag[1]),
	.getSet(otxWaitForMe),
	.GOGOGO_EXCLAMATION(itxExternalTrigger),
	.chargeTime(transducer_chargetime),
	.phaseDelay(transducer_phase[1]),
	.transducerOutput(otxTransducerOutput[1]),
	.fireComplete(fireComplete[1])
);

transducerOutput_Module c2(
	.clk(txCLK),
	.mask(transducer_mask[2]),
	.onYourMark(fireFlag[1]),
	.getSet(otxWaitForMe),
	.GOGOGO_EXCLAMATION(itxExternalTrigger),
	.chargeTime(transducer_chargetime),
	.phaseDelay(transducer_phase[2]),
	.transducerOutput(otxTransducerOutput[2]),
	.fireComplete(fireComplete[2])
);

transducerOutput_Module c3(
	.clk(txCLK),
	.mask(transducer_mask[3]),
	.onYourMark(fireFlag[1]),
	.getSet(otxWaitForMe),
	.GOGOGO_EXCLAMATION(itxExternalTrigger),
	.chargeTime(transducer_chargetime),
	.phaseDelay(transducer_phase[3]),
	.transducerOutput(otxTransducerOutput[3]),
	.fireComplete(fireComplete[3])
);

transducerOutput_Module c4(
	.clk(txCLK),
	.mask(transducer_mask[4]),
	.onYourMark(fireFlag[1]),
	.getSet(otxWaitForMe),
	.GOGOGO_EXCLAMATION(itxExternalTrigger),
	.chargeTime(transducer_chargetime),
	.phaseDelay(transducer_phase[4]),
	.transducerOutput(otxTransducerOutput[4]),
	.fireComplete(fireComplete[4])
);

transducerOutput_Module c5(
	.clk(txCLK),
	.mask(transducer_mask[5]),
	.onYourMark(fireFlag[1]),
	.getSet(otxWaitForMe),
	.GOGOGO_EXCLAMATION(itxExternalTrigger),
	.chargeTime(transducer_chargetime),
	.phaseDelay(transducer_phase[5]),
	.transducerOutput(otxTransducerOutput[5]),
	.fireComplete(fireComplete[5])
);

transducerOutput_Module c6(
	.clk(txCLK),
	.mask(transducer_mask[6]),
	.onYourMark(fireFlag[1]),
	.getSet(otxWaitForMe),
	.GOGOGO_EXCLAMATION(itxExternalTrigger),
	.chargeTime(transducer_chargetime),
	.phaseDelay(transducer_phase[6]),
	.transducerOutput(otxTransducerOutput[6]),
	.fireComplete(fireComplete[6])
);

transducerOutput_Module c7(
	.clk(txCLK),
	.mask(transducer_mask[7]),
	.onYourMark(fireFlag[1]),
	.getSet(otxWaitForMe),
	.GOGOGO_EXCLAMATION(itxExternalTrigger),
	.chargeTime(transducer_chargetime),
	.phaseDelay(transducer_phase[7]),
	.transducerOutput(otxTransducerOutput[7]),
	.fireComplete(fireComplete[7])
);

endmodule


