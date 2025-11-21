#include <systemc.h>

struct stimulator_initiator : sc_module
{
private:
    sc_event adder_resp_event;

public:
    tlm_utils::simple_initiator_socket<stimulator_initiator> stimulator_socket;

    void fw_stim_send()
    {
        //intantiate the geberic payload pointer
        //prepare data and populate the the generic payload pointer
        //call ``nb_transport_fw`` with phase BEGIN_REQ, cmd == WRITE, and set TLM status tp TLM_INCOMPLETE_RESPONSE
        //get the status in return to the above funtion call
        //if the status is TLM_UPDATED
            //means the request is completed or ended by the target. This does not mean the data is written at this point in time.
            //Wait for the event to trigger (Which means the data is written). Once the event triggered that means the write comepleted now
        //else
            //handle some stuff...
        
        //We can proceed with the read transaction in a similar way.
        //prepare data and populate the the generic payload pointer
        //call ``nb_transport_fw`` with phase BEGIN_REQ, cmd == WRITE, and set TLM status tp TLM_INCOMPLETE_RESPONSE
        //... Rest of the sequence is same as in the WRITE case


    }

    SC_CTOR(stimulator_initiator) : stimulator_socket("stimulator_socket")
    {
        stimulator_socket.register_nb_transport_bw(this, &stimulator_initiators::nb_transport_bw);
        SC_THREAD(fw_stim_send);
    }

    virtual tlm_sync_enum nb_transport_bw(tlm_generic_payload &trans,
                                          tlm_phase &phase,
                                          sc_time &delay)
    {

    }
}

struct adder_target : sc_module
{
private:
    sc_event initiator_resp_event;
    tlm_generic_payload *current_generic_pl = nullptr;
public:
    tlm_utils::simple_initiator_socket<adder_target> adder_socket;

    void do_adder()
    {

    }

    SC_CTOR(adder_target) : adder_socket("adder_socket")
    {
        adder_socket.register_nb_transport_fw(this, &adder_target::nb_transport_bw);
        SC_METHOD(do_adder);
        sensitive << initiator_resp_event;
    }

    virtual tlm_sync_enum nb_transport_fw(tlm_generic_payload &trans,
                                          tlm_phase &phase,
                                          sc_time &delay)
    {
        //check if the request is equals to the BEGIN_REQ
        // if true
            //Start updating the generic payload reference ``trans``
            //set the phase to END_REQ
            //check of the request is read/write
                //In case of write data simply copy the data from the data pointer of the generic payload to the memory address
                //In case of the read request simply puts ths data from the memory to the data pointer of the generic payload
            //set TLM response status in the generic paylod
            //Do processing (triggering an event with a delay, which will trigger the initiator in the form of the response)
            //handover trans to the current_generic_pl (Which will be used in the do_adder method)
            //return TLM_COMPLETED not
        //if false
            //return TLM_ACCEPTED (means:  means that the transaction has been accepted by the target, but is not yet complete)
    }

}

//g++ main.cpp     -I$SYSTEMC_HOME/src     -L$SYSTEMC_HOME/build/src     -lsystemc -pthread     -D SC_ALLOW_DEPRECATED_IEEE_API     -o tlm-basic
