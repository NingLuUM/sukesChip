module txControlModule(
	txCLK,
	ocramCLK,
	iSystemTrig,
	itxControlComms,
	itxUserChannelMask,
	itxDisableTransducerSafetyControls,
	
	itxOutputControlReg,
	itxTimingReg,
	itxLoopAddressReg,
	itxLoopCounterReg,
	itxADCTriggerAck,
	
	iArmInterruptComms,
	iArmNextTimeout,
	iProgramletteStartAddr,
	
	otxADCTriggerLine,
	otxLoopReadAddr,
	otxReadAddr,
	otxPinOutput,
	oleds,
	oArmInterrupt,
	state
);
	
// inputs
input txCLK, ocramCLK, iSystemTrig;

input			[7:0]		itxControlComms;
input			[31:0]		itxOutputControlReg;
input			[31:0]		itxTimingReg;
input			[31:0]		itxLoopAddressReg;
input			[31:0]		itxLoopCounterReg;
input			[7:0]		itxUserChannelMask;
input			[7:0]		itxDisableTransducerSafetyControls;
input						itxADCTriggerAck;

input			[7:0]		iArmInterruptComms;
input			[31:0]		iArmNextTimeout;
input			[14:0]		iProgramletteStartAddr;


output reg					otxADCTriggerLine;
output reg	[3:0]			otxLoopReadAddr;
output reg 	[15:0]		otxReadAddr;
output reg	[15:0]		otxPinOutput;
output reg	[7:0]			oleds;
output reg	[3:0]			state;
output reg					oArmInterrupt;


// no multiline comment folding!?
// description of states found at bottom of file

parameter txFinishedBit 					= 31;
parameter txArmInterruptBit				= 30;
parameter txWaitForExternalTrigBit 		= 29;
parameter txIsLoopEndPointBit 			= 28;
parameter txBeginDataAcqBit				= 25;
parameter txIsActiveBit						= 24;


wor txFinished;
assign txFinished 					= itxOutputControlReg[txFinishedBit];
assign txFinished						= outputControlRegBuffer[txFinishedBit];

wire txArmInterrupt, txWaitForExternalTrig;
wire txIsLoopEndPoint;
wire txBeginDataAcq;

assign txArmInterrupt 				= itxOutputControlReg[txArmInterruptBit];
assign txWaitForExternalTrig 		= itxOutputControlReg[txWaitForExternalTrigBit];
assign txIsLoopEndPoint				= itxOutputControlReg[txIsLoopEndPointBit];
assign txBeginDataAcq				= itxOutputControlReg[txBeginDataAcqBit];

reg [31:0] outputControlRegBuffer;
reg [31:0] timingRegBuffer;

reg [7:0][8:0]		transducerSafetyTimer;
reg [7:0]			transducerChargeTimeSafetyMask;

wire armAwake, releasedByArm;
assign armAwake 		= iArmInterruptComms[7];
assign releasedByArm	= iArmInterruptComms[0];

wand [7:0] transducerOutputMask;
assign transducerOutputMask = transducerChargeTimeSafetyMask[7:0];
assign transducerOutputMask = itxUserChannelMask[7:0];

wire [7:0] ledOutput;
wire [7:0] triggerOutput;
assign ledOutput 			= outputControlRegBuffer[15:8];
assign triggerOutput 	= outputControlRegBuffer[23:16];

parameter maxLoopAddr = 15;
reg [maxLoopAddr:0][15:0]	loopStartAddr;
reg [maxLoopAddr:0][15:0]	loopEndAddr;
reg [maxLoopAddr:0][15:0]	loopCounter;
reg [maxLoopAddr:0][15:0]	loopCounterRef;
reg [maxLoopAddr:0]			currentlyInLoop;
reg [3:0]						currentLoopAddr;
reg loopsInitialized;

reg [31:0] 			armNextTimeout;
reg [63:0]  		tx_clk_cntr;
reg [63:0]			absTimeClock;
reg [63:0]			timeOfNext;

