

import socket
import struct
import time
import select
import numpy as np
import sysv_ipc

void define_subprogram1(int prognum){
	extern TXsys *TX;
	
}

fpga_program = """

define_subprogram(1)  
// implicit: a_wait_for_sync(); // daughter
// OR 
// implicit: a_issue_sync(); // master

	a_wait_for_external_trig(); // master only
	// implicit: a_wait_for_sync(); // daughter

	a_fire().set_charge_time(500); // daughter only
	
	a_recv(); // master + daughter
	
	a_set_trig(15); // master only	
	a_set_led(10); // master only

	a_wait(100); // both
	
	a_set_trig(0);
	a_set_led(0);
	
	a_start_loop(1,100); // both
		
		a_fireFromBuffer(0).set_charge_time(100); // daughter only
		
		a_wait(100);
		
		a_set_led(10);
		a_set_trig(10);
		
		a_wait(100);
		
		a_set_trig(0);
		a_set_led(0);
		
	a_end_loop(1);
	
	a_wait(1000);

end_subprogram(1) 
// implicit: a_set_tx_interrupt();
// implicit: a_end_program();





// instr[0] = wait_for_sync_signal(); ALWAYS

for(int plsn = 0; plsn<100; plsn++){
	for(int loc=0; loc<100; loc++){	
	
		set_steering_loc(loc);
		recv(loc,plsn); // recv_now();
			
		execute_subprogram(0);
		
		// implicit: sync();
		// implicit: DREF32(WAIT_FOR_ME) = 1
		
	}	
}

"""



		






















