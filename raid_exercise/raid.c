/*
Description:    RAID 4 exercise
                Implement the missing functions write_raid() and read_raid()
                so that the test in main() passes.
Note:           Reading and writing must be possible after one of the disks failed.
                It is also possible that the disk containing the parity data fails.
                Change the test data as you fit.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <assert.h>

#define TOTAL_SIZE 4096
#define BLOCK_SIZE 512
#define NBR_DISKS 5
#define FAIL_DISK 1

uint8_t* disks[NBR_DISKS];

// initializes a raid array with NBR_DISK
// 0..NBR_DISKS - 2 are data disks NBR_DISKS - 1 the parity disk
void init_raid(void);

// write a byte to the raid array
// value    - the byte to write
// position - the position to write to
// the function calculates which disks to write to and writes parity
// NOTE: one of the disks (data or parity) may be offline
void write_raid(uint8_t value, long position);

// read a byte from thr raid array
// position - the position to read from 
// returns the vale read at position
// NOTE: one of the disks (data or parity) may be offline
uint8_t read_raid(long position);


// test program
int main(int argc, char *argv[])
{
    FILE* input;
    long pos = 0;
    char ch;

    init_raid();  // set up raid
    assert(NULL !=(input = fopen("testdata", "r")));  // open test data

    // write first part to raid
    while(EOF != (ch = fgetc(input)))
    {
        write_raid(ch, pos);
        if(++pos == TOTAL_SIZE / 2) {
            break;
        }
    }

    // kill a disk
    free(disks[FAIL_DISK]);
    disks[FAIL_DISK] = NULL;
    
    // continue writing second part to raid
    while(EOF != (ch = fgetc(input)))
    {
        write_raid(ch, pos);
        if(++pos == TOTAL_SIZE) {
            break;
        }
    }

    // compare file content with data saved in raid
    fseek(input, 0, SEEK_SET);  // return to start of dile
    pos = 0;
    while(EOF != (ch = fgetc(input)))
    {
        char read = read_raid(pos++);
        assert(read == ch);
    }

    // cleanup
    for (int i = 0; i < NBR_DISKS; i++) {
        if(disks[i] != NULL) {
            free(disks[i]);
            disks[i] = NULL;
        }
    }

    return 0;
}

uint8_t read_raid(long position)
{

}

void write_raid(uint8_t value, long position ){

}

void init_raid(void)
{
    // validate configuration
    assert(NBR_DISKS > 1);  // at least two disks (one for data, one for parity)
    assert(TOTAL_SIZE % (NBR_DISKS -1 ) == 0);  // size is even distributable to disks
    assert((TOTAL_SIZE / (NBR_DISKS -1)) % BLOCK_SIZE == 0);  // single disk is dividable by block size
    assert(FAIL_DISK <= NBR_DISKS);  // disk to fail is one of the raid array

    // allocate memory to simulate disks
    for(int i = 0; i < NBR_DISKS; i++) {
        uint8_t* disk;
        assert(NULL !=(disk = (uint8_t*)calloc(TOTAL_SIZE / (NBR_DISKS - 1), sizeof(uint8_t))));
        disks[i] = disk;
    }
}
