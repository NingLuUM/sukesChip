module transducerOutput_Module (
	input			clk,
	input			rst,
	input			mask,
	input			onYourMark,
	input			GOGOGO_EXCLAMATION,
	input [8:0]		chargeTime,
	input [15:0]	phaseDelay,
	output reg		transducerOutput,
	output reg		fireComplete,
	output reg		warning
);

reg		[15:0] 	pd;
reg 	[8:0] 	ct;
reg		[9:0]	dangerCntr;
wire	dangerValve;
assign dangerValve = dangerCntr[9];

reg	[1:0]	state;

parameter [1:0] MARK = 2'b01;
parameter [1:0] UNMARKED_GO = 2'b10;
parameter [1:0] GOGOGO = 2'b11;

initial
begin
	state = 2'b0;
	warning = 1'b0;
	ct = 9'b0;
	pd = 16'b0;
	dangerCntr = 10'b0;
end

always @(posedge clk)
begin

	if( !rst & !dangerValve )
	begin
	
		if( transducerOuput )
		begin
			dangerCntr <= dangerCntr + 1'b1;
		end
		else
		begin
			if( dangerCntr ) dangerCntr <= 10'b0;
		end
		
		if( onYourMark & !GOGOGO_EXCLAMATION & !state[0] )
		begin
			state[0] <= 1'b1;
		end
		
		if( onYourMark & GOGOGO_EXCLAMATION & !state[1] )
		begin
			state[1] <= 1'b1;
		end
		
		case( state )
			MARK:
				begin
					pd <= phaseDelay;
					ct <= chargeTime;
					if( fireComplete ) fireComplete <= 1'b0;
					if( transducerOutput ) transducerOutput <= 1'b0;
				end
				
			UNMARKED_GO:
				begin
					pd <= phaseDelay;
					ct <= chargeTime;
					if( fireComplete ) fireComplete <= 1'b0;
					if( transducerOutput ) transducerOutput <= 1'b0;
					if( onYourMark ) state[0] <= 1'b1;
				end
				
			GOGOGO:
				begin
					if( pd )
					begin
						pd <= pd - 1'b1;
					end
					else if ( ct )
					begin
						if( !transducerOutput & !mask ) transducerOutput <= 1'b1;
						ct <= ct - 1'b1;
					end
					else
					begin
						if( transducerOutput ) transducerOutput <= 1'b0;
						if( !fireComplete ) fireComplete <= 1'b1;
					end
				end
				
			default:
				begin
					if( transducerOutput ) transducerOutput <= 1'b0;
					if( !fireComplete ) fireComplete <= 1'b1;
				end
				
		endcase
		
	end
	else
	begin
	
		if( rst )
		begin
			state <= 2'b0;
			if( dangerCntr ) dangerCntr <= 10'b0;
			if( warning ) warning <= 1'b0;
			if( !fireComplete ) fireComplete <= 1'b1;
		end
		else if( !warning )
		begin
			warning <= 1'b1;
			fireComplete <= 1'b0;
		end

		if( transducerOutput ) transducerOutput <= 1'b0;
		
	end
	
end

endmodule	


