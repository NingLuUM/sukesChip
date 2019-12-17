
module Output_Control_Module_PIO (

	input 					txCLK,
	
	/*	all PIO registers will be 32bit wide and packed with multiple
		commands unless otherwise specified */
	
	// PIO register for controlling module outputs
	input	[31:0]			itxControlComms, 				// pio_tx_reg0
	
	// static settings for outputs	
	input	[7:0]			itxBoardIdentifiers,			// pio_tx_reg1
	input	[7:0]			itxTransducerOutputIsActive,	// pio_tx_reg1
	input	[4:0]			itxIOLineOutputRestLevels,		// pio_tx_reg1
	input					itxVarAttenRestLevel,			// pio_tx_reg1
	
	input	[31:0]			itxPioCommands, 				// pio_tx_reg2
	
	// PIO output control registers
	input	[31:0]			itxPioPhaseDelay_ch0_ch1, 		// pio_tx_reg3
	input	[31:0]			itxPioPhaseDelay_ch2_ch3, 		// pio_tx_reg4
	input	[31:0]			itxPioPhaseDelay_ch4_ch5, 		// pio_tx_reg5
	input	[31:0]			itxPioPhaseDelay_ch6_ch7, 		// pio_tx_reg6
	
	input	[8:0]			itxPioChargetime_reg,			// pio_tx_reg7
	input	[22:0]			itxFireDelay,					// pio_tx_reg7
	
	input	[31:0]			itxRecvTrigDelay,				// pio_tx_reg8
	
	input	[4:0][31:0]		itxIOLineOutputDurationAndDelay,		// pio_tx_reg9-24
	input	[31:0]			itxVarAttenOutputDurationAndDelay,		// pio_tx_reg9-24
	
	input	[1:0][31:0]		itxTimeUntilNextInterrupt,		// pio_tx_reg25-26
	
	// input triggers and 'fast' comm lines
	input					itxExternalTrigger,
	
	output [7:0]			otxTransducerOutput,
	
	output [4:0]			otxIOLineOutput,
	input  [4:0]			itxIOLineInput,
	
	output					oVAR_ATTEN,
	
	input					itxADCTriggerAck,
	output					otxADCTriggerLine,
	
	output reg	[31:0]		otxInterrupt
);



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

wire [4:0][10:0] itxIOLineOutputDuration;
wire [4:0][20:0] itxIOLineOutputDelay;
assign itxIOLineOutputDuration[0] 	= itxIOLineOutputDurationAndDelay[0][10:0];
assign itxIOLineOutputDelay[0] 		= itxIOLineOutputDurationAndDelay[0][31:11];
assign itxIOLineOutputDuration[1] 	= itxIOLineOutputDurationAndDelay[1][10:0];
assign itxIOLineOutputDelay[1] 		= itxIOLineOutputDurationAndDelay[1][31:11];
assign itxIOLineOutputDuration[2] 	= itxIOLineOutputDurationAndDelay[2][10:0];
assign itxIOLineOutputDelay[2] 		= itxIOLineOutputDurationAndDelay[2][31:11];
assign itxIOLineOutputDuration[3] 	= itxIOLineOutputDurationAndDelay[3][10:0];
assign itxIOLineOutputDelay[3] 		= itxIOLineOutputDurationAndDelay[3][31:11];
assign itxIOLineOutputDuration[4] 	= itxIOLineOutputDurationAndDelay[4][10:0];
assign itxIOLineOutputDelay[4] 		= itxIOLineOutputDurationAndDelay[4][31:11];

wire [15:0] varAttenDuration;
wire [15:0] varAttenDelay;
assign varAttenRestLvl = itxVarAttenOutputDurationAndDelay[15:0];
assign varAttenDuration = itxVarAttenOutputDurationAndDelay[31:16];


// needs to know if its master so it doesn't issue 'otxWaitForMe'
reg isSolo;
reg isMaster;
reg isExternallyTriggered;

// hopefully this works
reg otxADCTriggerLine_reg;
assign otxADCTriggerLine = otxADCTriggerLine_reg;

reg	internalTrigger;
wor triggerSignal;
assign triggerSignal = ( itxExternalTrigger & isExternallyTriggered );
assign triggerSignal = ( internalTrigger & !isExternallyTriggered );

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

reg					ioLineOutputFlag;
reg					ioLineOutputReset;
wire [4:0]			ioLineOutputComplete;

