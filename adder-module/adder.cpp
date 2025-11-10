#include <systemc.h>

SC_MODULE(adder)
{
    sc_in<sc_uint<51>> in1, in2;
    sc_in<bool> clk;

    uint64_t str_sum = 0;
    uint64_t str_crr = 0;

    typedef struct
    {
        sc_out<sc_uint<51>> out;
        sc_out<sc_uint<1>> carry;
    } adder_result;

    adder_result adderResult;
    sc_event event_triggered;

    void do_adder()
    {
        sc_uint<51> sum_val = in1.read() ^ in2.read();
        sc_uint<1> crr_val = in1.read() & in2.read();

        adderResult.out.write(sum_val);
        adderResult.carry.write(crr_val);

        // Trigger event if values changed
        if ((str_sum != sum_val) || (str_crr != crr_val))
        {
            event_triggered.notify(); // notify the real SystemC event
        }

        str_sum = sum_val;
        str_crr = crr_val;

        // cout << "[INFO] Event not triggered: "
        //      << "Sum = " << adderResult.out.read() << " (" << str_sum << "), "
        //      << "Carry = " << adderResult.carry.read() << " (" << str_crr << ")"
        //      << endl;

        if ((str_sum != adderResult.out.read()) && (str_crr != adderResult.carry.read()))
        {
            event_triggered.notify();
        }

        str_sum = adderResult.out.read();
        str_crr = adderResult.carry.read();
    }

    void monitor_event()
    {
        while (true)
        {
            wait(event_triggered); // wait for the real event

            cout << "inp1 = " << in1.read() << "inp2 = " << in2.read() << "sum = " << adderResult.out.read()
                 << "carry = " << adderResult.carry.read() << endl;
        }
    }

    SC_CTOR(adder)
    {
        SC_METHOD(do_adder);
        sensitive << clk.pos();
        SC_THREAD(monitor_event);
    };
};

typedef struct
{
    sc_signal<sc_uint<51>> out;
    sc_signal<sc_uint<1>> carry;
} adder_result;

int sc_main(int argc, char *argv[])
{
    sc_signal<sc_uint<51>> sig1, sig2;
    sc_clock clk("clk", 10, SC_NS);
    adder_result adderResult;

    adder adder_module("ANDER_Module");

    adder_module.in1(sig1);
    adder_module.in2(sig2);
    adder_module.clk(clk);
    adder_module.adderResult.out(adderResult.out);
    adder_module.adderResult.carry(adderResult.carry);

    sc_trace_file *tf = sc_create_vcd_trace_file("adder");
    sc_trace(tf, sig1, "in1");
    sc_trace(tf, sig2, "in2");
    sc_trace(tf, adderResult.out, "out");
    sc_trace(tf, adderResult.carry, "carry");
    sc_trace(tf, clk, "clk");

    sig1 = 10;
    sig2 = 20;

    sc_start(10, SC_NS);
    cout << "sum = " << adderResult.out << endl;
    cout << "carry = " << adderResult.carry << endl;

    sig1 = 10;
    sig2 = 20;

    sc_start(10, SC_NS);
    cout << "sum = " << adderResult.out << endl;
    cout << "carry = " << adderResult.carry << endl;

    sig1 = (1ULL << 51) - 1; // max 51-bit value
    sig2 = 5;

    sc_start(10, SC_NS);
    cout << "sum = " << adderResult.out << endl;
    cout << "carry = " << adderResult.carry << endl;

    sc_start(10, SC_NS);

    sc_close_vcd_trace_file(tf);
    return 1;
}