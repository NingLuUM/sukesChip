module ADC_Control_Module(
	input					adc_clkinp,
	input					frame_clk,
	input					bit_clk,
	
	input [7:0]				adc_control_comm,
	input [23:0]			adc_serial_cmd,
	
	output reg				ADC_RESET,
	
	output reg				ADC_SDATA,
	output reg				ADC_SEN,
	output reg				ADC_PDN,	
	
	input					ADC_SCLK,
	input [7:0]				ADC_INPUT_DATA_LINES,
	
	input					itxTrig,
	output reg				otxTrigAck,
	
	
	
	input [15:0]			iRecLength,
	input					iStateReset,
	output reg [7:0]	oRcvInterrupt,
	
	input	[3:0]				down_sample_clk_divisor,
	input	[3:0]				sampling_mode_opts,
	
	output reg [15:0]	oBYTEEN,
	output reg [127:0]	oADCData,
	
	output reg 		oWREN,
	output reg 		oCLKEN,
	output reg 		oCHIPSEL,

	output reg [14:0]		oWAddr

);


reg [ 7: 0][11:0] data_sr;

reg fclk_flag;

reg [7:0] adc_state;
reg [7:0] last_adc_control_comm;

reg [23:0] cmd_buff; // initialize the buffer to software reset
reg [4:0] senCnt;

reg [1:0] trig_received_flag;
reg write_complete_flag;

reg [14:0] 	waddr_cntr;
wire waddr_overrun;
assign waddr_overrun = waddr_cntr[14];

reg [3:0] first_pulse;
reg [3:0] sample_cntr;

//reg [7:0][11:0] data_out;
reg [127:0] data_to_ram;
reg [127:0] data_out;
wire [ 7: 0] data_out_h;
wire [ 7: 0] data_out_l;


initial
begin
	fclk_flag = 1'b0;
	ADC_RESET = 1'b0;
	ADC_SEN = 1'b1;
	ADC_PDN = 1'b0;
	ADC_SDATA = 1'b0;
	oWREN = 1'b0;
	oCLKEN = 1'b0;
	oCHIPSEL = 1'b0;
	oBYTEEN = 16'b0000000000000000;
	otxTrigAck = 1'b0;
	
	first_pulse = 4'b1111;
	sample_cntr = 4'b0;
	
	trig_received_flag = 2'b0;
	write_complete_flag = 1'b0;
	waddr_cntr = 16'b0;
	senCnt = 5'b0;
	cmd_buff = 24'b0;
	
	adc_state = 8'b0;
	last_adc_control_comm = 8'b0;
	
	oRcvInterrupt <= 8'b0;
	
	data_sr[0] = 12'b0; data_sr[1] = 12'b0; data_sr[2] = 12'b0; data_sr[3] = 12'b0;
	data_sr[4] = 12'b0; data_sr[5] = 12'b0; data_sr[6] = 12'b0; data_sr[7] = 12'b0;
	
	data_to_ram = 128'b0;
	data_out = 128'b0;
end


parameter [7:0] hardware_reset = 8'b11111111;
parameter [7:0] idle_state  = 8'b00000000;
parameter [7:0] buffer_serial_command 	= 8'b00000001;
parameter [7:0] issue_serial_command 	= 8'b00000010;
parameter [7:0] sync_adc 					= 8'b00000100;

always @(posedge adc_clkinp)
begin

	if ( !iStateReset )
	begin
		if ( !trig_received_flag && itxTrig )
		begin
			trig_received_flag <= 2'b11;
			waddr_cntr <= 15'b0;
			write_complete_flag <= 1'b0;
			first_pulse <= 4'b1111;
			sample_cntr <= 4'b0000;
			data_to_ram <= 128'b0;
			oRcvInterrupt <= 8'b0;
		end
	
		
		if ( trig_received_flag[0] )
		begin
			if ( !write_complete_flag )
			begin
				if ( !oWREN && !down_sample_clk_divisor ) 
				begin
					oWREN <= 1'b1;
					oCLKEN <= 1'b1;
					oCHIPSEL <= 1'b1;
					oBYTEEN <= 16'b1111111111111111;
					otxTrigAck <= 1'b1;
				end
				else if ( down_sample_clk_divisor && first_pulse )
				begin
					oCLKEN <= 1'b1;
					oCHIPSEL <= 1'b1;
					oBYTEEN <= 16'b1111111111111111;
					sample_cntr <= 4'b0;
					data_to_ram <= 128'b0;
				end
				
				if ( ( waddr_cntr < iRecLength ) && ( !waddr_overrun ) )
				begin
					
					if(!down_sample_clk_divisor)
					begin
						oWAddr <= waddr_cntr;
						oADCData <= data_out;
						waddr_cntr <= waddr_cntr + 1'b1;
					end
					else
					begin
						if( ( sample_cntr == 4'b0 ) && ( first_pulse == 4'b0 ) )
						begin
							oWREN <= 1'b1;
							oWAddr <= waddr_cntr;
							oADCData <= data_to_ram;
							data_to_ram <= data_out;
							waddr_cntr <= waddr_cntr + 1'b1;
							sample_cntr <=4'b0001;
						end
						else 
						begin
							if ( first_pulse ) first_pulse <= 4'b0000;
							data_to_ram[15:0] <= data_to_ram[15:0] + data_out[15:0];
							data_to_ram[31:16] <= data_to_ram[31:16] + data_out[31:16];
							data_to_ram[47:32] <= data_to_ram[47:32] + data_out[47:32];
							data_to_ram[63:48] <= data_to_ram[63:48] + data_out[63:48];
							data_to_ram[79:64] <= data_to_ram[79:64] + data_out[79:64];
							data_to_ram[95:80] <= data_to_ram[95:80] + data_out[95:80];
							data_to_ram[111:96] <= data_to_ram[111:96] + data_out[111:96];
							data_to_ram[127:112] <= data_to_ram[127:112] + data_out[127:112];
							oWREN <= 1'b0;
							if( sample_cntr == down_sample_clk_divisor )
							begin
								sample_cntr <= 4'b0000;
							end
							else
							begin
								sample_cntr <= sample_cntr+1'b1;
							end
						end
					
					end
			
				end
				else 
				begin
					oWREN <= 1'b0;
					oCLKEN <= 1'b0;
					oCHIPSEL <= 1'b0;
					oBYTEEN <= 16'b0000000000000000;
					first_pulse <= 4'b1111;
					sample_cntr <= 4'b0000;
					write_complete_flag <= 1'b1;
					trig_received_flag[0] <= 1'b0;
					oRcvInterrupt <= 8'b11111111;
					otxTrigAck <= 1'b0;
				end
			end
		end
	end
	else // iStateReset == 1
	begin
		oWREN <= 1'b0;
		oCLKEN <= 1'b0;
		oCHIPSEL <= 1'b0;
		oBYTEEN <= 16'b0000000000000000;
		first_pulse <= 4'b1111;
		sample_cntr <= 4'b0000;
		trig_received_flag <= 2'b00;
		write_complete_flag <= 1'b0;
		waddr_cntr <= 15'b0;
		oRcvInterrupt <= 8'b0;
		otxTrigAck <= 1'b0;
	end
