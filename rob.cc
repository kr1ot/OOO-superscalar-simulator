#include <vector>

#include <stdio.h>
#include <iostream>
using namespace std;

//contains all the values associated with a single
//rob entry
class rob_entry
{
	private:
        //rob index. used for rename and retire stage
		unsigned int rob_index; 
        //whether the entry is contains an active instrcution
        //in pipeline
        //valid = true --> contained in the pipeline
        //valid = false --> is available to store a new instrcution 
		bool valid; 
        //arf destination tag
		int arf_dst;
        //ready bit indicating the instruction is ready to be retired 
        //ready = true --> ready to retire to ARF
        //ready = false --> instrcution still in pipeline
		bool ready; 
        //age of the instruction
		unsigned int seq;
		unsigned long pc; 

    public:
        //setter getter methods for all the class variables
        //sets the rob index when a rob entry is given in rename stage
        void set_rob_index(unsigned int rob_entry_index){
            rob_index = rob_entry_index;
        }
        
        //valid bit
		void set_valid_bit(){
            valid = true;
        }
		bool get_valid_bit(){
            return valid;
        }		
		void clear_valid_bit(){
            valid = false;
        }

        //ready bits
		void set_ready_bit(){
            ready = true;
        }
        bool get_ready_bit(){
            return ready;
        }
        void clear_ready_bit(){
            ready = false;
        }

        //arf destination tag
		void set_arf_dst(unsigned int dst_tag){
            arf_dst = dst_tag;
        }
		int get_arf_dst(){
            return arf_dst;
        }

        //age matrix for instructions
        //sets the age matrix for an instruction
		void set_sequence(unsigned int sequence){
            seq = sequence;
        }
		unsigned int get_sequence(){
            return seq;
        }
		
        //set the pc for the entry
		void set_pc(unsigned long pc){
            pc = pc;
        }

		//display function for debugging purposes
		void display_line();
};

void rob_entry::display_line()
{
	printf("\n\tindex: %u\tdst: %d\trdy: %u\tv: %u\tpc: %lu\tseq: %u", rob_index, arf_dst, ready, valid, pc, seq);
}

class rob
{
	private:
        //create vector for all the rob entries
		vector<rob_entry> rob;
        //based on rob size will create the depth of the cyclic buffer
		unsigned int rob_size;
        //head and tail for popping and pushing the instructions in the buffer
		unsigned int rob_head;
		unsigned int rob_tail;
        //determines the number of instructions that need to be retired together
		unsigned int pipeline_width_for_rob_retire;
	
	public:
        //initalize rob class variables 
        //Also, create the rob buffer with the passed size
        //TODO: Create a constructor??
        void rob_initialize(unsigned int rob_size, unsigned int width);

        //make the rob entry ready. used by WB stage
        void set_rob_entry_ready(int rob_tag){
            rob[rob_tag].set_ready_bit();
        }

        //check if rob entry is ready. used by register read and issue queue stage
        //to send the instruction to execution
		bool is_rob_entry_ready(unsigned int rob_tag){
            return rob[rob_tag].get_ready_bit();
        }

		//sets the rob head for instruction retire to arf
        void set_head(unsigned int h){
            rob_head = h;
        }
        //get the rob head for retire
        //even I would retire if I got one! (o-O)
		unsigned int get_head(){
            return rob_head;
        }

        void set_tail(unsigned int t){
            rob_tail = t;
        }
        //used for checking whether the rob is empty
		unsigned int get_tail(){
            return rob_tail;
        }

        //allocates a rob entry ni the rename stage. This function also increments
        //the tail after allocation
        //Also, return the rob index where the entry was stored
        //useful for storing index in rename table
		unsigned int allocate_rob_entry(unsigned long pc_val, int dst_val, unsigned int seq);

        
        //check if the width number of spaces are available in rob. this is to ensure 
        //instructions dont move from rename to register read till all the width number
        //of instructions are stored in rob
        //TODO: Think and understand 
		bool check_width_amount_free_entries();
        
        //check if the instruction with a given rob tag is ready to retire
        //retire onlyw when this returns true
		bool is_ready_to_retire(unsigned int rob_tag){
            return rob[rob_tag].get_ready_bit();
        }
		
        //retires the rob entry
        //head is incremented in the main retire pipeline to ensure the width number of instructions
        //can be retired together
		void retire_entry(unsigned int rob_tag){
            rob[rob_tag].clear_valid_bit();
        }
		
        //get the age of the rob entry 
        //useful for printing before retiring
		unsigned int get_sequence_for_entry(unsigned int rob_tag){
            return rob[rob_tag].get_sequence();
        }

        //useful for updating RMT when retiring the instruction
        int get_arf_dst(unsigned int rob_tag){
            return rob[rob_tag].get_arf_dst();
        }
		
		void display_rob();
		//display function for debugging

};


void rob::rob_initialize(unsigned int rob_size, unsigned int width)
{
    //set the size of the rob
	this->rob_size = rob_size;
    //create the number of enteries in the rob based on the size
	rob.resize(rob_size);

    //point head and tail at the same index. let's say 0
    rob_head = 0;
	rob_tail = 0; 

    //width of the pipeline for rob retire
	pipeline_width_for_rob_retire = width;
    //clear all the valid bits at the start of simulation to ensure all the rob enteries
    //are available to be written
    //rest of the bits dont matter if valid = 0
	for(int i = 0; i < (int) rob_size; i++){
		rob[i].clear_valid_bit();
    }
}


unsigned int rob::allocate_rob_entry(unsigned long pc_val, int dst_val, unsigned int seq)
{
	//assign the previous tail index before incrementing 
    //the previous value is stored in rmt
	unsigned int prev_tail_index = rob_tail;

    //allocating blindly. call this function only if the rob is not full
    //creating cyclic buffer of rob size
	if(prev_tail_index == (rob_size - 1))
		set_tail(0);
	else
        //point to next index
		set_tail(rob_tail++);
	
    //set all the required metadatas for the rob entry
	rob[prev_tail_index].set_rob_index(prev_tail_index);
	rob[prev_tail_index].set_valid_bit();
	rob[prev_tail_index].set_arf_dst(dst_val);
	rob[prev_tail_index].clear_ready_bit();
	rob[prev_tail_index].set_sequence(seq);
	rob[prev_tail_index].set_pc(pc_val);

	return prev_tail_index;
}

void rob::display_rob()
{
	int robsize = rob.size();
	printf("\n=================ROB Contents=================\n");
	printf("\tHead = %u, Tail = %u", rob_head, rob_tail);
	for(int i = 0; i < robsize; i++)
	{
		rob[i].display_line();
	}
	cout << endl;
}

bool rob::check_width_amount_free_entries()
{
	unsigned int entries_free = 1;
	bool allow_push = false;
	unsigned int local_tail = rob_tail;

	for(int i = 0; i < (int) pipeline_width_for_rob_retire; i++)
	{
		if(rob[local_tail].get_valid_bit() == false)
			entries_free &= 1;
		else
			entries_free &= 0;

		if(local_tail == rob_size - 1)
			local_tail = 0;
		else
			local_tail++;
	}
	if(entries_free)
	{
		//super scalar width amount of entries are free
		//so we can allow pushing of that many entries in the rename stage
		allow_push = true;
	}
	else
		allow_push = false;

	return allow_push;
}
