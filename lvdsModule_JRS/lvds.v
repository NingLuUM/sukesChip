module lvds(
	clk_50,
	adcCLK,
	frame_clk,
	bit_clk,
	data_out,
	start_trigger,
	iTxTrigger,
	RESET,
	CLKINP,
	SCLK,
	SDATA,
	SDOUT,
	SEN,
	PDN,
	WHITE_LED,
	BLUE_LED,
	GREEN_LED,
	LVDS_DATA_LINES
);

// 1001 1010 

// INPUTS/OUTPUTS
input clk_50;			// FPGA 50Mhz clock
input adcCLK;
input bit_clk;			// DCLK
input frame_clk;		// FCLK
input start_trigger; // From SW0
input SDOUT;
input iTxTrigger;

// Analog signal inputs
input [7:0] LVDS_DATA_LINES;	

// Digital output (only one for now; create more by duplicating the ADC LOGIC section)
output reg [7:0][11:0] data_out;

output RESET;
output CLKINP;
output SCLK;
output SDATA;
output SEN;
output PDN;

// EVM board LEDs
output WHITE_LED;
output BLUE_LED;
output GREEN_LED;

wire [ 7: 0] data_out_h;
wire [ 7: 0] data_out_l;
reg [ 7: 0][11:0] data_sr;
reg [ 7: 0][9:0] data_prev;
//

// SIGNALS:
reg reset_flag = 1'b0;			// Flag for reset
reg led_flag = 1'b0;				// LED indicator signal
reg transaction_flag = 1'b0;	// Flag indicating serial transaction on SDATA
reg reset = 1'b0;					// Reset signal
reg clkin = 1'b0;					// Single-ended input data clock signal (clocks frame and bit clock outputs from ADC)

reg sclk = 1'b1;					// Serial communication clock	
reg sen = 1'b1;					// Serial enable signal (held down during transactions)
reg sdout = 1'b0;					// Serial input FROM ADC
reg sdata = 1'b0;					// Serial output TO the ADC
reg pdn = 1'b0;					// Power signal TO the ADC (low = on)


assign WHITE_LED = led_flag;
assign BLUE_LED = 1'b1;
assign GREEN_LED = pdn;

assign RESET = reset;
assign CLKINP = adcCLK;
assign SCLK = sclk;
assign SEN = sen;
assign SDATA = sdata;	
assign PDN = pdn;

// Counters
reg [31:0] reset_count = 0;
reg [31:0] led_count = 0;
reg [31:0] clkin_count = 0;
reg [31:0] sclk_count = 0;
reg [31:0] sclk_cycles = 0;
reg [31:0] seconds_on = 0;

// MAIN CLOCK: 

// Main clock (50Mhz) (20ns)
always @ (posedge clk_50)
begin

	// Handle start trigger
	if ( start_trigger | iTxTrigger )
	begin
		reset_flag <= 1'b1;
	end
	

	if (reset_flag == 1'b1)
	begin
		// Send reset pulse
		if (reset_count < 10)
		begin
			reset_count <= reset_count + 1;
			reset <= 1'b1;
		end
		else 
		begin
			reset <= 1'b0;
		end
		
		// Set SCLK (10Mhz) (100ns) THIS IS NOT 10 MHz!
		if (sclk_count < 5)
		begin
			sclk_count <= sclk_count + 1;
		end
		else
		begin
			sclk_count <= 0;
			sclk <= ~sclk;
		end

		// LED indicator for successful programming (1s up, 1s down)
		if (led_count < 50000000)
		begin
			led_count <= led_count + 1;
		end
		else 
		begin
			led_flag <= ~led_flag;
			led_count <= 0;
			seconds_on <= seconds_on + 1;
		end
		
		// Power down after 5 seconds transitions
		if (seconds_on >= 5)
		begin
			pdn <= 1'b1;
		end
	
	end
end
//

// SERIAL CLOCK AND COMMANDS:

// Serial commands (Address is 8 bits, register contents are 16 bits)
parameter software_reset			= 24'b000000000000000000000001; // software reset
parameter set_tgc_reg 				= 24'b000000000000000000000100; // enable acccess to TGC registers
parameter set_fixed_gain 			= 24'b100110010000000000001000; // 
parameter set_gain 					= 24'b100110100000000000001100; //
parameter set_ramp_test_pattern 	= 24'b000000101110000000000000; // Ramp test output
parameter set_sync_test_pattern 	= 24'b000000100010000000000000; // [111111000000] test output

reg [23:0] buff = software_reset; // initialize the buffer to software reset
reg [31:0] cycles = 0;

// Serial clock (10mhz) (100ns)
always @ (posedge sclk)
begin
	sclk_cycles <= sclk_cycles + 1;
	
	if (sclk_cycles > 23)
	begin
		sclk_cycles <= 0;
		transaction_flag <= ~transaction_flag;
		cycles <= cycles + 1;
	end
end

// SDATA bits should occur at falling edge
reg [31:0] msg_count = 0;
always @ (negedge sclk)
begin
	if (transaction_flag)
	begin
		sen <= 1'b0;
		sdata <= buff[23];
		buff <= {buff[22:0], 1'b0};
	end
	else
	begin
		sen <= 1'b1;
		sdata <= 1'b0;
			if (msg_count == 0)
			begin
				buff <= software_reset;
			end
			else if (msg_count == 1)
			begin
				buff <= set_tgc_reg; //set_tgc_reg;
			end
			else if (msg_count == 2) 
			begin
				buff <= set_fixed_gain;
			end
			else if (msg_count == 3)
			begin
				buff <= set_gain;
			end
	end
end
//

always @ (negedge sen) 
begin
	msg_count <= msg_count + 1;
end


// ADC LOGIC:
// Double-data rate register
// Converts the LVDS input to parallel double data outputs
ddio d0(
	.datain(LVDS_DATA_LINES),
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
	//data_out <= {data_sr[9], data_sr[10], data_sr[11], data_sr[0], data_sr[1], data_sr[2], data_sr[3], data_sr[4], data_sr[5], data_sr[6], data_sr[7], data_sr[8]};
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
