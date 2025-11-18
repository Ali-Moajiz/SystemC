#include "stim.h"
#include "common.h"

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
