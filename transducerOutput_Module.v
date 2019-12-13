module transducerOutput_Module (
	input			clk,
	input			rst,
	input			isActive,
	input			onYourMark,
	input			GOGOGO_EXCLAMATION,
	input [8:0]		chargeTime,
	input [15:0]	phaseDelay,
	output			transducerOutput,
	output 			fireComplete,
	output 			warning
);

reg	transducerOutput_reg;
reg	fireComplete_reg;
reg	warning_reg;

assign transducerOutput = transducerOutput_reg;
assign fireComplete = fireComplete_reg;
assign warning = warning_reg;

reg		[15:0] 	pd;
reg 	[8:0] 	ct;
reg		[9:0]	dangerCntr;

reg	[1:0]	state;

parameter [1:0] MARK = 2'b01;
parameter [1:0] UNMARKED_GO = 2'b10;
parameter [1:0] GOGOGO = 2'b11;

initial
begin
	transducerOutput_reg = 1'b0;
	fireComplete_reg = 1'b0;
	warning_reg = 1'b0;
	
	state = 2'b0;
	ct = 9'b0;
	pd = 16'b0;
	dangerCntr = 10'b0;
end

always @(posedge clk)
begin

	if( !rst & !dangerCntr[9] & isActive )
	begin
	
		if( transducerOutput_reg )
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
					if( transducerOutput_reg ) transducerOutput_reg <= 1'b0;
				end
				
			UNMARKED_GO:
				begin
					pd <= phaseDelay;
					ct <= chargeTime;
					if( transducerOutput_reg ) transducerOutput_reg <= 1'b0;
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
						if( !transducerOutput_reg ) transducerOutput_reg <= 1'b1;
						ct <= ct - 1'b1;
					end
					else
					begin
						if( transducerOutput_reg ) transducerOutput_reg <= 1'b0;
						if( !fireComplete_reg ) fireComplete_reg <= 1'b1;
					end
				end
				
			default:
				begin
					if( transducerOutput_reg ) transducerOutput_reg <= 1'b0;
					if( fireComplete_reg ) fireComplete_reg <= 1'b0;
				end
				
		endcase
		
	end
	else
	begin
	
		if ( state ) state <= 2'b0;
		if ( transducerOutput_reg ) transducerOutput_reg <= 1'b0;
		
		if ( dangerCntr[9] )
		begin
			warning_reg <= 1'b1;
			fireComplete_reg <= 1'b1;
		end	
		else if ( rst | !isActive )
		begin
			if( dangerCntr ) dangerCntr <= 10'b0;
			if( warning_reg ) warning_reg <= 1'b0;
			if( fireComplete_reg ) fireComplete_reg <= 1'b0;
		end
	end
	
end

endmodule	


