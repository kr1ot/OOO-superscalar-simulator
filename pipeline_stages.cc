//code the pipeline stages here
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "sim_proc.h"

#include "instruction.cc"
#include "rmt.cc"
#include "issue_queue.cc"
#include "rob.cc"


//increment the cycles in the current stage due to stall of the pipeline
void incr_cycles_in_current_stage_due_to_stall(unsigned int stage, vector<instruction>& instructions_in_pipeline)
{
	//loop through all the instructions in the pipeline and check against the stage which needs to be
	//stalled
	int no_of_instr_in_pipe = instructions_in_pipeline.size();
	for(int i = 0; i < no_of_instr_in_pipe; i++)
	{
		if(instructions_in_pipeline[i].get_current_stage() == stage)
		{
			//increment cycles_in_current_stage
			instructions_in_pipeline[i].incr_cycles_for_current_stage();
		}
	}
}

//fetch stage of the pipeline
//read from the file width instructions at a time
void fetch(pipeline_data *meta, proc_params *param, vector<instruction>& instructions_in_pipeline, FILE *FP)
{
	//1. Read width number of instructions in a single go
	//2. assign meatadata to each instruction
	//   -> seq number, pc, dst, src1, src2, op_type
	//3. stall if the deocde stage is completely filled

	//define a new instruction
	instruction new_instruction;
    unsigned int op_type;
	unsigned int dst;
	unsigned int src1; 
	unsigned int src2;  // Variables are read from trace file
	unsigned long pc;

	//get new instructions only if decode stage is not busy (or has enough space available)
	if(meta->decode_busy == false)
	{
		//fetch width number of instructions and store them onto a stack 
		unsigned int super_slot = 0;
		for(int i = 0; i < (int) param->width; i++) 
		{
			if(fscanf(FP, "%lx %d %d %d %d", &pc, &op_type, &dst, &src1, &src2) != EOF)
			{
				//increment the slot number
				super_slot++;
				
				//create a new instruction with required meta data
				new_instruction.instruction_initialize(pc, op_type, dst, src1, src2);

				//store the instruction number for the instruction
				new_instruction.set_sequence(meta->sequence);

				//setting super scalar slot to keep track of where the instruction is in the slot
				new_instruction.set_superscalar_slot(super_slot); 

				//set current stage to FETCH
				new_instruction.set_current_stage(FETCH);
				//increment the cycle for fetch stage before passing onto the next state
				new_instruction.incr_cycles_for_current_stage();
				//send to DECODE stage next
				new_instruction.set_current_stage(DECODE);
				//set the starting cycle of the instruction as overall simulation cycle
				new_instruction.set_start_cycle(meta->simulation_cycle);

				//push the new instruction in the pipeline to keep track of all the instructions
				//that enter the pipeline
				//To not let the size of the vector to become big, I'll erase all the instructions
				//in the retire stage. This way the size of the vector is dependent upon the width,
				//rob size and issue queue rather than the number of instructions fed to the simulator
				instructions_in_pipeline.push_back(new_instruction);
				//increment the sequence for next instruction
				//this is useful in rename, issue queue as well as during the retiring
				//the oldest instruction is stored first, issued first if multiple instructions are ready
				meta->sequence++; 
				//indicate the fetch stage is no more busy and take a new instruction
				//redudant tbh
				meta->fetch_busy = false;
			}
		}
	}
	//if stalled, incremenet the cycles in the current stage
	//TODO: Current code does not handle counting of cycles in the fetch stage
	//as the instruvtions that are fetched are basically moved to decode stage
	else
	{
		incr_cycles_in_current_stage_due_to_stall(FETCH, instructions_in_pipeline);
	}
}

