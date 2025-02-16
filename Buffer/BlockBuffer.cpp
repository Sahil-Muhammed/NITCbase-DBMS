#include "BlockBuffer.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>

// the declarations for these functions can be found in "BlockBuffer.h"

BlockBuffer::BlockBuffer(char blockType){
  printf("This constructor is called\n");
  int res = getFreeBlock(blockType);
  printf("getFreeBlock working fine.\n");
  if (res == E_DISKFULL){
    printf("Disk is full.\n");
  }

  this->blockNum = res;
}

BlockBuffer::BlockBuffer(int blockNum) {
  // initialise this.blockNum with the argument
  this->blockNum=blockNum;
}

RecBuffer::RecBuffer() : BlockBuffer('R') {}
// calls the parent class constructor
RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}

// load the block header into the argument pointer
int BlockBuffer::getHeader(struct HeadInfo *head) {
  unsigned char *bufferPtr;

  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS){
    return ret;
  }

  //unsigned char buffer[BLOCK_SIZE];
  //Disk::readBlock(buffer, this->blockNum);

  // populate the numEntries, numAttrs and numSlots fields in *head
  memcpy(&head->numSlots, bufferPtr + 24, 4);
  memcpy(&head->numEntries, bufferPtr + 16, 4);
  memcpy(&head->numAttrs, bufferPtr + 20, 4);
  memcpy(&head->rblock, bufferPtr + 12, 4);
  memcpy(&head->lblock, bufferPtr + 8, 4);
  memcpy(&head->pblock, bufferPtr + 4, 4);

  return SUCCESS;
}

// load the record at slotNum into the argument pointer
int RecBuffer::getRecord(union Attribute *rec, int slotNum) {
  unsigned char* bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS){
    return ret;
  }
  struct HeadInfo head;

  // get the header using this.getHeader() function
  BlockBuffer::getHeader(&head);

  int attrCount = head.numAttrs;
  int slotCount = head.numSlots;
  //unsigned char buffer[BLOCK_SIZE];

  // read the block at this.blockNum into a buffer
  //Disk::readBlock(buffer, this->blockNum);

  /* record at slotNum will be at offset HEADER_SIZE + slotMapSize + (recordSize * slotNum)
     - each record will have size attrCount * ATTR_SIZE
     - slotMap will be of size slotCount
  */
  

  int recordSize = attrCount * ATTR_SIZE;
  unsigned char *slotPointer = bufferPtr + (HEADER_SIZE + slotCount + (recordSize*slotNum)); //changed SLOTCOUNT_RELATIONCAT to slotCount

  // load the record into the rec data structure
  memcpy(rec, slotPointer, recordSize);

  return SUCCESS;
}

int RecBuffer::setRecord(union Attribute *rec, int slotNum) {

  unsigned char* bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);

  if (ret != SUCCESS){
    return ret;
  }

  struct HeadInfo head;

  // get the header using this.getHeader() function
  BlockBuffer::getHeader(&head);

  int attrCount = head.numAttrs;
  int slotCount = head.numSlots;
  unsigned char buffer[BLOCK_SIZE];

  if (slotNum < 0 || slotNum > slotCount){
    return E_OUTOFBOUND;
  }

  // read the block at this.blockNum into a buffer
  //Disk::readBlock(buffer, this->blockNum);

  /* record at slotNum will be at offset HEADER_SIZE + slotMapSize + (recordSize * slotNum)
     - each record will have size attrCount * ATTR_SIZE
     - slotMap will be of size slotCount
  */
  

  int recordSize = attrCount * ATTR_SIZE;
  unsigned char *slotPointer = bufferPtr + (HEADER_SIZE + slotCount + (recordSize*slotNum)); //changed SLOTMAP_SIZE_RELCAT_ATTRCAT to slotCount

  // load the record into the rec data structure
  memcpy(slotPointer, rec, recordSize);

  int out = StaticBuffer::setDirtyBit(this->blockNum);

  if (out != SUCCESS){
    printf("Error in setDirtyBit()\n");
  }

  //Disk::writeBlock(buffer,this->blockNum);

  return SUCCESS;
}

/*
Used to load a block to the buffer and get a pointer to it.
NOTE: this function expects the caller to allocate memory for the argument
*/
int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char** bufferPtr){
  //check whether block is already in buffer
  int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

  if (bufferNum != E_BLOCKNOTINBUFFER){
    for (int i = 0; i < BUFFER_CAPACITY; ++i){
      if (i == bufferNum){
        StaticBuffer::metainfo[i].timeStamp = 0;
      }
      else{
        StaticBuffer::metainfo[i].timeStamp += 1;
      }
    }
  }
  
  else{ 

    //allot a free buffer block number
    bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

    if (bufferNum == E_OUTOFBOUND){

      return E_OUTOFBOUND;

    }

    Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
  }

  //make pointer point to the buffer block
  *bufferPtr = StaticBuffer::blocks[bufferNum];

  return SUCCESS;
}

