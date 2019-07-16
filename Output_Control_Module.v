
module Output_Control_Module(

	input 					txCLK,
	
	input 					iSystemTrig,	
	input 					iExternalTrig,	
	input		[7:0]		itxControlComms,
	
	// procedural controls for instructions
	input		[63:0]		itxInstruction,
	input		[12:0]		itxSetInstructionReadAddr,
	output reg	[12:0]		oInstructionReadAddr,
	
	// phase delays for 'fire' cmd
	input		[127:0]		itxPhaseDelays,
	output reg	[13:0]		oPhaseDelayReadAddr,
	
	// phase delays for 'fireAt' cmd
	input		[127:0]		itxFireAtPhaseDelays,
	output reg	[7:0]		oFireAtPhaseDelayReadAddr,
	
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

// wires to split up itxInstruction into it's component parts
wire [15:0] iInstructionType;	assign iInstructionType = itxInstruction[63:48];
wire [15:0] iInstruction;		assign iInstruction = itxInstruction[47:32];
wire [31:0] iTimeUntilNextInstruction;		assign iTimeUntilNextInstruction = itxInstruction[31:0];

// transducer output value from transducer output module
wire	[7:0]	transducerModule_outputVal;

// And's together itxTransducerChannelMask and transducerOutput_Module to set actual transducer outputs
wand	[7:0]	transducerOuput;
assign transducerOuput = transducerModule_outputVal;
assign transducerOuput = itxTransducerChannelMask;

// flag from transducer output module indicating that pulse is currently firing 
wire 	[7:0]	transducerModule_txIsActive;
wire	[7:0]	txErrorFlag;

reg	[7:0]	trigRestLevel;

reg [31:0] timeUntilNextInstruction;
reg [1:0][31:0] timingReg;
reg [1:0][15:0] instructionType;
reg [1:0][15:0] instruction;
reg [1:0] instructionBufferFlag;

//reg [127:0] fire_fireAt_switch;
reg fire_fireAt_switch;
reg [127:0] fireCmdPhaseDelayReg;
reg [8:0] chargeTimeReg;


wire [7:0][15:0] phaseDelays;
assign phaseDelays[0] = fireCmdPhaseDelayReg[15:0];		//assign phaseDelays[0] = fireAtCmd_Phases[15:0];
assign phaseDelays[1] = fireCmdPhaseDelayReg[31:16];		//assign phaseDelays[1] = fireAtCmd_Phases[31:16];
assign phaseDelays[2] = fireCmdPhaseDelayReg[47:32];		//assign phaseDelays[2] = fireAtCmd_Phases[47:32];
assign phaseDelays[3] = fireCmdPhaseDelayReg[63:48];		//assign phaseDelays[3] = fireAtCmd_Phases[63:48];
assign phaseDelays[4] = fireCmdPhaseDelayReg[79:64];		//assign phaseDelays[4] = fireAtCmd_Phases[79:64];
assign phaseDelays[5] = fireCmdPhaseDelayReg[95:80];		//assign phaseDelays[5] = fireAtCmd_Phases[95:80];
assign phaseDelays[6] = fireCmdPhaseDelayReg[111:96];		//assign phaseDelays[6] = fireAtCmd_Phases[111:96];
assign phaseDelays[7] = fireCmdPhaseDelayReg[127:112];	//assign phaseDelays[7] = fireAtCmd_Phases[127:112];

wire [7:0][15:0] fireAtPhaseDelays;
assign fireAtPhaseDelays[0] = itxPhaseDelays[15:0];		//assign phaseDelays[0] = fireAtCmd_Phases[15:0];
assign fireAtPhaseDelays[1] = itxPhaseDelays[31:16];		//assign phaseDelays[1] = fireAtCmd_Phases[31:16];
assign fireAtPhaseDelays[2] = itxPhaseDelays[47:32];		//assign phaseDelays[2] = fireAtCmd_Phases[47:32];
assign fireAtPhaseDelays[3] = itxPhaseDelays[63:48];		//assign phaseDelays[3] = fireAtCmd_Phases[63:48];
assign fireAtPhaseDelays[4] = itxPhaseDelays[79:64];		//assign phaseDelays[4] = fireAtCmd_Phases[79:64];
assign fireAtPhaseDelays[5] = itxPhaseDelays[95:80];		//assign phaseDelays[5] = fireAtCmd_Phases[95:80];
assign fireAtPhaseDelays[6] = itxPhaseDelays[111:96];		//assign phaseDelays[6] = fireAtCmd_Phases[111:96];
assign fireAtPhaseDelays[7] = itxPhaseDelays[127:112];	//assign phaseDelays[7] = fireAtCmd_Phases[127:112];

wire [8:0] inputChargeTime;	assign inputChargeTime = instruction[0][8:0];
wire [6:0] trigVals;	assign trigVals = instruction[0][15:9];
wire [7:0] ledVals;		assign ledVals = instruction[0][7:0];

wire [12:0] loopStartAddr;	assign loopStartAddr = instruction[1][12:0];
wire [3:0] loopNum;			assign loopNum = timingReg[1][31:28];
wire [27:0] loopCounterRef;	assign loopCounterRef = timingReg[1][27:0];


parameter maxLoopAddr = 15;
reg [maxLoopAddr:0][27:0]	loopCounter;
reg [maxLoopAddr:0]			loopActive;
reg [maxLoopAddr:0]			isIterLoop;
reg [maxLoopAddr:0][13:0]	loopPhaseDelayStartAddr;
reg [maxLoopAddr:0][13:0]	loopPhaseDelayAddrIncr;
reg [3:0] lastLoopNum;

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
parameter [3:0] no_op = 4'b0000; // 0
parameter [3:0] set_trig = 4'b0001; // 1
parameter [3:0] set_leds = 4'b0010; // 2
parameter [3:0] trigger_recv = 4'b0011; // 3
parameter [3:0] trigger_recv_local = 4'b0100; // 4
parameter [3:0] is_loop_start_point = 4'b0101; // 5
parameter [3:0] is_loop_end_point = 4'b0110; // 6
parameter [3:0] fire_pulse = 4'b0111; // 7
parameter [3:0] fire_at = 4'b1000; // 8
parameter [3:0] set_charge_time = 4'b1001; // 9
parameter [3:0] generate_tx_interrupt = 4'b1010; // 10
parameter [3:0] wait_for_external_trigger = 4'b1011; // 11
parameter [3:0] wait_for_interrupt_to_resolve = 4'b1100; // 12
parameter [3:0] program_end_point = 4'b1111; // 15

parameter [8:0] ct500 = 9'b111110100;

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

reg fireCmdIssued_flag;
reg chargeTimeSet_flag;

initial
begin
	fireCmdPhaseDelayReg = 128'b0;
	fire_fireAt_switch = 1'b1;
	chargeTimeReg = ct500; 
	fireCmdIssued_flag = 1'b0;
	chargeTimeSet_flag = 1'b0;
	interruptError = 4'b0;
	trigRestLevel = 8'b0;
	armResponseFlag = 3'b0;
	tx_output_cmd = txout_reset_module;
	instructionBufferFlag = 2'b0;
	otxTransducerOutputError = 8'b0;
	dangerFlag = 1'b0;
	programRunningFlag = 1'b0;
	txCntrActive = 1'b0;
	loopActive = 16'b0;
	isIterLoop = 16'b0;
	lastLoopNum = 4'b0;
	timeUntilNextInstruction = 32'b0;
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
				fireCmdPhaseDelayReg <= 128'b0;
				fire_fireAt_switch <= 1'b1;
				fireCmdIssued_flag <= 1'b0;
				chargeTimeSet_flag <= 1'b0;
				otxTransducerOutputError <= 8'b0;
				trigRestLevel <= 8'b0;
				interruptError <= 4'b0;
				programRunningFlag <= 1'b0;
				instructionBufferFlag <= 2'b0;
				tx_output_cmd <= txout_reset_module;
				lastLoopNum <= 4'b0;
				if ( oInstructionReadAddr ) oInstructionReadAddr <= 15'b0;
			end
		
		soft_reset_tx_module:
			begin
				dangerFlag <= 1'b0;
				if( otxTransducerOutput ) otxTransducerOutput <= 8'b0;
				if( otxTriggerOutput ^ trigRestLevel ) otxTriggerOutput <= trigRestLevel;
				if( otxLedOutput ) otxLedOutput <= 8'b0;
				fireCmdPhaseDelayReg <= 128'b0;
				fire_fireAt_switch <= 1'b1;
				fireCmdIssued_flag <= 1'b0;
				chargeTimeSet_flag <= 1'b0;
				otxTransducerOutputError <= 8'b0;
				interruptError <= 4'b0;
				programRunningFlag <= 1'b0;
				instructionBufferFlag <= 2'b0;
				tx_output_cmd <= txout_reset_module;
				lastLoopNum <= 4'b0;
				if ( oInstructionReadAddr ) oInstructionReadAddr <= 15'b0;
			end
						
		load_program:
			begin
				dangerFlag <= 1'b0;			
				if( otxTransducerOutput ) otxTransducerOutput <= 8'b0;
				if( otxTriggerOutput ^ trigRestLevel ) otxTriggerOutput <= trigRestLevel;
				if( otxLedOutput ) otxLedOutput <= 8'b0;
				programRunningFlag <= 1'b0;
				
				if ( !instructionBufferFlag[1] )
				begin
					tx_output_cmd <= txout_reset_module;
					if (oInstructionReadAddr != itxSetInstructionReadAddr)
					begin
						oInstructionReadAddr <= itxSetInstructionReadAddr;
					end
					else if ( !instructionBufferFlag )
					begin
						instructionType[0] <= iInstructionType;
						instruction[0] <= iInstruction;
						timeUntilNextInstruction <= iTimeUntilNextInstruction;
						
						instructionBufferFlag[0] <= 1'b1;
						
						oInstructionReadAddr <= oInstructionReadAddr + 1'b1;
					end
					else if ( instructionBufferFlag[0] & !instructionBufferFlag[1] )
					begin
						instructionType[1] <= iInstructionType;
						instruction[1] <= iInstruction;
						timingReg[1] <= iTimeUntilNextInstruction;
						oInstructionReadAddr <= ( oInstructionReadAddr + 1'b1 );

						instructionBufferFlag[1] <= 1'b1;
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
				fireCmdPhaseDelayReg <= 128'b0;
				fire_fireAt_switch <= 1'b1;
				fireCmdIssued_flag <= 1'b0;
				otxTransducerOutputError <= 8'b0;
				interruptError <= 4'b0;
				programRunningFlag <= 1'b0;
				instructionBufferFlag <= 2'b0;
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
		if ( oArmInterrupt[0] ^ 32'hFFFF ) oArmInterrupt[0] <= 32'hFFFF;
		if ( oArmInterrupt[1] ^ 32'hFFFF ) oArmInterrupt[1] <= 32'hFFFF;
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

		
		if ( otxADCTriggerLine & itxADCTriggerAck )
		begin
			otxADCTriggerLine <= 1'b0;
		end

		if ( timeUntilNextInstruction )
		begin
			timeUntilNextInstruction <= timeUntilNextInstruction-1'b1;
		end
		else if ( !waitingForExternal & !waitingForInterruptToResolve )
		begin
			instructionType[1] <= iInstructionType;
			instruction[1] <= iInstruction;
			timingReg[1] <= iTimeUntilNextInstruction;
			
			instructionType[0] <= instructionType[1];
			instruction[0] <= instruction[1];
			timingReg[0] <= timingReg[1];

			if ( instructionType[1][is_loop_end_point] )
			begin
				timeUntilNextInstruction <= 32'b0;
				if ( !loopActive[loopNum] && ( loopCounterRef > 1 ) )
				begin
					lastLoopNum <= loopNum;
					oInstructionReadAddr <= loopStartAddr;
					loopCounter[loopNum] <= 28'h2;
					loopActive[loopNum] <= 1'b1;					
				end
				else if ( loopActive[loopNum] && ( loopCounter[loopNum] < loopCounterRef ) )
				begin
					lastLoopNum <= loopNum;
					oInstructionReadAddr <= loopStartAddr;
					loopCounter[loopNum] <= loopCounter[loopNum]+1'b1;	
				end
				else
				begin
					oInstructionReadAddr <= oInstructionReadAddr + 1'b1;
					loopCounter[loopNum] <= 28'b0;
					loopActive[loopNum] <= 1'b0;
					isIterLoop[loopNum] <= 1'b0;
				end
			end
			else if ( instructionType[1][is_loop_start_point] )
			begin
				loopCounter[loopNum] <= 28'h1;
				loopActive[loopNum] <= 1'b1;
				timeUntilNextInstruction <= 32'b0;
				oInstructionReadAddr <= ( oInstructionReadAddr + 1'b1 );
			end
			else if ( !instructionType[1][program_end_point] )
			begin
				timeUntilNextInstruction <= timingReg[1];
				oInstructionReadAddr <= ( oInstructionReadAddr + 1'b1 );
			end
			
			
			if ( instructionType[0][is_loop_end_point] & isIterLoop[lastLoopNum] & loopActive[lastLoopNum] )
				begin
					oPhaseDelayReadAddr[lastLoopNum] <= loopPhaseDelayStartAddr[lastLoopNum] + loopPhaseDelayAddrIncr[lastLoopNum];
					loopPhaseDelayStartAddr[lastLoopNum] <= loopPhaseDelayStartAddr[lastLoopNum] + loopPhaseDelayAddrIncr[lastLoopNum];
				end

			if ( instructionType[0][is_loop_start_point] )
				begin
					if( isIterLoop[lastLoopNum] )
					begin
						oPhaseDelayReadAddr[lastLoopNum] <= loopPhaseDelayStartAddr[lastLoopNum] + loopPhaseDelayAddrIncr[lastLoopNum];
						loopPhaseDelayStartAddr[lastLoopNum] <= loopPhaseDelayStartAddr[lastLoopNum] + loopPhaseDelayAddrIncr[lastLoopNum];
					end
					else
					begin
						oPhaseDelayReadAddr[lastLoopNum] <= instruction[0][13:0];
						loopPhaseDelayStartAddr[lastLoopNum] <= instruction[0][13:0];
						loopPhaseDelayAddrIncr[lastLoopNum] <= timingReg[0][13:0];
						isIterLoop[lastLoopNum] <= 1'b1;
					end
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
			
			if ( isFireCommand )
				begin
					if ( !txCntrActive )
					begin
						if ( instructionType[0][fire_pulse] & !instructionType[0][fire_at] )
						begin
							txCntrActive <= 1'b1;
							txCntr <= 32'b0;
							
							if ( !fireCmdIssued_flag )
							begin
								fireCmdPhaseDelayReg <= itxPhaseDelays;
								fire_fireAt_switch <= 1'b1;
								fireCmdIssued_flag <= 1'b1;
							end
							
							if( !chargeTimeSet_flag )
							begin
								if ( !set_charge_time )
								begin
									chargeTimeReg <= ct500;
								end
								else
								begin
									chargeTimeReg <= inputChargeTime;
								end
							end
							
							tx_output_cmd <= txout_fire_pulse;
							
						end
										
						else if ( instructionType[0][fire_at] & !instructionType[0][fire_pulse] )
						begin
							txCntrActive <= 1'b1;
							txCntr <= 32'b0;
							oPhaseDelayReadAddr <= instruction[0][13:0];
							fire_fireAt_switch <= 1'b0;
							
							if( !chargeTimeSet_flag )
							begin
								if ( !set_charge_time )
								begin
									chargeTimeReg <= ct500;
								end
								else
								begin
									chargeTimeReg <= inputChargeTime;
								end
							end
							
							tx_output_cmd <= txout_fire_pulse;
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
	.phaseDelay(phaseDelays[0]),
	.fireAtPhaseDelay(fireAtPhaseDelays[0]),
	.fireSwitch(fire_fireAt_switch),
	.chargeTime(chargeTimeReg),
	.txOutputState(transducerModule_outputVal[0]),
	.cmd(tx_output_cmd),
	.isActive(transducerModule_txIsActive[0]),
	.errorFlag(txErrorFlag[0])
);

transducerOutput_Module c1(
	.clk(txCLK),
	.cntr(txCntr),
	.phaseDelay(phaseDelays[1]),
	.fireAtPhaseDelay(fireAtPhaseDelays[1]),
	.fireSwitch(fire_fireAt_switch),
	.chargeTime(chargeTimeReg),
	.txOutputState(transducerModule_outputVal[1]),
	.cmd(tx_output_cmd),
	.isActive(transducerModule_txIsActive[1]),
	.errorFlag(txErrorFlag[1])
);

transducerOutput_Module c2(
	.clk(txCLK),
	.cntr(txCntr),
	.phaseDelay(phaseDelays[2]),
	.fireAtPhaseDelay(fireAtPhaseDelays[2]),
	.fireSwitch(fire_fireAt_switch),
	.chargeTime(chargeTimeReg),
	.txOutputState(transducerModule_outputVal[2]),
	.cmd(tx_output_cmd),
	.isActive(transducerModule_txIsActive[2]),
	.errorFlag(txErrorFlag[2])
);

transducerOutput_Module c3(
	.clk(txCLK),
	.cntr(txCntr),
	.phaseDelay(phaseDelays[3]),
	.fireAtPhaseDelay(fireAtPhaseDelays[3]),
	.fireSwitch(fire_fireAt_switch),
	.chargeTime(chargeTimeReg),
	.txOutputState(transducerModule_outputVal[3]),
	.cmd(tx_output_cmd),
	.isActive(transducerModule_txIsActive[3]),
	.errorFlag(txErrorFlag[3])
);

transducerOutput_Module c4(
	.clk(txCLK),
	.cntr(txCntr),
	.phaseDelay(phaseDelays[4]),
	.fireAtPhaseDelay(fireAtPhaseDelays[4]),
	.fireSwitch(fire_fireAt_switch),
	.chargeTime(chargeTimeReg),
	.txOutputState(transducerModule_outputVal[4]),
	.cmd(tx_output_cmd),
	.isActive(transducerModule_txIsActive[4]),
	.errorFlag(txErrorFlag[4])
);

transducerOutput_Module c5(
	.clk(txCLK),
	.cntr(txCntr),
	.phaseDelay(phaseDelays[5]),
	.fireAtPhaseDelay(fireAtPhaseDelays[5]),
	.fireSwitch(fire_fireAt_switch),
	.chargeTime(chargeTimeReg),
	.txOutputState(transducerModule_outputVal[5]),
	.cmd(tx_output_cmd),
	.isActive(transducerModule_txIsActive[5]),
	.errorFlag(txErrorFlag[5])
);

transducerOutput_Module c6(
	.clk(txCLK),
	.cntr(txCntr),
	.phaseDelay(phaseDelays[6]),
	.fireAtPhaseDelay(fireAtPhaseDelays[6]),
	.fireSwitch(fire_fireAt_switch),
	.chargeTime(chargeTimeReg),
	.txOutputState(transducerModule_outputVal[6]),
	.cmd(tx_output_cmd),
	.isActive(transducerModule_txIsActive[6]),
	.errorFlag(txErrorFlag[6])
);

transducerOutput_Module c7(
	.clk(txCLK),
	.cntr(txCntr),
	.phaseDelay(phaseDelays[7]),
	.fireAtPhaseDelay(fireAtPhaseDelays[7]),
	.fireSwitch(fire_fireAt_switch),
	.chargeTime(chargeTimeReg),
	.txOutputState(transducerModule_outputVal[7]),
	.cmd(tx_output_cmd),
	.isActive(transducerModule_txIsActive[7]),
	.errorFlag(txErrorFlag[7])
);

endmodule


