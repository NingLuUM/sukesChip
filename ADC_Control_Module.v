module ADC_Control_Module(
	input					adc_clkinp,
	input					bit_clk,
	
	input [31:0]			adc_control_comm,
	input [31:0]			adc_pio_settings,
	
	input [23:0]			adc_serial_cmd,
	
	output					oADC_RESET,
	output					oADC_SDATA,
	output					oADC_SEN,
	output					oADC_PDN,	
	output					oADC_SYNC,	
	
	input					iADC_SCLK,
	input [7:0]				iADC_INPUT_DATA_LINES,
	
	//output					oVAR_ATTEN,
	
	input					itxTrig,
	output					otxTrigAck,
	
	input [15:0]			iRecLength,
	input					iStateReset,
	output reg [31:0]		oRcvInterrupt,
	
	output reg [15:0]		oBYTEEN,
	output reg [127:0]		oADCData,
	
	output reg 				oWREN,
	output reg 				oCLKEN,
	output reg 				oCHIPSEL,

	output reg [14:0]		oWAddr

);

// controlled from adc_control_comm
wire [7:0] adc_control_state; 
assign adc_control_state = adc_control_comm[7:0];

reg	ADC_RESET;
reg	ADC_SDATA;
reg	ADC_SEN;
reg	ADC_PDN;
reg	ADC_SYNC;

assign oADC_SYNC 	= ADC_SYNC;
assign oADC_PDN 	= ADC_PDN;
assign oADC_RESET 	= ADC_RESET;
assign oADC_SDATA 	= ADC_SDATA;
assign oADC_SEN 	= ADC_SEN;	


// set from adc_pio_settings
//wire		varAtten;						//adc_pio_settings[0]
wire [3:0]	down_sample_clk_divisor;		//adc_pio_settings[4:1]
wire [2:0]	fclk_delay;						//adc_pio_settings[7:5]
wire [2:0]	sampling_mode_opts;				//adc_pio_settings[10:8]
wire [1:0]	compressor_opts;				//adc_pio_settings[12:11]
wire		interruptThyself;				//adc_pio_settings[13]

//assign varAtten 				= adc_pio_settings[0];
assign down_sample_clk_divisor 	= adc_pio_settings[4:1];
assign fclk_delay 				= adc_pio_settings[7:5];
assign sampling_mode_opts 		= adc_pio_settings[10:8];
assign compressor_opts 			= adc_pio_settings[12:11];
assign interruptThyself 		= adc_pio_settings[13];

//reg 		varAtten_reg; 					//adc_pio_settings[0]
reg [3:0]	down_sample_clk_divisor_reg;	//adc_pio_settings[4:1]
reg [2:0]	fclk_delay_reg;					//adc_pio_settings[7:5]
reg [2:0]	sampling_mode_opts_reg;			//adc_pio_settings[10:8]
reg [1:0]	compressor_opts_reg;			//adc_pio_settings[12:11]
reg			interruptThyself_reg;			//adc_pio_settings[13]
//assign oVAR_ATTEN = varAtten_reg;



reg otxTrigAck_reg;
assign otxTrigAck = otxTrigAck_reg;

reg [ 7: 0][11:0] data_sr;

reg 		fclk_flag;

reg [7:0] 	adc_state;
reg [7:0]	last_adc_control_state;

reg [23:0]	cmd_buff; // initialize the buffer to software reset
reg [4:0]	senCnt;

reg [1:0]	trig_received_flag;
reg			write_complete_flag;

reg [14:0]	waddr_cntr;

wire		waddr_overrun;
assign waddr_overrun = waddr_cntr[14];

reg 		first_pulse;
reg 		first_write;
reg [3:0] 	sample_cntr;

reg [2:0]	fclk_delay_reg_cntr;

reg [127:0]	data_to_ram;
reg [127:0]	data_out;

wire [7:0]	flipmedata_out_h;
wire [7:0]	flipmedata_out_l;
reg [7:0]	data_out_h;
reg [7:0]	data_out_l;

reg [1:0]	data_compressor_buff_cntr;
reg [95:0]	data_buffer;

reg [127:0]	last_data_out;
reg [63:0]	data_diff_out;

