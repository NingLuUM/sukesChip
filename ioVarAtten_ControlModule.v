module ioVarAtten_ControlModule (
	input			clk,
	input			rst,
	
	input 			restLevel,
	
	input			onYourMark,
	input			GOGOGO_EXCLAMATION,
	
	input [31:0]	duration,
	input [31:0]	delay,

	output			outputState,
	output 			outputComplete,
	
	input			hardStop
);

reg outputState_reg;
assign outputState = outputState_reg;

reg outputComplete_reg;
assign outputComplete = outputComplete_reg;

reg	[31:0] 	tldur;
reg [31:0] 	tldel;

reg	[1:0]	state;

parameter [1:0] MARK = 2'b01;
parameter [1:0] UNMARKED_GO = 2'b10;
parameter [1:0] GOGOGO = 2'b11;

initial
begin
	state = 2'b0;
	tldur = 32'b0;
	tldel = 32'b0;
	outputState_reg = 1'b0;
	outputComplete_reg = 1'b0;
end

always @(posedge clk)
begin

	if( !hardStop )
	begin
		if( !rst )
		begin
			
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
						tldur <= duration;
						tldel <= delay;
					end
					
				UNMARKED_GO:
					begin
						tldur <= duration;
						tldel <= delay;
						if( onYourMark ) state[0] <= 1'b1;
					end
					
				GOGOGO:
					begin
						if( tldel )
						begin
							tldel <= tldel - 1'b1;
						end
						else if ( tldur )
						begin
							if( outputState_reg == restLevel ) outputState_reg <= ~restLevel;
							tldur <= tldur - 1'b1;
						end
						else 
						begin
							if( !outputComplete_reg ) outputComplete_reg <= 1'b1;
							if( outputState_reg ^ restLevel ) outputState_reg <= restLevel;
						end
					end
					
				default:
					begin
						if ( outputComplete_reg ) outputComplete_reg <= 1'b0;
						if ( outputState_reg ^ restLevel )  outputState_reg <= restLevel;
					end
					
			endcase
			
		end
		else 
		begin
			state <= 2'b0;
			if( outputComplete_reg ) outputComplete_reg <= 1'b0;		
			if ( outputState_reg ^ restLevel )  outputState_reg <= restLevel;
		end
		
	end
	else
	begin
		if ( outputComplete_reg ) outputComplete_reg <= 1'b1;
		if ( outputState_reg ^ restLevel )  outputState_reg <= restLevel;
		state <= 2'b0;
		tldur = 32'b0;
		tldel = 32'b0;
	end
	
end

endmodule	


