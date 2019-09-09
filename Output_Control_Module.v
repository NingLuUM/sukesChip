
module Output_Control_Module(

	input 					txCLK,
	input 					iSystemTrig,	
	
	input		[7:0]		itxControlComms,
	
	// procedural controls for instructions
	input		[63:0]		itxInstruction,
	output reg	[12:0]		oInstructionReadAddr,
	
	// phase delays for 'fire' cmd
	input		[127:0]		itxPhaseDelays,
	output reg	[12:0]		oPhaseDelayReadAddr,
	
	// transducer output controls
	input		[7:0]		itxTransducerChannelMask,
	output reg	[7:0]		otxTransducerOutput,
	
	// trigger output controls
	input 					itxMasterToDaughter_Sync,
	output reg				otxToAll_Danger_KillProgram,
	output reg				otxDaughterToMaster_WaitForMe,
	
	// recv system
	input					itxADCTriggerAck,
	output reg				otxADCTriggerLine,
	

);

// wires to split up itxInstruction into it's component parts
wire [15:0] iInstructionType;				assign iInstructionType = itxInstruction[63:48];
wire [15:0] iInstruction;					assign iInstruction = itxInstruction[47:32];
wire [31:0] iTimeUntilNextInstruction;		assign iTimeUntilNextInstruction = itxInstruction[31:0];

// caches to buffer upcoming instructions
reg [1:0][31:0] timingReg;
reg [1:0][15:0] instructionType;
reg [1:0][15:0] instruction;
reg [31:0] timeUntilNextInstruction;

// start address of loops and mem idxs of phase delays are encoded in 'instruction' reg
wire [12:0] instrReadAddr;	assign instrReadAddr = instruction[1][12:0];

// wire-up loop controls, which are embedded in the instruction/timing registers
wire [3:0] loopNum;			assign loopNum = timingReg[1][31:28];
wire [27:0] loopCounterRef;	assign loopCounterRef = timingReg[1][27:0];

// phase delays + charge time
reg [7:0][15:0] phaseDelays;
reg [8:0] chargeTimeRef_Reg;

// transducer output value from transducer output module
wire	[7:0]	transducer_specifiedOutput;

// And's together itxTransducerChannelMask and transducer_specifiedOutput to set actual transducer outputs
wand	[7:0]	transducer_physicalOuput;
assign transducer_physicalOuput = transducer_specifiedOutput;
assign transducer_physicalOuput = itxTransducerChannelMask;


// local storage for loop variables
parameter maxLoopAddr = 15;
reg [maxLoopAddr:0][27:0]	loopCounter;
reg [maxLoopAddr:0]			loopActive;
reg [maxLoopAddr:0]			isSteeringLoop;
reg [maxLoopAddr:0][13:0]	loopPhaseDelayStartAddr;
reg [maxLoopAddr:0][13:0]	loopPhaseDelayAddrIncr;
reg [3:0] bufferedLoopNum;


// flags to control run-time
reg danger_FLAG;
reg [2:0] instructionBuffer_FLAG;
reg programRunning_FLAG = 1'b0;
	
// itxControlComms Output_Control_Module case states
parameter [7:0]	CASE_IDLE = 			8'b00000000;
parameter [7:0]	CASE_LOAD_PROGRAM =		8'b00000001;
parameter [7:0]	CASE_RUN_PROGRAM =		8'b10000000;

// instruction type list
parameter [3:0] no_op = 4'b0000; // 0
parameter [3:0] trigger_recv_sys = 4'b0011; // 3
parameter [3:0] is_loop_start_point = 4'b0100; // 4
parameter [3:0] is_loop_end_point = 4'b0101; // 5
parameter [3:0] fire_pulse = 4'b0110; // 6
parameter [3:0] set_charge_time = 4'b1000; // 8
parameter [3:0] generate_tx_interrupt = 4'b1001; // 9
parameter [3:0] wait_for_external_trigger = 4'b1010; // 10
parameter [3:0] wait_for_interrupt_to_resolve = 4'b1011; // 11
parameter [3:0] program_end_point = 4'b1111; // 15

