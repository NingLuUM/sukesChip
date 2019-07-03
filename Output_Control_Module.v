
module Output_Control_Module(

	input 					txCLK,
	
	input 					iSystemTrig,	
	input 					iExternalTrig,	
	input		[7:0]		itxControlComms,
	
	// procedural controls for instructions
	input		[31:0]		iTimeUntilNextInstruction,
	input		[15:0]		iNextInstructionType,
	input		[31:0]		iNextInstruction,
	input		[14:0]		iSetInstructionReadAddr,
	output reg	[14:0]		oInstructionReadAddr,
	
	// procedural control for loops
	input		[31:0]		itxLoopAddressReg,
	input		[31:0]		itxLoopCounterReg,
	output reg	[3:0]		otxLoopReadAddr,
	
	// transducer output controls
	input		[7:0][31:0]	itxPulsePhaseCharge,
	input		[7:0][31:0]	itxFireAtPhaseCharge,
	input		[7:0]		itxTransducerChannelMask,
	output reg	[7:0]		otxTransducerOutput,
	output reg	[7:0]		otxTransducerOutputError,
	
	
	// trigger output controls
	output reg	[7:0]		otxTriggerOutput,
	output reg	[7:0]		otxLedOutput,
	
	// recv system
	input					itxADCTriggerAck,
	output reg				otxADCTriggerLine,
	
	// arm interrupts
	output reg	[1:0][7:0]	oArmInterrupt,
	input		[1:0]		interruptResponse,
	output reg	[3:0]		interruptError,
	
	// allow trigs and leds to be set without running program
	input		[7:0]		ledReg,
	input		[7:0]		trigReg,
	input		[7:0]		trigRestLevelReg
);
	

// transducer output value from transducer output module
wire	[7:0]	transducerModule_outputVal;

// And's together itxTransducerChannelMask and transducerOutput_Module to set actual transducer outputs
wand	[7:0]	transducerOuput;
assign transducerOuput = transducerModule_outputVal;
assign transducerOuput = itxTransducerChannelMask;


// flag from transducer output module indicating that pulse is currently firing 
wire 	[7:0]	transducerModule_txIsActive;
wire	[7:0]	txErrorFlag;

reg		[7:0]	trigRestLevel;

reg [1:0][31:0] timeUntilNextInstruction;
reg [1:0][15:0] instructionType;
reg	[1:0][31:0] instruction;
reg [2:0] instructionBufferFlag;

reg [7:0][31:0] phaseCharge;	

wire [7:0] trigVals;
wire [7:0] ledVals;
wire [3:0] loopNumber;
wire [3:0] nextLoopNumber;
assign trigVals = instruction[0][7:0];
assign ledVals = instruction[0][15:8];
assign loopNumber = instruction[0][19:16];
assign nextLoopNumber = iNextInstruction[19:16];


parameter maxLoopAddr = 15;
reg [maxLoopAddr:0][15:0]	loopStartAddr;
reg [maxLoopAddr:0][15:0]	loopEndAddr;
reg [maxLoopAddr:0][15:0]	loopCounter;
reg [maxLoopAddr:0][15:0]	loopCounterRef;
reg [maxLoopAddr:0]			currentlyInLoop;
reg [3:0]					currentLoopAddr;
reg	inLocationLoop;
reg loopsInitialized;

reg fire_at_flag;

// itxControlComms Output_Control_Module case states
parameter [7:0]	hard_reset_tx_module = 	8'b00000000;
parameter [7:0]	soft_reset_tx_module = 	8'b00000001;
parameter [7:0] load_program = 			8'b00000010;
parameter [7:0] set_trig_output =		8'b00000100;
parameter [7:0] set_trig_rest_level =	8'b00000101;
parameter [7:0] test_trig_output =		8'b00000110;
parameter [7:0] set_led_output =		8'b00001000;
parameter [7:0] run_program = 			8'b10000000;

// control states for transducerOutput_Module
reg [1:0]		tx_output_cmd;
parameter [1:0] txout_wait_cmd = 2'b00;
parameter [1:0] txout_buffer_phase_charge = 2'b01;
parameter [1:0] txout_fire_pulse = 2'b10;
parameter [1:0] txout_reset_module = 2'b11;


