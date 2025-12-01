#include <vector>

#include <stdio.h>
#include <iostream>
using namespace std;

//metadata associated with an issue queue entry
class issue_queue_entry
{
	public:
        
        //if the entry is in issue queue valid = true
        //issue queue is open for a new entry, valid = false
		bool valid;
        //age matrix for the isntruction entry
        unsigned int seq;
        //tags for source register
		int src1;
		int src2;
        //tag to rob entry
		int dst_tag; 
        //to identify where the src registers sit
        //useful to indicate readiness of the registers and their
        //dependence on rob if they are sitting in rob
		bool is_src1_in_arf;
		bool is_src2_in_arf; 
        //is the src ready (can be udpated from the wakeup from execute)
		bool src2_rdy;
		bool src1_rdy;
        //count the number of cycles 
		unsigned int cycles;
};

class issue_queue
{
	private:
        //vector for creating issue queue
		vector<issue_queue_entry> iq;

        unsigned int iq_size;
		unsigned int iq_pipeline_width;

	public:
		

        //initialize issue queue
		void issue_queue_initialize(unsigned int iq_size, unsigned int width);

        //sets the age of the instruction in the pipeline
        void set_sequence(unsigned int index, unsigned int sequence)	{iq[index].seq = sequence;}
		unsigned int get_sequence(unsigned int idx)	{return iq[idx].seq;}


        int get_src1_rob(int index){
            return iq[index].src1;
        }

		int get_src2_rob(int index){
            return iq[index].src2;
        }

        bool get_is_src1_arf(int index){
            return iq[index].is_src1_in_arf;
        }
		bool get_is_src2_arf(int index){
            return iq[index].is_src2_in_arf;
        }

        void make_src1_rdy(int index){
            iq[index].src1_rdy = true;
        }
		void make_src2_rdy(int index){
            iq[index].src2_rdy = true;
        }

        //cycle related methods
        //get the cycle number for a givem index
		unsigned int get_cyc(int index){
            return iq[index].cycles;
        }
        //method to increment cycle
		void incr_cyc(int index){
            iq[index].cycles++;
        }
        //clear the cycles when sending to next stage
		void clear_cyc(int index){
            iq[index].cycles = 0;
        }
        //increment cycle for all the entries sitting in issue queue
		void incr_cyc_for_all_valid_entries();
		
		//to check if there is a valid entry. Useful for issuing instruction to execute
		bool has_valid_entries();

		//checks if issue queue has width amount of free entries
		bool check_for_width_free_entries();
		
		int find_oldest_ready_instr();
		//finds the old instruction in the IQ and returns its index in the IQ

		void set_iq_entry(int, int, int, unsigned int, int, bool, bool);
		//set values for a particular iq entry

		int get_free_entry();
		//returns a free entry in the IQ
		//
		//must be called after checking for width amount of free entries

		void display_contents();

		void free_up_entry(int index){
            iq[index].valid = false;
        }

		void make_entries_ready_with_src_as(int dst_in_rob);
		//this function will look for valid entries 
		//in the issue queue and make them ready
};


void issue_queue::issue_queue_initialize(unsigned int iq_size, unsigned int width)
{
	this->iq_size = iq_size;
	iq_pipeline_width = width;
	iq.resize(iq_size); //TODO: Check. changed from reserve to resize
    //all the entries are free at the start
	for(int i = 0; i < (int) iq_size; i++)
		iq[i].valid = false;
}

void issue_queue::incr_cyc_for_all_valid_entries()
{
	for(int i = 0; i < (int) iq_size; i++)
	{
		if(iq[i].valid == true)
			incr_cyc(i);
	}
}

void issue_queue::make_entries_ready_with_src_as(int dst_in_rob)
{
	vector<int> valid_indices;	
	//extract all the valid indices
	for(int i = 0; i < (int) iq_size; i++)
	{
		if(iq[i].valid == true)
			valid_indices.push_back(i);
	}

	if(valid_indices.size() != 0)
	{
		for(int i = 0; i < (int) valid_indices.size(); i++)
		{
			if(iq[valid_indices[i]].src1 == dst_in_rob)
				iq[valid_indices[i]].src1_rdy = true;

			if(iq[valid_indices[i]].src2 == dst_in_rob)
				iq[valid_indices[i]].src2_rdy = true;
		}
	}
}

bool issue_queue::has_valid_entries()
{
	bool has_valid_entry = false;
	for(int i = 0; i < (int) iq_size; i++)
	{
		if(iq[i].valid == true)
		{
			has_valid_entry = true;
			break;
		}
	}
	
	return has_valid_entry;
}

void issue_queue::display_contents()
{
	printf("\n-------------------IQ Contents-----------------\n");
	printf("\tSize: %u, SuperScalarWidth: %u\n", iq_size, iq_pipeline_width);
	for(int i = 0; i < (int) iq_size; i++)
	{
		printf("valid: %u, dst: %d, src1: %d,%u, src2: %d,%u seq: %u\n", iq[i].valid, iq[i].dst_tag, iq[i].src1, iq[i].src1_rdy, iq[i].src2, iq[i].src2_rdy, iq[i].seq);
	}
}

int issue_queue::get_free_entry()
{
	int free_entry_index = -1;
	for(int i = 0; i < (int) iq_size; i++)
	{
		if(iq[i].valid == false)
		{
			free_entry_index = i;
			break;
		}
	}
	return free_entry_index;
}

void issue_queue::set_iq_entry(int dst, int rs1, int rs2, unsigned int sequence, int index, bool src1_in_arf, bool src2_in_arf)
{
	iq[index].dst_tag = dst;
	iq[index].src1 = rs1;
	iq[index].src2 = rs2;
	iq[index].seq = sequence;
	iq[index].is_src1_in_arf = src1_in_arf;
	iq[index].is_src2_in_arf = src2_in_arf;
	iq[index].src1_rdy = false;
	iq[index].src2_rdy = false;
	iq[index].cycles = 1;

	//if source is in arf its always ready
	if(src1_in_arf == true)
		iq[index].src1_rdy = true;

	if(src2_in_arf == true)
		iq[index].src2_rdy = true;

	iq[index].valid = true;
}



int issue_queue::find_oldest_ready_instr()
{
	int oldest_instr_idx = 0;

	vector<int> valid_indices;

	//extract all the valid indices
	for(int i = 0; i < (int) iq_size; i++)
	{
		if(iq[i].valid == true && iq[i].src1_rdy == true && iq[i].src2_rdy == true)
			valid_indices.push_back(i);
	}

	if(valid_indices.size() != 0)
	{
		int valid_index = valid_indices[0];
		for(int j = 0; j < (int) valid_indices.size(); j++)
		{
			if(iq[valid_indices[j]].seq < iq[valid_index].seq)
				valid_index = valid_indices[j];
		}
		oldest_instr_idx = valid_index;
	}
	else
	{
		//nothing is ready
		oldest_instr_idx = -1;
	}

	return oldest_instr_idx;
}

bool issue_queue::check_for_width_free_entries()
{
	
	int num_free_entries = 0;
	bool dispatch = false;
	for(int i = 0; i < (int) iq_size; i++)
	{
		if(iq[i].valid == false)
			num_free_entries++;
	}

	//cout << "Debug from IQ class: number of free entries = " << num_free_entries << endl;

	if(num_free_entries >= (int) iq_pipeline_width)
		dispatch = true;
	else
		dispatch = false;
	return dispatch;
}
