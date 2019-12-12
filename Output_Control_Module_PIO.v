
module Output_Control_Module_PIO (

	input 					txCLK,
	
	/*	all PIO registers will be 32bit wide and packed with multiple
		commands unless otherwise specified */
	
	// PIO register for controlling module outputs
	input	[31:0]			itxControlComms, 				// pio_tx_reg0
	
	// static settings for outputs	
	input	[7:0]			itxBoardIdentifiers,			// pio_tx_reg1
	input	[7:0]			itxTransducerOutputIsActive,	// pio_tx_reg1
	input	[15:0]			itxPioTriggerLedRestLevels,		// pio_tx_reg1
	
	
	input	[31:0]			itxPioCommands, 				// pio_tx_reg2
	

	// PIO output control registers
	input	[31:0]			itxPioPhaseDelay_ch0_ch1, 		// pio_tx_reg3
	input	[31:0]			itxPioPhaseDelay_ch2_ch3, 		// pio_tx_reg4
	input	[31:0]			itxPioPhaseDelay_ch4_ch5, 		// pio_tx_reg5
	input	[31:0]			itxPioPhaseDelay_ch6_ch7, 		// pio_tx_reg6
	
	input	[8:0]			itxPioChargetime_reg,			// pio_tx_reg7
	input	[22:0]			itxFireDelay,					// pio_tx_reg7
	
	input	[31:0]			itxRecvTrigDelay,				// pio_tx_reg8
	
	input	[15:0][31:0]	itxTrigLedDurationAndDelay,		// pio_tx_reg9-24
	input	[1:0][31:0]		itxTimeUntilNextInterrupt,		// pio_tx_reg25-26
	

	// input triggers and 'fast' comm lines
	input					itxExternalTrigger,
	
	output [7:0]			otxTransducerOutput,
	
	output [15:0]			otxTriggerLedOutput,
	
	input					itxADCTriggerAck,
	output					otxADCTriggerLine,
	
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
parameter	[15:0]	PIO_CMD_IDLE					= 16'b0000000000000000;
parameter	[15:0]	PIO_CMD_SET_TRIG_LEDS			= 16'b0000000000000001;
parameter	[15:0]	PIO_CMD_ISSUE_RCV_TRIG			= 16'b0000000000000010;
parameter	[15:0]	PIO_CMD_FIRE 					= 16'b0000000000000100;
parameter	[15:0]	PIO_CMD_TIME_UNTIL_INTERRUPT	= 16'b0000000000001000;
parameter	[15:0]	PIO_CMD_SET_AMP		 			= 16'b0000000000010000;
parameter	[15:0]	PIO_CMD_SET_PHASE	 			= 16'b0000000000100000;
parameter	[15:0]	PIO_CMD_RESET_RCV_TRIG			= 16'b0010000000000000;
parameter	[15:0]	PIO_CMD_RESET_INTERRUPT			= 16'b0100000000000000;
parameter	[15:0]	PIO_CMD_NEW_COMMAND_FLAG		= 16'b1000000000000000;

/**********************************************************************/
/*** TX CONTROL REGISTER(S) *******************************************/
/**********************************************************************/
wire [1:0] control_state;
assign control_state = itxControlComms[1:0];

wire [15:0]	pio_output_commands;
assign pio_output_commands 				= itxPioCommands[15:0];

/**********************************************************************/
/*** PIO REGISTER ASSIGNMENTS *****************************************/
/**********************************************************************/

wire [15:0][10:0] itxTriggerLedDuration;
wire [15:0][20:0] itxTriggerLedDelay;
assign itxTriggerLedDuration[0] 	= itxTrigLedDurationAndDelay[0][10:0];
assign itxTriggerLedDelay[0] 		= itxTrigLedDurationAndDelay[0][31:11];
assign itxTriggerLedDuration[1] 	= itxTrigLedDurationAndDelay[1][10:0];
assign itxTriggerLedDelay[1] 		= itxTrigLedDurationAndDelay[1][31:11];
assign itxTriggerLedDuration[2] 	= itxTrigLedDurationAndDelay[2][10:0];
assign itxTriggerLedDelay[2] 		= itxTrigLedDurationAndDelay[2][31:11];
assign itxTriggerLedDuration[3] 	= itxTrigLedDurationAndDelay[3][10:0];
assign itxTriggerLedDelay[3] 		= itxTrigLedDurationAndDelay[3][31:11];
assign itxTriggerLedDuration[4] 	= itxTrigLedDurationAndDelay[4][10:0];
assign itxTriggerLedDelay[4] 		= itxTrigLedDurationAndDelay[4][31:11];
assign itxTriggerLedDuration[5] 	= itxTrigLedDurationAndDelay[5][10:0];
assign itxTriggerLedDelay[5] 		= itxTrigLedDurationAndDelay[5][31:11];
assign itxTriggerLedDuration[6] 	= itxTrigLedDurationAndDelay[6][10:0];
assign itxTriggerLedDelay[6] 		= itxTrigLedDurationAndDelay[6][31:11];
assign itxTriggerLedDuration[7] 	= itxTrigLedDurationAndDelay[7][10:0];
assign itxTriggerLedDelay[7] 		= itxTrigLedDurationAndDelay[7][31:11];
assign itxTriggerLedDuration[8] 	= itxTrigLedDurationAndDelay[8][10:0];
assign itxTriggerLedDelay[8] 		= itxTrigLedDurationAndDelay[8][31:11];
assign itxTriggerLedDuration[9] 	= itxTrigLedDurationAndDelay[9][10:0];
assign itxTriggerLedDelay[9] 		= itxTrigLedDurationAndDelay[9][31:11];
assign itxTriggerLedDuration[10] 	= itxTrigLedDurationAndDelay[10][10:0];
assign itxTriggerLedDelay[10] 		= itxTrigLedDurationAndDelay[10][31:11];
assign itxTriggerLedDuration[11] 	= itxTrigLedDurationAndDelay[11][10:0];
assign itxTriggerLedDelay[11] 		= itxTrigLedDurationAndDelay[11][31:11];
assign itxTriggerLedDuration[12] 	= itxTrigLedDurationAndDelay[12][10:0];
assign itxTriggerLedDelay[12] 		= itxTrigLedDurationAndDelay[12][31:11];
assign itxTriggerLedDuration[13] 	= itxTrigLedDurationAndDelay[13][10:0];
assign itxTriggerLedDelay[13] 		= itxTrigLedDurationAndDelay[13][31:11];
assign itxTriggerLedDuration[14] 	= itxTrigLedDurationAndDelay[14][10:0];
assign itxTriggerLedDelay[14] 		= itxTrigLedDurationAndDelay[14][31:11];
assign itxTriggerLedDuration[15] 	= itxTrigLedDurationAndDelay[15][10:0];
assign itxTriggerLedDelay[15] 		= itxTrigLedDurationAndDelay[15][31:11];


// needs to know if its master so it doesn't issue 'otxWaitForMe'
wire isSolo;
wire isMaster;
assign isSolo = itxBoardIdentifiers[0];
assign isMaster = itxBoardIdentifiers[1];

// hopefully this works
reg otxADCTriggerLine_reg;
assign otxADCTriggerLine = otxADCTriggerLine_reg;

reg	internalTrigger;
wor triggerSignal;
assign triggerSignal = itxExternalTrigger;
assign triggerSignal = internalTrigger;

// PIO commands only get issued when the PIO state changes, need to store previous state
reg [15:0]			pio_cmd_previous;

reg [63:0]			time_until_next_interrupt;
reg	[22:0]			fire_delay_timer;
reg	[31:0]			recv_trig_delay_timer;

reg [8:0]			transducer_chargetime;
reg [7:0][15:0]		transducer_phasedelay;
reg [7:0]			transducer_is_active;
reg [1:0]			adcTrigFlag;
reg					fireFlag;
reg					fireReset;
wire [7:0]			fireDanger;
wire [7:0]			fireComplete;

reg					trigLedFlag;
reg					trigLedReset;
wire [15:0]			trigLedComplete;

reg					fireDelayTimerFlag;
reg					recvTrigTimerFlag;
reg					interruptRequestTimerFlag;
reg					txResetFlag;
reg					trigLedStop;


initial
begin
	transducer_is_active = 8'b0;
	transducer_chargetime = 9'b0;
	transducer_phasedelay[0] = 16'b0;
	transducer_phasedelay[1] = 16'b0;
	transducer_phasedelay[2] = 16'b0;
	transducer_phasedelay[3] = 16'b0;
	transducer_phasedelay[4] = 16'b0;
	transducer_phasedelay[5] = 16'b0;
	transducer_phasedelay[6] = 16'b0;
	transducer_phasedelay[7] = 16'b0;
	
	fireFlag = 1'b0;
	fireDelayTimerFlag = 1'b0;
	trigLedFlag = 1'b0;
	recvTrigTimerFlag = 1'b0;
	txResetFlag = 1'b0;
	interruptRequestTimerFlag = 1'b0;
	adcTrigFlag = 2'b0;
	
	fire_delay_timer = 23'b0;
	recv_trig_delay_timer = 32'b0;
	time_until_next_interrupt = 64'b0;
	
	fireReset = 1'b1;
	trigLedReset = 1'b1;
	
	trigLedStop = 1'b0;
	
	internalTrigger = 1'b0;
	otxADCTriggerLine_reg = 1'b0;

	pio_cmd_previous = 16'b0;
	
	otxInterrupt = 32'b0;
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
				transducer_is_active <= 8'b0;
				transducer_chargetime <= 9'b0;
				transducer_phasedelay[0] <= 16'b0;
				transducer_phasedelay[1] <= 16'b0;
				transducer_phasedelay[2] <= 16'b0;
				transducer_phasedelay[3] <= 16'b0;
				transducer_phasedelay[4] <= 16'b0;
				transducer_phasedelay[5] <= 16'b0;
				transducer_phasedelay[6] <= 16'b0;
				transducer_phasedelay[7] <= 16'b0;
				
				fireFlag <= 1'b0;
				fireDelayTimerFlag <= 1'b0;
				trigLedFlag <= 1'b0;
				recvTrigTimerFlag <= 1'b0;
				txResetFlag <= 1'b0;
				interruptRequestTimerFlag <= 1'b0;
				adcTrigFlag <= 2'b0;
				
				fire_delay_timer <= 23'b0;
				recv_trig_delay_timer <= 32'b0;
				time_until_next_interrupt <= 64'b0;
				
				fireReset <= 1'b1;
				trigLedReset <= 1'b1;
				
				trigLedStop <= 1'b0;
				
				internalTrigger <= 1'b0;
				otxADCTriggerLine_reg <= 1'b0;

				pio_cmd_previous <= 16'b0;
				
				if ( otxInterrupt ) otxInterrupt <= 32'b0;
			end
		
		CASE_PIO_CONTROL:
			begin

				if ( !fireDanger )
				begin
				
					if ( trigLedStop ) trigLedStop <= 1'b0;
							
					if ( pio_output_commands ^ pio_cmd_previous ) 
					begin
						pio_cmd_previous <= pio_output_commands;
						otxInterrupt[31:16] <= pio_output_commands;
						otxInterrupt[1] <= 1'b1;
						/******************************************************/
						/*** GENERATE INTERRUPT *******************************/
						/******************************************************/
						if ( pio_output_commands & 16'b0000000000001111 ) 
						begin
							txResetFlag <= 1'b1;
							otxInterrupt[2] <= 1'b1;
						end
						
						/******************************************************/
						/*** INTERNAL TRIGGER FOR SOLO BOARDS *****************/
						/******************************************************/
						if ( pio_output_commands & 16'b0000000000000111 )
						begin
							if ( isSolo & !internalTrigger ) internalTrigger <= 1'b1;
							otxInterrupt[3] <= 1'b1;
						end
						
						/******************************************************/
						/*** SET FIRE/AMP/PHASE DELAY COMMANDS ****************/
						/******************************************************/
						if ( !fireFlag )
						begin
						
							/**************************************************/
							/*** FIRE FROM PIO COMMAND ************************/
							/**************************************************/
							if ( pio_output_commands & PIO_CMD_FIRE )
							begin
								fireDelayTimerFlag <= 1'b1;
								fire_delay_timer <= itxFireDelay;
								fireReset <= 1'b0;
								otxInterrupt[4] <= 1'b1;
							end
							
							/**************************************************/
							/*** SET CHARGETIME/AMP ***************************/
							/**************************************************/
							if ( pio_output_commands & PIO_CMD_SET_AMP )
							begin	
								transducer_chargetime <= itxPioChargetime_reg;
								otxInterrupt[5] <= 1'b1;
							end
							
							/**************************************************/
							/*** SET PHASE DELAYS *****************************/
							/**************************************************/
							if ( pio_output_commands & PIO_CMD_SET_PHASE )
							begin	
								transducer_phasedelay[0] <= itxPioPhaseDelay_ch0_ch1[15:0];
								transducer_phasedelay[1] <= itxPioPhaseDelay_ch0_ch1[31:16];
								transducer_phasedelay[2] <= itxPioPhaseDelay_ch2_ch3[15:0];
								transducer_phasedelay[3] <= itxPioPhaseDelay_ch2_ch3[31:16];
								transducer_phasedelay[4] <= itxPioPhaseDelay_ch4_ch5[15:0];
								transducer_phasedelay[5] <= itxPioPhaseDelay_ch4_ch5[31:16];
								transducer_phasedelay[6] <= itxPioPhaseDelay_ch6_ch7[15:0];
								transducer_phasedelay[7] <= itxPioPhaseDelay_ch6_ch7[31:16];
								otxInterrupt[6] <= 1'b1;
							end
						
						end

						/******************************************************/
						/*** SET TRIGS AND FROM PIO COMMAND *******************/
						/******************************************************/
						if( ( pio_output_commands & PIO_CMD_SET_TRIG_LEDS ) && !trigLedFlag )
						begin
							trigLedFlag <= 1'b1;
							trigLedReset <= 1'b0;
							otxInterrupt[7] <= 1'b1;
						end
						
						/******************************************************/
						/*** ISSUE RCV SYS TRIG FROM PIO COMMAND **************/
						/******************************************************/
						if( ( pio_output_commands & PIO_CMD_ISSUE_RCV_TRIG ) && !recvTrigTimerFlag && !adcTrigFlag )
						begin
							otxInterrupt[8] <= 1'b1;
							otxADCTriggerLine_reg <= 1'b0;
							recvTrigTimerFlag <= 1'b1;
							if ( itxRecvTrigDelay )
							begin
								recv_trig_delay_timer <= itxRecvTrigDelay;
								otxInterrupt[15] <= 1'b1;
							end
							else
							begin
								recv_trig_delay_timer <= 32'b0;
								otxInterrupt[14] <= 1'b1;
							end
						end
						
						
						/******************************************************/
						/*** INIT TIMER TO REQUEST NEXT COMMAND FROM ARM ******/
						/******************************************************/	
						if( ( pio_output_commands & PIO_CMD_TIME_UNTIL_INTERRUPT ) && !interruptRequestTimerFlag )
						begin
							interruptRequestTimerFlag <= 1'b1;
							time_until_next_interrupt <= {itxTimeUntilNextInterrupt[1],itxTimeUntilNextInterrupt[0]};
						end
						
						/******************************************************/
						/*** RESET RCV SYS TRIG FROM PIO COMMAND **************/
						/******************************************************/
						if( pio_output_commands & PIO_CMD_RESET_RCV_TRIG )
						begin
							otxADCTriggerLine_reg <= 1'b0;
							adcTrigFlag <= 2'b00;
							recvTrigTimerFlag <= 1'b0;
							recv_trig_delay_timer <= 32'b0;
						end
						
						/******************************************************/
						/*** RESET INTERRUPT FROM PIO COMMAND *****************/
						/******************************************************/
						if( pio_output_commands & PIO_CMD_RESET_INTERRUPT )
						begin
							internalTrigger <= 1'b0;
							if ( txResetFlag ) txResetFlag <= 1'b0;
							if ( otxInterrupt ) otxInterrupt <= 32'b0;
							if ( interruptRequestTimerFlag ) interruptRequestTimerFlag <= 1'b0;
						end

					end	
					else
					begin
					
						/******************************************************/
						/*** ACKNOWLEDGE RCV SYS's RESPONSE TO TRIGGER ********/
						/******************************************************/
						if ( adcTrigFlag[0] & !adcTrigFlag[1] & itxADCTriggerAck )
						begin
							otxADCTriggerLine_reg <= 1'b0;
							adcTrigFlag[1] <= 1'b1;
						end
						
						/******************************************************/
						/*** FIRE DELAY COUNTDOWN TIMER ***********************/
						/******************************************************/
						if ( fireDelayTimerFlag )
						begin
							if( fire_delay_timer )
							begin
								fire_delay_timer <= fire_delay_timer - 1'b1;
							end
							else
							begin
								fireFlag <= 1'b1;
								fireDelayTimerFlag <= 1'b0;
								otxInterrupt[9] <= 1'b1;
							end
						end
						else if ( ( fireComplete == 8'b11111111 ) && !fireReset )
						begin
							if ( fireFlag ) fireFlag <= 1'b0;
							fireReset <= 1'b1;
							otxInterrupt[10] <= 1'b1;
						end
						
						/******************************************************/
						/*** NO NEW FIRE CMDS UNTIL PREVIOUS FIRE COMPLETE ****/
						/******************************************************/
						if ( ( trigLedComplete == 16'b1111111111111111 ) && !trigLedReset )
						begin
							if ( trigLedFlag ) trigLedFlag <= 1'b0;
							trigLedReset <= 1'b1;
							otxInterrupt[11] <= 1'b1;
						end
						
						/******************************************************/
						/*** RECV SYSTEM TRIG DELAY COUNTDOWN TIMER ***********/
						/******************************************************/
						if ( recvTrigTimerFlag )
						begin
							if( recv_trig_delay_timer )
							begin
								if ( otxADCTriggerLine_reg ) otxADCTriggerLine_reg <= 1'b0;
								recv_trig_delay_timer <= recv_trig_delay_timer - 1'b1;
								if( !otxInterrupt[13] ) otxInterrupt[13] <= 1'b1;
							end
							else
							begin
								otxADCTriggerLine_reg <= 1'b1;
								recvTrigTimerFlag <= 1'b0;
								adcTrigFlag[0] <= 1'b1;
								otxInterrupt[12] <= 1'b1;
							end
						end
						
						/******************************************************/
						/*** SIGNAL ARM THAT SYNCHRONOUS COMMANDS FINISHED ****/
						/******************************************************/
						if ( ( trigLedReset & fireReset & txResetFlag & !recvTrigTimerFlag ) && !time_until_next_interrupt && !adcTrigFlag )
						begin
							if( !otxInterrupt[0] ) otxInterrupt[0] <= 1'b1;
						end
						else if ( time_until_next_interrupt )
						begin
							time_until_next_interrupt <= time_until_next_interrupt - 1'b1;
						end	
						
					end	
					
					
					
					
				end
				else if ( fireDanger ) 
				begin

					transducer_is_active <= 8'b0;
					transducer_chargetime <= 9'b0;
					transducer_phasedelay[0] <= 16'b0;
					transducer_phasedelay[1] <= 16'b0;
					transducer_phasedelay[2] <= 16'b0;
					transducer_phasedelay[3] <= 16'b0;
					transducer_phasedelay[4] <= 16'b0;
					transducer_phasedelay[5] <= 16'b0;
					transducer_phasedelay[6] <= 16'b0;
					transducer_phasedelay[7] <= 16'b0;
					
					fireFlag <= 1'b0;
					fireDelayTimerFlag <= 1'b0;
					trigLedFlag <= 1'b0;
					recvTrigTimerFlag <= 1'b0;
					txResetFlag <= 1'b0;
					interruptRequestTimerFlag <= 1'b0;
					adcTrigFlag <= 2'b0;
					
					fire_delay_timer <= 23'b0;
					recv_trig_delay_timer <= 32'b0;
					time_until_next_interrupt <= 64'b0;
					
					fireReset <= 1'b1;
					trigLedReset <= 1'b1;
					
					trigLedStop <= 1'b1;
					
					internalTrigger <= 1'b0;
					otxADCTriggerLine_reg <= 1'b0;

					pio_cmd_previous <= pio_output_commands;
					
					if ( !otxInterrupt[0] ) otxInterrupt <= {15'b0,1'b1,15'b0,1'b1};

				end
				
			end
		
		default:
			begin
				transducer_is_active <= 8'b0;
				transducer_chargetime <= 9'b0;
				transducer_phasedelay[0] <= 16'b0;
				transducer_phasedelay[1] <= 16'b0;
				transducer_phasedelay[2] <= 16'b0;
				transducer_phasedelay[3] <= 16'b0;
				transducer_phasedelay[4] <= 16'b0;
				transducer_phasedelay[5] <= 16'b0;
				transducer_phasedelay[6] <= 16'b0;
				transducer_phasedelay[7] <= 16'b0;
				
				fireFlag <= 1'b0;
				fireDelayTimerFlag <= 1'b0;
				trigLedFlag <= 1'b0;
				recvTrigTimerFlag <= 1'b0;
				txResetFlag <= 1'b0;
				interruptRequestTimerFlag <= 1'b0;
				adcTrigFlag <= 2'b0;
				
				fire_delay_timer <= 23'b0;
				recv_trig_delay_timer <= 32'b0;
				time_until_next_interrupt <= 64'b0;
				
				fireReset <= 1'b1;
				trigLedReset <= 1'b1;
				
				trigLedStop <= 1'b0;
				
				internalTrigger <= 1'b0;
				otxADCTriggerLine_reg <= 1'b0;

				pio_cmd_previous <= 16'b0;
				
				if ( otxInterrupt ) otxInterrupt <= 32'b0;
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
	.phaseDelay(transducer_phasedelay[0]),
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
	.phaseDelay(transducer_phasedelay[1]),
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
	.phaseDelay(transducer_phasedelay[2]),
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
	.phaseDelay(transducer_phasedelay[3]),
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
	.phaseDelay(transducer_phasedelay[4]),
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
	.phaseDelay(transducer_phasedelay[5]),
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
	.phaseDelay(transducer_phasedelay[6]),
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
	.phaseDelay(transducer_phasedelay[7]),
	.transducerOutput(otxTransducerOutput[7]),
	.fireComplete(fireComplete[7]),
	.warning(fireDanger[7])
);


triggerLedOutput_Module tl0(
	.clk(txCLK),
	.rst(trigLedReset),
	
	.restLevel(itxPioTriggerLedRestLevels[0]),
	
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
	
	.onYourMark(trigLedFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	
	.duration(itxTriggerLedDuration[15]),
	.delay(itxTriggerLedDelay[15]),
	
	.triggerLedOutput(otxTriggerLedOutput[15]),
	.trigLedComplete(trigLedComplete[15]),
	
	.hardStop(trigLedStop)
);


endmodule


