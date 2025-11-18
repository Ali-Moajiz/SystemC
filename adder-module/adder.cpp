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
    void do_adder()
    {
        sc_uint<53> full_sum = (sc_uint<53>)in1.read() + (sc_uint<53>)in2.read();

        sum_val = full_sum.range(51, 0);
        crr_val = full_sum[52].to_bool();

        // cout << "[ADDER]: " << "Adder inputs: " << in1.read() << " + " << in2.read();
        // cout << " = sum: " << sum_val << " carry: " << crr_val << endl;

        adderResult.out.write(sum_val);
        adderResult.carry.write(crr_val);

        if ((str_sum != sum_val) || (str_crr != crr_val))
            event_triggered.notify();

        str_sum = sum_val;
        str_crr = crr_val;
        event_to_stim.notify(SC_ZERO_TIME); // SC_ZERO_TIME
    }

    void monitor_event()
    {
        while (true)
        {
            wait(event_triggered);
            cout << endl;
            cout << "[ADDER]: " << "============ E V E N T - T R I G G E R E D ===============" << endl;
            cout << "[ADDER]: "
                 << sc_time_stamp()
                 << ":"
                 << " inp1 = " << in1.read()
                 << " inp2 = " << in2.read()
                 << " sum = " << sum_val
                 << " carry = " << crr_val
                 << endl;
            cout << endl;
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
    void event_monitor();

    SC_CTOR(stimulator)
    {
        SC_THREAD(sti_out);
        SC_THREAD(event_monitor);
    }
};

adder *ptr_adder;
stimulator *ptr_stim;

void stimulator::event_monitor()
{
    while (true)
    {
        wait(ptr_adder->event_triggered);
        wait(SC_ZERO_TIME); // Waiting for the next delta-cycle otherwise we will have a previouse value of the sum_in.read() and carr_in.read() because signals requires one delta cycle to update the values
        cout << endl;
        cout << "[STIMULATOR]: " << "============ E V E N T - T R I G G E R E D ===============" << endl;
        cout << "[STIMULATOR]: "
             << sc_time_stamp()
             << ":"
             << " sum = " << sum_in.read()
             << " carry = " << carr_in.read()
             << endl;
        cout << endl;
    }
}

void stimulator::sti_out()
{
    static sc_uint<2> state = 0;
    sc_uint<52> a = 0;
    sc_uint<52> b = 0;
    while (true)
    {

        wait(ptr_adder->event_to_stim);
        wait(SC_ZERO_TIME);

        sc_uint<52> sum = sum_in.read();
        sc_uint<1> carry = carr_in.read();

        // cout << "[STIMULATOR]: " << "Stimulator state " << state << " received sum=" << sum << " carry=" << carry;
        a = 0;
        b = 0;
        switch (state)
        {
        case 0:
            a = 100;
            b = 200;
            out1.write(a);
            out2.write(b);
            state = 1;
            break;

        case 1:
            a = sum >> 1;
            b = sum >> 1;
            out1.write(a);
            out2.write(b);
            state = 2;
            break;

        case 2:
            a = sum + carry;
            b = sum + 1;
            out1.write(a);
            out2.write(b);
            state = 3;
            break;
            // =============================  logic for same sum in several cycles for test
        case 3:
        {
            static int counter = 0;
            static sc_uint<52> target_sum = 0;
            static bool first_entry = true;

            // On first entry to state 3, calculate the target sum we want to maintain
            if (first_entry)
            {
                target_sum = sum + carry; // The sum we just received
                first_entry = false;
                // cout << "[STATE 3]: Target sum to maintain = " << target_sum << endl;
            }

            // Calculate inputs that will produce the same sum
            // Strategy: Keep one input constant, adjust the other
            // or use complementary adjustments

            // Method 1: Simple - split the target sum equally
            // a + b = target_sum, so a = target_sum/2, b = target_sum/2 (plus remainder)
            a = target_sum >> 1; // target_sum / 2
            b = target_sum - a;  // Remaining part to reach target_sum

            // Alternative methods you can use (comment/uncomment as needed):

            // Method 2: Vary inputs while maintaining sum
            // a = target_sum - counter;
            // b = counter;

            // Method 3: Use fixed offset pattern
            // sc_uint<52> offset = counter % 100;
            // a = (target_sum >> 1) + offset;
            // b = (target_sum >> 1) - offset;

            out1.write(a);
            out2.write(b);

            counter++;
            if (10 == counter)
            {
                counter = 0;
                first_entry = true; // Reset for next time we enter state 3
                state = 0;
            }
        }
        break;
        }
        cout << "[STIMULATOR]: " << sc_time_stamp() << ":" << " val1 = " << a << " val2 = " << b << endl;
    }
}

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
