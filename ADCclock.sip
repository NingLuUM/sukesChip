set_global_assignment -entity "ADCclock" -library "lib_ADCclock" -name IP_TOOL_NAME "altera_pll"
set_global_assignment -entity "ADCclock" -library "lib_ADCclock" -name IP_TOOL_VERSION "17.1"
set_global_assignment -entity "ADCclock" -library "lib_ADCclock" -name IP_TOOL_ENV "mwpim"
set_global_assignment -library "lib_ADCclock" -name SPD_FILE [file join $::quartus(sip_path) "ADCclock.spd"]

set_global_assignment -library "lib_ADCclock" -name MISC_FILE [file join $::quartus(sip_path) "ADCclock_sim/ADCclock.vo"]
