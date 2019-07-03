if ( !txIsLoopEndPoint ) begin
	otxReadAddr <= ( otxReadAddr + 1'b1 );
end else begin
	if ( loopEndAddr[0] && ( loopEndAddr[0] == otxReadAddr ) ) begin
		if ( loopCounter[0] > 1 ) begin
			currentlyInLoop[0] <= 1;
			loopCounter[0] <= ( loopCounter[0] - 1'b1 );
			otxReadAddr <= loopStartAddr[0];
		end else begin
			currentlyInLoop[0] <= 0;
			loopCounter[0] <= loopCounterRef[0];
			otxReadAddr <= ( otxReadAddr + 1'b1 );
		end

	end else if ( loopEndAddr[1] && ( loopEndAddr[1] == otxReadAddr ) ) begin
		if ( loopCounter[1] > 1 ) begin
			currentlyInLoop[1] <= 1;
			loopCounter[1] <= ( loopCounter[1] - 1'b1 );
			otxReadAddr <= loopStartAddr[1];
		end else begin
			currentlyInLoop[1] <= 0;
			loopCounter[1] <= loopCounterRef[1];
			otxReadAddr <= ( otxReadAddr + 1'b1 );
		end

	end else if ( loopEndAddr[2] && ( loopEndAddr[2] == otxReadAddr ) ) begin
		if ( loopCounter[2] > 1 ) begin
			currentlyInLoop[2] <= 1;
			loopCounter[2] <= ( loopCounter[2] - 1'b1 );
			otxReadAddr <= loopStartAddr[2];
		end else begin
			currentlyInLoop[2] <= 0;
			loopCounter[2] <= loopCounterRef[2];
			otxReadAddr <= ( otxReadAddr + 1'b1 );
		end

	end else if ( loopEndAddr[3] && ( loopEndAddr[3] == otxReadAddr ) ) begin
		if ( loopCounter[3] > 1 ) begin
			currentlyInLoop[3] <= 1;
			loopCounter[3] <= ( loopCounter[3] - 1'b1 );
			otxReadAddr <= loopStartAddr[3];
		end else begin
			currentlyInLoop[3] <= 0;
			loopCounter[3] <= loopCounterRef[3];
			otxReadAddr <= ( otxReadAddr + 1'b1 );
		end

	end else if ( loopEndAddr[4] && ( loopEndAddr[4] == otxReadAddr ) ) begin
		if ( loopCounter[4] > 1 ) begin
			currentlyInLoop[4] <= 1;
			loopCounter[4] <= ( loopCounter[4] - 1'b1 );
			otxReadAddr <= loopStartAddr[4];
		end else begin
			currentlyInLoop[4] <= 0;
			loopCounter[4] <= loopCounterRef[4];
			otxReadAddr <= ( otxReadAddr + 1'b1 );
		end

	end else if ( loopEndAddr[5] && ( loopEndAddr[5] == otxReadAddr ) ) begin
		if ( loopCounter[5] > 1 ) begin
			currentlyInLoop[5] <= 1;
			loopCounter[5] <= ( loopCounter[5] - 1'b1 );
			otxReadAddr <= loopStartAddr[5];
		end else begin
			currentlyInLoop[5] <= 0;
			loopCounter[5] <= loopCounterRef[5];
			otxReadAddr <= ( otxReadAddr + 1'b1 );
		end

	end else if ( loopEndAddr[6] && ( loopEndAddr[6] == otxReadAddr ) ) begin
		if ( loopCounter[6] > 1 ) begin
			currentlyInLoop[6] <= 1;
			loopCounter[6] <= ( loopCounter[6] - 1'b1 );
			otxReadAddr <= loopStartAddr[6];
		end else begin
			currentlyInLoop[6] <= 0;
			loopCounter[6] <= loopCounterRef[6];
			otxReadAddr <= ( otxReadAddr + 1'b1 );
		end

	end else if ( loopEndAddr[7] && ( loopEndAddr[7] == otxReadAddr ) ) begin
		if ( loopCounter[7] > 1 ) begin
			currentlyInLoop[7] <= 1;
			loopCounter[7] <= ( loopCounter[7] - 1'b1 );
			otxReadAddr <= loopStartAddr[7];
		end else begin
			currentlyInLoop[7] <= 0;
			loopCounter[7] <= loopCounterRef[7];
			otxReadAddr <= ( otxReadAddr + 1'b1 );
		end

	end else if ( loopEndAddr[8] && ( loopEndAddr[8] == otxReadAddr ) ) begin
		if ( loopCounter[8] > 1 ) begin
			currentlyInLoop[8] <= 1;
			loopCounter[8] <= ( loopCounter[8] - 1'b1 );
			otxReadAddr <= loopStartAddr[8];
		end else begin
			currentlyInLoop[8] <= 0;
			loopCounter[8] <= loopCounterRef[8];
			otxReadAddr <= ( otxReadAddr + 1'b1 );
		end

	end else if ( loopEndAddr[9] && ( loopEndAddr[9] == otxReadAddr ) ) begin
		if ( loopCounter[9] > 1 ) begin
			currentlyInLoop[9] <= 1;
			loopCounter[9] <= ( loopCounter[9] - 1'b1 );
			otxReadAddr <= loopStartAddr[9];
		end else begin
			currentlyInLoop[9] <= 0;
			loopCounter[9] <= loopCounterRef[9];
			otxReadAddr <= ( otxReadAddr + 1'b1 );
		end

	end else if ( loopEndAddr[10] && ( loopEndAddr[10] == otxReadAddr ) ) begin
		if ( loopCounter[10] > 1 ) begin
			currentlyInLoop[10] <= 1;
			loopCounter[10] <= ( loopCounter[10] - 1'b1 );
			otxReadAddr <= loopStartAddr[10];
		end else begin
			currentlyInLoop[10] <= 0;
			loopCounter[10] <= loopCounterRef[10];
			otxReadAddr <= ( otxReadAddr + 1'b1 );
		end

	end else if ( loopEndAddr[11] && ( loopEndAddr[11] == otxReadAddr ) ) begin
		if ( loopCounter[11] > 1 ) begin
			currentlyInLoop[11] <= 1;
			loopCounter[11] <= ( loopCounter[11] - 1'b1 );
			otxReadAddr <= loopStartAddr[11];
		end else begin
			currentlyInLoop[11] <= 0;
			loopCounter[11] <= loopCounterRef[11];
			otxReadAddr <= ( otxReadAddr + 1'b1 );
		end

	end else if ( loopEndAddr[12] && ( loopEndAddr[12] == otxReadAddr ) ) begin
		if ( loopCounter[12] > 1 ) begin
			currentlyInLoop[12] <= 1;
			loopCounter[12] <= ( loopCounter[12] - 1'b1 );
			otxReadAddr <= loopStartAddr[12];
		end else begin
			currentlyInLoop[12] <= 0;
			loopCounter[12] <= loopCounterRef[12];
			otxReadAddr <= ( otxReadAddr + 1'b1 );
		end

	end else if ( loopEndAddr[13] && ( loopEndAddr[13] == otxReadAddr ) ) begin
		if ( loopCounter[13] > 1 ) begin
			currentlyInLoop[13] <= 1;
			loopCounter[13] <= ( loopCounter[13] - 1'b1 );
			otxReadAddr <= loopStartAddr[13];
		end else begin
			currentlyInLoop[13] <= 0;
			loopCounter[13] <= loopCounterRef[13];
			otxReadAddr <= ( otxReadAddr + 1'b1 );
		end

	end else if ( loopEndAddr[14] && ( loopEndAddr[14] == otxReadAddr ) ) begin
		if ( loopCounter[14] > 1 ) begin
			currentlyInLoop[14] <= 1;
			loopCounter[14] <= ( loopCounter[14] - 1'b1 );
			otxReadAddr <= loopStartAddr[14];
		end else begin
			currentlyInLoop[14] <= 0;
			loopCounter[14] <= loopCounterRef[14];
			otxReadAddr <= ( otxReadAddr + 1'b1 );
		end

	end else if ( loopEndAddr[15] && ( loopEndAddr[15] == otxReadAddr ) ) begin
		if ( loopCounter[15] > 1 ) begin
			currentlyInLoop[15] <= 1;
			loopCounter[15] <= ( loopCounter[15] - 1'b1 );
			otxReadAddr <= loopStartAddr[15];
		end else begin
			currentlyInLoop[15] <= 0;
			loopCounter[15] <= loopCounterRef[15];
			otxReadAddr <= ( otxReadAddr + 1'b1 );
		end

	end else begin
		errorState0 <= 0;
		errorState1 <= loopEndAddr_isLoopEndPoint_Mismatch;
		state <= errorDetected;
	end
end
