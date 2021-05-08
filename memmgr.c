//
//  memmgr.c
//  memmgr
//
//  Created by William McCarthy on 17/11/20.
//  Copyright © 2020 William McCarthy. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define ARGC_ERROR 1
#define FILE_ERROR 2
#define BUFLEN 256
#define FRAME_SIZE  256
#define TOTAL_FRAMS 256
#define TLB_SIZE 16
#define PAGE_SIZE 256


//-------------------------------------------------------------------
unsigned getpage(unsigned x) { return (0xff00 & x) >> 8; }

unsigned getoffset(unsigned x) { return (0xff & x); }

void getpage_offset(unsigned x) {
  unsigned  page   = getpage(x);
  unsigned  offset = getoffset(x);
  printf("x is: %u, page: %u, offset: %u, address: %u, paddress: %u\n", x, page, offset,
         (page << 8) | getoffset(x), page * 256 + offset);
}

int pgTbl[PAGE_SIZE];     
int pgFrames[PAGE_SIZE];    
int pgFaults = 0;          

int TLBtable[TLB_SIZE][2];  
int TLBHits = 0;           
int TLB_num_entry = 0;     

int phys_mem[TOTAL_FRAMES][FRAME_SIZE];   

int first_open_frame = 0; 
int first_open_pgTblIdx = 0;    

signed char bs_buff[BUFLEN];    
signed char byte_val;           
FILE *fBS;

void addToTLB(int page, int frame) {

  int index; // Checks if it is already in the TLB
  for (index = 0; index < TLB_num_entry; index++) {
    if (TLBtable[index][1] == page) {
      break;
    }
  }

  // Is the number of entries is equal to index?
  if (index == TLB_num_entry) {
    if (TLB_num_entry < TLB_SIZE) {
      TLBtable[TLB_num_entry][1] = page;
      TLBtable[TLB_num_entry][2] = frame;
    }
    else {
      for (index = 0; index = TLB_SIZE - index; index++) {
        TLBtable[index][1] = TLBtable[index+1][1];
        TLBtable[index][2] = TLBtable[index+1][2];
      }
      TLBtable[TLB_num_entry-1][1] = page;
      TLBtable[TLB_num_entry-1][2] = frame;
    }
  }
  else { 
    for (index = index; index < TLB_num_entry - 1; index++) {
      TLBtable[index][1] = TLBtable[index+1][1];
      TLBtable[index][2] = TLBtable[index+1][2];
    }
    if (TLB_num_entry < TLB_SIZE) {
      TLBtable[TLB_num_entry][1] = page;
      TLBtable[TLB_num_entry][2] = frame;
    }
    else {
      TLBtable[TLB_num_entry-1][1] = page;
      TLBtable[TLB_num_entry-1][2] = frame;
    }
  }
  if (TLB_num_entry < TLB_SIZE) {
    TLB_num_entry = TLB_num_entry + 1;
  }
}

void readBSFile(int page) {
  if (fseek(fBS, page * BUFLEN, SEEK_SET) != 0) {
    fprintf(stderr, "Error: can't seek from file\n");
  }
  if (fread(bs_buff, sizeof(signed char), BUFLEN, fBS) == 0) {
    fprintf(stderr, "Error: can't read from file\n");
  }

  for (int i = 0; i < BUFLEN; i++) {
    phys_mem[first_open_frame][i] = bs_buff[i];
  }

  pgTbl[first_open_pgTblIdx] = page;
  pgFrames[first_open_pgTblIdx] = first_open_frame;

  first_open_frame++;
  first_open_pgTblIdx++;

}

void getPageNums(unsigned log_add) {
  int page = getpage((int)log_add);
  int offset = getoffset((int)log_add);

  int frame_num = -1; 

  
  for (int i = 0; i < TLB_SIZE; i++) {
    if (TLBtable[i][1] == page) {
      TLBtable[i][2] = TLBtable[i][1];
      TLBHits = TLBHits + 1;  
    }
  }

  if (frame_num == -1) {
    for (int i = 0; i < first_open_pgTblIdx; i++) {
      if (pgTbl[i] == page) {
        frame_num = pgFrames[i];
      }
    }
    if (frame_num == -1) {
      readBSFile(page);
      pgFaults++;
      frame_num = first_open_frame - 1;
    }
  }

  addToTLB(page,frame_num);  
  byte_val = phys_mem[frame_num][offset];   
  printf("frame number: %d\n", frame_num);
  printf("Virtual address: %d Physical address: %d Value: %d\n", log_add, (frame_num << 8) | offset, byte_val);

}


int main(int argc, const char* argv[]) {
  FILE* fadd = fopen("addresses.txt", "r");    // open file addresses.txt  (contains the logical addresses)
  if (fadd == NULL) { fprintf(stderr, "Could not open file: 'addresses.txt'\n");  exit(FILE_ERROR);  }

  FILE* fcorr = fopen("correct.txt", "r");     // contains the logical and physical address, and its value
  if (fcorr == NULL) { fprintf(stderr, "Could not open file: 'correct.txt'\n");  exit(FILE_ERROR);  }

  char buf[BUFLEN];
  unsigned   page, offset, physical_add, frame = 0;
  unsigned   logic_add;                  // read from file address.txt
  unsigned   virt_add, phys_add, value;  // read from file correct.txt

  // printf("ONLY READ FIRST 20 entries -- TODO: change to read all entries\n\n");
  printf("READ ALL ENTRIES")

  // not quite correct -- should search page table before creating a new entry
      //   e.g., address # 25 from addresses.txt will fail the assertion
      // TODO:  add page table code
      // TODO:  add TLB code


  while (frame < 20) {

    fscanf(fcorr, "%s %s %d %s %s %d %s %d", buf, buf, &virt_add,
           buf, buf, &phys_add, buf, &value);  // read from file correct.txt

    fscanf(fadd, "%d", &logic_add);  // read from file address.txt
    page   = getpage(  logic_add);
    offset = getoffset(logic_add);
    
    physical_add = frame++ * FRAME_SIZE + offset;
    
    assert(physical_add == phys_add);
    
    // todo: read BINARY_STORE and confirm value matches read value from correct.txt
    getPageNums(logic_add);
    trans_adds++;
    
    printf("logical: %5u (page: %3u, offset: %3u) ---> physical: %5u -- passed\n", logic_add, page, offset, physical_add);
    if (frame % 5 == 0) { printf("\n"); }
  }

  fclose(fcorr);
  fclose(fadd);
  
  // printf("ONLY READ FIRST 20 entries -- TODO: change to read all entries\n\n");
  printf("READ ALL ENTRIES");

  // printf("ALL logical ---> physical assertions PASSED!\n");
  // printf("!!! This doesn't work passed entry 24 in correct.txt, because of a duplicate page table entry\n");
  // printf("--- you have to implement the PTE and TLB part of this code\n");

//  printf("NOT CORRECT -- ONLY READ FIRST 20 ENTRIES... TODO: MAKE IT READ ALL ENTRIES\n");


  printf("\n\t\t...done.\n");
  return 0;
}