initial
begin
	fclk_flag = 1'b0;
	ADC_RESET = 1'b0;
	ADC_SEN = 1'b1;
	ADC_PDN = 1'b1;
	ADC_SDATA = 1'b0;
	ADC_SYNC = 1'b0;
	
	oWREN = 1'b0;
	oCLKEN = 1'b0;
	oCHIPSEL = 1'b0;
	oBYTEEN = 16'b0000000000000000;
	
	//varAtten_reg = 1'b0;
	down_sample_clk_divisor_reg = 4'b0;
	fclk_delay_reg = 3'b0;
	sampling_mode_opts_reg = 3'b0;
	compressor_opts_reg = 2'b0;
	interruptThyself_reg = 1'b0;
	
	fclk_delay_reg_cntr = 3'b0;
	
	otxTrigAck_reg = 1'b0;
	
	first_pulse = 1'b1;
	first_write = 1'b1;
	sample_cntr = 4'b0;
	
	data_compressor_buff_cntr = 2'b0;
	
	trig_received_flag = 2'b0;
	write_complete_flag = 1'b0;
	waddr_cntr = 15'b0;
	senCnt = 5'b0;
	cmd_buff = 24'b0;
	
	adc_state = 8'b0;
	last_adc_control_state = 8'b0;
	
	oRcvInterrupt = 32'b0;
	
	data_sr[0] = 12'b0; data_sr[1] = 12'b0; data_sr[2] = 12'b0; data_sr[3] = 12'b0;
	data_sr[4] = 12'b0; data_sr[5] = 12'b0; data_sr[6] = 12'b0; data_sr[7] = 12'b0;
	
	data_to_ram = 128'b0;
	data_out = 128'b0;
	data_buffer = 96'b0;
	
	last_data_out = 128'b0;
	data_diff_out = 64'b0;

end


parameter [7:0] STATE_ADC_IDLE  			= 8'b00000000;
parameter [7:0] STATE_ADC_HARDWARE_RESET 	= 8'b00000001;
parameter [7:0] STATE_ADC_BUFFER_SERIAL_CMD = 8'b00000010;
parameter [7:0] STATE_ADC_ISSUE_SERIAL_CMD 	= 8'b00000100;
parameter [7:0] STATE_ADC_SYNC 				= 8'b00001000;
parameter [7:0] STATE_ADC_PDN_ONE			= 8'b00010000;
parameter [7:0] STATE_ADC_PDN_ZERO			= 8'b00100000;

