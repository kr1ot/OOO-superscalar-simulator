#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "sim_proc.h"

#include "pipeline_stages.cc"


/*  argc holds the number of command line arguments
    argv[] holds the commands themselves

    Example:-
    sim 256 32 4 gcc_trace.txt
    argc = 5
    argv[0] = "sim"
    argv[1] = "256"
    argv[2] = "32"
    ... and so on
*/

bool Advance_Cycle(pipeline_data *meta)
{
	meta->simulation_cycle++;
	return meta->is_simulation_done;
}

int main (int argc, char* argv[])
{
    FILE *FP;               // File handler
    char *trace_file;       // Variable that holds trace file name;
    proc_params params;       // look at sim_bp.h header file for the the definition of struct proc_params
    
    if (argc != 5)
    {
        printf("Error: Wrong number of inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }
    
    params.rob_size     = strtoul(argv[1], NULL, 10);
    params.iq_size      = strtoul(argv[2], NULL, 10);
    params.width        = strtoul(argv[3], NULL, 10);
    trace_file          = argv[4];
    // printf("rob_size:%lu "
    //         "iq_size:%lu "
    //         "width:%lu "
    //         "tracefile:%s\n", params.rob_size, params.iq_size, params.width, trace_file);
    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if(FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }
    
    //creating rmt table
    rmt rmt_table;
    rmt_table.rmt_initialize();


    vector<instruction> instrs_in_pipe;
	pipeline_data m_data;
	rob rob;
	rmt rmt;
	issue_queue iq;
	iq.issue_queue_initialize(params.iq_size, params.width);
	rmt.rmt_initialize();
	rob.rob_initialize(params.rob_size, params.width);
	m_data.simulation_cycle = 0;
	m_data.is_simulation_done = false;
	m_data.sequence = 0;
	m_data.trace_depleted_f = false;
	m_data.rob_full = false;
	m_data.issue_queue_full = false;
	m_data.dispatch_busy = false;
	m_data.reg_read_busy = false;
	m_data.rename_busy = false;
	m_data.decode_busy = false;
	m_data.issue_queue_empty = true;

	do{
		retire(&m_data, &params, &rob, instrs_in_pipe, &rmt);

		writeback(&m_data, instrs_in_pipe, &rob);

		execute(&m_data, &params, instrs_in_pipe, &rob, &iq);
		
		issue(&m_data, &params, instrs_in_pipe, &iq, &rob);

		dispatch(&m_data, &params, instrs_in_pipe, &iq);

		regread(&m_data, &params, instrs_in_pipe, &rob);

		rename(&m_data, &params, instrs_in_pipe, &rmt, &rob);
		
		decode(&m_data, &params, instrs_in_pipe);

		fetch(&m_data, &params, instrs_in_pipe, FP);

	}while(!Advance_Cycle(&m_data));

	//int num_instr_in_pipe = instrs_in_pipe.size();
	//
	//for(int i = 0; i < num_instr_in_pipe; i++)
	//	instrs_in_pipe[i].display_instruction();

	//cout << "Simulation cycles: " << m_data.simulation_cycle << endl;
    //while(fscanf(FP, "%lx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2) != EOF)
    //    printf("%lx %d %d %d %d\n", pc, op_type, dest, src1, src2); //Print to check if inputs have been read correctly
	printf("# === Simulator Command =========\n");
	printf("# ./sim %lu %lu %lu %s\n", params.rob_size, params.iq_size, params.width, trace_file);
	printf("# === Processor Configuration ===\n");
	printf("# ROB_SIZE = %lu\n", params.rob_size);
	printf("# IQ_SIZE  = %lu\n", params.iq_size);
	printf("# WIDTH    = %lu\n", params.width);
	printf("# === Simulation Results ========\n");
	printf("# Dynamic Instruction Count    = %u\n", m_data.sequence);
	printf("# Cycles                       = %u\n", m_data.simulation_cycle);
	double IPC = (double) m_data.sequence / (double) m_data.simulation_cycle;
	printf("# Instructions Per Cycle (IPC) = %.2lf\n", IPC);
    return 0;
}
