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
	output reg				ADC_PDN,
	
	input					ADC_SCLK,
	input					ADC_SDOUT,	
	input [7:0]				ADC_INPUT_DATA_LINES,
	
	input					iSystemTrig,
	input					iTxTrigger,
	
	input [31:0]			iTrigDelay,
	input [12:0]			iRecLength,
	input					iStateReset,
	
	output reg				oTriggerAck,
	output reg [2:0][31:0]	oADCData,
	output reg [2:0]		oWREN,
	output reg [12:0]		oWAddr,
	
	output reg [7:0]		oArmInterrupt
);

initial
begin
	ADC_PDN = 1'b1;
	ADC_RESET = 1'b0;
	ADC_SEN = 1'b1;
	ADC_SYNC = 1'b0;
	ADC_SDATA = 1'b0;
end

// regs and wires
reg [1:0] trig_received_flag = 2'b0;
reg write_complete_flag = 1'b0;



reg [12:0] 	waddr_cntr;
reg [31:0] txReadyTimeout = 32'b0;


wire [7:0][11:0] data_out; 


wire [ 7: 0] data_out_h;
wire [ 7: 0] data_out_l;
reg [ 7: 0][11:0] data_sr;
reg [ 7: 0][9:0] data_prev;


reg powerCycleWaitFlag = 1'b1;
reg syncFlag = 1'b0;

// SERIAL CLOCK AND COMMANDS:
// Serial commands (Address is 8 bits, register contents are 16 bits)
//parameter software_reset	= 8'b00000000;//0000000000000001; // software reset
//parameter set_tgc_reg 		= 8'b00000000;//0000000000000100; // enable acccess to TGC registers
//parameter set_fixed_gain 	= 8'b10011001;//0000000000001000; // 
//parameter set_coarse_gain 	= 8'b10011010;//0000000000001100; //
//parameter set_fine_gain		= 8'b10011001;

wand [7:0] adc_state;
assign adc_state[0] = adc_control_comm[0]; assign adc_state[0] = adc_control_comm[7]; 
assign adc_state[1] = adc_control_comm[1]; assign adc_state[1] = adc_control_comm[7]; 
assign adc_state[2] = adc_control_comm[2]; assign adc_state[2] = adc_control_comm[7]; 
assign adc_state[3] = adc_control_comm[3]; assign adc_state[3] = adc_control_comm[7]; 
assign adc_state[4] = adc_control_comm[4]; assign adc_state[4] = adc_control_comm[7]; 
assign adc_state[5] = adc_control_comm[5]; assign adc_state[5] = adc_control_comm[7]; 
assign adc_state[6] = adc_control_comm[6]; assign adc_state[6] = adc_control_comm[7]; 
assign adc_state[7] = adc_control_comm[7]; 

reg [23:0] cmd_buff = 24'b0; // initialize the buffer to software reset
reg [4:0] senCnt = 5'b0;
reg [31:0] reset_counter = 32'b0;

reg interruptInUseFlag = 1'b0;

parameter [7:0] power_off = 8'b00000000;
parameter [7:0] power_on  = 8'b10000000;

parameter [7:0] buffer_serial_command 	= 8'b10000001;
parameter [7:0] issue_serial_command 	= 8'b10000010;
parameter [7:0] sync_adc 					= 8'b10000100;
parameter [7:0] set_interrupt				= 8'b10001000;
parameter [7:0] unset_interrupt			= 8'b10010000;