end


always @ (negedge ADC_SCLK) //SCLK = 2MHz
begin

	if( adc_control_comm != last_adc_control_comm )
	begin
		last_adc_control_comm <= adc_control_comm;
		adc_state <= adc_control_comm;
	end
	
	case( adc_state )
		hardware_reset:
			begin
				ADC_RESET <= 1'b1;
				ADC_SEN <= 1'b1;
				ADC_SDATA <= 1'b0;
				senCnt <= 5'b0;
			end
		
		idle_state:
			begin
				ADC_RESET <= 1'b0;
				ADC_SEN <= 1'b1;
				ADC_SDATA <= 1'b0;
				senCnt <= 5'b0;
			end
		
		buffer_serial_command:
			begin
				cmd_buff <= adc_serial_cmd;
				ADC_SDATA <= 1'b0;
				senCnt <= 5'b0;
				if ( !ADC_SEN ) ADC_SEN <= 1'b1;
			end
			
		issue_serial_command:
			begin
				if ( senCnt < 24 )
				begin
					if ( !senCnt ) ADC_SEN <= 1'b0;
					ADC_SDATA <= cmd_buff[23];
					cmd_buff <= {cmd_buff[22:0],1'b0};
					senCnt <= senCnt + 1'b1;
				end
				else
				begin
					ADC_SEN <= 1'b1;
					ADC_SDATA <= 1'b0;
				end	
			end
		
		sync_adc:
			begin
				adc_state <= idle_state;
			end
			
		default:
			begin
				adc_state <= idle_state;
			end
	endcase
end


// ADC LOGIC:
// Double-data rate register
// Converts the LVDS input to parallel double data outputs
ddio d0(
	.datain(ADC_INPUT_DATA_LINES),
	.inclock(bit_clk),
	.dataout_h(data_out_h),
	.dataout_l(data_out_l)
);

// Shift register
// Serializes the double data outputs
always @ (posedge bit_clk)
begin
	if( frame_clk & !fclk_flag )
	begin
		fclk_flag <= 1'b1;
		data_sr[0][11:10] <= {data_out_h[0], data_out_l[0]};
		data_sr[1][11:10] <= {data_out_h[1], data_out_l[1]};
		data_sr[2][11:10] <= {data_out_h[2], data_out_l[2]};
		data_sr[3][11:10] <= {data_out_h[3], data_out_l[3]};
		data_sr[4][11:10] <= {data_out_h[4], data_out_l[4]};
		data_sr[5][11:10] <= {data_out_h[5], data_out_l[5]};
		data_sr[6][11:10] <= {data_out_h[6], data_out_l[6]};
		data_sr[7][11:10] <= {data_out_h[7], data_out_l[7]};
	
	end
	else 
	begin
		if ( !frame_clk ) fclk_flag <= 1'b0;
		
		data_sr[0] <= {data_out_h[0], data_out_l[0], data_sr[0][11:2]};
		data_sr[1] <= {data_out_h[1], data_out_l[1], data_sr[1][11:2]};
		data_sr[2] <= {data_out_h[2], data_out_l[2], data_sr[2][11:2]};
		data_sr[3] <= {data_out_h[3], data_out_l[3], data_sr[3][11:2]};
		data_sr[4] <= {data_out_h[4], data_out_l[4], data_sr[4][11:2]};
		data_sr[5] <= {data_out_h[5], data_out_l[5], data_sr[5][11:2]};
		data_sr[6] <= {data_out_h[6], data_out_l[6], data_sr[6][11:2]};
		data_sr[7] <= {data_out_h[7], data_out_l[7], data_sr[7][11:2]};
	end
	
end

always @ (posedge frame_clk)
begin
	data_out[127:112] <= {4'b0000,data_sr[7]};
	data_out[111:96] <= {4'b0000,~data_sr[6]}; // wired backwards, bits need flipping
	data_out[95:80] <= {4'b0000,data_sr[5]};
	data_out[79:64] <= {4'b0000,data_sr[4]};
	data_out[63:48] <= {4'b0000,data_sr[3]};
	data_out[47:32] <= {4'b0000,data_sr[2]};
	data_out[31:16] <= {4'b0000,data_sr[1]}; 
	data_out[15:0] <= {4'b0000,data_sr[0]};
end


endmodule

