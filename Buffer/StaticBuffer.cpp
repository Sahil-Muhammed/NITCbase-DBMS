#include "StaticBuffer.h"
#include <cstdio>

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

StaticBuffer::StaticBuffer(){
    for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; ++bufferIndex){
        metainfo[bufferIndex].free = true;
    }
}

StaticBuffer::~StaticBuffer() {}

/*
At this stage, we are not writing back from the buffer to the disk since we are
not modifying the buffer. So, we will define an empty destructor for now. In
subsequent stages, we will implement the write-back functionality here.
*/

int StaticBuffer::getFreeBuffer(int blockNum){
    //check whether argument is valid
    if (blockNum < 0 || blockNum > DISK_BLOCKS){
        return E_OUTOFBOUND;
    }
    //declare variable to assign a free buffer block number
    int allocatedBuffer;
    //traverse through buffer to find first free buffer block
    for (int index = 0; index < BUFFER_CAPACITY; ++index){
        if (metainfo[index].free == true){
            allocatedBuffer = index;
            break;
        }
    }

    //update the details in the structure metainfo
    metainfo[allocatedBuffer].free = false;
    metainfo[allocatedBuffer].blockNum = blockNum;

    //return the allocated buffer block number
    return allocatedBuffer;
}

int StaticBuffer::getBufferNum(int blockNum){
    if (blockNum < 0 || blockNum > DISK_BLOCKS){
        return E_OUTOFBOUND;
    }

    for (int index = 0; index < BUFFER_CAPACITY; ++index){
        if (metainfo[index].blockNum == blockNum){
            return index;
        }
    }

    return E_BLOCKNOTINBUFFER;
}
