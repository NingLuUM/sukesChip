module ioRecvSysTrigger_Module (
	input			clk,
	input			rst,
	
	input			onYourMark,
	input			GOGOGO_EXCLAMATION,
	
	input [31:0]	delay,

	output			outputTrig,
	input 			adcAckLine,
	
	output			outputComplete

);

reg outputTrig_reg;
assign outputTrig = outputTrig_reg;

reg outputComplete_reg;
assign outputComplete = outputComplete_reg;

reg [31:0] 	rcv_delay;
reg	[2:0]	state;

parameter [2:0] IDLE = 3'b000;
parameter [2:0] MARK = 3'b001;
parameter [2:0] UNMARKED_GO = 3'b010;
parameter [2:0] GOGOGO = 3'b011;
parameter [2:0] WAIT_FOR_ACK = 3'b100;
parameter [2:0] RECV_COMPLETE = 3'b101;

initial
begin
	state = 3'b0;
	rcv_delay = 32'b0;
	outputTrig_reg = 1'b0;
	outputComplete_reg = 1'b0;
end

always @(posedge clk)
begin

	if( !rst )
	begin
		
		if( !state[2] )
		begin
			if( onYourMark & !GOGOGO_EXCLAMATION & !state[0] )
			begin
				state[0] <= 1'b1;
			end
			
			if( onYourMark & GOGOGO_EXCLAMATION & !state[1] )
			begin
				state[1] <= 1'b1;
			end	
		end

		case( state )
			IDLE:
				begin
					rcv_delay <= 32'b0;
					if( outputComplete_reg ) <= 1'b0;
					if( outputTrig_reg ) outputTrig_reg <= 1'b0;
				end
				
			MARK:
				begin
					rcv_delay <= delay;
					if( outputTrig_reg ) outputTrig_reg <= 1'b0;
				end
				
			UNMARKED_GO:
				begin
					rcv_delay <= delay;
					if( outputTrig_reg ) outputTrig_reg <= 1'b0;
					if( onYourMark ) state[0] <= 1'b1;
				end
				
			GOGOGO:
				begin
					if( rcv_delay )
					begin
						rcv_delay <= rcv_delay - 1'b1;
					end
					else 
					begin
						outputTrig_reg <= 1'b1;
						state <= WAIT_FOR_ACK;
					end
				end
				
			WAIT_FOR_ACK:
				begin
					if( adcAckLine )
					begin
						outputTrig_reg <= 1'b0;
						state <= RECV_COMPLETE;
					end
				end
				
			RECV_COMPLETE:
				begin
					if( !outputComplete_reg | outputTrig_reg )
					begin
						outputTrig_reg <= 1'b0;
						outputComplete_reg <= 1'b1;
					end
				end

			default:
				begin
					state <= RECV_COMPLETE;
				end
				
		endcase
		
	end
	else 
	begin
		state <= IDLE;
		if( !outputComplete_reg ) outputComplete_reg <= 1'b1;
		if( outputTrig_reg ) outputTrig_reg <= 1'b0;
		
	end

	
end

endmodule	