//decode stage
void decode(pipeline_data *meta, proc_params *params, vector<instruction>& instructions_in_pipeline)
{
	//based on the operation type, calculate the execution cycles

	//when the rename stage is not busy, can move the instructions from decode to rename
	//otherwise stall them
	if(meta->rename_busy == false)
	{
		//loop through all the instructions in the pipeline and get only those 
		//instructions that are in decode	
		int no_of_instr_in_pipe = instructions_in_pipeline.size();
		if(no_of_instr_in_pipe != 0)
		{
			//loop through for width number of instructions
			for(int j = 0; j < (int) params->width; j++) 
			{
				//get only those instructions that are in the decode stage
				for(int i = 0; i < no_of_instr_in_pipe; i++)
				{
					if( instructions_in_pipeline[i].get_current_stage() == DECODE)
					{
						//calculate the execution cycles for the instruction
						instructions_in_pipeline[i].calculate_latency();
						//increment the cycles in the decode stage
						instructions_in_pipeline[i].incr_cycles_for_current_stage();
						//since rename stage is not busy, move the instructions to rename 
						//stage
						instructions_in_pipeline[i].set_current_stage(RENAME);
						//break the loop for next instruction
						break;
					}
				}
			}
			//once all the instructions have been moved, make the stage available
			meta->decode_busy = false;
		}
		//if no instruction is in the pipeline then make the stage available
		else
		{
			meta->decode_busy = false;
		}
	}
	else
	{
		//since rename stage is busy/stalled, decode stage is also stalled
		meta->decode_busy = true;
		//increment number of cycles for each instruction in the decode stage when stalled
		incr_cycles_in_current_stage_due_to_stall(DECODE, instructions_in_pipeline);
	}
}

//rename stage
void rename(pipeline_data *meta, proc_params* param, vector<instruction>& instructions_in_pipeline, rmt *rmt, rob *rob)
{
	//rename stage functionality:
	//1. read the source register tags
	//  -> store whether they are to be read from ARF or ROB (decide the readiness of source registers)
	//2. renaming :
	//  i) src registers:
	//      -> only if the registers have a valid rmt entry, rename them
	//      -> if not then the tag is same as that of ARF
	//  ii) dst registers:
	//      -> store the tag into rob pointed by tail
	//      -> make the rmt entry valid
	if(instructions_in_pipeline.size() != 0)
	{
		if(meta->reg_read_busy == false)
		{
			//check if rob has free enteries
			if(rob->check_width_amount_free_entries())
			{
				//loop through upto width number of enteries
				for(int j = 0; j < (int) param->width; j++)
				{
					//look for all the instructions that are in RENAME stage in the pipeline
					for(int i = 0; i < (int) instructions_in_pipeline.size(); i++)
					{
						if(instructions_in_pipeline[i].get_current_stage() == RENAME)
						{
							//increment the cycles for rename stage
							instructions_in_pipeline[i].incr_cycles_for_current_stage();
							//metadata to be stored into rob
							//assign the src and dst registers
							int dst = instructions_in_pipeline[i].get_dst();
							int src1 = instructions_in_pipeline[i].get_src1();
							int src2 = instructions_in_pipeline[i].get_src2();
							//get the pc 
							unsigned long pc = instructions_in_pipeline[i].get_pc();
							unsigned int sequence = instructions_in_pipeline[i].get_sequence();
							//allocate the rob entry with the necessary metadata
							//get the rob tag for this entry
							//this also updates dst with -1 (when no dst is specified)
							unsigned int rob_tag = rob->allocate_rob_entry(pc, dst, sequence);
							//store the rob tag associated with this instruction
							//useful for subsequent stages
							instructions_in_pipeline[i].set_rob_entry(rob_tag);
							//store the rob entry in the rmt only  if dst register is available
							//if not available, then the rmt does not contain that rob entry
							if(dst != -1)
							{
								//store the rob entry in rmt indexed via dst reg 
								//also set the valid bit to indicate it is stored in rob
								rmt->set_rob_tag(dst, rob_tag);
								rmt->set_valid_bit(dst);
							}
							//check only if src have registers associated otherwise store them as
							//"-1" in the rob as well
							if(src1 != -1)
							{
								//if the src index has a valid rmt entry then only set the
								//src rob (rename the src register with rob entry)
								if(rmt->get_valid_bit(src1))
								{
									//get the src1 rob entry
									int src1_rob = rmt->get_rob_tag(src1);
									//store the metadata to be used later
									instructions_in_pipeline[i].set_src1_rob(src1_rob);
								}
							}
							else
							{
								//store "-1"  when source register is not used
								instructions_in_pipeline[i].set_src1_rob(-1);
							}
							//do similar stuff for src2
							if(src2 != -1)
							{
								if(rmt->get_valid_bit(src2))
								{
									int src2_rob = rmt->get_rob_tag(src2);
									instructions_in_pipeline[i].set_src2_rob(src2_rob);
								}
							}
							else
							{
								instructions_in_pipeline[i].set_src2_rob(-1);
							}
							//set stage for the registers to REG_READ for register reads							
							instructions_in_pipeline[i].set_current_stage(REG_READ);
							break;
						}
					}
				}
				//rename stage sent its instructions to register read
				//and hence has space available
				meta->rename_busy = false;
			}
			//wait till width number of spaces are available
			else
			{
				//stall the cycles till then
				meta->rename_busy = true;
				incr_cycles_in_current_stage_due_to_stall(RENAME, instructions_in_pipeline);
			}
		}
		else
		{
			//if reg_read is stalled
			incr_cycles_in_current_stage_due_to_stall(RENAME, instructions_in_pipeline);
			meta->rename_busy = true;
		}
	}
	else
	{
		//at the start when the entire pipeline is empty
		meta->rename_busy = false;
	}
}

