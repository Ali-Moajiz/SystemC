#include <systemc.h>
#include "common.h"

adder *ptr_adder;
stimulator *ptr_stim;

int sc_main(int argc, char *argv[])
{
    sc_signal<sc_uint<52>> addend1, addend2;

    sc_signal<sc_uint<52>> sum;
    sc_signal<sc_uint<1>> carry;

    sc_clock clk("clk", 10, SC_NS);

    ptr_adder = new adder("Adder_Module");
    ptr_stim = new stimulator("Stimulator");

    ptr_adder->in1(addend1);
    ptr_adder->in2(addend2);
    ptr_adder->clk(clk);
    ptr_adder->adderResult.out(sum);
    ptr_adder->adderResult.carry(carry);

    ptr_stim->sum_in(sum);
    ptr_stim->carr_in(carry);
    ptr_stim->out1(addend1);
    ptr_stim->out2(addend2);

    sc_trace_file *tf = sc_create_vcd_trace_file("adder");
    sc_trace(tf, addend1, "in1");
    sc_trace(tf, addend2, "in2");
    sc_trace(tf, sum, "out");
    sc_trace(tf, carry, "carry");
    sc_trace(tf, clk, "clk");

    addend1 = 30;
    addend2 = 20;

    sc_start(3000, SC_NS);
    sc_close_vcd_trace_file(tf);
    return 0;
}