reg					varAttenOutputFlag;
reg					varAttenOutputReset;
wire				varAttenOutputComplete;

reg	fireDelayTimerFlag;
reg	recvTrigTimerFlag;
reg	interruptRequestTimerFlag;
reg	txResetFlag;
reg	ioLineOutputStop;

/**********************************************************************/
/*** CASE VARIABLE DEFINITIONS ****************************************/
/**********************************************************************/
// CONTROL STATES
parameter	[1:0]	CASE_IDLE 			= 2'b00;
parameter	[1:0]	CASE_PIO_CONTROL	= 2'b01;

// COMMANDS THAT CAN BE ISSUED THROUGH PIOs
parameter	[15:0]	PIO_CMD_IDLE					= 16'b0000000000000000;
parameter	[15:0]	PIO_CMD_SET_TRIG_LEDS			= 16'b0000000000000001;
parameter	[15:0]	PIO_CMD_ISSUE_RCV_TRIG			= 16'b0000000000000010;
parameter	[15:0]	PIO_CMD_FIRE 					= 16'b0000000000000100;
parameter	[15:0]	PIO_CMD_SET_VAR_ATTEN			= 16'b0000000000001000;
parameter	[15:0]	PIO_CMD_TIME_UNTIL_INTERRUPT	= 16'b0000000000010000;
parameter	[15:0]	PIO_CMD_SET_AMP		 			= 16'b0000000000100000;
parameter	[15:0]	PIO_CMD_SET_PHASE	 			= 16'b0000000001000000;
parameter	[15:0]	PIO_CMD_RESET_RCV_TRIG			= 16'b0010000000000000;
parameter	[15:0]	PIO_CMD_RESET_INTERRUPT			= 16'b0100000000000000;
parameter	[15:0]	PIO_CMD_NEW_COMMAND_FLAG		= 16'b1000000000000000;

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
	ioLineOutputFlag = 1'b0;
	varAttenOutputFlag = 1'b0;
	recvTrigTimerFlag = 1'b0;
	txResetFlag = 1'b0;
	interruptRequestTimerFlag = 1'b0;
	adcTrigFlag = 2'b0;
	
	fire_delay_timer = 23'b0;
	recv_trig_delay_timer = 32'b0;
	time_until_next_interrupt = 64'b0;
	
	fireReset = 1'b1;
	ioLineOutputReset = 1'b1;
	varAttenOutputReset = 1'b1;
	
	ioLineOutputStop = 1'b0;
	
	internalTrigger = 1'b0;
	otxADCTriggerLine_reg = 1'b0;

	pio_cmd_previous = 16'b0;
	
	otxInterrupt = 32'b0;
end


