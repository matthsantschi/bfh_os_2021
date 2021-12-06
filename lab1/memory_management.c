#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "memory_management.h"

//pointer to free_frame_list
bool * free_frame_list_p;
int * page_table_p;
int * tlb_size_p;
int * length_offset_in_bits_p;
int * free_frames_number_p;

struct tlb
{
	int page_nr;
	int * frame_address;
	struct tlb *next;
};

struct tlb *head = NULL;
struct tlb *current = NULL;

uint64_t create_mask(uint64_t offset_length_p) {
  uint64_t mask = 0;
  for (size_t i = 0; i < offset_length_p; i++)
  {
    mask = mask + pow(2, i);
  }
  return mask;
}

void insert_tlb(int page_nr, int * frame_address) {
	struct tlb *link = (struct tlb*) malloc(sizeof(struct tlb));
	link->page_nr = page_nr;
	link->frame_address = frame_address;
	link->next = head;
	head = link;
}

uint64_t * seach_tlb(struct tlb* head, int page_nr) {
	struct tlb *ptr = head;
	if(ptr == NULL) {
			return NULL;
		}

	for (size_t i = 0; i < * tlb_size_p; i++)
	{
		if (ptr->page_nr == page_nr) {
			return ptr->frame_address;
		}
		
		ptr = ptr->next;
	}
	// if not found return 0
	return NULL;
}

void print_tlb(){
	struct tlb *ptr = head;
	while (ptr != NULL)
	{
		printf("%i -> %i \n", ptr->page_nr, * ptr->frame_address);
		ptr = ptr->next;
	}
	
}

//todo implement some cleanup for the linked list



int memory_init_data(int number_processes,
		     int free_frames_number,
		     int length_VPN_in_bits,
		     int length_PFN_in_bits,
		     int length_offset_in_bits,
		     int tlb_size)                
{
	//set pointers;
	*tlb_size_p = &tlb_size;
	*length_offset_in_bits_p = &length_offset_in_bits;
	*free_frames_number_p = &free_frames_number;
	// create free frame list
	*free_frame_list_p = (bool *) malloc(sizeof(bool) * free_frames_number);
	// set all entries == true
	for (size_t i = 0; i < free_frames_number; i++)
	{
		free_frame_list_p[i] = true;
	}

	// create page table
	// allocate array to store vpn-number -> frame-number
	int number_of_entries_in_page_table = (pow(2, length_VPN_in_bits) -1);
	page_table_p = (int *) malloc(sizeof(int *) * number_of_entries_in_page_table);
return 0;
}


int get_physical_address(uint64_t virtual_address,
			 int process_id,
			 uint64_t* physical_address,
			 int* tlb_hit)
{
	uint64_t page_number = (virtual_address >> * length_offset_in_bits_p);
	uint64_t offset = (virtual_address & create_mask(*length_offset_in_bits_p));
	uint64_t * physical_frame_p = seach_tlb(head, page_number);
	// if we fount the frame in tlb return it -> node;
	if(* physical_frame_p != NULL ){
		return * physical_frame_p;
	}
	physical_frame_p = &page_table_p[page_number];
	if(* physical_frame_p != NULL) {
		return * physical_frame_p;
	}
	// we did not find this page_number so we have to load it in memory
	* physical_frame_p = 0;
	while (free_frame_list_p[*physical_frame_p] == false && *physical_frame_p <= free_frames_number_p )
	{
		*physical_frame_p ++;
	}
	if(*physical_frame_p > free_frames_number_p) {
		// we dont have memory left, return NULL
		return NULL;
	}
	// if we are not out of memory physical_frame_p holds the frame number we can use
	free_frame_list_p[*physical_frame_p] = false;
	// update page_table
	page_table_p[page_number] = physical_frame_p;
	// update tlb
	insert_tlb(page_number, physical_frame_p); 
	
	physical_address = (*physical_frame_p << * length_offset_in_bits_p);
	uint64_t helper = (*physical_address | offset);

  return helper;
}

