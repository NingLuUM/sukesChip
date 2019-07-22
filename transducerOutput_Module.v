module transducerOutput_Module (
	clk,
	cntr,
	phaseDelay,
	fireAtPhaseDelay,
	fireSwitch,
	chargeTime,
	txOutputState,
	cmd,
	isActive,
	errorFlag
);

input 				clk;
input 		[31:0]	cntr;
input 		[15:0]	phaseDelay;
input 		[15:0]	fireAtPhaseDelay;
input				fireSwitch;
input		[8:0]	chargeTime;
output reg 			txOutputState = 1'b0;
input 		[1:0]	cmd;
output reg 			isActive = 1'b0;
output reg			errorFlag = 1'b0;

reg		[15:0] 	pd;
reg 	[8:0] 	ct;

reg cmdState = 1'b0;
reg [9:0] txSafetyValve = 10'b0;

parameter [1:0] wait_cmd = 2'b00;
parameter [1:0] fire_pulse = 2'b01;
parameter [1:0] reset_module = 2'b10;

always @(posedge clk)
begin
	if ( txOutputState )
	begin
		txSafetyValve <= txSafetyValve + 1'b1;
		if ( txSafetyValve[9] )
		begin
			txOutputState <= 1'b0;
			txSafetyValve <= 10'b0;
			errorFlag <= 1'b1;
		end
	end
	
	case( cmd )
		wait_cmd:
			begin
				if ( txOutputState ) txOutputState <= 1'b0;
				pd <= 16'b0;
				ct <= 9'b0;
				isActive <= 1'b0;
				cmdState <= 1'b0;
				txSafetyValve <= 10'b0;
			end
			
		fire_pulse:
			begin		
				if ( !cmdState & !isActive )
				begin
					cmdState <= 1'b1;
					if(fireSwitch)
					begin
						pd <= phaseDelay;
					end
					else
					begin
						pd <= fireAtPhaseDelay;
					end
					ct <= chargeTime;
					if ( !chargeTime )
					begin
						isActive <= 1'b0;
						txOutputState <= 1'b0;
						txSafetyValve <= 10'b0;
					end
					else
					begin
						isActive <= 1'b1;
						if( !phaseDelay ) txOutputState <= 1'b1;
					end
				end
				else if ( cmdState & isActive )
				begin
					if ( cntr == pd )
					begin
						txOutputState <= 1'b1;
					end
					else if ( cntr >= ( pd + ct ) )
					begin
						isActive <= 1'b0;
						if ( txOutputState ) 
						begin
							txOutputState <= 1'b0;
							txSafetyValve <= 10'b0;
						end
					end
				end
				else if ( txOutputState ) 
				begin
					txOutputState <= 1'b0;
					txSafetyValve <= 10'b0;
				end
			end
			
		reset_module:
			begin
				if ( txOutputState ) txOutputState <= 1'b0;
				pd <= 16'b0;
				ct <= 9'b0;
				isActive <= 1'b0;
				cmdState <= 1'b0;
				txSafetyValve <= 10'b0;
				errorFlag <= 1'b0;
			end
			
		default:
			begin
				if ( txOutputState ) txOutputState <= 1'b0;
				pd <= 16'b0;
				ct <= 9'b0;
				isActive <= 1'b0;
				cmdState <= 1'b0;
				txSafetyValve <= 10'b0;
				errorFlag <= 1'b0;
			end
		
	endcase
	
end

endmodule	


