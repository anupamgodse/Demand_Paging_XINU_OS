/* pfintr.S - pfintr */


    	   .text
pferrcode: .long 0
           .globl  pfintr,pferrcode 
pfintr: 

           popl pferrcode 	//pop error code
           pushfl		//store flags
	   cli			//clear interrupts
	   pushal		//save general registers
	   call pfint
	   popal
	   popfl
	   iret


