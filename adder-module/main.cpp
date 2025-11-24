#include <systemc.h>
#include <tlm.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

using namespace tlm;
using namespace tlm_utils;

struct stimulator_initiator : sc_module
{
private:
    enum MemRegisterIndex : uint8_t
    {
        SUM = 0,
        CARRY = 1,
        ADDEND1 = 2,
        ADDEND2 = 3,
        INITIATOR_MEM_SIZE = 4
    };
    sc_event adder_resp_event;
    uint64_t mem_registers[INITIATOR_MEM_SIZE] = {0};

public:
    tlm_utils::simple_initiator_socket<stimulator_initiator> stimulator_socket;

    // Pointer to adder target (needs to be set externally)
    class adder_target *ptr_adder;

    void sti_out()
    {
        static sc_uint<2> state = 0;
        sc_uint<52> a = 0;
        sc_uint<52> b = 0;
        // while (true)
        {
            // Wait for event from adder
            // wait(ptr_adder->event_to_stim);
            // wait(SC_ZERO_TIME);

            sc_uint<52> sum = mem_registers[SUM];
            sc_uint<1> carry = mem_registers[CARRY];

            a = 0;
            b = 0;
            switch (state)
            {
            case 0:
                a = 100;
                b = 200;
                mem_registers[ADDEND1] = a;
                mem_registers[ADDEND2] = b;
                state = 1;
                break;

            case 1:
                a = sum >> 1;
                b = sum >> 1;
                mem_registers[ADDEND1] = a;
                mem_registers[ADDEND2] = b;
                state = 2;
                break;

            case 2:
                a = sum + carry;
                b = sum + 1;
                mem_registers[ADDEND1] = a;
                mem_registers[ADDEND2] = b;
                state = 3;
                break;

            case 3:
            {
                static int counter = 0;
                static sc_uint<52> target_sum = 0;
                static bool first_entry = true;

                if (first_entry)
                {
                    target_sum = sum + carry;
                    first_entry = false;
                }

                a = target_sum >> 1;
                b = target_sum - a;

                mem_registers[ADDEND1] = a;
                mem_registers[ADDEND2] = b;

                counter++;
                if (10 == counter)
                {
                    counter = 0;
                    first_entry = true;
                    state = 0;
                }
            }
            break;
            }
            cout << "[STIMULATOR]: " << sc_time_stamp() << ":" << " val1 = " << a << " val2 = " << b << endl;
        }
    }

    void fw_stim_send()
    {
        wait(5, SC_NS);

        sti_out();
        tlm_generic_payload trans;

        trans.set_command(TLM_WRITE_COMMAND);
        trans.set_address(0x00);
        trans.set_data_ptr((unsigned char *)mem_registers);
        trans.set_data_length(sizeof(mem_registers));
        trans.set_streaming_width(sizeof(mem_registers));
        trans.set_byte_enable_ptr(0);
        trans.set_response_status(TLM_INCOMPLETE_RESPONSE);

        cout << "@" << sc_time_stamp() << " INITIATOR: Sending WRITE BEGIN_REQ" << endl;

        tlm_phase phase = BEGIN_REQ;
        sc_time delay = SC_ZERO_TIME;

        tlm_sync_enum status = stimulator_socket->nb_transport_fw(trans, phase, delay);

        if (status == TLM_UPDATED)
        {
            cout << "@" << sc_time_stamp() << " INITIATOR: Received "
                 << (phase == END_REQ ? "END_REQ" : "other phase") << endl;
        }

        wait(adder_resp_event);

        cout << "@" << sc_time_stamp() << " INITIATOR: Write complete!" << endl;

        wait(30, SC_NS);

        trans.set_command(TLM_READ_COMMAND);
        trans.set_address(0x00);
        trans.set_data_ptr((unsigned char *)mem_registers);
        trans.set_data_length(sizeof(mem_registers));
        trans.set_streaming_width(sizeof(mem_registers));
        trans.set_byte_enable_ptr(0);
        trans.set_response_status(TLM_INCOMPLETE_RESPONSE);

        cout << "@" << sc_time_stamp() << " INITIATOR: Sending READ BEGIN_REQ" << endl;

        phase = BEGIN_REQ;
        delay = SC_ZERO_TIME;

        status = stimulator_socket->nb_transport_fw(trans, phase, delay);

        if (status == TLM_UPDATED)
        {
            cout << "@" << sc_time_stamp() << " INITIATOR: Received "
                 << (phase == END_REQ ? "END_REQ" : "other phase") << endl;
        }

        wait(adder_resp_event);

        cout << "@" << sc_time_stamp() << " INITIATOR: READ complete!" << endl;
        for (int i = 0; i < 4; i++)
        {
            cout << "mem[" << i << "] = " << mem_registers[i] << endl;
        }

        wait(30, SC_NS);
    }

    SC_CTOR(stimulator_initiator) : stimulator_socket("stimulator_socket"), ptr_adder(nullptr)
    {
        stimulator_socket.register_nb_transport_bw(this, &stimulator_initiator::nb_transport_bw);
        SC_THREAD(fw_stim_send);
    }

    virtual tlm_sync_enum nb_transport_bw(tlm_generic_payload &trans,
                                          tlm_phase &phase,
                                          sc_time &delay)
    {
        if (BEGIN_RESP == phase)
        {
            phase = END_RESP;
            adder_resp_event.notify(delay);
            return TLM_COMPLETED;
        }
        return TLM_ACCEPTED;
    }
};

