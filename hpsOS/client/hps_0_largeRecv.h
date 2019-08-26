#ifndef _ALTERA_HPS_0_LARGERECV_H_
#define _ALTERA_HPS_0_LARGERECV_H_

/*
 * This file was automatically generated by the swinfo2header utility.
 * 
 * Created from SOPC Builder system 'soc_system' in
 * file 'soc_system.sopcinfo'.
 */

/*
 * This file contains macros for module 'hps_0' and devices
 * connected to the following masters:
 *   h2f_axi_master
 *   h2f_lw_axi_master
 * 
 * Do not include this header file and another header file created for a
 * different module or master group at the same time.
 * Doing so may result in duplicate macro names.
 * Instead, use the system header file which has macros with unique names.
 */

/*
 * Macros for device 'adc_ramBank0', class 'altera_avalon_onchip_memory2'
 * The macros are prefixed with 'ADC_RAMBANK0_'.
 * The prefix is the slave descriptor.
 */
#define ADC_RAMBANK0_COMPONENT_TYPE altera_avalon_onchip_memory2
#define ADC_RAMBANK0_COMPONENT_NAME adc_ramBank0
#define ADC_RAMBANK0_BASE 0x0
#define ADC_RAMBANK0_SPAN 262144
#define ADC_RAMBANK0_END 0x3ffff
#define ADC_RAMBANK0_ALLOW_IN_SYSTEM_MEMORY_CONTENT_EDITOR 0
#define ADC_RAMBANK0_ALLOW_MRAM_SIM_CONTENTS_ONLY_FILE 0
#define ADC_RAMBANK0_CONTENTS_INFO ""
#define ADC_RAMBANK0_DUAL_PORT 1
#define ADC_RAMBANK0_GUI_RAM_BLOCK_TYPE AUTO
#define ADC_RAMBANK0_INIT_CONTENTS_FILE soc_system_adc_ramBank0
#define ADC_RAMBANK0_INIT_MEM_CONTENT 1
#define ADC_RAMBANK0_INSTANCE_ID NONE
#define ADC_RAMBANK0_NON_DEFAULT_INIT_FILE_ENABLED 0
#define ADC_RAMBANK0_RAM_BLOCK_TYPE AUTO
#define ADC_RAMBANK0_READ_DURING_WRITE_MODE DONT_CARE
#define ADC_RAMBANK0_SINGLE_CLOCK_OP 0
#define ADC_RAMBANK0_SIZE_MULTIPLE 1
#define ADC_RAMBANK0_SIZE_VALUE 262144
#define ADC_RAMBANK0_WRITABLE 1
#define ADC_RAMBANK0_MEMORY_INFO_DAT_SYM_INSTALL_DIR SIM_DIR
#define ADC_RAMBANK0_MEMORY_INFO_GENERATE_DAT_SYM 1
#define ADC_RAMBANK0_MEMORY_INFO_GENERATE_HEX 1
#define ADC_RAMBANK0_MEMORY_INFO_HAS_BYTE_LANE 0
#define ADC_RAMBANK0_MEMORY_INFO_HEX_INSTALL_DIR QPF_DIR
#define ADC_RAMBANK0_MEMORY_INFO_MEM_INIT_DATA_WIDTH 64
#define ADC_RAMBANK0_MEMORY_INFO_MEM_INIT_FILENAME soc_system_adc_ramBank0

/*
 * Macros for device 'data_ready', class 'altera_avalon_pio'
 * The macros are prefixed with 'DATA_READY_'.
 * The prefix is the slave descriptor.
 */
#define DATA_READY_COMPONENT_TYPE altera_avalon_pio
#define DATA_READY_COMPONENT_NAME data_ready
#define DATA_READY_BASE 0x0
#define DATA_READY_SPAN 16
#define DATA_READY_END 0xf
#define DATA_READY_IRQ 0
#define DATA_READY_BIT_CLEARING_EDGE_REGISTER 1
#define DATA_READY_BIT_MODIFYING_OUTPUT_REGISTER 0
#define DATA_READY_CAPTURE 1
#define DATA_READY_DATA_WIDTH 8
#define DATA_READY_DO_TEST_BENCH_WIRING 0
#define DATA_READY_DRIVEN_SIM_VALUE 0
#define DATA_READY_EDGE_TYPE RISING
#define DATA_READY_FREQ 50000000
#define DATA_READY_HAS_IN 1
#define DATA_READY_HAS_OUT 0
#define DATA_READY_HAS_TRI 0
#define DATA_READY_IRQ_TYPE EDGE
#define DATA_READY_RESET_VALUE 255

/*
 * Macros for device 'pio_led', class 'altera_avalon_pio'
 * The macros are prefixed with 'PIO_LED_'.
 * The prefix is the slave descriptor.
 */
