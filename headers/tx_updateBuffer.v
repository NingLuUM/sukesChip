// all 'begin' statements have an 'end' statement
if ( !overflowTime ) begin

	if ( !buffering && itxOutputControlReg && ( itxOutputControlReg == lastOutputControlRegLoadedToBuffer ) && ( itxOutputControlReg == lastOutputControlRegLoadedToBuffer ) ) 
	begin
		state <= updateReadAddr;
		buffering <= 1'b1;	
	end
	 	
	else if ( buffering && ( currentBufferDepth < maxBufferAddr ) )
	begin
	
		if ( ( itxOutputControlReg ^ lastOutputControlRegLoadedToBuffer ) || ( itxTimingReg ^ lastTimingRegLoadedToBuffer ) )
		begin
			
			if ( tx_clk_cntr ^ activeTimingReg ) 
			begin
				endInBuffer <= ( txFinishedMaster || !itxOutputControlReg ) ? 1'b1 : endInBuffer;
				if ( itxOutputControlReg ) begin
					outputControlRegBuffer[ currentBufferDepth + 1'b1 ] <= itxOutputControlReg;
					timingRegBuffer[ currentBufferDepth + 1'b1 ] <= itxTimingReg;
					currentBufferDepth <= ( currentBufferDepth + 1'b1 );
					lastOutputControlRegLoadedToBuffer <= itxOutputControlReg;
					lastTimingRegLoadedToBuffer <= itxTimingReg;
				end else begin
					outputControlRegBuffer[currentBufferDepth] <= (1'b1 << 31);
					timingRegBuffer[currentBufferDepth] <= 0;
					lastOutputControlRegLoadedToBuffer <= (1'b1 << 31);
					lastTimingRegLoadedToBuffer <= 0;
				end
			end	
			state <= updateReadAddr;
		end
		end
		
	else if (  buffering && ( currentBufferDepth == maxBufferAddr ) || endInBuffer ) 
		begin
			buffering <= 1'b0;
		end


end else begin

	if ( overflowTime > 10 )
	begin
		errorState0 = 8'b10101010;
		errorState1 = 8'b01010101;
		state <= errorDetected;
	end
	else if ( ( itxOutputControlReg ^ lastOutputControlRegLoadedToBuffer ) || ( itxTimingReg ^ lastTimingRegLoadedToBuffer ) ) 
	begin
		if ( txFinishedMaster || !itxOutputControlReg ) begin
			state <= programDone;
		end
		else 
		begin
			buffering <= 1'b1;
			lastOutputControlRegLoadedToBuffer <= itxOutputControlReg;
			lastTimingRegLoadedToBuffer <= itxTimingReg;
			
			if ( ( ( timeOfNext + itxTimingReg + 1'b1 ) >= absTimeClock )  ) 
				begin
					outputControlRegBuffer[currentBufferDepth] <= itxOutputControlReg;
					timingRegBuffer[currentBufferDepth] <= itxTimingReg;
					currentBufferDepth <= ( currentBufferDepth + 1'b1 );
					tx_clk_cntr <= ( absTimeClock - ( timeOfNext + itxTimingReg + 1'b1 ) );
					
				end 
			else if ( txFinishedMaster || txArmProcessorHookMaster || txWaitForExternalTrigMaster ) 
				begin
					outputControlRegBuffer[0] <= ( itxOutputControlReg & masterCommandOnlyMask );
				end
			timeOfNext <= ( timeOfNext + itxTimingReg + 1'b1 );
			state <= updateReadAddr;
		end
	end
end

