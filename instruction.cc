#include "sim_proc.h"

#include <vector>
#include <stdio.h>
#include <iostream>

using namespace std;

//emulates an instruction entry in the pipeline
//contains data for an instruction
//1. sequence number (age of the instruction)
//2. pc
//3. current stage
//4. cycles in a given stage
//   -> also the cycle at which the instruction was fetched
//5. src1
//6. src2
//7. dst
//8. operation type (0,1,2)
//9. execution latency (defined via op type)
//10. ROB index
//11. the index of instruction for a super scalar width
class instruction
{
    private:
        unsigned int sequence;
        unsigned long pc;
        unsigned int current_stage;
        //different variables to count cycles for different stages of pipeline
        unsigned int cyc_in_fetch;
        unsigned int cyc_in_decode;
        unsigned int cyc_in_rename;
        unsigned int cyc_in_register_read;
        unsigned int cyc_in_dispatch;
        unsigned int cyc_in_issue_queue;
        unsigned int cyc_in_exec;
        unsigned int cyc_in_writeback;
        unsigned int cyc_in_retire;
        //register tags
        int src1;
        int src2;
        int dst;

        unsigned int operation_type;
        unsigned int execution_latency;
        //rob related
        int rob_index;
        //src tag in rob entries
        int src1_rob;
        int src2_rob;
        //weether src is ready in rob
        bool src1_rob_rdy; 
        bool src2_rob_rdy;

        unsigned int super_scalar_slot;
        unsigned int instr_cycle_at_fetch; 
	
	
	public:
        //initialize the instruction when it is fetched
        void instruction_initialize(unsigned long pc_val, unsigned int op_type, unsigned int dst_val, unsigned int src1_val, unsigned src2_val);

        //setter getter methods
        //for rob src registers
        void set_src1_rob_rdy(){
            src1_rob_rdy = true;
        }
        void set_src2_rob_rdy(){
            src2_rob_rdy = true;
        }
        void clear_src1_rob_rdy(){
            src1_rob_rdy = false;
        }
        void clear_src2_rob_rdy(){
            src2_rob_rdy = false;
        }

        bool get_src1_rob_rdy(){
            return src1_rob_rdy;
        }
        bool get_src2_rob_rdy(){
            return src2_rob_rdy;
        }

        unsigned long get_pc(){
            return pc;
        }

        //sets the age for the instruction via the sequence number
        void set_sequence(unsigned int seq){
            sequence = seq;
        }
        unsigned int get_sequence(){
            return sequence;
        }

        //for a given operation, calculate latency for an instruction
        void calculate_latency();
        //get the execution latency for a given operation type
        unsigned int get_execution_latency(){
            return execution_latency;
        }
        //set the current stage the instruction is in the pipeline
        void set_current_stage(unsigned int stage){
            current_stage = stage;
        }

        //set the cycles in the current stage of pipeline
        void set_cycles_in_current_stage(unsigned int cycles);
        //get the  cycles spent in the current stage
        //useful for moving the instruction to the next state
        unsigned int get_cycles_in_current_stage();
        //increment the number of cycles in the current stage of the instruction
        void incr_cycles_for_current_stage();
    
        //rob entry set in the rename stage
        void set_rob_entry(int robtag){
            rob_index = robtag;
        }

        //get rob entry for dispatch to issue and when retiring
        int get_rob_entry(){
            return rob_index;
        }
        
        void set_src1_rob(int tag){
            src1_rob = tag;
        }
        void set_src2_rob(int tag){
            src2_rob = tag;
        }
        
        int get_src1_rob(){
            return src1_rob;
        }
        int get_src2_rob(){
            return src2_rob;
        }
        int get_src1()	{
            return src1;
        }
        int get_src2()	{
            return src2;
        }
        int get_dst()	{
            return dst;
        }

        //function to set the superscalar slot for this instruction
        void set_superscalar_slot(unsigned int super_slot)	{super_scalar_slot = super_slot;}

        //get method for super scalar slot value
        unsigned int get_super_slot()					{return super_scalar_slot;}
        
        //this function is used when moving instructions from 1 stage to the next
        unsigned int get_current_stage()		{return current_stage;}

        
        //this function will be called by the fetch stage
        void set_start_cycle(unsigned int cyc)	{instr_cycle_at_fetch = cyc;}

        //print the isntruction stats after the retire and commit to ARF
        void printstats();
    
        //this function is only for debugging purposes
        //
        //prints all the metadata
        void display_instruction();
        
};