// instruction type list
parameter [3:0] wait_cmd = 4'b0000; // 0
parameter [3:0] set_trig = 4'b0001; // 1
parameter [3:0] set_leds = 4'b0010; // 2
parameter [3:0] trigger_recv = 4'b0011; // 3
parameter [3:0] is_loop_end_point = 4'b0100; // 4
parameter [3:0] is_location_loop_start_point = 4'b0101; // 5
parameter [3:0] wait_for_external_trigger = 4'b0110; // 6
parameter [3:0] fire_pulse = 4'b0111; // 7
parameter [3:0] fire_at = 4'b1000; // 8
parameter [3:0] fire_single = 4'b1001; // 9
parameter [3:0] set_loc = 4'b1010; // 10
parameter [3:0] program_end_point = 4'b1111; // 15

wor isFireCommand;
assign isFireCommand = instructionType[0][fire_pulse];
assign isFireCommand = instructionType[0][fire_at];
assign isFireCommand = instructionType[0][fire_single];

// arm interrupt messages
reg [7:0] armMsg;
parameter [7:0]	no_arm_msg = 				8'b00000000;
parameter [7:0]	fire_cmd_issued =			8'b00000001;
parameter [7:0] fire_at_cmd_issued =		8'b00000010;
parameter [7:0] fire_single_cmd_issued =	8'b00000100;
parameter [7:0] set_fire_at_loc =			8'b00001000;
parameter [7:0]	tx_error = 					8'b00100000;
parameter [7:0] last_fire_cmd_incomplete =	8'b01000000;
parameter [7:0] multiple_fire_cmds_issued =	8'b10000000;

// needed to make sure interrupts aren't rejected if they're issued
// before previous response from arm processor gets reset
reg [2:0] armResponseFlag;

// this signals whether itxControlComms is set to a case where
// the transducer/trigger/led outputs aren't set off by default
// if there is an error detected while outside of one of the 'off'
// cases, this lets the fpga know to shut everything off
reg dangerFlag;

reg programRunningFlag;
reg txCntrActive;

initial
begin
	phaseCharge[0] = 31'b0;
	phaseCharge[1] = 31'b0;
	phaseCharge[2] = 31'b0;
	phaseCharge[3] = 31'b0;
	phaseCharge[4] = 31'b0;
	phaseCharge[5] = 31'b0;
	phaseCharge[6] = 31'b0;
	phaseCharge[7] = 31'b0;
	interruptError = 4'b0;
	trigRestLevel = 8'b0;
	armResponseFlag = 3'b0;
	inLocationLoop = 1'b0;
	loopsInitialized = 1'b0;
	tx_output_cmd = txout_reset_module;
	fire_at_flag = 1'b0;
	instructionBufferFlag = 3'b0;
	otxTransducerOutputError = 8'b0;
	dangerFlag = 1'b0;
	programRunningFlag = 1'b0;
	txCntrActive = 1'b0;
end