//register read stage
void regread(pipeline_data *meta, proc_params *param, vector<instruction>& instructions_in_pipeline, rob *rob)
{
	//no modelling of the values
	//hence can jsut read the readiness and that is enough for dispatch and issue queue
	if(instructions_in_pipeline.size() !=0)
	{
		//dispatch state is not busy
		if(meta->dispatch_busy == false)
		{
			//go thriugh all the n width number of instructions
			for(int j = 0; j < (int) param->width; j++)
			{
				//loop through all the instructions in pipeline and work with only those instructions
				//that are in the reg_read stage
				for(int i = 0; i < (int) instructions_in_pipeline.size(); i++)
				{
					if(instructions_in_pipeline[i].get_current_stage() == REG_READ)
					{
						//get the src1 rob tag
						//if it is not assigned any register tag or is available from ARF,
						//it is assigned -1 (always ready)
						int src1_rob = instructions_in_pipeline[i].get_src1_rob();
						//check the rob entry to decide readiness
						if(src1_rob != -1) 
						{	
							//src1 is ready in rob
							if(rob->is_rob_entry_ready(src1_rob) == true) {
								instructions_in_pipeline[i].set_src1_rob_rdy();
							}
							else{
								instructions_in_pipeline[i].clear_src1_rob_rdy(); 
							}

							//look for all the rob entries getting ready in this stage due to pre-wakeup
							//from execution stage
							for(int k = 0; k < (int) meta->rob_destinations_ready_this_cycle.size(); k++)
							{
								//get the src1 rob entry
								if(src1_rob == meta->rob_destinations_ready_this_cycle[k])
								{
									//src1 is ready due to prewakeup
									instructions_in_pipeline[i].set_src1_rob_rdy();
								}
							}
						}
						//do the same logic for src2
						int src2_rob = instructions_in_pipeline[i].get_src2_rob();
						if(src2_rob != -1) 
						{
							if(rob->is_rob_entry_ready(src2_rob) == true)
							{
								instructions_in_pipeline[i].set_src2_rob_rdy();
							}
							else
								instructions_in_pipeline[i].clear_src2_rob_rdy();

							for(int k = 0; k < (int) meta->rob_destinations_ready_this_cycle.size(); k++)
							{
								if(src2_rob == meta->rob_destinations_ready_this_cycle[k])
								{
									//cout << "(RR) wakeup from ex for rs2" << endl;
									instructions_in_pipeline[i].set_src2_rob_rdy();
								}
							}
						}
						//increment number of cuycles in this reg_read stage
						instructions_in_pipeline[i].incr_cycles_for_current_stage();
						//send the instruction to DISPATCH stage
						instructions_in_pipeline[i].set_current_stage(DISPATCH);
						break;
					}
				}
			}
			meta->reg_read_busy = false;
		}
		//if dispatch stage is busy
		else
		{
			//stall in reg_read stage
			incr_cycles_in_current_stage_due_to_stall(REG_READ, instructions_in_pipeline);
			//even during stall, ensure the src registers are getting ready due to bypass
			//if the bundle exists in the reg_read stage then stall the upper stages
			bool bundle_exists = false;
			for(int j = 0; j < (int) param->width; j++)
			{
				for(int i = 0; i < (int) instructions_in_pipeline.size(); i++)
				{
					if(instructions_in_pipeline[i].get_current_stage() == REG_READ && (int) instructions_in_pipeline[i].get_super_slot() == j+1)
					{
						bundle_exists = true;
						
						//check the readiness just the way I did above
						int src1_rob = instructions_in_pipeline[i].get_src1_rob();
						if(src1_rob != -1) 
						{
							if(rob->is_rob_entry_ready(src1_rob) == true) 
							{
								instructions_in_pipeline[i].set_src1_rob_rdy();
							}
							else
								instructions_in_pipeline[i].clear_src1_rob_rdy();
							for(int k = 0; k < (int) meta->rob_destinations_ready_this_cycle.size(); k++)
							{
								if(src1_rob == meta->rob_destinations_ready_this_cycle[k])
								{
									instructions_in_pipeline[i].set_src1_rob_rdy();
								}
							}
						}

						int src2_rob = instructions_in_pipeline[i].get_src2_rob();
						if(src2_rob != -1) 
						{
							if(rob->is_rob_entry_ready(src2_rob) == true)
							{
								instructions_in_pipeline[i].set_src2_rob_rdy();
							}
							else
								instructions_in_pipeline[i].clear_src2_rob_rdy();
							for(int k = 0; k < (int) meta->rob_destinations_ready_this_cycle.size(); k++)
							{
								if(src2_rob == meta->rob_destinations_ready_this_cycle[k])
								{
									instructions_in_pipeline[i].set_src2_rob_rdy();
								}
							}
						}
					}
				}
			}

			//if dispatch is busy but reg_read is free. In that case, rename should sent
			//instructions to reg_read		
			//reg read is busy if the bundle is still in reg_read
			if(bundle_exists == true)
				meta->reg_read_busy = true;
			//reg read is free if the bundle has moved forward
			else if(bundle_exists == false)
				meta->reg_read_busy = false;
		}
	}
	else
	{
		//no instructions are in the reg_read stage
		meta->reg_read_busy = false;
	}
}

