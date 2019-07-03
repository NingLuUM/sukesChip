module lvds(
	clk_50,
	frame_clk,
	bit_clk,
	data_out,
	onboard_led,
	start_trigger,
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
	D1,
	D2,
	D3,
	D4,
	D5,
	D6,
	D7,
	D8
);

// 1001 1010 

	// INPUTS/OUTPUTS
	input clk_50;			// FPGA 50Mhz clock
	input bit_clk;			// DCLK
	input frame_clk;		// FCLK
	input start_trigger; // From SW0
	input SDOUT;
	
	// Analog signal inputs
	input D1;
	input D2;
	input D3;
	input D4;
	input D5;	
	input D6;
	input D7;
	input D8;	
	
	// Digital output (only one for now; create more by duplicating the ADC LOGIC section)
	output reg [11:0] data_out;
	
	output onboard_led;
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
	
	wire data_out_h;
	wire data_out_l;
	reg [21:0] data_sr;
	
	//
	
	// SIGNALS:
	reg reset_flag = 1'b0;			// Flag for reset
	reg led_flag = 1'b0;				// LED indicator signal
	reg transaction_flag = 1'b0;	// Flag indicating serial transaction on SDATA
	reg reset = 1'b0;					// Reset signal
	reg clkin = 1'b0;					// Single-ended input data clock signal (clocks frame and bit clock outputs from ADC)
//	wire clkin = clk_50;				// TEST with 50 MHz clock
	reg sclk = 1'b1;					// Serial communication clock	
	reg sen = 1'b1;					// Serial enable signal (held down during transactions)
	reg sdout = 1'b0;					// Serial input FROM ADC
	reg sdata = 1'b0;					// Serial output TO the ADC
	reg pdn = 1'b0;					// Power signal TO the ADC (low = on)
	
	assign onboard_led = led_flag;
	assign WHITE_LED = led_flag;
	assign BLUE_LED = 1'b1;
	assign GREEN_LED = pdn;
	
	assign RESET = reset;
	assign CLKINP = clkin;
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
	reg [10:0] trig_cntr;

	// MAIN CLOCK: 
	
	// Main clock (50Mhz) (20ns)
	always @ (posedge clk_50)
	begin
	
		// 12.5 MHz Data Clock
		if (clkin_count < 1) 
		begin
			clkin_count <= clkin_count + 1;
		end
		else
		begin
			clkin_count <= 0;
			clkin <= ~clkin;
		end
	
		// Data rate clock (25 MHz)
//		clkin <= ~clkin;
		
		
		// Handle start trigger
		if (~start_trigger)
		begin
			trig_cntr <= trig_cntr + 1;
			if (trig_cntr > 10)
			begin
				reset_flag <= 1'b1;
			end
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
	parameter set_tgc_reg 				= 24'b00000000_0000000000000100;
	parameter set_fixed_gain 			= 24'b10011001_0000000000001000;
	parameter set_gain 					= 24'b10011010_0000000000010000;
	parameter set_ramp_test_pattern 	= 24'b00000010_1110000000000000; // Ramp test output
	parameter set_sync_test_pattern 	= 24'b00000010_0010000000000000; // [111111000000] test output
	
	reg [23:0] buff = set_tgc_reg;
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
//					buff <= set_tgc_reg;
					end
				else if (msg_count == 1) 
					begin
//					buff <= set_fixed_gain;
					end
				else if (msg_count == 2)
					begin
//					buff <= set_gain;
					end
//				else if (msg_count == 3)
//					begin
//					buff <= set_ramp_test_pattern;
//					end
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
	ddio d1(
		.datain(D2),
		.inclock(bit_clk),
		.dataout_h(data_out_h),
		.dataout_l(data_out_l)
	);
	
	// Shift register
	// Serializes the double data outputs
	always @ (posedge bit_clk)
	begin
//		data_sr <= {data_sr[19:0], data_out_l, data_out_h};
		data_sr[21] <= data_sr[19];
		data_sr[20] <= data_sr[18];
		data_sr[19] <= data_sr[17];
		data_sr[18] <= data_sr[16];
		data_sr[17] <= data_sr[15];
		data_sr[16] <= data_sr[14];
		data_sr[15] <= data_sr[13];
		data_sr[14] <= data_sr[12];
		data_sr[13] <= data_sr[11];
		data_sr[12] <= data_sr[10];
		data_sr[11] <= data_sr[9];
		data_sr[10] <= data_sr[8];
		data_sr[9] <= data_sr[7];
		data_sr[8] <= data_sr[6];
		data_sr[7] <= data_sr[5];
		data_sr[6] <= data_sr[4];
		data_sr[5] <= data_sr[3];
		data_sr[4] <= data_sr[2];
		data_sr[3] <= data_sr[1];
		data_sr[2] <= data_sr[0];
		data_sr[1] <= data_out_l;
		data_sr[0] <= data_out_h;
	end
	
	always @ (posedge frame_clk)
	begin
		// Barrel shift right by 3 bits and reverse to fix signal
		//data_out <= {data_sr[9], data_sr[10], data_sr[11], data_sr[0], data_sr[1], data_sr[2], data_sr[3], data_sr[4], data_sr[5], data_sr[6], data_sr[7], data_sr[8]};
		data_out <= data_sr[21:10];
	end
	
	//
endmodule