parameter [8:0] ct500 = 9'b111110100;

initial
begin
	otxTransducerOutputError = 8'b0;
	txCntrActive = 1'b0;
	loopActive = 16'b0;
	isSteeringLoop = 16'b0;
	bufferedLoopNum = 4'b0;
	timeUntilNextInstruction = 32'b0;
	
	danger_FLAG = 1'b0;
	instructionBuffer_FLAG = 3'b0;
	programRunning_FLAG = 1'b0;

end

always @(posedge txCLK)
begin
	case( itxControlComms )
		CASE_IDLE:
			begin
				if(otxTransducerOutput) otxTransducerOutput <= 8'b00000000;
				programRunning_FLAG <= 1'b0;
				bufferInit_flag <= 1'b0;
			end
		
		CASE_LOAD_PROGRAM:
			begin
				if(otxTransducerOutput) otxTransducerOutput <= 8'b00000000;
				
				if( !instructionBuffer_FLAG[2] )
				begin
					if( !instructionBuffer_FLAG[0] )
					begin
						oInstructionReadAddr <= 13'b0;
						instructionBuffer_FLAG[0] <= 1'b1;
					end
					else if ( !instructionBuffer_FLAG[1] )
					begin
						instructionType[0] <= iInstructionType;
						instruction[0] <= iInstruction;
						timeUntilNextInstruction <= iTimeUntilNextInstruction;
						oInstructionReadAddr <= ( oInstructionReadAddr + 1'b1 );
						
						instructionBuffer_FLAG[1] <= 1'b1;
					end
					else if ( !instructionBuffer_FLAG[2] )
					begin
						instructionType[1] <= iInstructionType;
						instruction[1] <= iInstruction;
						timingReg[1] <= iTimeUntilNextInstruction;
						oInstructionReadAddr <= ( oInstructionReadAddr + 1'b1 );

						instructionBuffer_FLAG[2] <= 1'b1;
					end
				end
			end
				
		CASE_RUN_PROGRAM:
			begin
				programRunning_FLAG <= 1'b1;
				instructionBuffer_FLAG[0] <= 1'b0;
			end
			
		default:
			begin
				if(otxTransducerOutput) otxTransducerOutput <= 8'b00000000;
				programRunningFlag <= 1'b0;
				instructionBuffer_FLAG <= 3'b0;
			end
	
	endcase
	
	if( programRunningFlag )
	begin
		if ( timeUntilNextInstruction )
		begin
			timeUntilNextInstruction <= timeUntilNextInstruction-1'b1;
			
		end
		else if ( !waitingForExternalTrigger & !waitingForInterruptToResolve )
		begin
		
			// update instruction cache
			instructionType[1] <= iInstructionType;
			instruction[1] <= iInstruction;
			timingReg[1] <= iTimeUntilNextInstruction;
			
			instructionType[0] <= instructionType[1];
			instruction[0] <= instruction[1];
			timingReg[0] <= timingReg[1];

			if ( instructionType[1][is_loop_start_point] )
			begin
			
				timeUntilNextInstruction <= 32'b0;
				if( !loopActive[loopNum] )
				begin
					loopCounter[loopNum] <= loopCounterRef;
					loopActive[loopNum] <= 1'b1;
				end
				
				oInstructionReadAddr <= ( oInstructionReadAddr + 1'b1 );
				
			end
			else if ( instructionType[1][is_loop_end_point] )
			begin
			
				timeUntilNextInstruction <= 32'b0;
				if( loopCounter[loopNum] <= 1 )
				begin
					oInstructionReadAddr <= ( oInstructionReadAddr + 1'b1 );
					loopActive[loopNum] <= 1'b0;
				end
				else
				begin
					oInstructionReadAddr <= ( instrReadAddr + 1'b1 );
				end
				
			end
			else if ( !instructionType[1][program_end_point] )
			begin
				timeUntilNextInstruction <= timingReg[1];
				oInstructionReadAddr <= ( oInstructionReadAddr + 1'b1 );
			end
			
			
			
			
			
		end
	
	end


end



endmodule