//writeback stage is used to send bypass values to IQ and also update the ready bit in the rob
void writeback(pipeline_data *meta, vector<instruction>& instructions_in_pipeline, rob *rob)
{
	//For theinstructions in WB stage
	//1. Increment number of cycles in the stage
	//  -> instructions are never stalled in the wb stage
	//2. Get the rob entry for the instruction
	//  -> used to make that rob entry index ready 
	//3. Set the current stage of all the instructions in WB as RETIRE
	//when there are instrutions in the pipeline 
	if(instructions_in_pipeline.size() != 0)
	{
			//get only those instructions that are in writeback stage
		for(int j = 0; j < (int) instructions_in_pipeline.size(); j++)
		{
			if(instructions_in_pipeline[j].get_current_stage() == WRITE_BACK)
			{
				//increment the cycles for in pipeline for the WB stage
				instructions_in_pipeline[j].incr_cycles_for_current_stage();
				//get the rob index of all the instructions that are done with
				//execution and are in WB
				int rob_index = instructions_in_pipeline[j].get_rob_entry();
				//set that particular rob entry ready for retirement
				rob->set_rob_entry_ready(rob_index);

				//look for all the instructions that are ready after the execution
				//and free up their memory
				for(int k = 0; k < (int) meta->rob_destinations_ready_this_cycle.size(); k++)
				{
					if(meta->rob_destinations_ready_this_cycle[k] == rob_index)
					{
						//remove all the rob indexes for which the execution is done
						meta->rob_destinations_ready_this_cycle.erase(meta->rob_destinations_ready_this_cycle.begin() + k);
					}
				}
				//set the stage for these instructions to retire
				instructions_in_pipeline[j].set_current_stage(RETIRE);
			}
		}
	}
}


