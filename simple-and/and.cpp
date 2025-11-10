#include <systemc.h>

SC_MODULE(and2)
{
    sc_in<sc_uint<1>> a, b;
    sc_out<sc_uint<1>> f;
    sc_in<bool> clk;

    void func_and()
    {
        f.write(a.read() & b.read());
    }

    SC_CTOR(and2)
    {
        SC_METHOD(func_and); // Always run SC_METHOD
        sensitive << clk.pos();
    }
};

int sc_main(int argc, char *argv[])
{
    // Signals to connect the module ports
    sc_signal<sc_uint<1>> sig_a, sig_b, sig_f;
    sc_clock clk("clk", 10, SC_NS); // Clock with 10 ns period

    // Instantiate the module
    and2 and_gate("AND2_Module");

    // Connect signals to the module ports
    and_gate.a(sig_a);
    and_gate.b(sig_b);
    and_gate.f(sig_f);
    and_gate.clk(clk);

    // Trace file to view waveform (optional)
    sc_trace_file *tf = sc_create_vcd_trace_file("and2_wave");
    sc_trace(tf, sig_a, "a");
    sc_trace(tf, sig_b, "b");
    sc_trace(tf, sig_f, "f");
    sc_trace(tf, clk, "clk");

    // Initialize signals
    sig_a = 0;
    sig_b = 0;

    // Start simulation
    sc_start(10, SC_NS);
    sig_a = 1;
    sig_b = 0;
    sc_start(10, SC_NS);
    sig_a = 1;
    sig_b = 1;
    sc_start(10, SC_NS);

    sc_start(10, SC_NS);
    sig_a = 0;
    sig_b = 0;
    sc_start(10, SC_NS);

    sc_close_vcd_trace_file(tf);

    return 0;
}