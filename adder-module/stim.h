#ifndef MY_STIM_H
#define MY_STIM_H
#include <systemc.h>

SC_MODULE(stimulator)
{
    sc_in<sc_uint<52>> sum_in;
    sc_in<sc_uint<1>> carr_in;

    sc_out<sc_uint<52>> out1;
    sc_out<sc_uint<52>> out2;

    void sti_out();
    void event_monitor();

    SC_CTOR(stimulator)
    {
        SC_THREAD(sti_out);
        SC_THREAD(event_monitor);
    }
};

#endif