always @(posedge adc_clkinp)
begin
	
	if ( !iStateReset )
	begin
	
		if ( !trig_received_flag &&  itxTrig )
		begin
			trig_received_flag <= 2'b11;
			waddr_cntr <= 15'b0;
			write_complete_flag <= 1'b0;
			first_pulse <= 1'b1;
			first_write <= 1'b1;
			data_compressor_buff_cntr <= 2'b0;
			sample_cntr <= 4'b0000;
			data_to_ram <= 128'b0;
			data_buffer <= 96'b0;
			
			last_data_out <= 128'b0;
			data_diff_out <= 64'b0;
		end
	
		
		if ( trig_received_flag[0] )
		begin
			if ( !write_complete_flag )
			begin
			
				if ( !oWREN && !down_sample_clk_divisor_reg ) 
				begin
					if( !compressor_opts_reg ) oWREN <= 1'b1;
					oCLKEN <= 1'b1;
					oCHIPSEL <= 1'b1;
					if( !compressor_opts_reg[1] ) oBYTEEN <= 16'b1111111111111111;
					otxTrigAck_reg <= 1'b1;
				end
				else if ( down_sample_clk_divisor_reg && first_pulse )
				begin
					oCLKEN <= 1'b1;
					oCHIPSEL <= 1'b1;
					if( !compressor_opts_reg[1] ) oBYTEEN <= 16'b1111111111111111;
					sample_cntr <= 4'b0;
					data_to_ram <= 128'b0;
				end
				
				if ( ( waddr_cntr < iRecLength ) && ( !waddr_overrun ) )
				begin
					
					if(!down_sample_clk_divisor_reg)
					begin
						oWAddr <= waddr_cntr;
						
						if( !compressor_opts_reg )
						begin
							oADCData <= data_out;
							waddr_cntr <= waddr_cntr + 1'b1;
						end
						else
						begin

							if( data_compressor_buff_cntr == 2'b00 )
							begin
								oWREN <= 1'b0;
								data_buffer <= {
									data_out[11:0],
									data_out[27:16],
									data_out[43:32],
									data_out[59:48],
									data_out[75:64],
									data_out[91:80],
									data_out[107:96],
									data_out[123:112]			
								};
								
							end
							else if( data_compressor_buff_cntr == 2'b01 )
							begin
								oWREN <= 1'b1;
								oADCData <= { 	
									data_out[87:80],
									data_out[107:96],
									data_out[123:112], 
									data_buffer	
								};
								data_buffer[95:84] <= data_out[11:0];
								data_buffer[83:72] <= data_out[27:16];
								data_buffer[71:60] <= data_out[43:32];
								data_buffer[59:48] <= data_out[59:48];
								data_buffer[47:36] <= data_out[75:64];
								data_buffer[35:32] <= data_out[91:88];
								
							end
							else if( data_compressor_buff_cntr == 2'b10 )
							begin
								oADCData <= { 	
									data_out[35:32],
									data_out[59:48],
									data_out[79:64],
									data_out[91:80],
									data_out[107:96],
									data_out[123:112],
									data_buffer[95:32]							
								};
								data_buffer[31:20]<=data_out[11:0];
								data_buffer[19:8] <= data_out[27:16];
								data_buffer[7:0] <= data_out[43:36];
							end 
							else 
							begin
								oADCData <= { 	
									data_out[11:0],
									data_out[27:16],
									data_out[43:32],
									data_out[59:48],
									data_out[75:64],
									data_out[91:80],
									data_out[107:96],
									data_out[123:112],
									data_buffer[31:0]				
								};
							end
							data_compressor_buff_cntr <= data_compressor_buff_cntr + 1'b1;
							if ( data_compressor_buff_cntr ) waddr_cntr <= waddr_cntr + 1'b1;
							
						end
						
					end
					else
					begin
						if( ( sample_cntr == 4'b0 ) && ( first_pulse == 1'b0 ) )
						begin
							oWAddr <= waddr_cntr;
							data_to_ram <= data_out;
							sample_cntr <=4'b0001;
							if( !compressor_opts_reg )
							begin
								oWREN <= 1'b1;
								oWAddr <= waddr_cntr;
								oADCData <= data_to_ram;
								waddr_cntr <= waddr_cntr + 1'b1;
							end
							else
							begin
								if( data_compressor_buff_cntr == 2'b00 )
								begin
									oWREN <= 1'b0;
									data_buffer <= {
										data_to_ram[11:0],
										data_to_ram[27:16],
										data_to_ram[43:32],
										data_to_ram[59:48],
										data_to_ram[75:64],
										data_to_ram[91:80],
										data_to_ram[107:96],
										data_to_ram[123:112]			
									};
									
								end
								else if( data_compressor_buff_cntr == 2'b01 )
								begin
									oWREN <= 1'b1;
									oADCData <= { 	
										data_to_ram[87:80],
										data_to_ram[107:96],
										data_to_ram[123:112], 
										data_buffer	
									};
									data_buffer[95:84] <= data_to_ram[11:0];
									data_buffer[83:72] <= data_to_ram[27:16];
									data_buffer[71:60] <= data_to_ram[43:32];
									data_buffer[59:48] <= data_to_ram[59:48];
									data_buffer[47:36] <= data_to_ram[75:64];
									data_buffer[35:32] <= data_to_ram[91:88];
									
								end
								else if( data_compressor_buff_cntr == 2'b10 )
								begin
									oWREN <= 1'b1;
									oADCData <= { 	
										data_to_ram[35:32],
										data_to_ram[59:48],
										data_to_ram[79:64],
										data_to_ram[91:80],
										data_to_ram[107:96],
										data_to_ram[123:112],
										data_buffer[95:32]							
									};
									data_buffer[31:20] <= data_to_ram[11:0];
									data_buffer[19:8] <= data_to_ram[27:16];
									data_buffer[7:0] <= data_to_ram[43:36];
								end 
								else 
								begin
									oWREN <= 1'b1;
									oADCData <= { 	
										data_to_ram[11:0],
										data_to_ram[27:16],
										data_to_ram[43:32],
										data_to_ram[59:48],
										data_to_ram[75:64],
										data_to_ram[91:80],
										data_to_ram[107:96],
										data_to_ram[123:112],
										data_buffer[31:0]				
									};
								end
								data_compressor_buff_cntr <= data_compressor_buff_cntr + 1'b1;
								if ( data_compressor_buff_cntr ) waddr_cntr <= waddr_cntr + 1'b1;
							end
							
						end
						else 
						begin
							if ( first_pulse ) first_pulse <= 1'b0;
							
							if( !sampling_mode_opts_reg ) // capture every Nth timepoint
							begin
								data_to_ram <= data_out;
							end
							else if( sampling_mode_opts_reg == 3'b001 ) // temporal mean
							begin
							
								data_to_ram[15:0] <= data_to_ram[15:0] + data_out[15:0];
								data_to_ram[31:16] <= data_to_ram[31:16] + data_out[31:16];
								data_to_ram[47:32] <= data_to_ram[47:32] + data_out[47:32];
								data_to_ram[63:48] <= data_to_ram[63:48] + data_out[63:48];
								data_to_ram[79:64] <= data_to_ram[79:64] + data_out[79:64];
								data_to_ram[95:80] <= data_to_ram[95:80] + data_out[95:80];
								data_to_ram[111:96] <= data_to_ram[111:96] + data_out[111:96];
								data_to_ram[127:112] <= data_to_ram[127:112] + data_out[127:112];
								
							end
							
							
							oWREN <= 1'b0;
							if( sample_cntr == down_sample_clk_divisor_reg )
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
					first_pulse <= 1'b1;
					first_write <= 1'b1;
					data_compressor_buff_cntr <= 2'b0;
					sample_cntr <= 4'b0000;
					
					data_to_ram <= 128'b0;
					data_buffer <= 96'b0;
					
					last_data_out <= 128'b0;
					data_diff_out <= 64'b0;
			
					write_complete_flag <= 1'b1;
					trig_received_flag[0] <= 1'b0;
					oRcvInterrupt[0] <= 1'b1;
					otxTrigAck_reg <= 1'b0;
				end
			end
		end
	end
	else // iStateReset == 1
	begin
		//if ( varAtten_reg ^ varAtten ) 
		//begin
		//	varAtten_reg <= varAtten;
		//end
		
		if( down_sample_clk_divisor_reg ^ down_sample_clk_divisor ) 
		begin
			down_sample_clk_divisor_reg <= down_sample_clk_divisor;
		end
		
		if( fclk_delay_reg ^ fclk_delay ) 
		begin
			fclk_delay_reg <= fclk_delay;
		end
		
		if( sampling_mode_opts_reg ^ sampling_mode_opts ) 
		begin
			sampling_mode_opts_reg <= sampling_mode_opts;
		end
		
		if( compressor_opts_reg ^ compressor_opts )
		begin
			compressor_opts_reg <= compressor_opts;
		end
		
		if( interruptThyself_reg ^ interruptThyself )
		begin
			interruptThyself_reg <= interruptThyself;
		end
		
		oWREN <= 1'b0;
		oCLKEN <= 1'b0;
		oCHIPSEL <= 1'b0;
		oBYTEEN <= 16'b0000000000000000;
		first_pulse <= 1'b1;
		first_write <= 1'b1;
		data_compressor_buff_cntr <= 2'b0;
		sample_cntr <= 4'b0000;
		
		data_to_ram <= 128'b0;
		data_buffer <= 96'b0;
		
		last_data_out <= 128'b0;
		data_diff_out <= 64'b0;
			
		trig_received_flag <= 2'b00;
		write_complete_flag <= 1'b0;
		waddr_cntr <= 15'b0;
		if( !interruptThyself_reg )
		begin
			if( oRcvInterrupt ) oRcvInterrupt <= 32'b0;
		end
		else
		begin
			if( !oRcvInterrupt ) oRcvInterrupt <= 32'b1111;
		end
		otxTrigAck_reg <= 1'b0;
	end
end


always @ (negedge iADC_SCLK) //SCLK = 2MHz
begin

	if( adc_control_state != last_adc_control_state )
	begin
		last_adc_control_state <= adc_control_state;
		adc_state <= adc_control_state;
	end
	
	case( adc_state )
		STATE_ADC_HARDWARE_RESET:
			begin
				if( !ADC_RESET ) ADC_RESET <= 1'b1;
				if( !ADC_SEN ) ADC_SEN <= 1'b1;
				if( ADC_SDATA ) ADC_SDATA <= 1'b0;
				senCnt <= 5'b0;
			end
		
		STATE_ADC_IDLE:
			begin
				if( ADC_RESET ) ADC_RESET <= 1'b0;
				if( !ADC_SEN ) ADC_SEN <= 1'b1;
				if( ADC_SDATA ) ADC_SDATA <= 1'b0;
				senCnt <= 5'b0;
			end
		
		STATE_ADC_BUFFER_SERIAL_CMD:
			begin
				cmd_buff <= adc_serial_cmd;
				senCnt <= 5'b0;
				
				if( ADC_SDATA ) ADC_SDATA <= 1'b0;
				if( !ADC_SEN ) ADC_SEN <= 1'b1;
			end
			
		STATE_ADC_ISSUE_SERIAL_CMD:
			begin
				if ( senCnt < 24 )
				begin
					if( !senCnt ) ADC_SEN <= 1'b0;
					ADC_SDATA <= cmd_buff[23];
					cmd_buff <= {cmd_buff[22:0],1'b0};
					senCnt <= senCnt + 1'b1;
				end
				else
				begin
					if( !ADC_SEN ) ADC_SEN <= 1'b1;
					if( ADC_SDATA ) ADC_SDATA <= 1'b0;
				end	
			end
		
		STATE_ADC_SYNC:
			begin
				if ( adc_state ^ STATE_ADC_IDLE ) adc_state <= STATE_ADC_IDLE;
			end
			
		STATE_ADC_PDN_ONE:
			begin
				if( !ADC_PDN ) ADC_PDN <= 1'b1;
				if( !ADC_SEN ) ADC_SEN <= 1'b1;
				if( ADC_SDATA ) ADC_SDATA <= 1'b0;
				senCnt <= 5'b0;
			end
			
		STATE_ADC_PDN_ZERO:
			begin
				if( ADC_PDN ) ADC_PDN <= 1'b0;
				if( !ADC_SEN ) ADC_SEN <= 1'b1;
				if( ADC_SDATA ) ADC_SDATA <= 1'b0;
				senCnt <= 5'b0;
			end
			
		default:
			begin
				adc_state <= STATE_ADC_IDLE;
			end
	endcase
end


// ADC LOGIC:
// Double-data rate register
// Converts the LVDS input to parallel double data outputs
ddio d0(
	.datain(iADC_INPUT_DATA_LINES),
	.inclock(bit_clk),
	.dataout_h(flipmedata_out_h),
	.dataout_l(flipmedata_out_l)
);


always @ (posedge bit_clk)
begin
	
	data_out_h[0] <= ~flipmedata_out_h[0];
	data_out_h[1] <= ~flipmedata_out_h[1];
	data_out_h[2] <= flipmedata_out_h[2];
	data_out_h[3] <= ~flipmedata_out_h[3];
	data_out_h[4] <= ~flipmedata_out_h[4];
	data_out_h[5] <= ~flipmedata_out_h[5];
	data_out_h[6] <= flipmedata_out_h[6];
	data_out_h[7] <= ~flipmedata_out_h[7];
	
	data_out_l[0] <= ~flipmedata_out_l[0];
	data_out_l[1] <= ~flipmedata_out_l[1];
	data_out_l[2] <= flipmedata_out_l[2];
	data_out_l[3] <= ~flipmedata_out_l[3];
	data_out_l[4] <= ~flipmedata_out_l[4];
	data_out_l[5] <= ~flipmedata_out_l[5];
	data_out_l[6] <= flipmedata_out_l[6];
	data_out_l[7] <= ~flipmedata_out_l[7];
	
	
	if( adc_clkinp & !fclk_flag )
	begin
	
		fclk_flag <= 1'b1;
		fclk_delay_reg_cntr <= 3'b0;
			
	end
	else 
	begin
	
		if( !adc_clkinp & fclk_flag ) fclk_flag <= 1'b0;
		fclk_delay_reg_cntr <= fclk_delay_reg_cntr + 1'b1;
			
	end

	if(fclk_delay_reg_cntr == fclk_delay_reg)
	begin
	
		data_sr[0] <= {data_out_h[0], data_out_l[0], 10'b0};
		data_sr[1] <= {data_out_h[1], data_out_l[1], 10'b0};
		data_sr[2] <= {data_out_h[2], data_out_l[2], 10'b0};
		data_sr[3] <= {data_out_h[3], data_out_l[3], 10'b0};
		data_sr[4] <= {data_out_h[4], data_out_l[4], 10'b0};
		data_sr[5] <= {data_out_h[5], data_out_l[5], 10'b0};
		data_sr[6] <= {data_out_h[6], data_out_l[6], 10'b0};
		data_sr[7] <= {data_out_h[7], data_out_l[7], 10'b0};
		
		data_out[127:112] <= {4'b0000,data_sr[7]}; 
		data_out[111:96] <= {4'b0000,data_sr[6]}; 
		data_out[95:80] <= {4'b0000,data_sr[5]}; 
		data_out[79:64] <= {4'b0000,data_sr[4]}; 
		data_out[63:48] <= {4'b0000,data_sr[3]};
		data_out[47:32] <= {4'b0000,data_sr[2]};
		data_out[31:16] <= {4'b0000,data_sr[1]};
		data_out[15:0] <= {4'b0000,data_sr[0]};

	end
	else 
	begin	
	
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

endmodule

