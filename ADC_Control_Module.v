module ADC_Control_Module(
	input					ref_frame_clk,
	input					frame_clk,
	input					bit_clk,
	
	input [7:0]				adc_control_comm,
	input [23:0]			adc_serial_cmd,
	
	output reg				ADC_RESET,
	output reg				ADC_SYNC,
	
	output reg				ADC_SDATA,
	output reg				ADC_SEN,
	
	input					ADC_SCLK,
	input					ADC_SDOUT,	
	input [7:0]				ADC_INPUT_DATA_LINES,
	
	input					iSystemTrig,
	
	input [15:0]			iRecLength,
	input					iStateReset,
	output reg [7:0]	oDataReady,
	
	output reg [7:0]	oBYTEEN0,
	output reg [63:0]	oADCData0,
	
	output reg [3:0]	oBYTEEN1,
	output reg [31:0]	oADCData1,
	
	output reg [1:0]		oWREN,
	output reg [1:0]		oCLKEN,
	output reg [1:0]		oCHIPSEL,

	output reg [14:0]		oWAddr

);


reg [ 7: 0][11:0] data_sr;
reg [ 7: 0][9:0] data_prev;

reg syncFlag;

// SERIAL CLOCK AND COMMANDS:
// Serial commands (Address is 8 bits, register contents are 16 bits)
//parameter software_reset	= 8'b00000000;//0000000000000001; // software reset
//parameter set_tgc_reg 		= 8'b00000000;//0000000000000100; // enable acccess to TGC registers
//parameter set_fixed_gain 	= 8'b10011001;//0000000000001000; // 
//parameter set_coarse_gain 	= 8'b10011010;//0000000000001100; //
//parameter set_fine_gain		= 8'b10011001;

reg [7:0] adc_state;
reg [7:0] last_adc_control_comm;

reg [23:0] cmd_buff; // initialize the buffer to software reset
reg [4:0] senCnt;

reg [1:0] trig_received_flag;
reg write_complete_flag;

reg [15:0] 	waddr_cntr;
wire waddr_overrun;
assign waddr_overrun = waddr_cntr[15];

initial
begin
	ADC_RESET = 1'b0;
	ADC_SEN = 1'b1;				//***
	ADC_SYNC = 1'b0;
	ADC_SDATA = 1'b0;
	oWREN = 2'b00;
	oCLKEN = 2'b00;
	oCHIPSEL = 2'b00;
	oBYTEEN0 = 8'b00000000;
	oBYTEEN1 = 4'b0000;
	
	trig_received_flag = 2'b0;
	write_complete_flag = 1'b0;
	waddr_cntr = 16'b0;
	senCnt = 5'b0;
	cmd_buff = 24'b0;
	
	adc_state = 8'b0;
	last_adc_control_comm = 8'b0;
	syncFlag = 1'b0;
	
	oDataReady <= 8'b0;
	
	data_sr[0] = 12'b0; data_sr[1] = 12'b0; data_sr[2] = 12'b0; data_sr[3] = 12'b0;
	data_sr[4] = 12'b0; data_sr[5] = 12'b0; data_sr[6] = 12'b0; data_sr[7] = 12'b0;
	
	data_prev[0] = 10'b0; data_prev[1] = 10'b0; data_prev[2] = 10'b0; data_prev[3] = 10'b0;
	data_prev[4] = 10'b0; data_prev[5] = 10'b0; data_prev[6] = 10'b0; data_prev[7] = 10'b0;
end




wire [7:0][11:0] data_out;
wire [ 7: 0] data_out_h;
wire [ 7: 0] data_out_l;


parameter [7:0] hardware_reset = 8'b11111111;
parameter [7:0] idle_state  = 8'b00000000;
parameter [7:0] buffer_serial_command 	= 8'b00000001;
parameter [7:0] issue_serial_command 	= 8'b00000010;
parameter [7:0] sync_adc 					= 8'b00000100;