always @(posedge ref_frame_clk)
begin

	if ( !iStateReset )
	begin
		if ( !trig_received_flag && iTxTrigger )
		begin
			oTriggerAck <= 1'b1;
			trig_received_flag <= 2'b11;
			waddr_cntr <= 13'b0;
			write_complete_flag <= 1'b0;
			txReadyTimeout <= 32'b0;
		end
		else if ( trig_received_flag[1] && !iTxTrigger )
		begin
			trig_received_flag[1]<=1'b0;
			txReadyTimeout <= txReadyTimeout + 1'b1;
		end
		else if ( !trig_received_flag && !iTxTrigger && txReadyTimeout[31] && write_complete_flag )
		begin
			oWREN[2:0] <= 3'b000;
			trig_received_flag <= 2'b00;
			write_complete_flag <= 1'b0;
			waddr_cntr <= 13'b0;
		end
		else
		begin
			txReadyTimeout <= txReadyTimeout + 1'b1;
		end
	
		
		if ( trig_received_flag )
		begin
			if ( !write_complete_flag )
			begin
				if ( oTriggerAck ) oTriggerAck <= 1'b0;
				if ( !oWREN ) oWREN[2:0] <= 3'b111;
				oWAddr <= waddr_cntr;
				
				oADCData[0][11:0] <= data_out[0]; 
				oADCData[0][23:12] <= data_out[1];
				oADCData[0][31:24] <= data_out[6][11:4];
				
				oADCData[1][11:0] <= data_out[2]; 
				oADCData[1][23:12] <= data_out[3];
				oADCData[1][27:24] <= data_out[7][11:8];
				oADCData[1][31:28] <= data_out[6][3:0];
				
				oADCData[2][11:0] <= data_out[4]; 
				oADCData[2][23:12] <= data_out[5];
				oADCData[2][31:24] <= data_out[7][7:0];
			
				waddr_cntr <= waddr_cntr + 1'b1;
				
				if ( waddr_cntr == iRecLength ) // iRecLength
				begin
					oWREN[2:0] <= 3'b000;
					write_complete_flag <= 1'b1;
					trig_received_flag[0] <= 1'b0;
				end
			end
		end
	end
	else // iStateReset == 1
	begin
		oWREN[2:0] <= 3'b000;
		trig_received_flag <= 2'b00;
		write_complete_flag <= 1'b0;
		waddr_cntr <= 13'b0;
	end
end


always @ (negedge ADC_SCLK) //SCLK = 2MHz
begin
	if ( write_complete_flag )
	begin
		interruptInUseFlag <= 1'b1;
		oArmInterrupt <= 8'b00000001;
	end
	else
	begin
		interruptInUseFlag <= 1'b0;
		if ( oArmInterrupt ) oArmInterrupt <= 8'b0;
	end
	
	case( adc_state )
		power_off:
			begin
				if( !ADC_PDN )
				begin
					ADC_PDN <= 1'b1;
					ADC_RESET <= 1'b0;
					ADC_SEN <= 1'b1;
					ADC_SYNC <= 1'b0;
					ADC_SDATA <= 1'b0;	
					syncFlag <= 1'b0;
					powerCycleWaitFlag <= 1'b1;
					reset_counter <= 32'b0;
					senCnt <= 5'b0;
				end
			end
		
		power_on:
			begin
				syncFlag <= 1'b0;
				if ( ADC_PDN )
				begin
					ADC_PDN <= 1'b0;
					ADC_RESET <= 1'b0;
					ADC_SEN <= 1'b1;
					ADC_SYNC <= 1'b0;
					ADC_SDATA <= 1'b0;
					reset_counter <= 32'b0;
					senCnt <= 5'b0;
					powerCycleWaitFlag <= 1'b1;
				end
				else if ( !ADC_PDN & powerCycleWaitFlag )
				begin
					if ( reset_counter[17] ) // --> ~66ms (min. 12ms)
					begin
						ADC_RESET <= ~ADC_RESET;
						if ( ADC_RESET ) powerCycleWaitFlag <= 1'b0;
					end
					reset_counter <= reset_counter + 1'b1;	
				end
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
		
		set_interrupt:
			begin
				if ( !oArmInterrupt & !interruptInUseFlag & !write_complete_flag )
				begin
					oArmInterrupt <= 8'b1;
				end
			end
			
		unset_interrupt:
			begin
				if ( oArmInterrupt & !interruptInUseFlag & !write_complete_flag )
				begin
					oArmInterrupt <= 8'b0;
				end
			end
			
		default:
			begin
				if( !ADC_PDN | !ADC_SEN )
				begin
					ADC_PDN <= 1'b1;
					ADC_RESET <= 1'b0;
					ADC_SEN <= 1'b1;
					ADC_SYNC <= 1'b0;
					ADC_SDATA <= 1'b0;	
					syncFlag <= 1'b0;
					powerCycleWaitFlag <= 1'b1;
					reset_counter <= 32'b0;
					senCnt <= 5'b0;
				end
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

