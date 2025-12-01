#ifndef SIM_PROC_H
#define SIM_PROC_H

#include <vector>

typedef struct proc_params{
    unsigned long int rob_size;
    unsigned long int iq_size;
    unsigned long int width;
}proc_params;

// Put additional data structures here as per your requirement
enum {
	FETCH = 1,
	DECODE = 2,
	RENAME = 3,
	REGREAD = 4,
	DISPATCH = 5,
	ISSQUE = 6,
	EXEC = 7,
	WRTBACK = 8,
	RETIRE = 9
};

//keep track of various parameters in the simulation
typedef struct pipeline_data{

	//tracks cycles for the entire simulation
	unsigned int simulation_cycle; 

    //to know whether simulation is completed
	bool is_simulation_done;

	//keep track of the age of an instruction
	unsigned int sequence;

	//varaiables reflecting readiness of various stages
    //when head = tail -> rob is full
    //no more instructions can be stored in rob
	bool rob_full;
    //issue queue is full. no more instructions to be sent
    //from dispatch to issue queue
	bool issue_queue_full;
    //TODO: Rename these variables
    //dispacth stage is not empty
	bool dispatch_busy;
	bool reg_read_busy;
	bool rename_busy;
	bool decode_busy;
	bool fetch_busy;

	//signals to wakeup appropriate stages 
    //TODO: Remove
	int dst_in_rob_ready;

	//multiple instruction may be finishing execution in the same cycle
	//so we need to wakeup all the dependent instructions in each stage
	std::vector<int> rob_destinations_ready_this_cycle;

	//signals signifying end of simulation
	bool trace_depleted_f;
	bool rob_head_equal_tail;
	bool issue_queue_empty;

}pipeline_data;

#endif
