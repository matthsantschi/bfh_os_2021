#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <math.h>
#include "memory_management.h"

// pointer to free_frame_list
static bool *free_frame_list_p;
static int **page_table_p;
static int tlb_size_c;
static int length_offset_in_bits_c;
static int free_frames_number_c;
static int number_of_entryes_in_page_table_c;
static struct tlb *head;
static int nbr_of_processes_c;
pthread_mutex_t mutex;
pthread_mutex_t mutex_tlb;

struct tlb
{
  uint64_t page_nr;
  uint64_t frame_address;
  uint64_t process_id;
  struct tlb *next;
};

uint64_t create_mask(uint64_t offset_length_p)
{
  uint64_t mask = 0;
  for (size_t i = 0; i < offset_length_p; i++)
  {
    mask = mask + pow(2, i);
  }
  return mask;
}

void print_tlb()
{
  printf("*** PRINT TLB **** \n");
  struct tlb *pointer = head;
  if (pointer == NULL)
  {
    return;
  }
  while (pointer != NULL)
  {
    printf("Loop-tlb: page: %llx, frame %llx, process_id: %llx \n", pointer->page_nr, pointer->frame_address, pointer->process_id);
    pointer = pointer->next;
  }
}
/**
 * @brief allocates memory for a new struct tlb and ads it on head[process_id]
 * 
 * @param process_id 
 * @param page_nr 
 * @param frame_address 
 * @return ** void 
 */
void insert_tlb(int process_id, uint64_t *page_nr, uint64_t *frame_address)
{
  // printf("---INSERT INTO TLB process_id: %i, %llx, %llx \n", process_id, *page_nr, *frame_address);
  struct tlb *link = calloc(1, sizeof(struct tlb));
  link->page_nr = *page_nr;
  link->frame_address = *frame_address;
  link->process_id = process_id;
  link->next = head;
  head = link;
}
/**
 * @brief searching the linked list but only in the given amount tlb_size_c
 * 
 * 
 * @param process_id 
 * @param page_nr 
 * @return uint64_t return 0 if search was unsuccessfull
 */
uint64_t seach_tlb(int process_id, uint64_t *page_nr)
{
  struct tlb *ptr = head;
  if (ptr == NULL)
  {
    return 0;
  }

  for (size_t i = 0; i <= tlb_size_c; i++)
  {
    if (ptr == NULL)
    {
      return 0;
    }
    if (ptr->page_nr == *page_nr && ptr->process_id == process_id)
    {
      return ptr->frame_address;
    }
    // if we reach the max size, free ptr->next
    if( i == tlb_size_c ){
      free(ptr->next);
      ptr->next = NULL;
      return 0;
    }
    ptr = ptr->next;
  }
  // if not found return 0
  return 0;
}

uint64_t load_into_memory()
{
  int physical_frame = 0;
  while (free_frame_list_p[physical_frame] == false && physical_frame <= free_frames_number_c)
  {
    physical_frame++;
  }
  if (physical_frame <= free_frames_number_c)
  {
    free_frame_list_p[physical_frame] = false;
    return physical_frame;
  }
  return -1;
}
// todo implement some cleanup for the linked list

int memory_init_data(int number_processes,
                     int free_frames_number,
                     int length_VPN_in_bits,
                     int length_PFN_in_bits,
                     int length_offset_in_bits,
                     int tlb_size)
{
  // set pointers;
  tlb_size_c = tlb_size-1;
  length_offset_in_bits_c = length_offset_in_bits;
  free_frames_number_c = free_frames_number + 1;
  nbr_of_processes_c = number_processes;
  // initialize pointers for each process head[process_id] -> first note of linked list
  head = NULL;

  free_frame_list_p = (bool *)calloc(free_frames_number_c + 1, sizeof(bool *));
  // set all entries == true
  free_frame_list_p[0] = false; // we use 0 as error code so we cant use add 0
  for (size_t i = 1; i < free_frames_number + 1; i++)
  {
    free_frame_list_p[i] = true;
  }

  // create page table for each process
  // allocate array to store vpn-number -> frame-number
  number_of_entryes_in_page_table_c = (pow(2, length_VPN_in_bits) - 1);
  page_table_p = calloc(number_processes, sizeof(int *));
  for (size_t i = 0; i < number_processes; i++)
  {
    page_table_p[i] = calloc(number_of_entryes_in_page_table_c, sizeof(int));
  }
  // initialize mutex
  if (pthread_mutex_init(&mutex, NULL) != 0 || pthread_mutex_init(&mutex_tlb, NULL) != 0)
  {
    // returns 0 if successfully created mutex
    return 1;
  };

  return 0;
}

void combine_page_and_offset(uint64_t *physical_address_p, uint64_t *page_number_p, uint64_t *offset_p)
{
  uint64_t page_number = *page_number_p;
  uint64_t offset = *offset_p;
  uint64_t physical_address = (page_number << length_offset_in_bits_c);
  physical_address = (physical_address | offset);
  *physical_address_p = physical_address;
}

int get_physical_address(uint64_t virtual_address,
                         int process_id,
                         uint64_t *physical_address,
                         int *tlb_hit)
{

  uint64_t page_number = (virtual_address >> length_offset_in_bits_c);
  if (page_number > number_of_entryes_in_page_table_c)
  {
    // page_number is out of bound
    return 1;
  }
  pthread_mutex_lock(&mutex_tlb);
  print_tlb();
  pthread_mutex_unlock(&mutex_tlb);
  uint64_t offset = (virtual_address & create_mask(length_offset_in_bits_c));
  uint64_t physical_frame = 0;
  pthread_mutex_lock(&mutex_tlb);
  physical_frame = seach_tlb(process_id, &page_number);
  pthread_mutex_unlock(&mutex_tlb);

  // if we fount the frame in tlb return it -> node;
  if (physical_frame != 0)
  {
    *tlb_hit = 1;
    combine_page_and_offset(physical_address, &physical_frame, &offset);
    return 0;
  }
  physical_frame = page_table_p[process_id][page_number];
  if (physical_frame != 0)
  {
    *tlb_hit = 0;
    combine_page_and_offset(physical_address, &physical_frame, &offset);
    pthread_mutex_lock(&mutex_tlb);
    insert_tlb(process_id, &page_number, &physical_frame);
    pthread_mutex_unlock(&mutex_tlb);
    return 0;
  }

  // we did not find this page_number so we have to load it in memory
  pthread_mutex_lock(&mutex);
  physical_frame = load_into_memory();
  pthread_mutex_unlock(&mutex);
  if (physical_frame == -1)
  {
    return 1;
  }
  // update page_table
  page_table_p[process_id][page_number] = physical_frame;
  // update tlb
  pthread_mutex_lock(&mutex_tlb);
  insert_tlb(process_id, &page_number, &physical_frame);
  pthread_mutex_unlock(&mutex_tlb);

  *tlb_hit = 0;
  combine_page_and_offset(physical_address, &physical_frame, &offset);
  return 0;
};

int free_memory()
{
  free(free_frame_list_p);

  while (head->next != NULL)
  {
    struct tlb *temp = head;
    head = temp->next;
    free(temp);
  }
  for (size_t i = 0; i < nbr_of_processes_c; i++)
  {
    free(page_table_p[i]);
  }
  pthread_mutex_destroy(&mutex);
  pthread_mutex_destroy(&mutex_tlb);
  free(page_table_p);
  free(head);
  return 0;
};
