/*Malia Bowman and Nelson Swasono*/
#include "tips.h"

/* The following two functions are defined in util.c */

/* finds the highest 1 bit, and returns its position, else 0xFFFFFFFF */
unsigned int uint_log2(word w); 

/* return random int from 0..x-1 */
int randomint( int x );

/*
  This function allows the lfu information to be displayed

    assoc_index - the cache unit that contains the block to be modified
    block_index - the index of the block to be modified

  returns a string representation of the lfu information
 */
char* lfu_to_string(int assoc_index, int block_index)
{
  /* Buffer to print lfu information -- increase size as needed. */
  static char buffer[9];
  sprintf(buffer, "%u", cache[assoc_index].block[block_index].accessCount);

  return buffer;
}

/*
  This function allows the lru information to be displayed

    assoc_index - the cache unit that contains the block to be modified
    block_index - the index of the block to be modified

  returns a string representation of the lru information
 */
char* lru_to_string(int assoc_index, int block_index)
{
  /* Buffer to print lru information -- increase size as needed. */
  static char buffer[9];
  sprintf(buffer, "%u", cache[assoc_index].block[block_index].lru.value);

  return buffer;
}

/*
  This function initializes the lfu information

    assoc_index - the cache unit that contains the block to be modified
    block_number - the index of the block to be modified

*/
void init_lfu(int assoc_index, int block_index)
{
  cache[assoc_index].block[block_index].accessCount = 0;
}

/*
  This function initializes the lru information

    assoc_index - the cache unit that contains the block to be modified
    block_number - the index of the block to be modified

*/
void init_lru(int assoc_index, int block_index)
{
  cache[assoc_index].block[block_index].lru.value = 0;
}