#define PIO_LED_COMPONENT_TYPE altera_avalon_pio
#define PIO_LED_COMPONENT_NAME pio_led
#define PIO_LED_BASE 0x50
#define PIO_LED_SPAN 16
#define PIO_LED_END 0x5f
#define PIO_LED_BIT_CLEARING_EDGE_REGISTER 0
#define PIO_LED_BIT_MODIFYING_OUTPUT_REGISTER 0
#define PIO_LED_CAPTURE 0
#define PIO_LED_DATA_WIDTH 8
#define PIO_LED_DO_TEST_BENCH_WIRING 0
#define PIO_LED_DRIVEN_SIM_VALUE 0
#define PIO_LED_EDGE_TYPE NONE
#define PIO_LED_FREQ 50000000
#define PIO_LED_HAS_IN 0
#define PIO_LED_HAS_OUT 1
#define PIO_LED_HAS_TRI 0
#define PIO_LED_IRQ_TYPE NONE
#define PIO_LED_RESET_VALUE 255

/*
 * Macros for device 'pio_var_gain_setting', class 'altera_avalon_pio'
 * The macros are prefixed with 'PIO_VAR_GAIN_SETTING_'.
 * The prefix is the slave descriptor.
 */
#define PIO_VAR_GAIN_SETTING_COMPONENT_TYPE altera_avalon_pio
#define PIO_VAR_GAIN_SETTING_COMPONENT_NAME pio_var_gain_setting
#define PIO_VAR_GAIN_SETTING_BASE 0x60
#define PIO_VAR_GAIN_SETTING_SPAN 16
#define PIO_VAR_GAIN_SETTING_END 0x6f
#define PIO_VAR_GAIN_SETTING_BIT_CLEARING_EDGE_REGISTER 0
#define PIO_VAR_GAIN_SETTING_BIT_MODIFYING_OUTPUT_REGISTER 0
#define PIO_VAR_GAIN_SETTING_CAPTURE 0
#define PIO_VAR_GAIN_SETTING_DATA_WIDTH 8
#define PIO_VAR_GAIN_SETTING_DO_TEST_BENCH_WIRING 0
#define PIO_VAR_GAIN_SETTING_DRIVEN_SIM_VALUE 0
#define PIO_VAR_GAIN_SETTING_EDGE_TYPE NONE
#define PIO_VAR_GAIN_SETTING_FREQ 50000000
#define PIO_VAR_GAIN_SETTING_HAS_IN 0
#define PIO_VAR_GAIN_SETTING_HAS_OUT 1
#define PIO_VAR_GAIN_SETTING_HAS_TRI 0
#define PIO_VAR_GAIN_SETTING_IRQ_TYPE NONE
#define PIO_VAR_GAIN_SETTING_RESET_VALUE 255

/*
 * Macros for device 'pio_adc_serial_command', class 'altera_avalon_pio'
 * The macros are prefixed with 'PIO_ADC_SERIAL_COMMAND_'.
 * The prefix is the slave descriptor.
 */
#define PIO_ADC_SERIAL_COMMAND_COMPONENT_TYPE altera_avalon_pio
#define PIO_ADC_SERIAL_COMMAND_COMPONENT_NAME pio_adc_serial_command
#define PIO_ADC_SERIAL_COMMAND_BASE 0x80
#define PIO_ADC_SERIAL_COMMAND_SPAN 16
#define PIO_ADC_SERIAL_COMMAND_END 0x8f
#define PIO_ADC_SERIAL_COMMAND_BIT_CLEARING_EDGE_REGISTER 0
#define PIO_ADC_SERIAL_COMMAND_BIT_MODIFYING_OUTPUT_REGISTER 0
#define PIO_ADC_SERIAL_COMMAND_CAPTURE 0
#define PIO_ADC_SERIAL_COMMAND_DATA_WIDTH 24
#define PIO_ADC_SERIAL_COMMAND_DO_TEST_BENCH_WIRING 0
#define PIO_ADC_SERIAL_COMMAND_DRIVEN_SIM_VALUE 0
#define PIO_ADC_SERIAL_COMMAND_EDGE_TYPE NONE
#define PIO_ADC_SERIAL_COMMAND_FREQ 50000000
#define PIO_ADC_SERIAL_COMMAND_HAS_IN 0
#define PIO_ADC_SERIAL_COMMAND_HAS_OUT 1
#define PIO_ADC_SERIAL_COMMAND_HAS_TRI 0
#define PIO_ADC_SERIAL_COMMAND_IRQ_TYPE NONE
#define PIO_ADC_SERIAL_COMMAND_RESET_VALUE 0

/*
 * Macros for device 'pio_adc_control_comms', class 'altera_avalon_pio'
 * The macros are prefixed with 'PIO_ADC_CONTROL_COMMS_'.
 * The prefix is the slave descriptor.
 */
