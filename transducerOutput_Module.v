module transducerOutput_Module (
	input			clk,
	input			rst,
	input			isActive,
	input			onYourMark,
	input			GOGOGO_EXCLAMATION,
	input [8:0]		chargeTime,
	input [15:0]	phaseDelay,
	input [31:0]	fireDelay,
	output			transducerOutput,
	output 			fireComplete
	
);

reg	transducerOutput_reg;
reg	fireComplete_reg;

assign transducerOutput = transducerOutput_reg;
assign fireComplete = fireComplete_reg;

reg		[32:0] 	pd;
reg 	[8:0] 	ct;

reg	[1:0]	state;
reg 		marked_flag;

parameter [1:0] MARK = 2'b01;
parameter [1:0] UNMARKED_GO = 2'b10;
parameter [1:0] GOGOGO = 2'b11;

initial
begin
	transducerOutput_reg = 1'b0;
	fireComplete_reg = 1'b0;
	
	state = 2'b0;
	marked_flag = 1'b0;
	ct = 9'b0;
	pd = 33'b0;
end

always @(posedge clk)
begin

	if( isActive )
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
						if( !marked_flag )
						begin
							pd <= phaseDelay+fireDelay;
							ct <= chargeTime;
							marked_flag <= 1'b1;
						end
						if( transducerOutput_reg ) transducerOutput_reg <= 1'b0;
					end
					
				UNMARKED_GO:
					begin
						if( !marked_flag )
						begin
							pd <= phaseDelay+fireDelay;
							ct <= chargeTime;
							marked_flag <= 1'b1;
						end
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
						if( marked_flag ) marked_flag <= 1'b0;
					end
					
			endcase
			
		end
		else
		begin
			if ( marked_flag ) <= 1'b0;
			if ( state ) state <= 2'b0;
			if ( transducerOutput_reg ) transducerOutput_reg <= 1'b0;
			if( fireComplete_reg ) fireComplete_reg <= 1'b0;
			
		end
		
	end
	else
	begin
		if ( marked_flag ) <= 1'b0;
		if ( state ) state <= 2'b0;
		if ( transducerOutput_reg ) transducerOutput_reg <= 1'b0;
		if( !fireComplete_reg ) fireComplete_reg <= 1'b1;
		
	end
	
end

endmodule	


