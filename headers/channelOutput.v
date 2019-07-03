module channelOuput (
	clk,
	phaseDelay,
	chargeTime,
	txOutputState,
	txMask,
	cmd,
	isActive
);

input 			clk;
input [15:0]	phaseDelay;
input [8:0]		chargeTime;
output reg 		txOutputState = 1'b0;
input 			txMask;
input [1:0]		cmd;
output reg 		isActive = 1'b0;

reg [30:0] pd = 31'b0;
reg [8:0] ct = 9'b0;
	
reg cmdState = 1'b0;

parameter [1:0] buffer_phase_charge = 2'b01;
parameter [1:0] fire_pulse = 2'b10;
parameter [1:0] reset_module = 2'b11;

always @(posedge clk)
begin
	
	case( cmd )
		buffer_phase_charge:
			begin
				if ( !isActive )
				begin
					pd <= phaseDelay;
					ct <= chargeTime;
					cmdState <= 1'b0;
				end
				else
				begin
					if ( txOutputState )
					begin
						txOutputState <= 1'b0;
					end	
				end
			end
			
		fire_pulse:
			begin		
				if ( !cmdState & !isActive )
				begin
					cmdState <= 1'b1;	
					if ( !ct )
					begin
						isActive <= 1'b0;
						txOutputState <= 1'b0;
					end
					else
					begin
						isActive <= 1'b1;
						if( pd )
						begin
							pd <= ( pd - 1'b1 );
						end
						else
						begin
							txOutputState <= txMask;
							ct <= ( ct - 1'b1 );
						end
					end
				end
				else if ( cmdState & isActive )
				begin
					if ( !ct )
					begin
						isActive <= 1'b0;
						txOutputState <= 1'b0;
					end
					else
					begin
						if ( pd )
						begin
							pd <= ( pd - 1'b1 );
						end
						else 
						begin
							ct <= ( ct - 1'b1 );
							if( !txOutputState & txMask ) txOutputState <= txMask;	
						end
					end	
				end
				else
				begin
					if ( txOutputState )
					begin
						txOutputState <= 1'b0;
					end	
				end
			end
			
		reset_module:
			begin
				if ( txOutputState ) txOutputState <= 1'b0;
				if ( pd ) pd <= 31'b0;
				if ( ct ) ct <= 9'b0;
				if ( isActive ) isActive <= 1'b0;
				if ( cmdState ) cmdState <= 1'b0;
			end
			
		default:
			begin
			end
		
	endcase
	
end

endmodule	


