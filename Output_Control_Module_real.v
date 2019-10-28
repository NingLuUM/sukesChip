
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
	input	[7:0]			itxPioLedRestLevels,		// pio_tx_reg7
	input	[7:0]			itxTransducerOutputMask,	// pio_tx_reg7
	
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

wire [1:0] control_state;
assign control_state = itxControlComms[1:0];

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


wire [7:0] pio_output_commands;
wire [7:0] pio_trigger_outputs;
wire [7:0] pio_led_output;
wire [1:0] pio_charge_selector;
wire pio_adc_trigger;
wire pio_no_interrupt;
wire ram_pio_control_toggle_indicator;


assign pio_output_commands 	= itxControlComms[10:2];
assign pio_trigger_outputs 	= itxControlComms[18:11];
assign pio_led_outputs 		= itxControlComms[26:19];
assign pio_charge_selector	= itxControlComms[28:27];
assign pio_adc_trigger 		= itxControlComms[29];
assign pio_no_interrupt		= itxControlComms[30];
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
parameter	[8:0]	PIO_CMD_DISABLE				= 9'b000000000;
parameter	[8:0]	PIO_CMD_IDLE				= 9'b000000001;
parameter	[8:0]	PIO_CMD_FIRE 				= 9'b000000010;
parameter	[8:0]	PIO_CMD_SET_AMP 			= 9'b000000100;
parameter	[8:0]	PIO_CMD_SET_TRIGS 			= 9'b000001000;
parameter	[8:0]	PIO_CMD_SET_LEDS			= 9'b000010000;
parameter	[8:0]	PIO_CMD_ISSUE_RCV_TRIG		= 9'b000100000;
parameter	[8:0]	PIO_CMD_ASYNC_RCV_TRIG		= 9'b001000000;
parameter	[8:0]	PIO_CMD_RESET_RCV_TRIG		= 9'b010000000;
parameter	[8:0]	PIO_CMD_RESET_INTERRUPT		= 9'b100000000;

// PIO commands only get issued when the PIO state changes, need to store previous state
reg [8:0]			pio_cmd_previous;


reg [7:0][15:0] 	transducer_phase;
reg [8:0]			transducer_chargetime;
reg [7:0]			transducer_mask;
reg [7:0]			trig_output_buffer;
reg [7:0]			led_output_buffer;
reg [1:0]			adcTrigFlag;
reg					fireFlag;
reg					fireReset;
wire [7:0]			fireDanger;
wire [7:0]			fireComplete;



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
			end
		
		CASE_PIO_CONTROL:
			begin
				if ( pio_output_commands &&  !otxEmergency && !fireDanger )
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
						
						if( ( pio_output_commands & 9'b001111111 ) ) 
						begin
							
							/******************************************************/
							/*** FIRE FROM PIO COMMAND ****************************/
							/******************************************************/
							if( ( pio_output_commands & PIO_CMD_FIRE ) && !fireFlag )
							begin
								transducer_phase[0] <= itxPioPhaseDelay[0];
								transducer_phase[1] <= itxPioPhaseDelay[1];
								transducer_phase[2] <= itxPioPhaseDelay[2];
								transducer_phase[3] <= itxPioPhaseDelay[3];
								transducer_phase[4] <= itxPioPhaseDelay[4];
								transducer_phase[5] <= itxPioPhaseDelay[5];
								transducer_phase[6] <= itxPioPhaseDelay[6];
								transducer_phase[7] <= itxPioPhaseDelay[7];
								
								fireFlag <= 1'b1;
								if ( fireReset ) fireReset <= 1'b0;
							end
							else
							begin
								otxInterrupt[31] <= 1'b1;
								otxEmergency <= 1'b1;
							end
							
							/******************************************************/
							/*** SET CHARGETIME/AMP OF PULSES *********************/
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
							/*** SET TRIGS FROM PIO COMMAND ***********************/
							/******************************************************/
							if( pio_output_commands & PIO_CMD_SET_TRIGS )
							begin
								trig_output_buffer <= pio_trigger_outputs ^ itxPioTriggerRestLevels;
							end
							
							/******************************************************/
							/*** SET LEDS FROM PIO COMMAND ************************/
							/******************************************************/
							if( pio_output_commands & PIO_CMD_SET_LEDS )
							begin
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
					
					if( otxWaitForMe & itxExternalTrigger )
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
				else if ( ( !ram_pio_control_toggle_indicator | otxEmergency ) || fireDanger ) 
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
	.rst(fireReset),
	.mask(transducer_mask[0]),
	.onYourMark(fireFlag),
	.GOGOGO_EXCLAMATION(itxExternalTrigger),
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
	.GOGOGO_EXCLAMATION(itxExternalTrigger),
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
	.GOGOGO_EXCLAMATION(itxExternalTrigger),
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
	.GOGOGO_EXCLAMATION(itxExternalTrigger),
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
	.GOGOGO_EXCLAMATION(itxExternalTrigger),
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
	.GOGOGO_EXCLAMATION(itxExternalTrigger),
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
	.GOGOGO_EXCLAMATION(itxExternalTrigger),
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
	.GOGOGO_EXCLAMATION(itxExternalTrigger),
	.chargeTime(transducer_chargetime),
	.phaseDelay(transducer_phase[7]),
	.transducerOutput(otxTransducerOutput[7]),
	.fireComplete(fireComplete[7]),
	.warning(fireDanger[7])
);

endmodule


