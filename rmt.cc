#include "sim_proc.h"
#include <vector>

#include <iostream>
#include <stdio.h>
using namespace std;

//to emulate the rmt
class rmt
{
    //the rmt has same size as the number of registers in the architecture=67
    //each rmt entry is associated with a valid bit and rob entry where it is stored
    //valid = true --> stored in rob
    //valid = false --> stored in ARF (do not care about rob entry)
	private:
        bool valid[67];
        unsigned int rob_entry[67];
	
	public:
        //constructor for creating the rmt table
        void rmt_initialize();

        //gets the rob tag in rename stage for the source registers
        unsigned int get_rob_tag(int reg_index)	{
            return rob_entry[reg_index];
        }
        //set the rob tag in rename stage. this is used when sending an entry into
        //ROB
        void set_rob_tag(int reg_index, unsigned int rob_tag){
	        rob_entry[reg_index] = rob_tag;	
        }

        //get the valid bit value for a given rmt entry
        bool get_valid_bit(int reg_index){
            return valid[reg_index];
        }

        //set the valid bit for a given rmt entry
        //is used in the rename stage for updating rob entry of a destination
        void set_valid_bit(int reg_index){
            valid[reg_index] = true;
        }

        //clears the valid bit when an instrcution retires from rob         
        void clear_valid_bit(int reg_index){
            valid[reg_index] = false;
        }

        //for checking the entries
        void display_rmt();
};


//TODO: Create a constructor???
void rmt::rmt_initialize()
{
	//at the start of simulation all the
    //rmt entries point to ARF registers 
    //do that by making valid = false for all entries
	for(int i = 0; i < 67; i++)
		valid[i] = false;
}

void rmt::display_rmt()
{
	printf("\n-------------RMT------------\n");
	for(int i = 0; i < 67; i++)
	{
		printf("valid: %u\trob_tag: %u\n", valid[i], rob_entry[i]);
	}
}
