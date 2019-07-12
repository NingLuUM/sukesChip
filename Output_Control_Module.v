
module Output_Control_Module(

	input 					txCLK,
	
	input 					iSystemTrig,	
	input 					iExternalTrig,	
	input		[7:0]		itxControlComms,
	
	// procedural controls for instructions
	

	input		[63:0]		iNextInstruction,
	input		[127:0]		iPhaseDelays,
	input		[14:0]		iSetInstructionReadAddr,
	output reg	[13:0]		oInstructionReadAddr,
	output reg	[13:0]		oPhaseDelayReadAddr,
	
	
	// transducer output controls
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
	output reg	[1:0][31:0]	oArmInterrupt,
	input		[31:0]		interruptResponse,
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
reg [1:0][15:0] instruction;
reg [2:0] instructionBufferFlag;

reg [127:0] fire_fireAt_switch;
reg [127:0] phases;

wand [127:0] fireCmd_Phases;
assign fireCmd_Phases = fire_fireAt_switch;
assign fireCmd_Phases = phases;

wand [127:0] fireAtCmd_Phases;
assign fireAtCmd_Phases = ~fire_fireAt_switch;
assign fireAtCmd_Phases = iPhaseDelays;

wor [127:0] activePhases;
assign activePhases = fireCmd_Phases;
assign activePhases = fireAtCmd_Phases;

wire [7:0][15:0] phaseCharge;	

wire [7:0] trigVals;
wire [7:0] ledVals;
assign trigVals = instruction[0][7:0];
assign ledVals = instruction[0][15:8];

wire [14:0] nextLoopStartAddr;
wire [3:0] nextLoopNum;
wire [31:0] nextLoopCounterRef;

assign nextLoopStartAddr = iNextInstruction[14:0];
assign nextLoopNum = iTimeUntilNextInstruction[31:28];
assign nextLoopCounterRef = iTimeUntilNextInstruction[27:0];

parameter maxLoopAddr = 15;
reg [maxLoopAddr:0][27:0]	loopCounter;
reg [maxLoopAddr:0]			loopActive;


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
parameter [3:0] set_trig = 4'b0000; // 0
parameter [3:0] set_leds = 4'b0001; // 1
parameter [3:0] trigger_recv = 4'b0010; // 2
parameter [3:0] is_loop_start_point = 4'b0011; // 3
parameter [3:0] is_loop_end_point = 4'b0100; // 4
parameter [3:0] fire_pulse = 4'b0101; // 5
parameter [3:0] fire_at = 4'b0110; // 6
parameter [3:0] generate_tx_interrupt = 4'b0111; // 7
parameter [3:0] wait_for_external_trigger = 4'b1000; // 8
parameter [3:0] wait_for_interrupt_to_resolve = 4'b1001; // 9
parameter [3:0] program_end_point = 4'b1111; // 15

wor isFireCommand;
assign isFireCommand = instructionType[0][fire_pulse];
assign isFireCommand = instructionType[0][fire_at];

wor waitingForExternal;
assign waitingForExternal = instructionType[0][wait_for_external_trigger];
assign waitingForExternal = !iExternalTrig;

wor waitingForInterruptToResolve;
assign waitingForInterruptToResolve = instructionType[0][wait_for_interrupt_to_resolve];
assign waitingForInterruptToResolve = !interruptResponse;


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
	phaseCharge[0] = 32'b0;
	phaseCharge[1] = 32'b0;
	phaseCharge[2] = 32'b0;
	phaseCharge[3] = 32'b0;
	phaseCharge[4] = 32'b0;
	phaseCharge[5] = 32'b0;
	phaseCharge[6] = 32'b0;
	phaseCharge[7] = 32'b0;
	interruptError = 4'b0;
	trigRestLevel = 8'b0;
	armResponseFlag = 3'b0;
	tx_output_cmd = txout_reset_module;
	instructionBufferFlag = 3'b0;
	otxTransducerOutputError = 8'b0;
	dangerFlag = 1'b0;
	programRunningFlag = 1'b0;
	txCntrActive = 1'b0;
	loopActive = 16'b0;
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
				if ( oInstructionReadAddr ) oInstructionReadAddr <= 15'b0;
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
				if ( oInstructionReadAddr ) oInstructionReadAddr <= 15'b0;
			end
						
		load_program:
			begin
				dangerFlag <= 1'b0;			
				if( otxTransducerOutput ) otxTransducerOutput <= 8'b0;
				if( otxTriggerOutput ^ trigRestLevel ) otxTriggerOutput <= trigRestLevel;
				if( otxLedOutput ) otxLedOutput <= 8'b0;
				programRunningFlag <= 1'b0;
				
				if ( !instructionBufferFlag[2] )
				begin
					tx_output_cmd <= txout_reset_module;
					if (oInstructionReadAddr != iSetInstructionReadAddr)
					begin
						oInstructionReadAddr <= iSetInstructionReadAddr;
					end
					else if ( !instructionBufferFlag[0] )
					begin
						instructionType[0] <= iNextInstructionType;
						instruction[0] <= iNextInstruction;
						timeUntilNextInstruction[0] <= iTimeUntilNextInstruction;
						
						instructionBufferFlag[0] <= 1'b1;
					end
					else if ( instructionBufferFlag[0] & !instructionBufferFlag[1] )
					begin
						if ( instructionType[0][is_loop_end_point] )
						begin
							timeUntilNextInstruction[0] <= 32'b0;
							if ( !loopActive[nextLoopNum] && ( nextLoopCounterRef > 1 ) )
							begin
								oInstructionReadAddr <= nextLoopStartAddr;
								loopCounter[nextLoopNum] <= {27'b0,1'b1};
								loopActive[nextLoopNum] <= 1'b1;
							end
							else if ( loopActive[nextLoopNum] && ( loopCounter[nextLoopNum] < nextLoopCounterRef ) )
							begin
								oInstructionReadAddr <= nextLoopStartAddr;
								loopCounter[nextLoopNum] <= loopCounter[nextLoopNum]+1'b1;
								loopActive[nextLoopNum] <= 1'b1;
							end
							else
							begin
								oInstructionReadAddr <= oInstructionReadAddr + 1'b1;
								loopCounter[nextLoopNum] <= 28'b0;
								loopActive[nextLoopNum] <= 1'b0;
							end
						end
						else
						begin
							oInstructionReadAddr <= oInstructionReadAddr + 1'b1;
						end
						instructionBufferFlag[1] <= 1'b1;
					end
					else if ( instructionBufferFlag[0] & instructionBufferFlag[1] )
					begin
						instructionType[1] <= iNextInstructionType;
						instruction[1] <= iNextInstruction;
						if ( iNextInstructionType[is_loop_end_point] )
						begin
							timeUntilNextInstruction[1] <= 32'b0;
							if ( !loopActive[nextLoopNum] && ( nextLoopCounterRef > 1 ) )
							begin
								oInstructionReadAddr <= nextLoopStartAddr;
								loopCounter[nextLoopNum] <= {27'b0,1'b1};
								loopActive[nextLoopNum] <= 1'b1;
							end
							else if ( loopActive[nextLoopNum] && ( loopCounter[nextLoopNum] < nextLoopCounterRef ) )
							begin
								oInstructionReadAddr <= nextLoopStartAddr;
								loopCounter[nextLoopNum] <= loopCounter[nextLoopNum]+1'b1;
								loopActive[nextLoopNum] <= 1'b1;
							end
							else
							begin
								oInstructionReadAddr <= oInstructionReadAddr + 1'b1;
								loopCounter[nextLoopNum] <= 28'b0;
								loopActive[nextLoopNum] <= 1'b0;
							end
						end
						else
						begin
							timeUntilNextInstruction[1] <= iTimeUntilNextInstruction;
							oInstructionReadAddr <= ( oInstructionReadAddr + 1'b1 );
						end
						otxCurrentlyInLoop <= 32'b0;
						otxCurrentLoopIteration <= 32'b0;
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
				if ( oInstructionReadAddr ) oInstructionReadAddr <= 15'b0;
			end
	
	endcase

	if ( txErrorFlag )
	begin
		interruptError[3] <= 1'b1;
		otxTransducerOutputError <= txErrorFlag;
		otxTransducerOutput <= 8'b0;
		programRunningFlag <= 1'b0;
		tx_output_cmd <= txout_wait_cmd;
		if ( oArmInterrupt[0] ^ 32'b1 ) oArmInterrupt[0] <= 32'b1;
		if ( oArmInterrupt[1] ^ 32'b1 ) oArmInterrupt[1] <= 32'b1;
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
				tx_output_cmd <= txout_wait_cmd;
				txCntrActive <= 1'b0;
			end	
		end

		if ( interruptResponse && !armResponseFlag )
		begin
			if( oArmInterrupt[0] ) oArmInterrupt[0] <= 32'b0;
			armResponseFlag <= 3'b1;
		end
		else if ( !interruptResponse && armResponseFlag )
		begin
			armResponseFlag <= 3'b0;
		end
		
		if ( otxADCTriggerLine & itxADCTriggerAck )
		begin
			otxADCTriggerLine <= 1'b0;
		end

		if ( timeUntilNextInstruction[0] )
		begin
			timeUntilNextInstruction[0] <= timeUntilNextInstruction[0]-1'b1;
		end
		else if ( !waitingForExternal & !waitingForInterruptToResolve )
		begin
			instructionType[0] <= instructionType[1];
			instruction[0] <= instruction[1];
			timeUntilNextInstruction[0] <= timeUntilNextInstruction[1];

			instructionType[1] <= iNextInstructionType;
			instruction[1] <= iNextInstruction;
			if ( iNextInstructionType[is_loop_end_point] )
			begin
				timeUntilNextInstruction[1] <= 32'b0;
				if ( !loopActive[nextLoopNum] && ( nextLoopCounterRef > 1 ) )
				begin
					oInstructionReadAddr <= nextLoopStartAddr;
					loopCounter[nextLoopNum] <= {27'b0,1'b1};
					loopActive[nextLoopNum] <= 1'b1;
					otxCurrentlyInLoop <= nextLoopNum;
					otxCurrentLoopIteration <= 1'b1;
				end
				else if ( loopActive[nextLoopNum] && ( loopCounter[nextLoopNum] < nextLoopCounterRef ) )
				begin
					oInstructionReadAddr <= nextLoopStartAddr;
					loopCounter[nextLoopNum] <= loopCounter[nextLoopNum]+1'b1;
					loopActive[nextLoopNum] <= 1'b1;
					otxCurrentlyInLoop <= nextLoopNum;
					otxCurrentLoopIteration <= loopCounter[nextLoopNum]+1'b1;
				end
				else
				begin
					oInstructionReadAddr <= oInstructionReadAddr + 1'b1;
					loopCounter[nextLoopNum] <= 28'b0;
					loopActive[nextLoopNum] <= 1'b0;
					otxCurrentlyInLoop <= 32'b0;
					otxCurrentLoopIteration <= 32'b0;
				end
			end
			else if ( !iNextInstructionType[program_end_point] )
			begin
				timeUntilNextInstruction[1] <= iTimeUntilNextInstruction;
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
				
			if ( instructionType[0][is_loop_start_point] )
				begin
					oArmInterrupt[0] <= instructionType[0];
				end
			
			if ( isFireCommand )
				begin
					if ( !txCntrActive )
					begin
						if ( instructionType[0][fire_pulse] & !instructionType[0][fire_at] )
						begin
							txCntrActive <= 1'b1;
							txCntr <= 32'b0;
							phaseCharge <= itxPulsePhaseCharge;
							tx_output_cmd <= txout_fire_pulse;
							
							if ( instructionType[0][generate_tx_interrupt] )
							begin
								oArmInterrupt[0] <= instructionType[0];
							end
							
						end
										
						else if ( instructionType[0][fire_at] & !instructionType[0][fire_pulse] )
						begin
							txCntrActive <= 1'b1;
							txCntr <= 32'b0;
							phaseCharge <= itxFireAtPhaseCharge;
							tx_output_cmd <= txout_fire_pulse;
							
							if ( instructionType[0][generate_tx_interrupt] )
							begin
								oArmInterrupt[0] <= instructionType[0];
							end
							
						end
						
						else
						begin
							oArmInterrupt[0] <= instructionType[0];
							oArmInterrupt[1] <= 32'b1;
						end
	
					end
					else
					begin
						oArmInterrupt[0] <= 32'b1;
						oArmInterrupt[1] <= 32'b1;
					end

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


