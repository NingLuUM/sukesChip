
if( phaseDelay[0] == txOutputCntr ) 
begin
	otxTransducerOutput[0] <= itxTransducerChannelMask[0];
end

if( phaseDelay[1] == txOutputCntr ) 
begin
	otxTransducerOutput[1] <= itxTransducerChannelMask[1];
end

if( phaseDelay[2] == txOutputCntr ) 
begin
	otxTransducerOutput[2] <= itxTransducerChannelMask[2];
end

if( phaseDelay[3] == txOutputCntr )
begin
	otxTransducerOutput[3] <= itxTransducerChannelMask[3];
end

if( phaseDelay[4] == txOutputCntr )
begin
	otxTransducerOutput[4] <= itxTransducerChannelMask[4];
end

if( phaseDelay[5] == txOutputCntr )
begin
	otxTransducerOutput[5] <= itxTransducerChannelMask[5];
end

if( phaseDelay[6] == txOutputCntr )
begin
	otxTransducerOutput[6] <= itxTransducerChannelMask[6];
end

if( phaseDelay[7] == txOutputCntr )
begin
	otxTransducerOutput[7] <= itxTransducerChannelMask[7];
end

if( chargeTime[0] ) 
begin
	chargeTime[0] <= ( chargeTime[0]-1'b1 );
end 
else 
begin
	otxTransducerOutput[0] <= 1'b0;
end

if( chargeTime[1] ) 
begin
	chargeTime[1] <= ( chargeTime[1]-1'b1 );
end 
else 
begin
	otxTransducerOutput[1] <= 1'b0;
end

if( chargeTime[2] ) 
begin
	chargeTime[2] <= ( chargeTime[2]-1'b1 );
end 
else 
begin
	otxTransducerOutput[2] <= 1'b0;
end

if( chargeTime[3] ) 
begin
	chargeTime[3] <= ( chargeTime[3]-1'b1 );
end 
else 
begin
	otxTransducerOutput[3] <= 1'b0;
end

if( chargeTime[4] ) 
begin
	chargeTime[4] <= ( chargeTime[4]-1'b1 );
end 
else 
begin
	otxTransducerOutput[4] <= 1'b0;
end

if( chargeTime[5] ) 
begin
	chargeTime[5] <= ( chargeTime[5]-1'b1 );
end 
else 
begin
	otxTransducerOutput[5] <= 1'b0;
end

if( chargeTime[6] ) 
begin
	chargeTime[6] <= ( chargeTime[6]-1'b1 );
end 
else 
begin
	otxTransducerOutput[6] <= 1'b0;
end

if( chargeTime[7] ) 
begin
	chargeTime[7] <= ( chargeTime[7]-1'b1 );
end 
else 
begin
	otxTransducerOutput[7] <= 1'b0;
end
