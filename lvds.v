module lvds(
	lvds_control,
	frame_clk,
	bit_clk,
	data_out,
	lvdsSerCmdAddr,
	lvdsSerCmd,
	RESET,
	SYNC,
	SCLK,
	SDATA,
	SDOUT,
	SEN,
	PDN,
	WHITE_LED,
	BLUE_LED,
	GREEN_LED,
	LVDS_ANALOG_INPUT,
	interrupter
);

// 1001 1010 

// INPUTS/OUTPUTS
input [7:0] lvds_control;

input SCLK;
input bit_clk;			// DCLK
input frame_clk;		// FCLK
input SDOUT;
input [7:0] lvdsSerCmdAddr;
input [15:0] lvdsSerCmd;

// Analog signal inputs
input [7:0] LVDS_ANALOG_INPUT;	

output reg [7:0] interrupter;

// Digital output (only one for now; create more by duplicating the ADC LOGIC section)
output reg [7:0][11:0] data_out;

output reg SYNC;
output reg RESET;
output reg SDATA;
output reg SEN;
output reg PDN;

output reg WHITE_LED;
output reg BLUE_LED;
output reg GREEN_LED;


initial
begin
	PDN = 1'b1;
	RESET = 1'b0;
	SEN = 1'b1;
	SYNC = 1'b0;
	SDATA = 1'b0;	
	WHITE_LED = 1'b1;
	BLUE_LED = 1'b1;
	GREEN_LED = 1'b1;
end


wire [ 7: 0] data_out_h;
wire [ 7: 0] data_out_l;
reg [ 7: 0][11:0] data_sr;
reg [ 7: 0][9:0] data_prev;
//


reg sdout = 1'b0;					// Serial input FROM ADC

reg powerCycleWaitFlag = 1'b1;
reg syncFlag = 1'b0;

// SERIAL CLOCK AND COMMANDS:
// Serial commands (Address is 8 bits, register contents are 16 bits)
//parameter software_reset	= 8'b00000000;//0000000000000001; // software reset
//parameter set_tgc_reg 		= 8'b00000000;//0000000000000100; // enable acccess to TGC registers
//parameter set_fixed_gain 	= 8'b10011001;//0000000000001000; // 
//parameter set_coarse_gain 	= 8'b10011010;//0000000000001100; //
//parameter set_fine_gain		= 8'b10011001;

wand [7:0] lvds_state;
assign lvds_state[0] = lvds_control[0]; assign lvds_state[0] = lvds_control[7]; 
assign lvds_state[1] = lvds_control[1]; assign lvds_state[1] = lvds_control[7]; 
assign lvds_state[2] = lvds_control[2]; assign lvds_state[2] = lvds_control[7]; 
assign lvds_state[3] = lvds_control[3]; assign lvds_state[3] = lvds_control[7]; 
assign lvds_state[4] = lvds_control[4]; assign lvds_state[4] = lvds_control[7]; 
assign lvds_state[5] = lvds_control[5]; assign lvds_state[5] = lvds_control[7]; 
assign lvds_state[6] = lvds_control[6]; assign lvds_state[6] = lvds_control[7]; 
assign lvds_state[7] = lvds_control[7]; 

reg [23:0] cmd_buff = 24'b0; // initialize the buffer to software reset
reg [4:0] senCnt = 5'b0;
reg [31:0] reset_counter = 32'b0;

parameter [7:0] power_off = 8'b00000000;
parameter [7:0] power_on  = 8'b10000000;

parameter [7:0] buffer_serial_command 	= 8'b10000001;
parameter [7:0] issue_serial_command 	= 8'b10000010;
parameter [7:0] sync_lvds 					= 8'b10000100;
parameter [7:0] set_interrupt				= 8'b10001000;
parameter [7:0] unset_interrupt			= 8'b10010000;

always @ (negedge SCLK) //SCLK = 1MHz
begin
	case( lvds_state )
		power_off:
			begin
				if( !PDN )
				begin
					PDN <= 1'b1;
					RESET <= 1'b0;
					SEN <= 1'b1;
					SYNC <= 1'b0;
					SDATA <= 1'b0;	
					syncFlag <= 1'b0;
					powerCycleWaitFlag <= 1'b1;
					reset_counter <= 32'b0;
					senCnt <= 5'b0;
					interrupter <= 8'b0;
				end
			end
		
		power_on:
			begin
				syncFlag <= 1'b0;
				if ( PDN )
				begin
					PDN <= 1'b0;
					RESET <= 1'b0;
					SEN <= 1'b1;
					SYNC <= 1'b0;
					SDATA <= 1'b0;
					reset_counter <= 32'b0;
					senCnt <= 5'b0;
					powerCycleWaitFlag <= 1'b1;
					interrupter <= 8'b0;
				end
				else if ( !PDN & powerCycleWaitFlag )
				begin
					if ( reset_counter[17] ) // --> ~66ms (min. 12ms)
					begin
						RESET <= ~RESET;
						if ( RESET ) powerCycleWaitFlag <= 1'b0;
					end
					reset_counter <= reset_counter + 1'b1;	
				end
			end
		
		buffer_serial_command:
			begin
				cmd_buff[23:16] <= lvdsSerCmdAddr;
				cmd_buff[15:0] <= lvdsSerCmd;
				SDATA <= 1'b0;
				senCnt <= 5'b0;
				if ( !SEN ) SEN <= 1'b1;
			end
			
		issue_serial_command:
			begin
				if ( senCnt < 24 )
				begin
					if ( !senCnt ) SEN <= 1'b0;
					SDATA <= cmd_buff[23];
					cmd_buff <= {cmd_buff[22:0],1'b0};
					senCnt <= senCnt + 1'b1;
				end
				else
				begin
					SEN <= 1'b1;
					SDATA <= 1'b0;
				end	
			end
		
		sync_lvds:
			begin
				if ( !syncFlag )
				begin
					SYNC <= ~SYNC;
					if ( SYNC ) syncFlag <= 1'b1;
				end	
			end
		
		set_interrupt:
			begin
				if (!interrupter)
				begin
					interrupter <= 8'b1;
				end
			end
			
		unset_interrupt:
			begin
				if (interrupter)
				begin
					interrupter <= 8'b0;
				end
			end
			
		default:
			begin
				if( !PDN | !SEN )
				begin
					PDN <= 1'b1;
					RESET <= 1'b0;
					SEN <= 1'b1;
					SYNC <= 1'b0;
					SDATA <= 1'b0;	
					syncFlag <= 1'b0;
					powerCycleWaitFlag <= 1'b1;
					reset_counter <= 32'b0;
					senCnt <= 5'b0;
				end
			end
	endcase
end

//lvds_rx_test d1 (
//	.rx_enable,
//	.rx_in,
//	.rx_inclock,
//	.rx_out
//);


// ADC LOGIC:
// Double-data rate register
// Converts the LVDS input to parallel double data outputs
ddio d0(
	.datain(LVDS_ANALOG_INPUT),
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
	
	//
endmodule
