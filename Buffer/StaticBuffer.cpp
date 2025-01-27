#include "StaticBuffer.h"
#include <cstdio>

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

StaticBuffer::StaticBuffer(){
    for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; ++bufferIndex){
        metainfo[bufferIndex].free = true;
        metainfo[bufferIndex].dirty = false;
        metainfo[bufferIndex].blockNum = -1;
        metainfo[bufferIndex].timeStamp = -1;
    }
}

StaticBuffer::~StaticBuffer() {
    for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; ++bufferIndex){
        if (metainfo[bufferIndex].free == false &&
            metainfo[bufferIndex].dirty == true){
            Disk::writeBlock(blocks[bufferIndex], metainfo[bufferIndex].blockNum); //doubtful
        }
    }
}

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

    for (int i = 0; i < BUFFER_CAPACITY; ++i){
        if (metainfo[i].free == false){
            metainfo[i].timeStamp++;
        }
    }
    //declare variable to assign a free buffer block number
    int allocatedBuffer = -1;
    //traverse through buffer to find first free buffer block
    for (int index = 0; index < BUFFER_CAPACITY; ++index){
        if (metainfo[index].free == true){
            allocatedBuffer = index;
            break;
        }
    }

    if (allocatedBuffer == -1){
        int largest = 0;
        for (int j = 1; j < BUFFER_CAPACITY; ++j){
            if (metainfo[largest].timeStamp < metainfo[j].timeStamp){
                largest = j;
            }
        }

        if (metainfo[largest].dirty == true){
            Disk::writeBlock(blocks[largest], metainfo[largest].blockNum); //doubtful
            allocatedBuffer = metainfo[largest].blockNum;
        }
    }

    //update the details in the structure metainfo
    metainfo[allocatedBuffer].free = false;
    metainfo[allocatedBuffer].dirty = false;
    metainfo[allocatedBuffer].blockNum = blockNum;
    metainfo[allocatedBuffer].timeStamp = 0;

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

int StaticBuffer::setDirtyBit(int blockNum){
    int bufIndex = getBufferNum(blockNum);

    if (bufIndex == E_BLOCKNOTINBUFFER){
        return E_BLOCKNOTINBUFFER;
    }

    if (bufIndex == E_OUTOFBOUND){
        return E_OUTOFBOUND;
    }

    metainfo[bufIndex].dirty = true; //should be inside else, but checking if this works

    return SUCCESS;
}
