module triggerLedOutput_Module (
	input			clk,
	input			rst,
	
	input 			restLevel,
	
	input			onYourMark,
	input			GOGOGO_EXCLAMATION,
	
	input [10:0]	duration,
	input [20:0]	delay,

	output reg 		triggerLedOutput,
	output reg 		trigLedComplete,
	
	input			hardStop
);

reg		[10:0] 	tldur;
reg 	[20:0] 	tldel;

reg	[1:0]	state;

parameter [1:0] MARK = 2'b01;
parameter [1:0] UNMARKED_GO = 2'b10;
parameter [1:0] GOGOGO = 2'b11;

initial
begin
	state = 2'b0;
	tldur = 11'b0;
	tldel = 21'b0;
	trigLedComplete = 1'b1;
end

always @(posedge clk)
begin

	if( !hardStop )
	begin
		if( !rst )
		begin
			if( !state )
			begin
				if( trigLedComplete ) trigLedComplete <= 1'b0;
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
							if( triggerLedOutput == restLevel ) triggerLedOutput <= ~restLevel;
							tldur <= tldur - 1'b1;
						end
						else if( !trigLedComplete )
						begin
							triggerLedOutput <= restLevel;
							trigLedComplete <= 1'b1;
						end
					end
					
				default:
					begin
						if( !trigLedComplete ) trigLedComplete <= 1'b1;
						if ( triggerLedOutput ^ restLevel )  triggerLedOutput <= restLevel;
					end
					
			endcase
			
		end
		else 
		begin
			state <= 2'b0;
			if( !trigLedComplete ) trigLedComplete <= 1'b1;		
			if ( triggerLedOutput ^ restLevel )  triggerLedOutput <= restLevel;
		end
		
	end
	else
	begin
		if ( !trigLedComplete ) trigLedComplete <= 1'b1;
		if ( triggerLedOutput ^ restLevel )  triggerLedOutput <= restLevel;
		state <= 2'b0;
		tldur = 11'b0;
		tldel = 21'b0;
	end
	
end

endmodule	