parameter [8:0] transducerSafetyTimeout = 9'b1111111111;
parameter [31:0] masterCommandOnlyMask = ( 8'b11111111 << 24 );

reg commFlag = 1'b1;
reg absTimeResetFlag = 1'b0;
reg inWaitState = 1'b0;

// error code: led display
reg [7:0] errorState0 = 8'b0;
reg [7:0] errorState1 = 8'b0;
parameter loopEndAddr_isLoopEndPoint_Mismatch = 8'b00000001;


// cases
parameter [3:0] initState = 0;
parameter [3:0] waitForExternalTrigger = 1;
parameter [3:0] txRunning = 2;
parameter [3:0] interruptRequest_DMA = 3;
parameter [3:0] armInterruptRequest = 4;
parameter [3:0] checkMessagesFromArm = 5;


parameter [3:0] errorDetected = 14;
parameter [3:0] programDone = 15;

always @(posedge txCLK)
begin
	if ( !itxControlComms ) begin
		otxReadAddr <= 16'b0;
		transducerSafetyTimer[0] <= transducerSafetyTimeout;
		transducerSafetyTimer[1] <= transducerSafetyTimeout;
		transducerSafetyTimer[2] <= transducerSafetyTimeout;
		transducerSafetyTimer[3] <= transducerSafetyTimeout;
		transducerSafetyTimer[4] <= transducerSafetyTimeout;
		transducerSafetyTimer[5] <= transducerSafetyTimeout;
		transducerSafetyTimer[6] <= transducerSafetyTimeout;
		transducerSafetyTimer[7] <= transducerSafetyTimeout;
		
		oArmInterrupt <= 1'b0;
		commFlag <= 1'b1;
		
		loopsInitialized <= 1'b0;
		currentLoopAddr <= 4'b0;
		absTimeResetFlag <= 1'b0;
		
		tx_clk_cntr <= 64'b0;
		absTimeClock <= 64'b0;
		otxADCTriggerLine <= 1'b0;
		
		state <= initState;
		
	end
	else if ( !itxControlComms[7:4] ) 
	begin
		otxPinOutput <= 16'b0;
		if ( commFlag ) 
		begin
			oleds <= itxControlComms;
		end
	end
	
	else if ( !itxControlComms[7:5] & itxControlComms[4] & !itxControlComms[3:0] & commFlag ) 
	begin
		otxReadAddr <= 15'b0;
	end
	else if ( itxControlComms[7] & commFlag ) 
	begin
		if ( !loopsInitialized ) 
		begin
			if ( currentLoopAddr <= maxLoopAddr ) 
			begin
				loopStartAddr[currentLoopAddr]	<= itxLoopAddressReg[15:0];
				loopEndAddr[currentLoopAddr]		<= itxLoopAddressReg[31:16];
				loopCounter[currentLoopAddr]		<= itxLoopCounterReg[15:0];
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
					tx_clk_cntr <= 64'b0;
				end
			end
		end 	
		else 
		begin
			commFlag <= 1'b0;
			otxPinOutput <= 16'b0;	
			otxLoopReadAddr <= 4'b0;
			currentLoopAddr <= 4'b0;
			
			tx_clk_cntr <= 64'b0;
			if ( state != errorDetected ) 
			begin
				outputControlRegBuffer <= itxOutputControlReg;
				timingRegBuffer <= itxTimingReg;
				`include "./headers/tx_updateReadAddr.v";
				inWaitState <= 1'b1;
				state <= waitForExternalTrigger;
			end
		end	
	end	
	
	if ( itxControlComms[7] & !commFlag )
	begin	
		if ( !inWaitState && ( state < errorDetected ) ) 
		begin
			absTimeClock <= ( absTimeResetFlag ) ? 64'b0 : ( absTimeClock + 1'b1 );
			timeOfNext   <= ( absTimeResetFlag ) ? ( timingRegBuffer  + 1'b1 ) : timeOfNext;
			
			`include "./headers/tx_transducerOutputControl.v"
			if ( tx_clk_cntr[31:0] < timingRegBuffer ) 
			begin
				tx_clk_cntr <= tx_clk_cntr + 1'b1;
			end 
			
			if ( itxADCTriggerAck ) 
			begin
				otxADCTriggerLine <= 1'b0;
			end
			
			if ( tx_clk_cntr[31:0] == 0 ) 
			begin
				transducerSafetyTimer[0] <= ( ( otxPinOutput[0] ^ outputControlRegBuffer[0] ) & transducerChargeTimeSafetyMask[0] ) ? 9'b0 : transducerSafetyTimer[0];
				transducerSafetyTimer[1] <= ( ( otxPinOutput[1] ^ outputControlRegBuffer[1] ) & transducerChargeTimeSafetyMask[1] ) ? 9'b0 : transducerSafetyTimer[1];
				transducerSafetyTimer[2] <= ( ( otxPinOutput[2] ^ outputControlRegBuffer[2] ) & transducerChargeTimeSafetyMask[2] ) ? 9'b0 : transducerSafetyTimer[2];
				transducerSafetyTimer[3] <= ( ( otxPinOutput[3] ^ outputControlRegBuffer[3] ) & transducerChargeTimeSafetyMask[3] ) ? 9'b0 : transducerSafetyTimer[3];
				transducerSafetyTimer[4] <= ( ( otxPinOutput[4] ^ outputControlRegBuffer[4] ) & transducerChargeTimeSafetyMask[4] ) ? 9'b0 : transducerSafetyTimer[4];
				transducerSafetyTimer[5] <= ( ( otxPinOutput[5] ^ outputControlRegBuffer[5] ) & transducerChargeTimeSafetyMask[5] ) ? 9'b0 : transducerSafetyTimer[5];
				transducerSafetyTimer[6] <= ( ( otxPinOutput[6] ^ outputControlRegBuffer[6] ) & transducerChargeTimeSafetyMask[6] ) ? 9'b0 : transducerSafetyTimer[6];
				transducerSafetyTimer[7] <= ( ( otxPinOutput[7] ^ outputControlRegBuffer[7] ) & transducerChargeTimeSafetyMask[7] ) ? 9'b0 : transducerSafetyTimer[7];
				otxPinOutput[7:0] <= ( outputControlRegBuffer[7:0] & transducerOutputMask );
				otxPinOutput[15:8] <= outputControlRegBuffer[15:8];	
			end
			
			if ( tx_clk_cntr[31:0] == timingRegBuffer ) 
			begin
				tx_clk_cntr <= 64'b0;
				
				outputControlRegBuffer <= itxOutputControlReg;
				timingRegBuffer <= itxTimingReg;
				timeOfNext  <= ( timeOfNext + itxTimingReg  + 1'b1 );
				`include "./headers/tx_updateReadAddr.v";
				
				if ( txBeginDataAcq ) 
				begin
					otxADCTriggerLine <= 1'b1;
				end
				
				if ( txFinished )
				begin
					state <= programDone;
				end
				else if ( txWaitForExternalTrig & !txArmInterrupt ) 
				begin
					inWaitState <= 1'b1;
					state <= waitForExternalTrigger;
				end 
				else if ( txArmInterrupt )
				begin
					inWaitState <= 1'b1;
					state <= armInterruptRequest;
				end
			end
		end
		
		case (state)
			initState:
				begin
					errorState0 <= 8'b0;
				end
				
			waitForExternalTrigger:
				begin
					oleds <= waitForExternalTrigger;
					if ( !itxControlComms[1] ) 
					begin
						absTimeResetFlag <= 1'b1;
					end 
					else 
					begin
						absTimeResetFlag <= 1'b0;
						inWaitState <= 1'b0;
						tx_clk_cntr <= 64'b0;
						state <= txRunning;
					end
				end
					
			txRunning:
				begin
					oleds <= txRunning;
					errorState0 <= 8'b0;
				end
				
			armInterruptRequest:
				begin
					oleds <= armInterruptRequest;
					tx_clk_cntr <= ( tx_clk_cntr + 1'b1 );
					if ( ( tx_clk_cntr[63:32] ) ) 
					begin
						errorState0 <= 8'b0;
						errorState1 <= 8'b00000011;
						state <= errorDetected;
						
					end 
					else if ( !armAwake && ( !tx_clk_cntr[63:32] ) ) 
					begin
						oArmInterrupt <= 1'b1;
								
					end 
					else if ( armAwake && ( tx_clk_cntr[63:32] ) && !releasedByArm ) 
					begin
						oArmInterrupt <= 1'b0;
							
					end 
					else if ( armAwake && ( tx_clk_cntr[63:32] ) && releasedByArm ) 
					begin
						state <= checkMessagesFromArm;
					end
				end
				
			checkMessagesFromArm:
				begin
					oleds <= checkMessagesFromArm;
					tx_clk_cntr <= ( tx_clk_cntr + 1'b1 );
					if ( tx_clk_cntr[31:0] == iArmNextTimeout ) 
					begin
						absTimeResetFlag <= 1'b0;
						inWaitState <= 1'b0;
						
						outputControlRegBuffer <= itxOutputControlReg;
						timingRegBuffer <= itxTimingReg;
						
						tx_clk_cntr <= 64'b0;
						otxReadAddr <= iProgramletteStartAddr;
						state <= txRunning;
					end
				
				end
				
			errorDetected:
				begin
					otxPinOutput <= 16'b0;
					if( tx_clk_cntr < 20000000 ) 
					begin
						oleds <= errorState0;
						tx_clk_cntr <= ( tx_clk_cntr + 1'b1 );
					end 
					else if ( (tx_clk_cntr >= 20000000) && (tx_clk_cntr <= 40000000) ) 
					begin
						oleds <= errorState1;
						tx_clk_cntr <= ( tx_clk_cntr + 1'b1 );
					end 
					else 
					begin
						tx_clk_cntr <= 64'b0;
					end
				end
				
			programDone:
				begin
					otxPinOutput <= 16'b0;
					if( tx_clk_cntr < 50000000 ) 
					begin
						oleds <= 8'b00000000;
						tx_clk_cntr <= ( tx_clk_cntr + 1'b1 );
					end 
					else if ( (tx_clk_cntr >= 50000000) && (tx_clk_cntr < 100000000) ) 
					begin
						oleds <= 8'b11111111;
						tx_clk_cntr <= ( tx_clk_cntr + 1'b1 );
					end 
					else 
					begin
						tx_clk_cntr <= 64'b0;
					end	
				end
			
			default:
				begin
					otxPinOutput <= 16'b0;
					state <= programDone;
				end
				
		endcase
	end
end


endmodule

/* description of bits [31:24] otxOutputControlReg
	these bits encode commands govering output
	
	transducer safety timeout applies in all states where pin outputs aren't explicitly disabled
	
	
	txFinished: evaluates immediately
		- disables all outputs
		- goes to programDone state
	
	
	(what this will be used for currently up in the air)
	(right now it'll basically just wait ~40seconds and put)
	(the system in the errorDetected state)
	txArmInterrupt: evaluates immediately
		- leaves outputs in current state
		- goes to armInterrupt state
		- waits for explicit input from user before being released
	
	
	(seems to work as intended, not fully tested)
	txWaitForExternalTrigger: evaluates immediately
		- leaves outputs in current state
		- disables the timer
		- loads the pinoutput and timing registers 
			of the next timepoint into buffer
		- upon receiving input trigger:
			- re-enables timer
			- resumes program starting with values
				stored in buffer
	
	
	(works... behavior is undefined if multiple loops share same end point)
	(will probably loop infinitely though)
	txIsLoopEndPoint: evaluates at end of current timer
		- resets the current address (otxReadAddr)
			to the corresponding start address stored
			in the loop address register
		- decrements the value stored in loop counter register
		- if loop counter <= 1:
			- loop counter is reset to the reference value
			- otxReadAddr is incremented by 1
	
	
	(DMA controller not implemented, so neither is this.)
	(but this is what it should do...)
	txMakeDMARequest: evaluates immediately
		- sets the otxMakeDMARequest which signals the
			hps to load the next set of outputs to the
			DMA registers
	
	
	(partly implemented for testing while waiting on DMA controller)
	(this is what it should do...)
	txIsDMAStartPoint: evaluates immediately
		- sets the location from which pinoutputs/timing
			values are loaded to be address0 of the DMA registers
		- the value stored in the itxTimingReg when txIsDMAStartPoint
			is set should correspond to the total duration which the 
			outputs are controlled from the DMA registers
			- when the duration stored in itxTimingReg has elapsed
				the program resumes normal operation from the next 
				address after txIsDMAStartPoint was set in the 
				otxOutputControl and otxTiming registers, whether or 
				not the end point within the DMA programlette has been 
				reached
		- from within the DMA registers, the txIsDMAStartPoint bit
			is used to signify the end of the DMA programlette

	(not implemented yet, but is just a flag...)
	txBeginDataAcq: evaluates immediately
		- sets a flag to tell the comm module to begin storing data

	txIsActive: just a placeholder for now
*/
