#ifndef MY_ADDER_H
#define MY_ADDER_H
#include <systemc.h>

SC_MODULE(adder)
{
    sc_in<sc_uint<52>> in1, in2;
    sc_in<bool> clk;

    uint64_t str_sum = 0;
    uint64_t str_crr = 0;

    struct adder_ports
    {
        sc_out<sc_uint<52>> out;
        sc_out<sc_uint<1>> carry;
    } adderResult;

    sc_event event_triggered;
    sc_event event_to_stim;

    sc_uint<52> sum_val;
    sc_uint<1> crr_val;

    void do_adder();
    void monitor_event();

    SC_CTOR(adder)
    {
        SC_METHOD(do_adder);
        sensitive << clk.pos();
        SC_THREAD(monitor_event);
    }
};


#endif