always @(posedge txCLK)
begin

	case ( itxControlComms )
		hard_reset_tx_module:
			begin
				dangerFlag <= 1'b0;
				if( otxTransducerOutput ) otxTransducerOutput <= 8'b0;
				if( otxTriggerOutput ) otxTriggerOutput <= 8'b0;
				if( otxLedOutput ) otxLedOutput <= 8'b0;
				otxTransducerOutputError <= 8'b0;
				trigRestLevel <= 8'b0;
				interruptError <= 4'b0;
				programRunningFlag <= 1'b0;
				instructionBufferFlag <= 3'b0;
				tx_output_cmd <= txout_reset_module;
				currentLoopAddr <= 15'b0;
				fire_at_flag <= 1'b0;
				if ( oInstructionReadAddr ) oInstructionReadAddr <= 12'b0;
				if ( otxLoopReadAddr ) otxLoopReadAddr <= 15'b0;
			end
		
		soft_reset_tx_module:
			begin
				dangerFlag <= 1'b0;
				if( otxTransducerOutput ) otxTransducerOutput <= 8'b0;
				if( otxTriggerOutput ^ trigRestLevel ) otxTriggerOutput <= trigRestLevel;
				if( otxLedOutput ) otxLedOutput <= 8'b0;
				otxTransducerOutputError <= 8'b0;
				interruptError <= 4'b0;
				programRunningFlag <= 1'b0;
				instructionBufferFlag <= 3'b0;
				tx_output_cmd <= txout_reset_module;
				currentLoopAddr <= 15'b0;
				fire_at_flag <= 1'b0;
				if ( oInstructionReadAddr ) oInstructionReadAddr <= 12'b0;
				if ( otxLoopReadAddr ) otxLoopReadAddr <= 15'b0;
			end
						
		load_program:
			begin
				dangerFlag <= 1'b0;			
				if( otxTransducerOutput ) otxTransducerOutput <= 8'b0;
				if( otxTriggerOutput ^ trigRestLevel ) otxTriggerOutput <= trigRestLevel;
				if( otxLedOutput ) otxLedOutput <= 8'b0;
				programRunningFlag <= 1'b0;
				if ( !loopsInitialized ) 
				begin
					if ( currentLoopAddr <= maxLoopAddr ) 
					begin
						loopStartAddr[currentLoopAddr]	<= itxLoopAddressReg[15:0];
						loopEndAddr[currentLoopAddr]	<= itxLoopAddressReg[31:16];
						loopCounter[currentLoopAddr]	<= itxLoopCounterReg[15:0];
						loopCounterRef[currentLoopAddr]	<= itxLoopCounterReg[15:0];		
						if( currentLoopAddr < maxLoopAddr ) 
						begin
							otxLoopReadAddr <= ( otxLoopReadAddr + 1'b1 );
							currentLoopAddr <= ( currentLoopAddr + 1'b1 );	
						end 
						else 
						begin
							loopsInitialized <= 1'b1;
							currentLoopAddr <= 4'b0;
						end
					end
				end
				
				else if ( loopsInitialized && !instructionBufferFlag[2] )
				begin
					tx_output_cmd <= txout_buffer_phase_charge;
					if (oInstructionReadAddr != iSetInstructionReadAddr)
					begin
						oInstructionReadAddr <= iSetInstructionReadAddr;
					end
					else if ( !instructionBufferFlag[0] )
					begin
						timeUntilNextInstruction[0] <= iTimeUntilNextInstruction;
						instructionType[0] <= iNextInstructionType;
						instruction[0] <= iNextInstruction;
						instructionBufferFlag[0] <= 1'b1;
					end
					else if ( instructionBufferFlag[0] & !instructionBufferFlag[1] )
					begin
						if ( !instructionType[0][is_loop_end_point] )
						begin
							oInstructionReadAddr <= oInstructionReadAddr + 1'b1;
						end
						else if ( instructionType[0][is_location_loop_start_point] )
						begin
							inLocationLoop <= 1'b1;
						end
						else
						begin
							inLocationLoop <= 1'b0;
							if ( loopCounter[loopNumber] > 1 )
							begin
								currentlyInLoop[loopNumber] <= 1'b1;
								loopCounter[loopNumber] <= loopCounter[loopNumber] - 1'b1;
								oInstructionReadAddr <= loopStartAddr[loopNumber];
							end
							else
							begin
								currentlyInLoop[loopNumber] <= 1'b0;
								loopCounter[loopNumber] <= loopCounterRef[loopNumber];
								oInstructionReadAddr <= ( oInstructionReadAddr + 1'b1 );
							end
						end
						instructionBufferFlag[1] <= 1'b1;
					end
					else if ( instructionBufferFlag[0] & instructionBufferFlag[1] )
					begin
						timeUntilNextInstruction[1] <= iTimeUntilNextInstruction;
						instructionType[1] <= iNextInstructionType;
						instruction[1] <= iNextInstruction;
						if ( iNextInstructionType[is_loop_end_point] && loopCounter[iNextInstruction[19:16]] > 1 )
						begin	
							oInstructionReadAddr <= loopStartAddr[iNextInstruction[19:16]];
						end
						else
						begin
							oInstructionReadAddr <= ( oInstructionReadAddr + 1'b1 );
						end
							
						instructionBufferFlag[2] <= 1'b1;
					end
				end				
			end
		
		set_led_output:
			begin
				dangerFlag <= 1'b1;
				if ( !programRunningFlag )
				begin
					if ( otxLedOutput ^ ledReg ) otxLedOutput <= ledReg;
				end
			end
			
		set_trig_output:
			begin
				dangerFlag <= 1'b1;
				if ( !programRunningFlag )
				begin
					if ( otxTriggerOutput ^ trigReg ) otxTriggerOutput <= trigReg;
				end
			end
		
		set_trig_rest_level:
			begin
				dangerFlag <= 1'b1;
				if ( !programRunningFlag )
				begin
					if ( trigRestLevel ^ trigRestLevelReg ) trigRestLevel <= trigRestLevelReg;
				end
			end
		
		test_trig_output:
			begin
				dangerFlag <= 1'b1;
				if ( !programRunningFlag )
				begin
					if ( otxTriggerOutput != ( trigReg ^ trigRestLevel ) ) otxTriggerOutput <= ( trigReg ^ trigRestLevel );
				end
			end
					
		run_program:
			begin
				dangerFlag <= 1'b1;
				programRunningFlag <= 1'b1;
			end
		
		default:
			begin
				dangerFlag <= 1'b0;
				if( otxTransducerOutput ) otxTransducerOutput <= 8'b0;
				if( otxTriggerOutput ^ trigRestLevel ) otxTriggerOutput <= trigRestLevel;
				if( otxLedOutput ) otxLedOutput <= 8'b0;
				otxTransducerOutputError <= 8'b0;
				interruptError <= 4'b0;
				programRunningFlag <= 1'b0;
				instructionBufferFlag <= 3'b0;
				tx_output_cmd <= txout_reset_module;
				currentLoopAddr <= 15'b0;
				fire_at_flag <= 1'b0;
				if ( oInstructionReadAddr ) oInstructionReadAddr <= 12'b0;
				if ( otxLoopReadAddr ) otxLoopReadAddr <= 15'b0;
			end
	
	endcase

	if ( txErrorFlag )
	begin
		interruptError[3] <= 1'b1;
		otxTransducerOutputError <= txErrorFlag;
		otxTransducerOutput <= 8'b0;
		programRunningFlag <= 1'b0;
		tx_output_cmd <= txout_wait_cmd;
		if ( oArmInterrupt[0] ^ tx_error ) oArmInterrupt[0] <= tx_error;
		if ( oArmInterrupt[1] ^ tx_error ) oArmInterrupt[1] <= tx_error;
	end
	
	if ( programRunningFlag & !interruptError & !txErrorFlag )
	begin
		if ( txCntrActive )
		begin
			if ( otxTransducerOutput ^ transducerOuput ) otxTransducerOutput <= transducerOuput;
			if ( transducerModule_txIsActive )
			begin
				txCntr <= txCntr + 1'b1;
			end
			else
			begin
				fire_at_flag <= 1'b0;
				txCntrActive <= 1'b0;
			end
			
			if ( !fire_at_flag && ( armMsg == set_fire_at_loc ) )
			begin
				if ( !oArmInterrupt[0] )
				begin
					oArmInterrupt[0] <= armMsg;
				end
				else if ( !oArmInterrupt[1] )
				begin
					oArmInterrupt[1] <= armMsg;
				end
				else
				begin
					interruptError[2] <= 1'b1;
				end
			end
		end	
		
		else if ( !txCntrActive )
		begin
			otxTransducerOutput <= 8'b0;
			fire_at_flag <= 1'b0;
			if ( ( tx_output_cmd == txout_fire_pulse ) || ( armMsg == set_fire_at_loc ) ) 
			begin
				tx_output_cmd <= txout_buffer_phase_charge;
				if ( armMsg )
				begin
					if ( !oArmInterrupt[0] )
					begin
						oArmInterrupt[0] <= armMsg;
					end
					else if ( !oArmInterrupt[1] )
					begin
						oArmInterrupt[1] <= armMsg;
					end
					else
					begin
						interruptError[2] <= 1'b1;
					end
				end
			end
		end
			
		if ( interruptResponse[0] )
		begin
			if ( !armResponseFlag[0] )
			begin
				if ( oArmInterrupt[0] )
				begin
					armResponseFlag[0] <= 1'b1;
					oArmInterrupt[0] <= no_arm_msg;
				end
				else
				begin
					interruptError[0] <= 1'b1;
				end
			end
		end
		else
		begin
			if ( armResponseFlag[0] ) armResponseFlag[0] <= 1'b0;
		end
			
		if ( interruptResponse[1] )
		begin
			if ( !armResponseFlag[1] )
			begin
				if ( oArmInterrupt[1] )
				begin
					armResponseFlag[1] <= 1'b1;
					oArmInterrupt[1] <= no_arm_msg;
				end
				else
				begin
					interruptError[1] <= 1'b1;
				end
			end
		end
		else
		begin
			if ( armResponseFlag[1] ) armResponseFlag[1] <= 1'b0;
		end
		
		if ( otxADCTriggerLine & itxADCTriggerAck )
		begin
			otxADCTriggerLine <= 1'b0;
		end

		if ( timeUntilNextInstruction[0] )
		begin
			timeUntilNextInstruction[0] <= timeUntilNextInstruction[0]-1'b1;
		end
		else if ( !instructionType[0][wait_for_external_trigger] | iExternalTrig )
		begin
			timeUntilNextInstruction[0] <= timeUntilNextInstruction[1];
			instructionType[0] <= instructionType[1];
			instruction[0] <= instruction[1];
			timeUntilNextInstruction[1] <= iTimeUntilNextInstruction;
			instructionType[1] <= iNextInstructionType;
			instruction[1] <= iNextInstruction;
			if ( iNextInstructionType[is_loop_end_point] && loopCounter[nextLoopNumber] > 1 )
			begin	
				oInstructionReadAddr <= loopStartAddr[nextLoopNumber];
			end
			else if ( !iNextInstructionType[program_end_point] )
			begin
				oInstructionReadAddr <= ( oInstructionReadAddr + 1'b1 );
			end
			
			if ( instructionType[0][set_trig] )
				begin
					otxTriggerOutput <= trigVals ^ trigRestLevel;
				end
			
			if ( instructionType[0][set_leds] )
				begin
					otxLedOutput <= ledVals;
				end
			
			if ( instructionType[0][trigger_recv] )
				begin
					otxADCTriggerLine <= 1'b1;
				end
				
			if ( instructionType[0][is_loop_end_point] )
				begin
					inLocationLoop <= 1'b0;
					if ( loopCounter[loopNumber] > 1 ) 
					begin
						currentlyInLoop[loopNumber] <= 1'b1;
						loopCounter[loopNumber] <= ( loopCounter[loopNumber] - 1'b1 );
						oInstructionReadAddr <= loopStartAddr[loopNumber];
					end 
					else 
					begin
						currentlyInLoop[loopNumber] <= 1'b0;
						loopCounter[loopNumber] <= loopCounterRef[loopNumber];
						oInstructionReadAddr <= ( oInstructionReadAddr + 1'b1 );
					end
				end
				
			if ( instructionType[0][is_location_loop_start_point] )
				begin
					inLocationLoop <= 1'b1;
				end
				
			if ( isFireCommand )
				begin
					if ( !txCntrActive )
					begin
						if ( instructionType[0][fire_pulse] & !instructionType[0][fire_at] & !instructionType[0][fire_single] )
						begin
							txCntrActive <= 1'b1;
							txCntr <= 32'b0;
							phaseCharge <= itxPulsePhaseCharge;
							tx_output_cmd <= txout_fire_pulse;
							
							if ( inLocationLoop | instructionType[0][is_location_loop_start_point] )
							begin
								armMsg <= fire_cmd_issued;
							end
							else
							begin
								armMsg <= no_arm_msg;
							end
						end
										
						else if ( !instructionType[0][fire_pulse] & instructionType[0][fire_at] & !instructionType[0][fire_single] )
						begin
							fire_at_flag <= 1'b1;
							txCntrActive <= 1'b1;
							txCntr <= 32'b0;
							phaseCharge <= itxFireAtPhaseCharge;
							tx_output_cmd <= txout_fire_pulse;
							armMsg <= fire_at_cmd_issued;
						end
						
						else if ( !instructionType[0][fire_pulse] & !instructionType[0][fire_at] & instructionType[0][fire_single] )
						begin
							fire_at_flag <= 1'b1;
							txCntrActive <= 1'b1;
							txCntr <= 32'b0;
							phaseCharge <= itxFireAtPhaseCharge;
							tx_output_cmd <= txout_fire_pulse;
							armMsg <= no_arm_msg;
						end
						
						else
						begin
							armMsg[7] <= 1'b1;
							armMsg[0] <= instructionType[0][fire_pulse];
							armMsg[1] <= instructionType[0][fire_at];
							armMsg[2] <= instructionType[0][fire_single];
						end
					end
					else
					begin
						armMsg[6] <= 1'b1;
						armMsg[0] <= instructionType[0][fire_pulse];
						armMsg[1] <= instructionType[0][fire_at];
						armMsg[2] <= instructionType[0][fire_single];
					end

				end

			if ( instructionType[0][set_loc] && !fire_at_flag )
				begin
					armMsg <= set_fire_at_loc;
				end
				
		end
		
	end
	else
	begin
		if ( dangerFlag )
		begin
			if( otxTransducerOutput ) otxTransducerOutput <= 8'b0;
			if( otxTriggerOutput ^ trigRestLevel ) otxTriggerOutput <= trigRestLevel;
			if( otxLedOutput ) otxLedOutput <= 8'b0;
		end
	end

end




transducerOutput_Module c0(
	.clk(txCLK),
	.cntr(txCntr),
	.phaseCharge(phaseCharge[0]),
	.txOutputState(transducerModule_outputVal[0]),
	.cmd(tx_output_cmd),
	.isActive(transducerModule_txIsActive[0]),
	.errorFlag(txErrorFlag[0])
);

transducerOutput_Module c1(
	.clk(txCLK),
	.cntr(txCntr),
	.phaseCharge(phaseCharge[1]),
	.txOutputState(transducerModule_outputVal[1]),
	.cmd(tx_output_cmd),
	.isActive(transducerModule_txIsActive[1]),
	.errorFlag(txErrorFlag[1])
);

transducerOutput_Module c2(
	.clk(txCLK),
	.cntr(txCntr),
	.phaseCharge(phaseCharge[2]),
	.txOutputState(transducerModule_outputVal[2]),
	.cmd(tx_output_cmd),
	.isActive(transducerModule_txIsActive[2]),
	.errorFlag(txErrorFlag[2])
);

transducerOutput_Module c3(
	.clk(txCLK),
	.cntr(txCntr),
	.phaseCharge(phaseCharge[3]),
	.txOutputState(transducerModule_outputVal[3]),
	.cmd(tx_output_cmd),
	.isActive(transducerModule_txIsActive[3]),
	.errorFlag(txErrorFlag[3])
);

transducerOutput_Module c4(
	.clk(txCLK),
	.cntr(txCntr),
	.phaseCharge(phaseCharge[4]),
	.txOutputState(transducerModule_outputVal[4]),
	.cmd(tx_output_cmd),
	.isActive(transducerModule_txIsActive[4]),
	.errorFlag(txErrorFlag[4])
);

transducerOutput_Module c5(
	.clk(txCLK),
	.cntr(txCntr),
	.phaseCharge(phaseCharge[5]),
	.txOutputState(transducerModule_outputVal[5]),
	.cmd(tx_output_cmd),
	.isActive(transducerModule_txIsActive[5]),
	.errorFlag(txErrorFlag[5])
);

transducerOutput_Module c6(
	.clk(txCLK),
	.cntr(txCntr),
	.phaseCharge(phaseCharge[6]),
	.txOutputState(transducerModule_outputVal[6]),
	.cmd(tx_output_cmd),
	.isActive(transducerModule_txIsActive[6]),
	.errorFlag(txErrorFlag[6])
);

transducerOutput_Module c7(
	.clk(txCLK),
	.cntr(txCntr),
	.phaseCharge(phaseCharge[7]),
	.txOutputState(transducerModule_outputVal[7]),
	.cmd(tx_output_cmd),
	.isActive(transducerModule_txIsActive[7]),
	.errorFlag(txErrorFlag[7])
);

endmodule


