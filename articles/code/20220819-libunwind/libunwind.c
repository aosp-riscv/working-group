#include <stdio.h>
#include "libunwind.h"

void unwind_by_libunwind()
{
    unw_cursor_t    cursor;
    unw_context_t   context;

    // get a snapshot of the CPU registers (machine-state). 
    unw_getcontext(&context);
    // initialize an unwind cursor based on this snapshot.
    // The cursor now points to the current frame, that is, the stack frame 
    // that corresponds to the current activation of function F().
    unw_init_local(&cursor, &context);

    // The unwind cursor can then be moved ``up'' (towards earlier stack frames) 
    // by calling unw_step(). By repeatedly calling this routine, you can uncover
    // the entire call-chain that led to the activation of function F().
    //
    // A positive return value from unw_step() indicates that there are more 
    // frames in the chain, zero indicates that the end of the chain has been 
    // reached, and any negative value indicates that some sort of error has occurred.
    while (unw_step(&cursor) > 0) {
        unw_word_t ip, sp;
        unw_word_t  offset;
        char name[64] = {0};

        // Given an unwind cursor, it is possible to read and write the CPU 
        // registers that were preserved for the current stack frame (as 
        // identified by the cursor).
        // unw_get_reg() reads an integer (general) register, unw_get_fpreg() 
        // reads a floating-point register.
        // we can also write the registers by unw_set_reg()/unw_set_fpreg()
        unw_get_reg(&cursor, UNW_REG_IP, &ip);
        unw_get_reg(&cursor, UNW_REG_SP, &sp);
        // unw_get_proc_name() returns the name of the procedure that 
        // created the stack frame identified by cursor, this function also 
        // return the byte-offset of the instruction-pointer saved in the 
        // stack frame identified by cursor, relative to the start of the procedure.
        unw_get_proc_name(&cursor, name, sizeof(name), &offset);
        
        printf ("ip = %lx, sp = %lx : (%s + 0x%08lx)\n", (long)ip, (long)sp, name, offset);
    }
}


