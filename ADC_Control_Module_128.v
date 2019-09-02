module ADC_Control_Module_128(
	input					adc_clkinp,
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
	
	output reg [15:0]	oBYTEEN,
	output reg [127:0]	oADCData,
	
	output reg 		oWREN,
	output reg 		oCLKEN,
	output reg		oCHIPSEL,

	output reg [13:0]		oWAddr

);


reg [ 7: 0][11:0] data_sr;

reg syncFlag;
reg fclk_flag;
reg [2:0] data_pos;
reg [63:0] data_buff;

reg [7:0] adc_state;
reg [7:0] last_adc_control_comm;

reg [23:0] cmd_buff; // initialize the buffer to software reset
reg [4:0] senCnt;

reg [1:0] trig_received_flag;
reg write_complete_flag;

reg [14:0] 	waddr_cntr;
reg [15:0]	write_cntr;
reg data_remaining_flag;

reg [7:0][11:0] data_out;
wire [ 7: 0] data_out_h;
wire [ 7: 0] data_out_l;


initial
begin
	fclk_flag = 1'b0;
	ADC_RESET = 1'b0;
	ADC_SEN = 1'b1;				//***
	ADC_SYNC = 1'b0;
	ADC_SDATA = 1'b0;
	oWREN = 1'b0;
	oCLKEN = 1'b0;
	oCHIPSEL = 1'b0;
	oBYTEEN = 16'b0;
	
	data_pos = 3'b0;
	data_buff = 64'b0;
	
	trig_received_flag = 2'b0;
	write_complete_flag = 1'b0;
	waddr_cntr = 15'b0;
	senCnt = 5'b0;
	cmd_buff = 24'b0;
	write_cntr = 16'b0;
	data_remaining_flag = 1'b0;
	
	adc_state = 8'b0;
	last_adc_control_comm = 8'b0;
	syncFlag = 1'b0;
	
	oDataReady <= 8'b0;
	
	data_sr[0] = 12'b0; data_sr[1] = 12'b0; data_sr[2] = 12'b0; data_sr[3] = 12'b0;
	data_sr[4] = 12'b0; data_sr[5] = 12'b0; data_sr[6] = 12'b0; data_sr[7] = 12'b0;
	
end

parameter [15:0] max_recLen = 16'b0101010101010100; //21844
parameter [7:0] hardware_reset = 8'b11111111;
parameter [7:0] idle_state  = 8'b00000000;
parameter [7:0] buffer_serial_command 	= 8'b00000001;
parameter [7:0] issue_serial_command 	= 8'b00000010;
parameter [7:0] sync_adc 					= 8'b00000100;

always @(posedge adc_clkinp)
begin

	if ( !iStateReset )
	begin
		if ( !trig_received_flag && iSystemTrig )
		begin
			trig_received_flag <= 2'b11;
			waddr_cntr <= 15'b0;
			write_cntr <= 16'b0;
			write_complete_flag <= 1'b0;
			oDataReady <= 8'b0;
			data_remaining_flag <= 1'b0;
			data_pos <= 3'b000;
		end
	
		
		if ( trig_received_flag[0] )
		begin
			if ( !write_complete_flag )
			begin
				if ( !oWREN ) 
				begin
					oWREN <= 1'b1;
					oCLKEN <= 1'b1;
					oCHIPSEL <= 1'b1;
					oBYTEEN <= 16'b1111_1111_1111_0000;
				end
				if ( ( write_cntr < iRecLength ) || ( ( write_cntr == iRecLength ) && ( data_remaining_flag ) ) )
				begin
					write_cntr <= write_cntr + 1'b1;
					oWAddr <= waddr_cntr[13:0];
					
					if( !data_pos )
					begin
						oBYTEEN <= 16'b1111_1111_1111_0000;
						oADCData[127:32] <= {data_out[0],data_out[1],data_out[2],data_out[3],data_out[4],data_out[5],data_out[6],data_out[7]};
						data_pos <= 3'b001;
						data_remaining_flag <= 1'b0;
					end
					else if ( data_pos == 3'b001 )
					begin
						oBYTEEN <= 16'b0000_0000_0000_1111;
						oADCData[31:0] <= {data_out[0],data_out[1],data_out[2][11:4]};
						data_buff <= {data_out[2][3:0],data_out[3],data_out[4],data_out[5],data_out[6],data_out[7]};
						data_pos <= 3'b010;
						waddr_cntr <= waddr_cntr + 1'b1;
						data_remaining_flag <= 1'b1;
					end
					else if ( data_pos == 3'b010 )
					begin
						oBYTEEN <= 16'b1111_1111_1111_1111;
						oADCData <= {data_buff,data_out[0],data_out[1],data_out[2],data_out[3],data_out[4],data_out[5][11:8]};
						data_buff[31:0] <= {data_out[5][7:0],data_out[6],data_out[7]};
						data_pos <= 3'b100;
						waddr_cntr <= waddr_cntr + 1'b1;
						data_remaining_flag <= 1'b1;
					end
					else if ( data_pos == 3'b100 )
					begin
						oBYTEEN <= 16'b1111_1111_1111_1111;
						oADCData <= {data_buff[31:0],data_out[0],data_out[1],data_out[2],data_out[3],data_out[4],data_out[5],data_out[6],data_out[7]};
						data_pos <= 3'b000;
						waddr_cntr <= waddr_cntr + 1'b1;
						data_remaining_flag <= 1'b0;
					end		
				end
				else //if ( waddr_cntr == iRecLength ) // iRecLength
				begin
					oWREN <= 1'b0;
					oCLKEN <= 1'b0;
					oCHIPSEL <= 1'b0;
					oBYTEEN <= 16'b0;
					write_complete_flag <= 1'b1;
					trig_received_flag[0] <= 1'b0;
					oDataReady <= 8'b11111111;
				end
			end
		end
	end
	else // iStateReset == 1
	begin
		oWREN <= 1'b0;
		oCLKEN <= 1'b0;
		oCHIPSEL <= 1'b0;
		oBYTEEN <= 16'b0;
		trig_received_flag <= 2'b00;
		write_complete_flag <= 1'b0;
		waddr_cntr <= 15'b0;
		oDataReady <= 8'b0;
		data_remaining_flag <= 1'b0;
		data_pos <= 3'b000;
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
	data_out <= data_sr;
end


endmodule