always @(posedge ref_frame_clk)
begin

	if ( !iStateReset )
	begin
		if ( !trig_received_flag && iSystemTrig )
		begin
			trig_received_flag <= 2'b11;
			waddr_cntr <= 16'b0;
			write_complete_flag <= 1'b0;
			oDataReady <= 8'b0;
		end
	
		
		if ( trig_received_flag[0] )
		begin
			if ( !write_complete_flag )
			begin
				if ( !oWREN ) 
				begin
					oWREN <= 2'b11;
					oCLKEN <= 2'b11;
					oCHIPSEL <= 2'b11;
					oBYTEEN0 <= 8'b11111111;
					oBYTEEN1 <= 4'b1111;
				end
				if ( ( waddr_cntr < iRecLength ) && ( !waddr_overrun ) )
				begin
					oWAddr <= waddr_cntr;
					
					oADCData0[11:0] <= data_out[0]; 
					oADCData0[23:12] <= data_out[1];
					oADCData0[35:24] <= data_out[2];
					oADCData0[47:36] <= data_out[3];
					oADCData0[59:48] <= data_out[4];
					oADCData0[63:60] <= data_out[5][3:0];
					
					oADCData1[7:0] <= data_out[5][11:4]; 
					oADCData1[19:8] <= data_out[6];
					oADCData1[31:20] <= data_out[7];
				
					waddr_cntr <= waddr_cntr + 1'b1;
				end
				else //if ( waddr_cntr == iRecLength ) // iRecLength
				begin
					oWREN <= 2'b00;
					oCLKEN <= 2'b00;
					oCHIPSEL <= 2'b00;
					oBYTEEN0 <= 8'b00000000;
					oBYTEEN1 <= 4'b0000;
					write_complete_flag <= 1'b1;
					trig_received_flag[0] <= 1'b0;
					oDataReady <= 8'b11111111;
				end
			end
		end
	end
	else // iStateReset == 1
	begin
		oWREN <= 2'b00;
		oCLKEN <= 2'b00;
		oCHIPSEL <= 2'b00;
		oBYTEEN0 <= 8'b00000000;
		oBYTEEN1 <= 4'b0000;
		trig_received_flag <= 2'b00;
		write_complete_flag <= 1'b0;
		waddr_cntr <= 16'b0;
		oDataReady <= 8'b0;
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
				syncFlag <= 1'b0;
				ADC_RESET <= 1'b1;
				ADC_SEN <= 1'b1;
				ADC_SYNC <= 1'b0;
				ADC_SDATA <= 1'b0;
				senCnt <= 5'b0;
			end
		
		idle_state:
			begin
				syncFlag <= 1'b0;
				ADC_RESET <= 1'b0;
				ADC_SEN <= 1'b1;
				ADC_SYNC <= 1'b0;
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
				if ( !syncFlag )
				begin
					ADC_SYNC <= ~ADC_SYNC;
					if ( ADC_SYNC ) syncFlag <= 1'b1;
				end	
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
	data_sr[0] <= {data_sr[0][9:0], data_out_l[0], data_out_h[0]};
	data_sr[1] <= {data_sr[1][9:0], data_out_l[1], data_out_h[1]};
	data_sr[2] <= {data_sr[2][9:0], data_out_l[2], data_out_h[2]};
	data_sr[3] <= {data_sr[3][9:0], data_out_l[3], data_out_h[3]};
	data_sr[4] <= {data_sr[4][9:0], data_out_l[4], data_out_h[4]};
	data_sr[5] <= {data_sr[5][9:0], data_out_l[5], data_out_h[5]};
	data_sr[6] <= {data_sr[6][9:0], data_out_l[6], data_out_h[6]};
	data_sr[7] <= {data_sr[7][9:0], data_out_l[7], data_out_h[7]};
	
end

always @ (posedge frame_clk)
begin
	// Barrel shift right by 3 bits and reverse to fix signal
	data_prev[0][9:0] <= data_sr[0][9:0];
	data_prev[1][9:0] <= data_sr[1][9:0];
	data_prev[2][9:0] <= data_sr[2][9:0];
	data_prev[3][9:0] <= data_sr[3][9:0];
	data_prev[4][9:0] <= data_sr[4][9:0];
	data_prev[5][9:0] <= data_sr[5][9:0];
	data_prev[6][9:0] <= data_sr[6][9:0];
	data_prev[7][9:0] <= data_sr[7][9:0];
	
	data_out[0] <= {data_sr[0][10], data_sr[0][11], data_prev[0][0], data_prev[0][1], data_prev[0][2], data_prev[0][3], data_prev[0][4], data_prev[0][5], data_prev[0][6], data_prev[0][7], data_prev[0][8], data_prev[0][9]};
	data_out[1] <= {data_sr[1][10], data_sr[1][11], data_prev[1][0], data_prev[1][1], data_prev[1][2], data_prev[1][3], data_prev[1][4], data_prev[1][5], data_prev[1][6], data_prev[1][7], data_prev[1][8], data_prev[1][9]};
	data_out[2] <= {data_sr[2][10], data_sr[2][11], data_prev[2][0], data_prev[2][1], data_prev[2][2], data_prev[2][3], data_prev[2][4], data_prev[2][5], data_prev[2][6], data_prev[2][7], data_prev[2][8], data_prev[2][9]};
	data_out[3] <= {data_sr[3][10], data_sr[3][11], data_prev[3][0], data_prev[3][1], data_prev[3][2], data_prev[3][3], data_prev[3][4], data_prev[3][5], data_prev[3][6], data_prev[3][7], data_prev[3][8], data_prev[3][9]};
	data_out[4] <= {data_sr[4][10], data_sr[4][11], data_prev[4][0], data_prev[4][1], data_prev[4][2], data_prev[4][3], data_prev[4][4], data_prev[4][5], data_prev[4][6], data_prev[4][7], data_prev[4][8], data_prev[4][9]};
	data_out[5] <= {data_sr[5][10], data_sr[5][11], data_prev[5][0], data_prev[5][1], data_prev[5][2], data_prev[5][3], data_prev[5][4], data_prev[5][5], data_prev[5][6], data_prev[5][7], data_prev[5][8], data_prev[5][9]};
	data_out[6] <= {data_sr[6][10], data_sr[6][11], data_prev[6][0], data_prev[6][1], data_prev[6][2], data_prev[6][3], data_prev[6][4], data_prev[6][5], data_prev[6][6], data_prev[6][7], data_prev[6][8], data_prev[6][9]};
	data_out[7] <= {data_sr[7][10], data_sr[7][11], data_prev[7][0], data_prev[7][1], data_prev[7][2], data_prev[7][3], data_prev[7][4], data_prev[7][5], data_prev[7][6], data_prev[7][7], data_prev[7][8], data_prev[7][9]};
end


endmodule

