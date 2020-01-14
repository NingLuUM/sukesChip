
module Output_Control_Module_PIO (

	input 					txCLK,
	
	/*	all PIO registers will be 32bit wide and packed with multiple
		commands unless otherwise specified */
	
	// PIO register for controlling module outputs
	input	[31:0]			itxControlComms, 				// pio_tx_reg0
	
	// static settings for outputs	
	input	[31:0]			itxStaticSettings,				// pio_tx_reg1
	
	input	[31:0]			itxPioCommands, 				// pio_tx_reg2
	
	// PIO output control registers
	input	[31:0]			itxPioPhaseDelay_ch0_ch1, 		// pio_tx_reg3
	input	[31:0]			itxPioPhaseDelay_ch2_ch3, 		// pio_tx_reg4
	input	[31:0]			itxPioPhaseDelay_ch4_ch5, 		// pio_tx_reg5
	input	[31:0]			itxPioPhaseDelay_ch6_ch7, 		// pio_tx_reg6
	
	input	[8:0]			itxPioChargetime_reg,			// pio_tx_reg7[8:0]
	input	[7:0]			itxPioTmpFireMask,				// pio_tx_reg7[16:9]
	
	input	[31:0]			itxFireDelay,					// pio_tx_reg8
	
	input	[31:0]			itxRecvTrigDelay,				// pio_tx_reg9
	
	input	[31:0]			itxVarAttenOutputDuration,		// pio_tx_reg10
	input	[31:0]			itxVarAttenOutputDelay,			// pio_tx_reg11
	
	input	[4:0][31:0]		itxIOLineOutputDuration,		// pio_tx_reg12-16
	input	[4:0][31:0]		itxIOLineOutputDelay,			// pio_tx_reg17-21
	
	
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


wire [7:0]			itxBoardIdentifiers;			// pio_tx_reg1
wire [7:0]			itxTransducerOutputIsActive;	// pio_tx_reg1
wire [4:0]			itxIOLineOutputRestLevels;		// pio_tx_reg1
wire				itxVarAttenRestLevel;			// pio_tx_reg1
assign itxBoardIdentifiers			= itxStaticSettings[7:0];	// tx_pio_reg1
assign itxTransducerOutputIsActive	= itxStaticSettings[15:8];	// tx_pio_reg1
assign itxIOLineOutputRestLevels	= itxStaticSettings[20:16];
assign itxVarAttenRestLevel			= itxStaticSettings[21];


wire [15:0]	pio_output_commands;
assign pio_output_commands 				= itxPioCommands[15:0];

/**********************************************************************/
/*** PIO REGISTER ASSIGNMENTS *****************************************/
/**********************************************************************/

// needs to know if its master so it doesn't issue 'otxWaitForMe'
reg isSolo;
reg isMaster;
reg isChild;
reg isExternallyTriggered;

// hopefully this works
//~ reg otxADCTriggerLine_reg;
//~ assign otxADCTriggerLine = otxADCTriggerLine_reg;

reg	internalTrigger;
wor triggerSignal;
assign triggerSignal = ( itxExternalTrigger & isExternallyTriggered );
assign triggerSignal = ( internalTrigger & !isExternallyTriggered );

// PIO commands only get issued when the PIO state changes, need to store previous state
reg [15:0]			pio_cmd_previous;

reg [63:0]			time_until_next_interrupt;
reg	[31:0]			fire_delay_timer;
reg	[31:0]			recv_trig_delay_timer;

reg [8:0]			transducer_chargetime;
reg [7:0][15:0]		transducer_phasedelay;
reg [7:0]			transducer_is_active;
reg [7:0]			transducer_tmp_mask;

reg					fireFlag;
reg	[1:0]			fireReset;
wire [7:0]			fireComplete;

reg					ioLineOutputFlag;
reg	[1:0]			ioLineOutputReset;
wire [3:0]			ioLineOutputComplete;

reg					varAttenOutputFlag;
reg	[1:0]			varAttenOutputReset;
wire				varAttenOutputComplete;

reg					recvTrigFlag;
reg	[1:0]			recvTrigReset;
wire				recvTrigComplete;

reg 				startInstructionTimerFlag;
reg					otxSync_reg;

reg	fireDelayTimerFlag;
reg	interruptRequestTimerFlag;
reg	txResetFlag;
reg	ioLineOutputStop;

wire THEYRE_READY;
assign THEYRE_READY = itxIOLineInput[4];

reg IM_READY;
assign otxIOLineOutput[4] = IM_READY;

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

parameter	[15:0]	PIO_CMD_ALLOW_OPEN_ENDED_RECV	= 16'b0001000000000000;
parameter	[15:0]	PIO_CMD_RESET_RCV_TRIG			= 16'b0010000000000000;
parameter	[15:0]	PIO_CMD_RESET_INTERRUPT			= 16'b0100000000000000;
parameter	[15:0]	PIO_CMD_NEW_COMMAND_FLAG		= 16'b1000000000000000;

initial
begin
	transducer_is_active = 8'b0;
	transducer_chargetime = 9'b0;
	transducer_tmp_mask = 8'b11111111;
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
	recvTrigFlag = 1'b0;
	txResetFlag = 1'b0;
	interruptRequestTimerFlag = 1'b0;
	
	fire_delay_timer = 32'b0;
	recv_trig_delay_timer = 32'b0;
	time_until_next_interrupt = 64'b0;
	
	fireReset = 2'b11;
	ioLineOutputReset = 2'b11;
	varAttenOutputReset = 2'b11;
	recvTrigReset = 2'b11;
	
	ioLineOutputStop = 1'b0;
	
	internalTrigger = 1'b0;

	pio_cmd_previous = 16'b0;
	
	IM_READY = 1'b0;
	otxSync_reg = 1'b0;
	
	otxInterrupt = 32'b0;
	isSolo = 1'b0;
	isMaster = 1'b0;
	isChild = 1'b1;
	
	startInstructionTimerFlag = 1'b0;
end


always @(posedge txCLK)
begin
	
	case ( control_state )
		CASE_IDLE:
			begin
				if( transducer_is_active ^ itxTransducerOutputIsActive )
				begin
					transducer_is_active <= itxTransducerOutputIsActive;
				end
				
				if( isSolo ^ itxBoardIdentifiers[0] ) isSolo <= itxBoardIdentifiers[0];
				if( isMaster ^ itxBoardIdentifiers[1] ) isMaster <= itxBoardIdentifiers[1];
				
				if ( isSolo | isMaster )
				begin
					if( isChild ) isChild <= 1'b0;
					if( isExternallyTriggered ^ itxBoardIdentifiers[2] ) isExternallyTriggered <= itxBoardIdentifiers[2];
				end
				else
				begin
					if( !isChild ) isChild <= 1'b1;
					if( !isExternallyTriggered ) isExternallyTriggered <= 1'b1;
				end
				
				fireFlag <= 1'b0;
				fireDelayTimerFlag <= 1'b0;
				ioLineOutputFlag <= 1'b0;
				varAttenOutputFlag <= 1'b0;
				recvTrigFlag <= 1'b0;
				txResetFlag <= 1'b0;
				interruptRequestTimerFlag <= 1'b0;

				transducer_tmp_mask <= 8'b11111111;
				
				fire_delay_timer <= 32'b0;
				recv_trig_delay_timer <= 32'b0;
				time_until_next_interrupt <= 64'b0;
				
				fireReset <= 2'b11;
				ioLineOutputReset <= 2'b11;
				varAttenOutputReset <= 2'b11;
				recvTrigReset <= 2'b11;
				
				startInstructionTimerFlag <= 1'b0;
				
				ioLineOutputStop <= 1'b0;
				
				internalTrigger <= 1'b0;
				
				IM_READY <= 1'b0;
				
				pio_cmd_previous <= 16'b0;
				
				if ( otxInterrupt ) otxInterrupt <= 32'b0;
			end
		
		CASE_PIO_CONTROL:
			begin
				
				if ( ioLineOutputStop ) ioLineOutputStop <= 1'b0;
						
				if ( pio_output_commands ^ pio_cmd_previous ) 
				begin
					pio_cmd_previous <= pio_output_commands;
					
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
							startInstructionTimerFlag <= 1'b1;
							internalTrigger <= 1'b1;
						end
						else
						begin
							startInstructionTimerFlag <= 1'b0;
						end
						IM_READY <= 1'b1;
						otxInterrupt[3] <= 1'b1;
					end
					else
					begin
						startInstructionTimerFlag <= 1'b1;
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
							fire_delay_timer <= itxFireDelay;
							fireFlag <= 1'b1;
							fireReset <= 2'b10;
							otxInterrupt[4] <= 1'b1;
						end
						
						/**************************************************/
						/*** SET CHARGETIME/AMP ***************************/
						/**************************************************/
						if ( pio_output_commands & PIO_CMD_SET_AMP )
						begin	
							transducer_chargetime <= itxPioChargetime_reg;
							transducer_tmp_mask <= ~itxPioTmpFireMask;
							//otxInterrupt[5] <= 1'b1;
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
							//otxInterrupt[6] <= 1'b1;
						end
					
					end

					/******************************************************/
					/*** SET TRIGS AND FROM PIO COMMAND *******************/
					/******************************************************/
					if( ( pio_output_commands & PIO_CMD_SET_TRIG_LEDS ) && !ioLineOutputFlag )
					begin
						ioLineOutputFlag <= 1'b1;
						ioLineOutputReset <= 2'b10;
						otxInterrupt[5] <= 1'b1;
					end
					
					
					/******************************************************/
					/*** SET TRIGS AND FROM PIO COMMAND *******************/
					/******************************************************/
					if( ( pio_output_commands & PIO_CMD_SET_VAR_ATTEN ) && !varAttenOutputFlag )
					begin
						varAttenOutputFlag <= 1'b1;
						varAttenOutputReset <= 2'b10;
						otxInterrupt[6] <= 1'b1;
					end
					
					
					/******************************************************/
					/*** ISSUE RCV SYS TRIG FROM PIO COMMAND **************/
					/******************************************************/
					if( ( pio_output_commands & PIO_CMD_ISSUE_RCV_TRIG ) && !recvTrigFlag )
					begin
						recvTrigFlag <= 1'b1;
						recvTrigReset <= 2'b10;
						recv_trig_delay_timer <= itxRecvTrigDelay;
						otxInterrupt[7] <= 1'b1;
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
						recvTrigFlag <= 1'b0;
						recvTrigReset <= 2'b11;
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
				
					if ( isMaster )
					begin
					
						if ( IM_READY & THEYRE_READY & !otxSync_reg )
						begin
						
							if ( !isExternallyTriggered )
							begin
								internalTrigger <= 1'b1;
								otxSync_reg <= 1'b1;
								startInstructionTimerFlag <= 1'b1;
							end
							else if ( triggerSignal )
							begin
								otxSync_reg <= 1'b1;
								startInstructionTimerFlag <= 1'b1;
							end
							
						end
						else if ( otxSync_reg )
						begin
						
							otxSync_reg <= 1'b0;
							IM_READY <= 1'b0;
							
						end
						
					end
					else
					begin
					
						if ( IM_READY & triggerSignal )
						begin
						
							IM_READY <= 1'b0;
							startInstructionTimerFlag <= 1'b1;
							
						end
						
					end
					
					
					/******************************************************/
					/*** FIRE DELAY COUNTDOWN TIMER ***********************/
					/******************************************************/
					if ( ( fireComplete == 8'b11111111 ) && !fireReset )
					begin
						fireFlag <= 1'b0;
						fireReset <= 2'b11;
						otxInterrupt[10] <= 1'b1;
						transducer_tmp_mask <= 8'b11111111;
						fire_delay_timer <= 32'b0;
						
					end
					else if ( fireReset == 2'b10 )
					begin
						fireReset <= 2'b00;
					end
					else
					begin
						if( otxInterrupt[23:16] ^ fireComplete ) otxInterrupt[23:16] <= fireComplete;
						if( otxInterrupt[24]^fireReset[0]) otxInterrupt[24] <= fireReset[0];
					end
					
					/******************************************************/
					/*** NO NEW FIRE CMDS UNTIL PREVIOUS FIRE COMPLETE ****/
					/******************************************************/
					if ( ( ioLineOutputComplete == 4'b1111 ) && !ioLineOutputReset )
					begin
						ioLineOutputFlag <= 1'b0;
						ioLineOutputReset <= 2'b11;
						otxInterrupt[11] <= 1'b1;
					end
					else if ( ioLineOutputReset <= 2'b10 )
					begin
						ioLineOutputReset <= 2'b0;
					end
					else
					begin
						if (otxInterrupt[28:25]^ioLineOutputComplete) otxInterrupt[28:25] <= ioLineOutputComplete;
						if( otxInterrupt[30]^ioLineOutputReset[0]) otxInterrupt[30] <= ioLineOutputReset[0];
					end
					
					/******************************************************/
					/*** NO NEW FIRE CMDS UNTIL PREVIOUS FIRE COMPLETE ****/
					/******************************************************/
					if ( varAttenOutputComplete && !varAttenOutputReset )
					begin
						varAttenOutputFlag <= 1'b0;
						varAttenOutputReset <= 2'b11;
					end
					else if ( varAttenOutputReset == 2'b10 ) 
					begin
						varAttenOutputReset <= 2'b0;
					end
					else
					begin
						if( otxInterrupt[31] ^ varAttenOutputReset[0] ) otxInterrupt[31] <= varAttenOutputReset[0];
					end
					
					
					if ( recvTrigComplete && !recvTrigReset )
					begin
						recvTrigFlag <= 1'b0;
						recvTrigReset <= 2'b11;
						otxInterrupt[8] <= 1'b1;
					end
					else if ( recvTrigReset == 2'b10 )
					begin
						recvTrigReset <= 2'b0;
					end
					else
					begin
						otxInterrupt[9] <= 1'b1;
					end
					
					
					
					/******************************************************/
					/*** SIGNAL ARM THAT SYNCHRONOUS COMMANDS FINISHED ****/
					/******************************************************/
					if ( ( ( ioLineOutputReset == 2'b11 ) && ( varAttenOutputReset == 2'b11 ) && ( fireReset == 2'b11 ) && txResetFlag && ( recvTrigReset == 2'b11 ) ) && !time_until_next_interrupt )
					begin
						if( !otxInterrupt[0] ) otxInterrupt[0] <= 1'b1;
					end
					else if ( time_until_next_interrupt && startInstructionTimerFlag )
					begin
						time_until_next_interrupt <= time_until_next_interrupt - 1'b1;
					end	
					
				end	
					
				
			end
		
		default:
			begin
				transducer_is_active <= 8'b0;
				transducer_chargetime <= 9'b0;
				transducer_tmp_mask <= 8'b11111111;
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
				recvTrigFlag <= 1'b0;
				txResetFlag <= 1'b0;
				interruptRequestTimerFlag <= 1'b0;
				recvTrigFlag <= 1'b0;
				
				fire_delay_timer <= 32'b0;
				recv_trig_delay_timer <= 32'b0;
				time_until_next_interrupt <= 64'b0;
				
				fireReset <= 2'b11;
				ioLineOutputReset <= 2'b11;
				varAttenOutputReset <= 2'b11;
				recvTrigReset <= 2'b11;
				
				startInstructionTimerFlag <= 1'b0;
				
				ioLineOutputStop <= 1'b0;
				
				internalTrigger <= 1'b0;

				pio_cmd_previous <= 16'b0;
				
				if ( otxInterrupt ) otxInterrupt <= 32'b0;
			end
	endcase
end


transducerOutput_Module c0(
	.clk(txCLK),
	.rst(fireReset[0]),
	.isActive( ( transducer_is_active[0] & transducer_tmp_mask[0] ) ),
	.onYourMark(fireFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	.chargeTime(transducer_chargetime),
	.phaseDelay(transducer_phasedelay[0]),
	.fireDelay(fire_delay_timer),
	.transducerOutput(otxTransducerOutput[0]),
	.fireComplete(fireComplete[0])
);

transducerOutput_Module c1(
	.clk(txCLK),
	.rst(fireReset[0]),
	.isActive( ( transducer_is_active[1] & transducer_tmp_mask[1] ) ),
	.onYourMark(fireFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	.chargeTime(transducer_chargetime),
	.phaseDelay(transducer_phasedelay[1]),
	.fireDelay(fire_delay_timer),
	.transducerOutput(otxTransducerOutput[1]),
	.fireComplete(fireComplete[1])
);

transducerOutput_Module c2(
	.clk(txCLK),
	.rst(fireReset[0]),
	.isActive( ( transducer_is_active[2] & transducer_tmp_mask[2] ) ),
	.onYourMark(fireFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	.chargeTime(transducer_chargetime),
	.phaseDelay(transducer_phasedelay[2]),
	.fireDelay(fire_delay_timer),
	.transducerOutput(otxTransducerOutput[2]),
	.fireComplete(fireComplete[2])
);

transducerOutput_Module c3(
	.clk(txCLK),
	.rst(fireReset[0]),
	.isActive( ( transducer_is_active[3] & transducer_tmp_mask[3] ) ),
	.onYourMark(fireFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	.chargeTime(transducer_chargetime),
	.phaseDelay(transducer_phasedelay[3]),
	.fireDelay(fire_delay_timer),
	.transducerOutput(otxTransducerOutput[3]),
	.fireComplete(fireComplete[3])
);

transducerOutput_Module c4(
	.clk(txCLK),
	.rst(fireReset[0]),
	.isActive( ( transducer_is_active[4] & transducer_tmp_mask[4] ) ),
	.onYourMark(fireFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	.chargeTime(transducer_chargetime),
	.phaseDelay(transducer_phasedelay[4]),
	.fireDelay(fire_delay_timer),
	.transducerOutput(otxTransducerOutput[4]),
	.fireComplete(fireComplete[4])
);

transducerOutput_Module c5(
	.clk(txCLK),
	.rst(fireReset[0]),
	.isActive( ( transducer_is_active[5] & transducer_tmp_mask[5] ) ),
	.onYourMark(fireFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	.chargeTime(transducer_chargetime),
	.phaseDelay(transducer_phasedelay[5]),
	.fireDelay(fire_delay_timer),
	.transducerOutput(otxTransducerOutput[5]),
	.fireComplete(fireComplete[5])
);

transducerOutput_Module c6(
	.clk(txCLK),
	.rst(fireReset[0]),
	.isActive( ( transducer_is_active[6] & transducer_tmp_mask[6] ) ),
	.onYourMark(fireFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	.chargeTime(transducer_chargetime),
	.phaseDelay(transducer_phasedelay[6]),
	.fireDelay(fire_delay_timer),
	.transducerOutput(otxTransducerOutput[6]),
	.fireComplete(fireComplete[6])
);

transducerOutput_Module c7(
	.clk(txCLK),
	.rst(fireReset[0]),
	.isActive( ( transducer_is_active[7] & transducer_tmp_mask[7] ) ),
	.onYourMark(fireFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	.chargeTime(transducer_chargetime),
	.phaseDelay(transducer_phasedelay[7]),
	.fireDelay(fire_delay_timer),
	.transducerOutput(otxTransducerOutput[7]),
	.fireComplete(fireComplete[7])
);


ioOutputLine_ControlModule tl0(
	.clk(txCLK),
	.rst(ioLineOutputReset[0]),
	
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
	.rst(ioLineOutputReset[0]),
	
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
	.rst(ioLineOutputReset[0]),
	
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
	.rst(ioLineOutputReset[0]),
	
	.restLevel(itxIOLineOutputRestLevels[3]),
	
	.onYourMark(ioLineOutputFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	
	.duration(itxIOLineOutputDuration[3]),
	.delay(itxIOLineOutputDelay[3]),
	
	.outputState(otxIOLineOutput[3]),
	.outputComplete(ioLineOutputComplete[3]),
	
	.hardStop(ioLineOutputStop)
);


ioVarAtten_ControlModule va0(
	.clk(txCLK),
	.rst(varAttenOutputReset[0]),
	
	.restLevel(itxVarAttenRestLevel),
	
	.onYourMark(varAttenOutputFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	
	.duration(itxVarAttenOutputDuration),
	.delay(itxVarAttenOutputDelay),
	
	.outputState(oVAR_ATTEN),
	.outputComplete(varAttenOutputComplete),
	
	.hardStop(ioLineOutputStop)
);


ioRecvSysTrigger_Module rs0(
	.clk(txCLK),
	.rst(recvTrigReset[0]),
	
	.onYourMark(recvTrigFlag),
	.GOGOGO_EXCLAMATION(triggerSignal),
	
	.delay(recv_trig_delay_timer),

	.outputTrig(otxADCTriggerLine),
	.adcAckLine(itxADCTriggerAck),
	
	.outputComplete(recvTrigComplete)
);


endmodule