void instruction::printstats()
{
    printf("%u ",sequence);
    printf("fu{%u} ",operation_type);
    printf("src{%d,%d} ",src1,src2);
    printf("dst{%d} ",dst);

    //cycles in a given pipeline stage
    unsigned int cycles;
    cycles = instr_cycle_at_fetch;
    printf("FE{%u,%u} ",cycles, cyc_in_fetch);
    cycles = cycles + cyc_in_fetch;
    printf("DE{%u,%u} ",cycles, cyc_in_decode);
    cycles = cycles + cyc_in_decode;
    printf("RN{%u,%u} ",cycles, cyc_in_rename);
    cycles = cycles + cyc_in_rename;
    printf("RR{%u,%u} ",cycles, cyc_in_register_read);
    cycles = cycles + cyc_in_register_read;
    printf("DI{%u,%u} ",cycles, cyc_in_dispatch);
    cycles = cycles + cyc_in_dispatch;
    printf("IS{%u,%u} ",cycles, cyc_in_issue_queue);
    cycles = cycles + cyc_in_issue_queue;
    printf("EX{%u,%u} ",cycles, cyc_in_exec);
    cycles = cycles + cyc_in_exec;
    printf("WB{%u,%u} ",cycles, cyc_in_writeback);
    cycles = cycles + cyc_in_writeback;
    printf("RT{%u,%u} ",cycles, cyc_in_retire);

    printf("\n");
}

void instruction::display_instruction()
{
	//information that is relevant at any clock cycle
	//
	//1. which stage is this instruction in?
	//2. sequence number in the trace file
	//3. execution latency and operation type
	//4. destination and source registers
	//5. rob entry and renamed source registers
	printf("\n\tCurrent Stage		:%u\n", current_stage);
	printf("\tSequence num 		:%u\n", sequence);
	printf("\tExecution latency: %u, Operation type: %u\n", execution_latency, operation_type);
	printf("\tDst: %d, Src1: %d, Src2: %d\n", dst, src1, src2);
	printf("\tROB dst: %d, ROB src1: %d, ROB src2: %d\n", rob_index, src1_rob, src2_rob);
}

unsigned int instruction::get_cycles_in_current_stage()
{
	unsigned int cycles;
	switch(current_stage)
	{
		case FETCH:
			cycles = cyc_in_fetch;
			break;
		case DECODE:
			cycles = cyc_in_decode;
			break;
		case DISPATCH:
			cycles = cyc_in_dispatch;
			break;
		case RENAME:
			cycles = cyc_in_rename;
			break;
		case REGREAD:
			cycles = cyc_in_register_read;
			break;
		case ISSQUE:
			cycles = cyc_in_issue_queue;
			break;
		case EXEC:
			cycles = cyc_in_exec;
			break;
		case WRTBACK:
			cycles = cyc_in_writeback;
			break;
		case RETIRE:
			cycles = cyc_in_retire;
			break;
		default:
			cycles = 0;
			cout << "incorrect current stage" << endl;
			break;
	}
	
	return cycles;
}

void instruction::set_cycles_in_current_stage(unsigned int cycles)
{
	switch(current_stage)
	{
		case FETCH:
			cyc_in_fetch = cycles;
			break;
		case DECODE:
			cyc_in_decode = cycles;
			break;
		case DISPATCH:
			cyc_in_dispatch = cycles;
			break;
		case RENAME:
			cyc_in_rename = cycles;
			break;
		case REGREAD:
			cyc_in_register_read = cycles;
			break;
		case ISSQUE:
			cyc_in_issue_queue = cycles;
			break;
		case EXEC:
			cyc_in_exec = cycles;
			break;
		case WRTBACK:
			cyc_in_writeback = cycles;
			break;
		case RETIRE:
			cyc_in_retire = cycles;
			break;
		default:
			break;
	}
}

void instruction::instruction_initialize(unsigned long pc_val, unsigned int op_type, unsigned int dst_val, unsigned int src1_val, unsigned src2_val)
{
	operation_type = op_type;
	pc = pc_val;
	dst = dst_val;
	src1 = src1_val;
	src2 = src2_val;

	//-1 indicates that it doesn't need a rob index
	src1_rob = -1;
	src2_rob = -1;
	cyc_in_fetch = 0;
	cyc_in_decode = 0;
	cyc_in_rename = 0;
	cyc_in_register_read = 0;
	cyc_in_dispatch = 0;
	cyc_in_issue_queue = 0;
	cyc_in_exec = 0;
	cyc_in_writeback = 0;
	cyc_in_retire = 0;
}

void instruction::incr_cycles_for_current_stage()
{
	switch(current_stage)
	{
		case FETCH:
			cyc_in_fetch++;
			break;
		case DECODE:
			cyc_in_decode++;
			break;
		case DISPATCH:
			cyc_in_dispatch++;
			break;
		case RENAME:
			cyc_in_rename++;
			break;
		case REGREAD:
			cyc_in_register_read++;
			break;
		case ISSQUE:
			cyc_in_issue_queue++;
			break;
		case EXEC:
			cyc_in_exec++;
			break;
		case WRTBACK:
			cyc_in_writeback++;
			break;
		case RETIRE:
			cyc_in_retire++;
			break;
		default:
			cout << "incorrect current stage" << endl;
	}
}

void instruction::calculate_latency()
{
	switch(operation_type)
	{
		case 0:
			execution_latency = 1;
			break;
		case 1:
			execution_latency = 2;
			break;
		case 2:
			execution_latency = 5;
			break;
	}
}