#define PIO_ADC_CONTROL_COMMS_COMPONENT_TYPE altera_avalon_pio
#define PIO_ADC_CONTROL_COMMS_COMPONENT_NAME pio_adc_control_comms
#define PIO_ADC_CONTROL_COMMS_BASE 0x90
#define PIO_ADC_CONTROL_COMMS_SPAN 16
#define PIO_ADC_CONTROL_COMMS_END 0x9f
#define PIO_ADC_CONTROL_COMMS_BIT_CLEARING_EDGE_REGISTER 0
#define PIO_ADC_CONTROL_COMMS_BIT_MODIFYING_OUTPUT_REGISTER 0
#define PIO_ADC_CONTROL_COMMS_CAPTURE 0
#define PIO_ADC_CONTROL_COMMS_DATA_WIDTH 8
#define PIO_ADC_CONTROL_COMMS_DO_TEST_BENCH_WIRING 0
#define PIO_ADC_CONTROL_COMMS_DRIVEN_SIM_VALUE 0
#define PIO_ADC_CONTROL_COMMS_EDGE_TYPE NONE
#define PIO_ADC_CONTROL_COMMS_FREQ 50000000
#define PIO_ADC_CONTROL_COMMS_HAS_IN 0
#define PIO_ADC_CONTROL_COMMS_HAS_OUT 1
#define PIO_ADC_CONTROL_COMMS_HAS_TRI 0
#define PIO_ADC_CONTROL_COMMS_IRQ_TYPE NONE
#define PIO_ADC_CONTROL_COMMS_RESET_VALUE 0

/*
 * Macros for device 'pio_adc_fpga_state_reset', class 'altera_avalon_pio'
 * The macros are prefixed with 'PIO_ADC_FPGA_STATE_RESET_'.
 * The prefix is the slave descriptor.
 */
#define PIO_ADC_FPGA_STATE_RESET_COMPONENT_TYPE altera_avalon_pio
#define PIO_ADC_FPGA_STATE_RESET_COMPONENT_NAME pio_adc_fpga_state_reset
#define PIO_ADC_FPGA_STATE_RESET_BASE 0x100
#define PIO_ADC_FPGA_STATE_RESET_SPAN 16
#define PIO_ADC_FPGA_STATE_RESET_END 0x10f
#define PIO_ADC_FPGA_STATE_RESET_BIT_CLEARING_EDGE_REGISTER 0
#define PIO_ADC_FPGA_STATE_RESET_BIT_MODIFYING_OUTPUT_REGISTER 0
#define PIO_ADC_FPGA_STATE_RESET_CAPTURE 0
#define PIO_ADC_FPGA_STATE_RESET_DATA_WIDTH 1
#define PIO_ADC_FPGA_STATE_RESET_DO_TEST_BENCH_WIRING 0
#define PIO_ADC_FPGA_STATE_RESET_DRIVEN_SIM_VALUE 0
#define PIO_ADC_FPGA_STATE_RESET_EDGE_TYPE NONE
#define PIO_ADC_FPGA_STATE_RESET_FREQ 50000000
#define PIO_ADC_FPGA_STATE_RESET_HAS_IN 0
#define PIO_ADC_FPGA_STATE_RESET_HAS_OUT 1
#define PIO_ADC_FPGA_STATE_RESET_HAS_TRI 0
#define PIO_ADC_FPGA_STATE_RESET_IRQ_TYPE NONE
#define PIO_ADC_FPGA_STATE_RESET_RESET_VALUE 0

/*
 * Macros for device 'pio_set_adc_record_length', class 'altera_avalon_pio'
 * The macros are prefixed with 'PIO_SET_ADC_RECORD_LENGTH_'.
 * The prefix is the slave descriptor.
 */
#define PIO_SET_ADC_RECORD_LENGTH_COMPONENT_TYPE altera_avalon_pio
#define PIO_SET_ADC_RECORD_LENGTH_COMPONENT_NAME pio_set_adc_record_length
#define PIO_SET_ADC_RECORD_LENGTH_BASE 0x110
#define PIO_SET_ADC_RECORD_LENGTH_SPAN 16
#define PIO_SET_ADC_RECORD_LENGTH_END 0x11f
#define PIO_SET_ADC_RECORD_LENGTH_BIT_CLEARING_EDGE_REGISTER 0
#define PIO_SET_ADC_RECORD_LENGTH_BIT_MODIFYING_OUTPUT_REGISTER 0
#define PIO_SET_ADC_RECORD_LENGTH_CAPTURE 0
#define PIO_SET_ADC_RECORD_LENGTH_DATA_WIDTH 16
#define PIO_SET_ADC_RECORD_LENGTH_DO_TEST_BENCH_WIRING 0
#define PIO_SET_ADC_RECORD_LENGTH_DRIVEN_SIM_VALUE 0
#define PIO_SET_ADC_RECORD_LENGTH_EDGE_TYPE NONE
#define PIO_SET_ADC_RECORD_LENGTH_FREQ 50000000
#define PIO_SET_ADC_RECORD_LENGTH_HAS_IN 0
#define PIO_SET_ADC_RECORD_LENGTH_HAS_OUT 1
#define PIO_SET_ADC_RECORD_LENGTH_HAS_TRI 0
#define PIO_SET_ADC_RECORD_LENGTH_IRQ_TYPE NONE
#define PIO_SET_ADC_RECORD_LENGTH_RESET_VALUE 0

