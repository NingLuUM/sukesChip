
module Output_Control_Module(

	input 					txCLK,
	input	[7:0]			itxControlComms,
	
	input	[8:0]			iChargeTime1,
	input	[8:0]			iChargeTime2,	
	
	input					itxADCTriggerAck,
	output reg				otxADCTriggerLine,
	
	output reg [1:0]		otxTransducerOutput
	
);

reg [1:0] adcTrigFlag;

reg [1:0] fireFlag1;
reg [1:0] fireFlag2;

reg [9:0] ctCounter1;
reg	[9:0] ctCounter2;

initial
begin
	otxTransducerOutput = 2'b0;
	otxADCTriggerLine = 1'b0;
	ctCounter1 = 9'b0;
	ctCounter2 = 9'b0;
end


parameter	[7:0]	CASE_IDLE = 8'b00000000;
parameter	[7:0]	CASE_FIRE = 8'b00000001;



always @(posedge txCLK)
begin
	case( itxControlComms )
		CASE_IDLE:
			begin
				if(otxTransducerOutput) otxTransducerOutput <= 2'b00;
				if(otxADCTriggerLine) otxADCTriggerLine <= 1'b0;
				adcTrigFlag <= 2'b00;
				fireFlag1 <= 2'b00;
				fireFlag2 <= 2'b00;
				ctCounter1 <= 9'b000000000;
				ctCounter2 <= 9'b000000000;
			end
		
		CASE_FIRE:
			begin
				if( !adcTrigFlag )
				begin
					otxADCTriggerLine <= 1'b1;
					adcTrigFlag[0] <= 1'b1;
				end
				else if ( adcTrigFlag[0] & !adcTrigFlag[1] & itxADCTriggerAck )
				begin
					otxADCTriggerLine <= 1'b0;
					adcTrigFlag[1] <= 1'b1;
				end
				
				if( !fireFlag1 && ( ctCounter1 < iChargeTime1 ) )
				begin
					fireFlag1[0] <= 1'b1;
					otxTransducerOutput[0] <= 1'b1;
					ctCounter1 <= ctCounter1 + 1'b1;
				end
				else if ( ( fireFlag1[0] & !fireFlag1[1] ) && ( ( ctCounter1 >= iChargeTime1 ) || ctCounter1[9] ) )
				begin
					fireFlag1[1] <= 1'b1;
					otxTransducerOutput[0] <= 1'b0;
					ctCounter1 <= ctCounter1 + 1'b1;
				end
				
				if( !fireFlag2 && ( ctCounter2 < iChargeTime2 ) )
				begin
					fireFlag2[0] <= 1'b1;
					otxTransducerOutput[1] <= 1'b1;
					ctCounter2 <= ctCounter2 + 1'b1;
				end
				else if ( ( fireFlag2[0] & !fireFlag2[1] ) && ( ( ctCounter2 >= iChargeTime2 ) || ctCounter2[9] ) )
				begin
					fireFlag2[1] <= 1'b1;
					otxTransducerOutput[1] <= 1'b0;
					ctCounter2 <= ctCounter2 + 1'b1;
				end

			end
				

		default:
			begin
				if(otxTransducerOutput) otxTransducerOutput <= 2'b00;
				if(otxADCTriggerLine) otxADCTriggerLine <= 1'b0;
				adcTrigFlag <= 2'b00;
				fireFlag1 <= 2'b00;
				fireFlag2 <= 2'b00;
				ctCounter1 <= 9'b000000000;
				ctCounter2 <= 9'b000000000;
			end
	
	endcase
	
	
end



endmodule