/*
  This is the primary function you are filling out,
  You are free to add helper functions if you need them

  @param addr 32-bit byte address
  @param data a pointer to a SINGLE word (32-bits of data)
  @param we   if we == READ, then data used to return
              information back to CPU

              if we == WRITE, then data used to
              update Cache/DRAM
*/
void accessMemory(address addr, word* data, WriteEnable we)
{
  /* Declare variables here */
		/*We want to create variables for tag, index, and offset
		 so we can separate them based on our diagram of |tag|index|offset|
		 To do so, we need to know how many bits we need for each of these fields*/
	unsigned int index_bit = uint_log2(set_count);
	unsigned int offset_bit = uint_log2(block_size);
	unsigned int tag_bit = 32 - offset_bit - index_bit;	
	
	/*Replacement policy variables*/
	unsigned int HIT = 0, LRU_index = 0, LRU_value = 0;
	
	TransferUnit byte_amount = 0;

	/*INDEX*/
	unsigned int index_val = addr << tag_bit;
	index_val = index_val >> (tag_bit + offset_bit);
	
	/*OFFSET*/
	unsigned int offset_val = addr << (offset_bit + tag_bit);
	offset_val = offset_val >> (offset_bit + tag_bit);
	
	/*TAG*/
	unsigned int tag_val = addr >> tag_bit;
	
	/*BYTE*/
	unsigned int byte_amt = uint_log2(block_size);
	
  /* handle the case of no cache at all - leave this in */
  if(assoc == 0) {
    accessDRAM(addr, (byte*)data, WORD_SIZE, we);
    return;
  }

  /*
  You need to read/write between memory (via the accessDRAM() function) and
  the cache (via the cache[] global structure defined in tips.h)

  Remember to read tips.h for all the global variables that tell you the
  cache parameters

  The same code should handle random, LFU, and LRU policies. Test the policy
  variable (see tips.h) to decide which policy to execute. The LRU policy
  should be written such that no two blocks (when their valid bit is VALID)
  will ever be a candidate for replacement. In the case of a tie in the
  least number of accesses for LFU, you use the LRU information to determine
  which block to replace.

  Your cache should be able to support write-through mode (any writes to
  the cache get immediately copied to main memory also) and write-back mode
  (and writes to the cache only gets copied to main memory when the block
  is kicked out of the cache.

  Also, cache should do allocate-on-write. This means, a write operation
  will bring in an entire block if the block is not already in the cache.

  To properly work with the GUI, the code needs to tell the GUI code
  when to redraw and when to flash things. Descriptions of the animation
  functions can be found in tips.h
  */

  /* Start adding code here */
 
	if (we == READ) { //READ HERE
		for (int i = 0; i < assoc; i++) {
			if ((cache[index_val].block[i].valid == 1) && (tag_val == cache[index_val].block[i].tag)) {		//BLOCK HIT
				HIT = 1;
				highlight_offset(index_val, i, offset, HIT);
				highlight_block(index, i);
				cache[index_val].block[i].lru.value++;
				cache[index_val].block[i].valid = 1;
				/*Below line: copies data from the element + the offset, then transfers 4 Bytes*/
				memcpy(data,cache[index_val].block[i].data + offset_val, 4); 
			}
		}
		
		if (HIT == 0) { //i.e. if there is a miss
			/*LRU REPLACEMENT*/
			highlight_offset(index_val, i, offset, MISS);
			highlight_block(index, i);
			if (policy == LRU) {
				for (int i = 0; i < assoc; i++) {
					cache[index_val].block[i].lru.value++; // increment the lru value
					if (LRU_value < cache[index_val].block[i].lru.value) { //if needed, replace LRU  
						LRU_index = i;
						LRU_val = cache[index_val].block[i].lru.value;
					}
				}
			} else if (policy == RANDOM) {
				LRU_index = randomint(assoc);
			} 
			/*Below line: checking if DIRTY */
			 if (cache[index_val].block[LRU_index].dirty == DIRTY) { 
				address oldAddr = cache[index_val].block[LRU_index].tag << ((index_bit + offset_bit) + (index_val << offset_bit));
				accessDRAM(oldAddr, (cache[index_val].block[LRU_index].data), byte_amount, WRITE);
			}
			 
			accessDRAM(addr, cache[index_val].block[LRU_index].data, byte_amount, READ);
			
			cache[index_val].block[LRU_index].dirty = VIRGIN;
			cache[index_val].block[LRU_index].tag = tag_val;
			cache[index_val].block[LRU_index].lru.value = 0;
			cache[index_val].block[LRU_index].valid = 1;
			
			/*Below line: copies data from the element + the offset, then transfers 4 Bytes*/
			memcpy(data,cache[index_val].block[LRU_index].data + offset_val, 4);
		}
	} /*WRITE*/ 
	 else { 
		HIT = 0;
		if (memory_sync_policy == WRITE_BACK) {							//write back
			for (int i = 0; i < assoc; i++) {
				if (cache[index_val].block[i].valid == 1) {
					memcpy((cache[index_val].block[i].data + offset_val), data, 4);
					cache[index_val].block[i].dirty = DIRTY;
					cache[index_val].block[i].lru.value = 0;
					cache[index_val].block[i].valid = 1;
					HIT = 1;
				}
			}
			if(HIT==0) {									
				if(policy == LRU){
					for(int i=0; i<assoc; i++) {
						cache[index_val].block[i].lru.value++;
						if(LRU_value < cache[index_val].block[i].lru.value) {
							LRU_index = i;
							LRU_value = cache[index_val].block[i].lru.value;
						}					
					}
				}else if(policy == RANDOM) {
					LRU_index = randomint(assoc);
				}
				if(cache[index_val].block[LRU_index].dirty == DIRTY) {
					address oldAddr = cache[index_val].block[LRU_index].tag << ((index_bit + offset_bit) + (index_val << offset_bit)); //calculate old address
					accessDRAM(oldAddr, (cache[index_val].block[LRU_index].data), byte_amount, WRITE);							//write data into accessDRAM
				}

				accessDRAM(addr, cache[index_val].block[LRU_index].data, byte_amount, READ);
			
				cache[index_val].block[LRU_index].dirty = VIRGIN;
				cache[index_val].block[LRU_index].tag = tag_val;
				cache[index_val].block[LRU_index].lru.value = 0;
				cache[index_val].block[LRU_index].valid = 1;
				
				/*Below line: copies data from the element + the offset, then transfers 4 Bytes*/
				memcpy(data,cache[index_val].block[LRU_index].data + offset_val, 4);
			
			}	
		} else { //Write Through
			for (int i = 0; i < assoc; i++) {
				if(tag_val == cache[index_val].block[i].valid == 1 && cache[index_val].block[i].tag == tag_val) {
					memcpy((cache[index_val].block[i].data + offset_val), data, 4);
					cache[index_val].block[i].valid = 1;
					cache[index_val].block[i].dirty = VIRGIN;
					HIT = 1;
					accessDRAM(addr, cache[index_val].block[LRU_index].data, byte_amount, WRITE);
				}
			}
			
			if(HIT==0) {
				if (policy == LRU){
					
					for (int i = 0; i < assoc; i++) {
						if (LRU_value < cache[index_val].block[i].lru.value) {
							LRU_index = i;
							LRU_value = cache[index_val].block[i].lru.value;
						}
					}
				} else if (policy == RANDOM) {
					LRU_index = randomint(assoc);
				}
				
				accessDRAM(addr, cache[index_val].block[LRU_index].data, byte_amount, READ);
			
				cache[index_val].block[LRU_index].dirty = VIRGIN;
				cache[index_val].block[LRU_index].tag = tag_val;
				cache[index_val].block[LRU_index].lru.value = 0;
				cache[index_val].block[LRU_index].valid = 1;
				
				/*Below line: copies data from the element + the offset, then transfers 4 Bytes*/
				memcpy(data,cache[index_val].block[LRU_index].data + offset_val, 4);
			}
		}		
	}
  /* This call to accessDRAM occurs when you modify any of the
     cache parameters. It is provided as a stop gap solution.
     At some point, ONCE YOU HAVE MORE OF YOUR CACHELOGIC IN PLACE,
     THIS LINE SHOULD BE REMOVED.
  */
  //accessDRAM(addr, (byte*)data, WORD_SIZE, we);
}