struct adder_target : sc_module
{
private:
    sc_event initiator_resp_event;
    tlm_generic_payload *current_generic_pl = nullptr;
    uint64_t str_sum = 0;
    uint64_t str_crr = 0;
    sc_uint<52> sum_val;
    sc_uint<1> crr_val;
    enum MemRegisterIndex : uint8_t
    {
        SUM = 0,
        CARRY = 1,
        ADDEND1 = 2,
        ADDEND2 = 3,
        ADDER_MEM_SIZE = 4
    };
    uint64_t adder_mem[ADDER_MEM_SIZE] = {0};

public:
    tlm_utils::simple_target_socket<adder_target> adder_socket;
    sc_event event_to_stim;

    void adder_logic()
    {
        sc_uint<53> full_sum = (sc_uint<53>)adder_mem[ADDEND1] + (sc_uint<53>)adder_mem[ADDEND2];

        sum_val = full_sum.range(51, 0);
        crr_val = full_sum[52].to_bool();

        // cout << "[ADDER]: " << "Adder inputs: " << in1.read() << " + " << in2.read();
        // cout << " = sum: " << sum_val << " carry: " << crr_val << endl;

        // adderResult.out.write(sum_val);
        // adderResult.carry.write(crr_val);
        adder_mem[SUM] = sum_val;
        adder_mem[CARRY] = crr_val;

        // if ((str_sum != sum_val) || (str_crr != crr_val)) //TODO
        //     event_triggered.notify();

        str_sum = sum_val;
        str_crr = crr_val;

        cout << "DEBUG-ADDER: " << "addend1 and addend2 in the memory = " << adder_mem[ADDEND1] << " " << adder_mem[ADDEND1] << endl;
        cout << "DEBUG-ADDER: " << "sum and carry in the memory = " << adder_mem[SUM] << " " << adder_mem[CARRY] << endl;
        // event_to_stim.notify(SC_ZERO_TIME); // SC_ZERO_TIME //TODO
    }

    void reponse()
    {
        if (current_generic_pl)
        {
            cout << "@" << sc_time_stamp() << " TARGET: Sending BEGIN_RESP" << endl;

            tlm_phase phase = BEGIN_RESP;
            sc_time delay = SC_ZERO_TIME;

            // Send response back on backward path
            adder_socket->nb_transport_bw(*current_generic_pl, phase, delay);
            cout << "@" << sc_time_stamp() << " TARGET: Received " << phase << endl;

            current_generic_pl = nullptr;
        }
    }

    SC_CTOR(adder_target) : adder_socket("adder_socket")
    {
        adder_socket.register_nb_transport_fw(this, &adder_target::nb_transport_fw);
        SC_METHOD(reponse);
        sensitive << initiator_resp_event;
        dont_initialize();
    }

    virtual tlm_sync_enum nb_transport_fw(tlm_generic_payload &trans,
                                          tlm_phase &phase,
                                          sc_time &delay)
    {
        if (BEGIN_REQ == phase)
        {
            cout << "@" << sc_time_stamp() << " TARGET: Received BEGIN_REQ" << endl;
            phase = END_REQ;

            uint64_t addr = trans.get_address();
            unsigned char *data_ptr = trans.get_data_ptr();
            uint64_t *data = reinterpret_cast<uint64_t *>(data_ptr); // Cast to uint64_t*
            tlm_command cmd = trans.get_command();

            if (TLM_WRITE_COMMAND == cmd)
            {
                adder_mem[ADDEND1] = data[ADDEND1];
                adder_mem[ADDEND2] = data[ADDEND2];
                cout << "DEBUG-ADDER: addend1 and addend2 in memory = "
                     << adder_mem[ADDEND1] << " " << adder_mem[ADDEND2] << endl;
                adder_logic();
            }
            else if (TLM_READ_COMMAND == cmd)
            {
                data[SUM] = adder_mem[SUM];
                data[CARRY] = adder_mem[CARRY];
                cout << "DEBUG-ADDER: sum and carry in memory = "
                     << data[SUM] << " " << data[CARRY] << endl;
            }

            trans.set_response_status(TLM_OK_RESPONSE);
            delay = sc_time(10, SC_NS);
            initiator_resp_event.notify(delay);
            current_generic_pl = &trans;
            return TLM_UPDATED;
        }
        return TLM_ACCEPTED;
    }
};

struct Top : sc_module
{
    stimulator_initiator *initiator;
    adder_target *target;

    SC_CTOR(Top)
    {
        initiator = new stimulator_initiator("initiator");
        target = new adder_target("target");

        // Bind sockets
        initiator->stimulator_socket.bind(target->adder_socket);

        // Set pointer for cross-referencing
        initiator->ptr_adder = target;
    }

    ~Top()
    {
        delete initiator;
        delete target;
    }
};

int sc_main(int argc, char *argv[])
{
    Top top("top");
    sc_start(100, SC_NS);
    return 0;
}

// Compile with:
// g++ main.cpp -I$SYSTEMC_HOME/include -L$SYSTEMC_HOME/lib-linux64 -lsystemc -pthread -DSC_INCLUDE_DYNAMIC_PROCESSES -o tlm-basic