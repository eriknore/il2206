
#        .equ    delaycount,     170 # simulator
        .equ    delaycount,     12000 # DE2-board
        .text                   # Instructions follow
        .global simulated_load  # Makes "simulated_load" globally known

simulated_load:  	beq     r4,r0,fin       # exit outer loop
			
			        movi    r8,delaycount   # delay estimation for 1ms

inner:  			beq     r8,r0,outer     # exit from inner loop

			        subi    r8,r8,1         # decrement inner counter
			        
			        br      inner
        
outer:  			subi    r4,r4,1         # decrement outer counter
			        br      simulated_load


fin:    			ret

