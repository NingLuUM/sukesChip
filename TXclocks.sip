set_global_assignment -entity "TXclocks" -library "lib_TXclocks" -name IP_TOOL_NAME "altera_pll"
set_global_assignment -entity "TXclocks" -library "lib_TXclocks" -name IP_TOOL_VERSION "17.1"
set_global_assignment -entity "TXclocks" -library "lib_TXclocks" -name IP_TOOL_ENV "mwpim"
set_global_assignment -library "lib_TXclocks" -name SPD_FILE [file join $::quartus(sip_path) "TXclocks.spd"]

set_global_assignment -library "lib_TXclocks" -name MISC_FILE [file join $::quartus(sip_path) "TXclocks_sim/TXclocks.vo"]
