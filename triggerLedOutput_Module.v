module triggerLedOutput_Module (
	input			clk,
	input			rst,
	
	input 			restLevel,
	input 			isActive,
	
	input			onYourMark,
	input			GOGOGO_EXCLAMATION,
	
	input [15:0]	duration,
	input [15:0]	delay,

	output reg 		triggerLedOutput,
	output reg 		trigLedComplete,
	
	input			hardStop
);

reg				tlinfdur;
reg				tlinfval;
reg		[14:0] 	tldur;
reg 	[15:0] 	tldel;

reg	[2:0]	state;

parameter [2:0] MARK = 3'b001;
parameter [2:0] UNMARKED_GO = 3'b010;
parameter [2:0] GOGOGO = 3'b011;
parameter [2:0]	DO_NOT_UPDATE = 3'b100;

initial
begin
	state = 3'b0;
	tldur = 15'b0;
	tldel = 16'b0;
	trigLedComplete = 1'b1;
end

always @(posedge clk)
begin

	if( !hardStop )
	begin
		if( !rst )
		begin
			
			if( isActive )
			begin
				if( onYourMark & !GOGOGO_EXCLAMATION & !state[0] )
				begin
					state[0] <= 1'b1;
				end
				
				if( onYourMark & GOGOGO_EXCLAMATION & !state[1] )
				begin
					state[1] <= 1'b1;
					if ( state[2] ) state[2] <= 1'b0;
				end	

			end
			else
			begin
				state <= 3'b100;
			end
			
			case( state )
				MARK:
					begin
						tlinfdur <= duration[15];
						if ( !duration[15] )
						begin
							tldur <= duration[14:0];
						end
						else
						begin
							tldur <= 15'b0;
							tlinfval <= duration[14];
						end
						tldel <= delay;
						if( trigLedComplete ) trigLedComplete <= 1'b0;
					end
					
				UNMARKED_GO:
					begin
						tlinfdur <= duration[15];
						if ( !duration[15] )
						begin
							tldur <= duration[14:0];
						end
						else
						begin
							tldur <= 15'b0;
							tlinfval <= duration[14];
						end
						tldel <= delay;
						if( trigLedComplete ) trigLedComplete <= 1'b0;
						if( onYourMark ) state[0] <= 1'b1;
					end
					
				GOGOGO:
					begin
						if( tldel )
						begin
							tldel <= tldel - 1'b1;
						end
						else if ( !tlinfdur )
						begin
							if ( tldur )
							begin
								if( triggerLedOutput == restLevel ) triggerLedOutput <= ~restLevel;
								tldur <= tldur - 1'b1;
							end
							else if( !trigLedComplete ) 
							begin
								if( triggerLedOutput ^ restLevel ) triggerLedOutput <= restLevel;
								trigLedComplete <= 1'b1;
							end
						end
						else if ( !trigLedComplete )
						begin
							if ( triggerLedOutput ^ tlinfval ) triggerLedOutput <= tlinfval;
							trigLedComplete <= 1'b1;
						end
					end
					
				DO_NOT_UPDATE:
					begin
						if( !trigLedComplete ) trigLedComplete <= 1'b1;
					end
					
				default:
					begin
						if ( !tlinfdur )
						begin
							if ( triggerLedOutput ^ restLevel )  triggerLedOutput <= restLevel;
						end
						else
						begin
							if ( triggerLedOutput ^ tlinfval )  triggerLedOutput <= tlinfval;
						end	
					end
					
			endcase
			
		end
		else 
		begin
			if( !trigLedComplete ) trigLedComplete <= 1'b1;
			
			if( isActive )
			begin
				if ( !tlinfdur )
				begin
					if ( triggerLedOutput ^ restLevel )  triggerLedOutput <= restLevel;
					state <= 3'b0;
				end
				else
				begin
					if ( triggerLedOutput ^ tlinfval )  triggerLedOutput <= tlinfval;
					state <= 3'b100;
				end		
			end
			else
			begin
				state <= 3'b100;
			end
		end
		
	end
	else
	begin
		if ( !trigLedComplete ) trigLedComplete <= 1'b1;
		if ( triggerLedOutput ^ restLevel )  triggerLedOutput <= restLevel;
		state <= 3'b0;
		tldur = 15'b0;
		tldel = 16'b0;
	end
	
end

endmodule	


