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

    void do_adder()
    {
        sc_uint<53> full_sum = (sc_uint<53>)in1.read() + (sc_uint<53>)in2.read();

        sc_uint<52> sum_val = full_sum.range(51, 0);
        sc_uint<1> crr_val = full_sum[52].to_bool();

        adderResult.out.write(sum_val);
        adderResult.carry.write(crr_val);

        if ((str_sum != sum_val) || (str_crr != crr_val))
            event_triggered.notify();

        str_sum = sum_val;
        str_crr = crr_val;
    }

    void monitor_event()
    {
        while (true)
        {
            wait(event_triggered);
            cout << "inp1 = " << in1.read()
                 << " inp2 = " << in2.read()
                 << " sum = " << adderResult.out.read()
                 << " carry = " << adderResult.carry.read()
                 << endl;
        }
    }

    SC_CTOR(adder)
    {
        SC_METHOD(do_adder);
        sensitive << clk.pos();
        SC_THREAD(monitor_event);
    }
};

SC_MODULE(stimulator)
{
    sc_in<sc_uint<52>> sum_in;
    sc_in<sc_uint<1>> carr_in;

    sc_out<sc_uint<52>> out1;
    sc_out<sc_uint<52>> out2;

    void sti_out();

    SC_CTOR(stimulator)
    {
        SC_METHOD(sti_out);
        sensitive << sum_in << carr_in;
        dont_initialize();
    }
};

void stimulator::sti_out()
{
    static sc_uint<2> state = 0;

    sc_uint<52> sum = sum_in.read();
    sc_uint<1> carry = carr_in.read();

    switch (state)
    {
    case 0:
        out1.write(10);
        out2.write(20);
        state = 1;
        break;

    case 1:
        out1.write(sum >> 1);
        out2.write(sum >> 1);
        state = 2;
        break;

    case 2:
        out1.write(sum + carry);
        out2.write(sum + 1);
        state = 0;
        break;
    }
}

struct adder_signals
{
    sc_signal<sc_uint<52>> out;
    sc_signal<sc_uint<1>> carry;
};

int sc_main(int argc, char *argv[])
{
    sc_signal<sc_uint<52>> sig1, sig2;
    sc_signal<sc_uint<52>> out1, out2;

    sc_clock clk("clk", 10, SC_NS);

    adder_signals adderResult;

    adder adder_module("Adder_Module");
    stimulator stimulator_module("Stimulator");

    adder_module.in1(sig1);
    adder_module.in2(sig2);
    adder_module.clk(clk);
    adder_module.adderResult.out(adderResult.out);
    adder_module.adderResult.carry(adderResult.carry);

    stimulator_module.sum_in(adderResult.out);
    stimulator_module.carr_in(adderResult.carry);
    stimulator_module.out1(out1);
    stimulator_module.out2(out2);

    sc_trace_file *tf = sc_create_vcd_trace_file("adder");
    sc_trace(tf, sig1, "in1");
    sc_trace(tf, sig2, "in2");
    sc_trace(tf, adderResult.out, "out");
    sc_trace(tf, adderResult.carry, "carry");
    sc_trace(tf, out1, "stim_out1");
    sc_trace(tf, out2, "stim_out2");
    sc_trace(tf, clk, "clk");

    sig1 = 30;
    sig2 = 20;

    sc_start(20, SC_NS);
    cout << "sum = " << adderResult.out.read()
         << " carry = " << adderResult.carry.read()
         << endl;

    sig1 = out1.read();
    sig2 = out2.read();

    sc_start(20, SC_NS);
    cout << "sum = " << adderResult.out.read()
         << " carry = " << adderResult.carry.read()
         << endl;

    sig1 = out1.read();
    sig2 = out2.read();

    sc_start(20, SC_NS);
    cout << "sum = " << adderResult.out.read()
         << " carry = " << adderResult.carry.read()
         << endl;

    sig1 = out1.read();
    sig2 = out2.read();

    sc_start(20, SC_NS);
    cout << "sum = " << adderResult.out.read()
         << " carry = " << adderResult.carry.read()
         << endl;

    sc_start(20, SC_NS);
    sc_close_vcd_trace_file(tf);
    return 0;
}