int RecBuffer::getSlotMap(unsigned char* slotmap){
  unsigned char* bufferPtr;

  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS)
    return ret;

  struct HeadInfo head;
  BlockBuffer::getHeader(&head);
  int slotCount = head.numSlots;

  unsigned char* slotMapInBuffer = bufferPtr + HEADER_SIZE;
  memcpy(slotmap, slotMapInBuffer, slotCount);

  return SUCCESS;
}

int RecBuffer::setSlotMap(unsigned char* slotmap){
  unsigned char* bufferPtr;

  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS){
    return ret;
  }

  HeadInfo head;
  BlockBuffer::getHeader(&head);

  int numSlots = head.numSlots;
  unsigned char* slotMapInBuffer = bufferPtr + HEADER_SIZE;
  memcpy(slotMapInBuffer, slotmap, numSlots);

  int res = StaticBuffer::setDirtyBit(this->blockNum);
  if (res != SUCCESS){
    return res;
  }
  return SUCCESS;
}

int compareAttrs(union Attribute attr1, union Attribute attr2, int attrType){
  double diff;
  if (attrType == STRING)
    diff = strcmp(attr1.sVal, attr2.sVal);
  else 
    diff = attr1.nVal - attr2.nVal;
  if (diff > 0)
    return 1;
  if (diff < 0)
    return -1;
  //else implies diff == 0
  return 0;
}

//Mistakes I made:
//1. in calculating the offset for slotPointer, I used a constant (whose value was 20) instead of using the variable slotCount.
//2. In setRecord(), still used buffer and disk operation directly instead of using bufferPtr.

int BlockBuffer::setHeader(struct HeadInfo* head){
  unsigned char* bufferPtr;

  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS){
    return ret;
  }

  struct HeadInfo* bufferHeader = (struct HeadInfo*)bufferPtr;

  bufferHeader->numSlots = head->numSlots;
  bufferHeader->blockType = head->blockType;
  bufferHeader->lblock = head->lblock;
  bufferHeader->numAttrs = head->numAttrs;
  bufferHeader->numEntries = head->numEntries;
  bufferHeader->numSlots = head->numSlots;
  bufferHeader->pblock = head->pblock;
  bufferHeader->rblock = head->rblock;

  int result = StaticBuffer::setDirtyBit(this->blockNum); //not sure if this works

  if (result != SUCCESS){
    printf("Some error with setDirtyBit().\n");
    return result;
  }

  return SUCCESS;
}

int BlockBuffer::setBlockType(int blockType){
  unsigned char* bufferPtr;

  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS){
    return ret;
  }

  *((int32_t *)bufferPtr) = blockType;

  StaticBuffer::blockAllocMap[this->blockNum] = blockType; //skeptical; maybe should use bufferPtr

  int result = StaticBuffer::setDirtyBit(this->blockNum); //not sure if this works

  if (result != SUCCESS){
    printf("Some error with setDirtyBit().\n");
    return result;
  }

  return SUCCESS;
}

int BlockBuffer::getFreeBlock(int blockType){
  int index = -1;
  for (int i = 0; i < DISK_BLOCKS; ++i){
    if (StaticBuffer::blockAllocMap[i] == UNUSED_BLK){ 
      index = i;
      break;
    }
  }
  if (index == -1){
    return E_DISKFULL;
  }

  this->blockNum = index;
  int freeBuf = StaticBuffer::getFreeBuffer(this->blockNum);
  // if (freeBuf == E_OUTOFBOUND){
  //   return E_OUTOFBOUND;
  // }

  HeadInfo head;
  head.pblock = -1;
  head.rblock = -1;
  head.lblock = -1;
  head.numAttrs = 0;
  head.numEntries = 0;
  head.numSlots = 0;
  int ret = setHeader(&head);
  int ret1 = setBlockType(blockType);  //mistake: used argument blockNum instead of blockType
  return index;
}

int BlockBuffer::getBlockNum(){
  if (this->blockNum == 0){
    printf("chinna problem chetta\n");
  }
  return this->blockNum;
}

//mistakes: 1. used HeadInfo* head, should have used w/o pointer

void BlockBuffer::releaseBlock(){
  
  if (this->blockNum == INVALID_BLOCKNUM){
    return;
  }

  else{
    int bufNum = StaticBuffer::getBufferNum(this->blockNum);
    if (bufNum == E_BLOCKNOTINBUFFER){
      printf("Block not in buffer.\n");
      return;
    }

    StaticBuffer::metainfo[this->blockNum].free = true;
    StaticBuffer::blockAllocMap[this->blockNum] = UNUSED_BLK;
    this->blockNum = INVALID_BLOCKNUM;
  }
}