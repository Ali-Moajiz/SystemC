#include "adder.h"


void adder::do_adder()
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

void adder::monitor_event()
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
