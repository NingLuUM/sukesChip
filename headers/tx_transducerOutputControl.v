if ( !transducerSafetyTimer[0] && !otxPinOutput[0] ) begin
	transducerChargeTimeSafetyMask[0] <= itxDisableTransducerSafetyControls[0];
	transducerSafetyTimer[0] <= ( transducerSafetyTimer[0]+1'b1 );
end
else if ( transducerSafetyTimer[0] < transducerSafetyTimeout )
begin
	transducerSafetyTimer[0] <= ( transducerSafetyTimer[0]+1'b1 );
end
else if ( transducerSafetyTimer[0] == transducerSafetyTimeout )
begin
	if ( otxPinOutput[0] & !itxDisableTransducerSafetyControls[0] ) begin
		otxPinOutput[0] <= 0;
		transducerChargeTimeSafetyMask[0] <= 0;
		transducerSafetyTimer[0] <= 9'b0;
	end else begin
		transducerChargeTimeSafetyMask[0] <= 1;
	end
end

if ( !transducerSafetyTimer[1] && !otxPinOutput[1] ) begin
	transducerChargeTimeSafetyMask[1] <= itxDisableTransducerSafetyControls[1];
	transducerSafetyTimer[1] <= ( transducerSafetyTimer[1]+1'b1 );
end
else if ( transducerSafetyTimer[1] < transducerSafetyTimeout )
begin
	transducerSafetyTimer[1] <= ( transducerSafetyTimer[1]+1'b1 );
end
else if ( transducerSafetyTimer[1] == transducerSafetyTimeout )
begin
	if ( otxPinOutput[1] & !itxDisableTransducerSafetyControls[1] ) begin
		otxPinOutput[1] <= 0;
		transducerChargeTimeSafetyMask[1] <= 0;
		transducerSafetyTimer[1] <= 9'b0;
	end else begin
		transducerChargeTimeSafetyMask[1] <= 1;
	end
end

if ( !transducerSafetyTimer[2] && !otxPinOutput[2] ) begin
	transducerChargeTimeSafetyMask[2] <= itxDisableTransducerSafetyControls[2];
	transducerSafetyTimer[2] <= ( transducerSafetyTimer[2]+1'b1 );
end
else if ( transducerSafetyTimer[2] < transducerSafetyTimeout )
begin
	transducerSafetyTimer[2] <= ( transducerSafetyTimer[2]+1'b1 );
end
else if ( transducerSafetyTimer[2] == transducerSafetyTimeout )
begin
	if ( otxPinOutput[2] & !itxDisableTransducerSafetyControls[2] ) begin
		otxPinOutput[2] <= 0;
		transducerChargeTimeSafetyMask[2] <= 0;
		transducerSafetyTimer[2] <= 9'b0;
	end else begin
		transducerChargeTimeSafetyMask[2] <= 1;
	end
end

if ( !transducerSafetyTimer[3] && !otxPinOutput[3] ) begin
	transducerChargeTimeSafetyMask[3] <= itxDisableTransducerSafetyControls[3];
	transducerSafetyTimer[3] <= ( transducerSafetyTimer[3]+1'b1 );
end
else if ( transducerSafetyTimer[3] < transducerSafetyTimeout )
begin
	transducerSafetyTimer[3] <= ( transducerSafetyTimer[3]+1'b1 );
end
else if ( transducerSafetyTimer[3] == transducerSafetyTimeout )
begin
	if ( otxPinOutput[3] & !itxDisableTransducerSafetyControls[3] ) begin
		otxPinOutput[3] <= 0;
		transducerChargeTimeSafetyMask[3] <= 0;
		transducerSafetyTimer[3] <= 9'b0;
	end else begin
		transducerChargeTimeSafetyMask[3] <= 1;
	end
end

if ( !transducerSafetyTimer[4] && !otxPinOutput[4] ) begin
	transducerChargeTimeSafetyMask[4] <= itxDisableTransducerSafetyControls[4];
	transducerSafetyTimer[4] <= ( transducerSafetyTimer[4]+1'b1 );
end
else if ( transducerSafetyTimer[4] < transducerSafetyTimeout )
begin
	transducerSafetyTimer[4] <= ( transducerSafetyTimer[4]+1'b1 );
end
else if ( transducerSafetyTimer[4] == transducerSafetyTimeout )
begin
	if ( otxPinOutput[4] & !itxDisableTransducerSafetyControls[4] ) begin
		otxPinOutput[4] <= 0;
		transducerChargeTimeSafetyMask[4] <= 0;
		transducerSafetyTimer[4] <= 9'b0;
	end else begin
		transducerChargeTimeSafetyMask[4] <= 1;
	end
end

if ( !transducerSafetyTimer[5] && !otxPinOutput[5] ) begin
	transducerChargeTimeSafetyMask[5] <= itxDisableTransducerSafetyControls[5];
	transducerSafetyTimer[5] <= ( transducerSafetyTimer[5]+1'b1 );
end
else if ( transducerSafetyTimer[5] < transducerSafetyTimeout )
begin
	transducerSafetyTimer[5] <= ( transducerSafetyTimer[5]+1'b1 );
end
else if ( transducerSafetyTimer[5] == transducerSafetyTimeout )
begin
	if ( otxPinOutput[5] & !itxDisableTransducerSafetyControls[5] ) begin
		otxPinOutput[5] <= 0;
		transducerChargeTimeSafetyMask[5] <= 0;
		transducerSafetyTimer[5] <= 9'b0;
	end else begin
		transducerChargeTimeSafetyMask[5] <= 1;
	end
end

if ( !transducerSafetyTimer[6] && !otxPinOutput[6] ) begin
	transducerChargeTimeSafetyMask[6] <= itxDisableTransducerSafetyControls[6];
	transducerSafetyTimer[6] <= ( transducerSafetyTimer[6]+1'b1 );
end
else if ( transducerSafetyTimer[6] < transducerSafetyTimeout )
begin
	transducerSafetyTimer[6] <= ( transducerSafetyTimer[6]+1'b1 );
end
else if ( transducerSafetyTimer[6] == transducerSafetyTimeout )
begin
	if ( otxPinOutput[6] & !itxDisableTransducerSafetyControls[6] ) begin
		otxPinOutput[6] <= 0;
		transducerChargeTimeSafetyMask[6] <= 0;
		transducerSafetyTimer[6] <= 9'b0;
	end else begin
		transducerChargeTimeSafetyMask[6] <= 1;
	end
end

if ( !transducerSafetyTimer[7] && !otxPinOutput[7] ) begin
	transducerChargeTimeSafetyMask[7] <= itxDisableTransducerSafetyControls[7];
	transducerSafetyTimer[7] <= ( transducerSafetyTimer[7]+1'b1 );
end
else if ( transducerSafetyTimer[7] < transducerSafetyTimeout )
begin
	transducerSafetyTimer[7] <= ( transducerSafetyTimer[7]+1'b1 );
end
else if ( transducerSafetyTimer[7] == transducerSafetyTimeout )
begin
	if ( otxPinOutput[7] & !itxDisableTransducerSafetyControls[7] ) begin
		otxPinOutput[7] <= 0;
		transducerChargeTimeSafetyMask[7] <= 0;
		transducerSafetyTimer[7] <= 9'b0;
	end else begin
		transducerChargeTimeSafetyMask[7] <= 1;
	end
end