always @(posedge txCLK)
begin
	
	if( isSolo ^ itxBoardIdentifiers[0] ) isSolo <= itxBoardIdentifiers[0];
	if( isMaster ^ itxBoardIdentifiers[1]) isMaster <= itxBoardIdentifiers[1];
	if( isExternallyTriggered ^ itxBoardIdentifiers[2] ) isExternallyTriggered <= itxBoardIdentifiers[2];
	
	case ( control_state )
		CASE_IDLE:
			begin
				if( transducer_is_active ^ itxTransducerOutputIsActive )
				begin
					transducer_is_active <= itxTransducerOutputIsActive;
				end
				
				fireFlag <= 1'b0;
				fireDelayTimerFlag <= 1'b0;
				ioLineOutputFlag <= 1'b0;
				varAttenOutputFlag <= 1'b0;
				recvTrigTimerFlag <= 1'b0;
				txResetFlag <= 1'b0;
				interruptRequestTimerFlag <= 1'b0;
				adcTrigFlag <= 2'b0;
				
				fire_delay_timer <= 23'b0;
				recv_trig_delay_timer <= 32'b0;
				time_until_next_interrupt <= 64'b0;
				
				fireReset <= 1'b1;
				ioLineOutputReset <= 1'b1;
				varAttenOutputReset <= 1'b1;
				
				ioLineOutputStop <= 1'b0;
				
				internalTrigger <= 1'b0;
				otxADCTriggerLine_reg <= 1'b0;

				pio_cmd_previous <= 16'b0;
				
				if ( otxInterrupt ) otxInterrupt <= 32'b0;
			end
		
		CASE_PIO_CONTROL:
			begin

				if ( !fireDanger )
				begin
				
					if ( ioLineOutputStop ) ioLineOutputStop <= 1'b0;
							
					if ( pio_output_commands ^ pio_cmd_previous ) 
					begin
						pio_cmd_previous <= pio_output_commands;
						//otxInterrupt[31:16] <= pio_output_commands;
						otxInterrupt[1] <= 1'b1;
						/******************************************************/
						/*** GENERATE INTERRUPT *******************************/
						/******************************************************/
						if ( pio_output_commands & 16'b0000000000011111 ) 
						begin
							txResetFlag <= 1'b1;
							otxInterrupt[2] <= 1'b1;
						end
						
						/******************************************************/
						/*** INTERNAL TRIGGER FOR SOLO BOARDS *****************/
						/******************************************************/
						if ( pio_output_commands & 16'b0000000000001111 )
						begin
							if ( isSolo & !isExternallyTriggered )
							begin
								internalTrigger <= 1'b1;
							end
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
						if( ( pio_output_commands & PIO_CMD_SET_TRIG_LEDS ) && !ioLineOutputFlag )
						begin
							ioLineOutputFlag <= 1'b1;
							ioLineOutputReset <= 1'b0;
							otxInterrupt[7] <= 1'b1;
						end
						
						/******************************************************/
						/*** SET TRIGS AND FROM PIO COMMAND *******************/
						/******************************************************/
						if( ( pio_output_commands & PIO_CMD_SET_VAR_ATTEN ) && !varAttenOutputFlag )
						begin
							varAttenOutputFlag <= 1'b1;
							varAttenOutputReset <= 1'b0;
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
							txResetFlag <= 1'b0;
							otxInterrupt <= 32'b0;
							interruptRequestTimerFlag <= 1'b0;
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
							if( otxInterrupt[23:16]^fireComplete ) otxInterrupt[23:16] <= fireComplete;
							if( otxInterrupt[24]^fireReset) otxInterrupt[24] <= fireReset;
						end
						else if ( ( fireComplete == 8'b11111111 ) && !fireReset )
						begin
							fireFlag <= 1'b0;
							fireReset <= 1'b1;
							otxInterrupt[10] <= 1'b1;
						end
						else
						begin
							if( otxInterrupt[23:16] ^ fireComplete ) otxInterrupt[23:16] <= fireComplete;
							if( otxInterrupt[24]^fireReset) otxInterrupt[24] <= fireReset;
						end
						
						/******************************************************/
						/*** NO NEW FIRE CMDS UNTIL PREVIOUS FIRE COMPLETE ****/
						/******************************************************/
						if ( ( ioLineOutputComplete == 5'b11111 ) && !ioLineOutputReset )
						begin
							ioLineOutputFlag <= 1'b0;
							ioLineOutputReset <= 1'b1;
							otxInterrupt[11] <= 1'b1;
						end
						else
						begin
							if (otxInterrupt[29:25]^ioLineOutputComplete) otxInterrupt[29:25] <= ioLineOutputComplete;
							if( otxInterrupt[30]^ioLineOutputReset) otxInterrupt[30] <= ioLineOutputReset;
						end
						
						/******************************************************/
						/*** NO NEW FIRE CMDS UNTIL PREVIOUS FIRE COMPLETE ****/
						/******************************************************/
						if ( ( varAttenOutputComplete == 1'b1 ) && !varAttenOutputReset )
						begin
							varAttenOutputFlag <= 1'b0;
							varAttenOutputReset <= 1'b1;
						end
						else
						begin
							if( otxInterrupt[31] ^ varAttenOutputReset ) otxInterrupt[31] <= varAttenOutputReset;
						end
						
						/******************************************************/
						/*** RECV SYSTEM TRIG DELAY COUNTDOWN TIMER ***********/
						/******************************************************/
						if ( recvTrigTimerFlag )
						begin
							if( recv_trig_delay_timer )
							begin
								recv_trig_delay_timer <= recv_trig_delay_timer - 1'b1;
								if ( otxADCTriggerLine_reg ) otxADCTriggerLine_reg <= 1'b0;
								if ( !otxInterrupt[13] ) otxInterrupt[13] <= 1'b1;
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
						if ( ( ioLineOutputReset & varAttenOutputReset & fireReset & txResetFlag & !recvTrigTimerFlag ) && !time_until_next_interrupt && !adcTrigFlag )
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
					ioLineOutputFlag <= 1'b0;
					varAttenOutputFlag <= 1'b0;
					recvTrigTimerFlag <= 1'b0;
					txResetFlag <= 1'b0;
					interruptRequestTimerFlag <= 1'b0;
					adcTrigFlag <= 2'b0;
					
					fire_delay_timer <= 23'b0;
					recv_trig_delay_timer <= 32'b0;
					time_until_next_interrupt <= 64'b0;
					
					fireReset <= 1'b1;
					ioLineOutputReset <= 1'b1;
					varAttenOutputReset <= 1'b1;
					
					ioLineOutputStop <= 1'b1;
					
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
				ioLineOutputFlag <= 1'b0;
				varAttenOutputFlag <= 1'b0;
				recvTrigTimerFlag <= 1'b0;
				txResetFlag <= 1'b0;
				interruptRequestTimerFlag <= 1'b0;
				adcTrigFlag <= 2'b0;
				
				fire_delay_timer <= 23'b0;
				recv_trig_delay_timer <= 32'b0;
				time_until_next_interrupt <= 64'b0;
				
				fireReset <= 1'b1;
				ioLineOutputReset <= 1'b1;
				varAttenOutputReset <= 1'b1;
				
				ioLineOutputStop <= 1'b0;
				
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


ioOutputLine_ControlModule tl0(
	.clk(txCLK),
	.rst(ioLineOutputReset),
	
	.restLevel(itxIOLineOutputRestLevels[0]),
	
	.onYourMark(ioLineOutputFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	
	.duration(itxIOLineOutputDuration[0]),
	.delay(itxIOLineOutputDelay[0]),
	
	.outputState(otxIOLineOutput[0]),
	.outputComplete(ioLineOutputComplete[0]),
	
	.hardStop(ioLineOutputStop)
);

ioOutputLine_ControlModule tl1(
	.clk(txCLK),
	.rst(ioLineOutputReset),
	
	.restLevel(itxIOLineOutputRestLevels[1]),
	
	.onYourMark(ioLineOutputFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	
	.duration(itxIOLineOutputDuration[1]),
	.delay(itxIOLineOutputDelay[1]),
	
	.outputState(otxIOLineOutput[1]),
	.outputComplete(ioLineOutputComplete[1]),
	
	.hardStop(ioLineOutputStop)
);

ioOutputLine_ControlModule tl2(
	.clk(txCLK),
	.rst(ioLineOutputReset),
	
	.restLevel(itxIOLineOutputRestLevels[2]),
	
	.onYourMark(ioLineOutputFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	
	.duration(itxIOLineOutputDuration[2]),
	.delay(itxIOLineOutputDelay[2]),
	
	.outputState(otxIOLineOutput[2]),
	.outputComplete(ioLineOutputComplete[2]),
	
	.hardStop(ioLineOutputStop)
);

ioOutputLine_ControlModule tl3(
	.clk(txCLK),
	.rst(ioLineOutputReset),
	
	.restLevel(itxIOLineOutputRestLevels[3]),
	
	.onYourMark(ioLineOutputFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	
	.duration(itxIOLineOutputDuration[3]),
	.delay(itxIOLineOutputDelay[3]),
	
	.outputState(otxIOLineOutput[3]),
	.outputComplete(ioLineOutputComplete[3]),
	
	.hardStop(ioLineOutputStop)
);

ioOutputLine_ControlModule tl4(
	.clk(txCLK),
	.rst(ioLineOutputReset),
	
	.restLevel(itxIOLineOutputRestLevels[4]),
	
	.onYourMark(ioLineOutputFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	
	.duration(itxIOLineOutputDuration[4]),
	.delay(itxIOLineOutputDelay[4]),
	
	.outputState(otxIOLineOutput[4]),
	.outputComplete(ioLineOutputComplete[4]),
	
	.hardStop(ioLineOutputStop)
);

ioVarAtten_ControlModule va0(
	.clk(txCLK),
	.rst(varAttenOutputReset),
	
	.restLevel(itxVarAttenRestLevel),
	
	.onYourMark(varAttenOutputFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	
	.duration(varAttenDuration),
	.delay(varAttenDelay),
	
	.outputState(oVAR_ATTEN),
	.outputComplete(varAttenOutputComplete),
	
	.hardStop(ioLineOutputStop)
);


endmodule