/*
 * Macros for device 'sysid_qsys', class 'altera_avalon_sysid_qsys'
 * The macros are prefixed with 'SYSID_QSYS_'.
 * The prefix is the slave descriptor.
 */
#define SYSID_QSYS_COMPONENT_TYPE altera_avalon_sysid_qsys
#define SYSID_QSYS_COMPONENT_NAME sysid_qsys
#define SYSID_QSYS_BASE 0x10000
#define SYSID_QSYS_SPAN 8
#define SYSID_QSYS_END 0x10007
#define SYSID_QSYS_ID 2899645186
#define SYSID_QSYS_TIMESTAMP 1566822507

/*
 * Macros for device 'jtag_uart', class 'altera_avalon_jtag_uart'
 * The macros are prefixed with 'JTAG_UART_'.
 * The prefix is the slave descriptor.
 */
#define JTAG_UART_COMPONENT_TYPE altera_avalon_jtag_uart
#define JTAG_UART_COMPONENT_NAME jtag_uart
#define JTAG_UART_BASE 0x20000
#define JTAG_UART_SPAN 8
#define JTAG_UART_END 0x20007
#define JTAG_UART_IRQ 2
#define JTAG_UART_READ_DEPTH 64
#define JTAG_UART_READ_THRESHOLD 8
#define JTAG_UART_WRITE_DEPTH 64
#define JTAG_UART_WRITE_THRESHOLD 8

/*
 * Macros for device 'adc_ramBank1', class 'altera_avalon_onchip_memory2'
 * The macros are prefixed with 'ADC_RAMBANK1_'.
 * The prefix is the slave descriptor.
 */
#define ADC_RAMBANK1_COMPONENT_TYPE altera_avalon_onchip_memory2
#define ADC_RAMBANK1_COMPONENT_NAME adc_ramBank1
#define ADC_RAMBANK1_BASE 0x40000
#define ADC_RAMBANK1_SPAN 131072
#define ADC_RAMBANK1_END 0x5ffff
#define ADC_RAMBANK1_ALLOW_IN_SYSTEM_MEMORY_CONTENT_EDITOR 0
#define ADC_RAMBANK1_ALLOW_MRAM_SIM_CONTENTS_ONLY_FILE 0
#define ADC_RAMBANK1_CONTENTS_INFO ""
#define ADC_RAMBANK1_DUAL_PORT 1
#define ADC_RAMBANK1_GUI_RAM_BLOCK_TYPE AUTO
#define ADC_RAMBANK1_INIT_CONTENTS_FILE soc_system_adc_ramBank1
#define ADC_RAMBANK1_INIT_MEM_CONTENT 1
#define ADC_RAMBANK1_INSTANCE_ID NONE
#define ADC_RAMBANK1_NON_DEFAULT_INIT_FILE_ENABLED 0
#define ADC_RAMBANK1_RAM_BLOCK_TYPE AUTO
#define ADC_RAMBANK1_READ_DURING_WRITE_MODE DONT_CARE
#define ADC_RAMBANK1_SINGLE_CLOCK_OP 0
#define ADC_RAMBANK1_SIZE_MULTIPLE 1
#define ADC_RAMBANK1_SIZE_VALUE 131072
#define ADC_RAMBANK1_WRITABLE 1
#define ADC_RAMBANK1_MEMORY_INFO_DAT_SYM_INSTALL_DIR SIM_DIR
#define ADC_RAMBANK1_MEMORY_INFO_GENERATE_DAT_SYM 1
#define ADC_RAMBANK1_MEMORY_INFO_GENERATE_HEX 1
#define ADC_RAMBANK1_MEMORY_INFO_HAS_BYTE_LANE 0
#define ADC_RAMBANK1_MEMORY_INFO_HEX_INSTALL_DIR QPF_DIR
#define ADC_RAMBANK1_MEMORY_INFO_MEM_INIT_DATA_WIDTH 32
#define ADC_RAMBANK1_MEMORY_INFO_MEM_INIT_FILENAME soc_system_adc_ramBank1


#endif /* _ALTERA_HPS_0_LARGERECV_H_ */
