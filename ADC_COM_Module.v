module ADC_COM_Module(
	frameCLKINP,
	//ocramCLK,
	iADCData,
	iSystemTrig,
	iTxTrigger,
	
	iTrigDelay,
	iRecLength,
	iStateReset,
	
	state,
	oTriggerAck,
	oADCData,
	oWREN,
	oWAddr,
	oTransmitReady
	);
	
// inputs
input frameCLKINP, iSystemTrig,  iStateReset, iTxTrigger;
input [7:0][11:0] iADCData; 
input [31:0] iTrigDelay;
input [12:0] iRecLength;

// outputs
output reg oTransmitReady, oTriggerAck;
output reg [2:0] oWREN;
output reg [2:0][31:0] oADCData;
output reg [12:0] oWAddr;
output reg [1:0]  state;

// regs and wires
reg [1:0] trig_received_flag = 2'b0;
reg write_complete_flag = 1'b0;

reg [12:0] 	waddr_cntr;
reg [31:0] txReadyTimeout = 32'b0;



always @(posedge frameCLKINP)
begin

	if ( !iStateReset )
	begin
		if ( !trig_received_flag && iTxTrigger )
		begin
			oTriggerAck <= 1'b1;
			trig_received_flag <= 2'b11;
			oTransmitReady <= 1'b0;
			waddr_cntr <= 13'b0;
			write_complete_flag <= 1'b0;
			txReadyTimeout <= 32'b0;
		end
		else if ( trig_received_flag[1] && !iTxTrigger )
		begin
			trig_received_flag[1]<=1'b0;
			txReadyTimeout <= txReadyTimeout + 1'b1;
		end
		else if ( !trig_received_flag && !iTxTrigger && txReadyTimeout[31] && oTransmitReady )
		begin
			oWREN[2:0] <= 3'b000;
			oTransmitReady <= 1'b0;	
			trig_received_flag <= 2'b00;
			write_complete_flag <= 1'b0;
			waddr_cntr <= 13'b0;
		end
		else
		begin
			txReadyTimeout <= txReadyTimeout + 1'b1;
		end
	
		
		if ( trig_received_flag )
		begin
			if ( !write_complete_flag )
			begin
				if ( oTriggerAck ) oTriggerAck <= 1'b0;
				if ( !oWREN ) oWREN[2:0] <= 3'b111;
				oWAddr <= waddr_cntr;
				
				oADCData[0][11:0] <= iADCData[0]; 
				oADCData[0][23:12] <= iADCData[1];
				oADCData[0][31:24] <= iADCData[6][11:4];
				
				oADCData[1][11:0] <= iADCData[2]; 
				oADCData[1][23:12] <= iADCData[3];
				oADCData[1][27:24] <= iADCData[7][11:8];
				oADCData[1][31:28] <= iADCData[6][3:0];
				
				oADCData[2][11:0] <= iADCData[4]; 
				oADCData[2][23:12] <= iADCData[5];
				oADCData[2][31:24] <= iADCData[7][7:0];
			
				waddr_cntr <= waddr_cntr + 1'b1;
				
				if ( waddr_cntr == iRecLength ) // iRecLength
				begin
					oWREN[2:0] <= 3'b000;
					write_complete_flag <= 1'b1;
					oTransmitReady <= 1'b1;
					trig_received_flag[0] <= 1'b0;
				end
			end
		end
	end
	else // iStateReset == 1
	begin
		oWREN[2:0] <= 3'b000;
		oTransmitReady <= 1'b0;	
		trig_received_flag <= 2'b00;
		write_complete_flag <= 1'b0;
		waddr_cntr <= 13'b0;
	end

end

endmodule