//retire stage for the pipeline
//retire width number of instructions from rob into ARF
void retire(pipeline_data *meta, proc_params *param, rob *rob, vector<instruction>& instructions_in_pipeline, rmt *rmt)
{
	//steps in retire stage
	//1. get all the instructions in the retire stage
	//2. increment the cycle number for the instructions that are yet to be retired (head has not reached them yet)
	//3. retire upto width number of instructions if they are ready (and head has reached there)
	//4. free up all the rob indexes from which instructions have retired
	//5. no need to reset rmt entry if the destintation was -1
	//   -> reset the rmt entries if rob index matches the rmt entry
	//get head and tail
	unsigned int head = rob->get_head();
	unsigned int tail = rob->get_tail();
	//when there are instructions in the pipeline
	if(instructions_in_pipeline.size() != 0)
	{
		//check the stage in which all the instructions are available
		for(int i = 0; i < (int) instructions_in_pipeline.size(); i++)
		{
			if(instructions_in_pipeline[i].get_current_stage() == RETIRE)
			{
				//increment the cycle number for all the instructions in this stage
				instructions_in_pipeline[i].incr_cycles_for_current_stage();
			}
		}
		//check upto width number for instructions for retiring
		for(int i = 0; i < (int) param->width; i++)
		{
			//if the instruction at head is ready to retire
			//this condition inherently takes care of the rob being empty
			//if the rob is empty => no instruction is ready and it does not
			//increment the head
			if(rob->is_ready_to_retire(head))
			{
				//get the age/sequence of the instruction that will be retired
				//useful for printing at  the end when instruction is retired
				unsigned int sequence = rob->get_sequence_for_entry(head);
				//retire the instruction pointed by head
				rob->retire_entry(head);

				//get the rmt table index to remove it from rmt
				int rmt_reg_index = rob->get_arf_dst(head);
				//check for only those instructions that have a dst register
				if(rmt_reg_index != -1)
				{
					//if the register index matches the entry of rob in rmt
					//clear that entry in rmt
					if(rmt->get_rob_tag(rmt_reg_index) == head){
						rmt->clear_valid_bit(rmt_reg_index);
					}
				}

				//emulate the cyclic buffer when incrementing head
				if(head == (param->rob_size) - 1)
					rob->set_head(0);
				else
					rob->set_head(head++);

				for(int i = 0; i < (int) instructions_in_pipeline.size(); i++)
				{
					if(instructions_in_pipeline[i].get_sequence() == sequence)
					{
						//before commiting instruction in ARF, print the contents of the instruction	
						instructions_in_pipeline[i].printstats();
						//remove the vector from memory
						instructions_in_pipeline.erase(instructions_in_pipeline.begin() + i);
						//if all instructions are removed, the simulation is done
						if(instructions_in_pipeline.size() == 0)
						{
							//all instructions in pipeline are committed
							//simulation is done
							meta->is_simulation_done = true;
						}
					}
				}
			}
		}
	}
	else
	{
		//instructions are yet to be sent to peipeline
		meta->is_simulation_done = false;
	}

}

