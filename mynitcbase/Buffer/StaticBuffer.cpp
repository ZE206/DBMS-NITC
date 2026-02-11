#include "StaticBuffer.h"

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];

struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

unsigned char StaticBuffer::blockAllocMap[DISK_BLOCKS];

StaticBuffer::StaticBuffer() {
    // initialise all blocks as free

    for(int i = 0, index = 0; i < 4; i++) {
        unsigned char buffer[BLOCK_SIZE];
        Disk::readBlock(buffer,i);

        for(int j = 0; j < BLOCK_SIZE; j++, index++) {
            blockAllocMap[index] = buffer[j] ;
        }
    }

    for (int bufferIndex = 0;bufferIndex < BUFFER_CAPACITY;bufferIndex++) {
        metainfo[bufferIndex].free = true;
        metainfo[bufferIndex].dirty = false ;
        metainfo[bufferIndex].timeStamp = -1 ;
        metainfo[bufferIndex].blockNum = -1 ;
    }
}

StaticBuffer::~StaticBuffer() {
    for(int i = 0, index = 0; i < 4; i++) {
        unsigned char buffer[BLOCK_SIZE];
        for(int j = 0; j < BLOCK_SIZE; j++, index++) {
            buffer[j] = blockAllocMap[index];
        }
        Disk::writeBlock(buffer, i);
    }

    for(int i = 0; i < BUFFER_CAPACITY; i++) {
        if(!metainfo[i].free && metainfo[i].dirty) {
            Disk::writeBlock(blocks[i],metainfo[i].blockNum);
        }
    }
}

int StaticBuffer::getFreeBuffer(int blockNum) {
    if (blockNum < 0 || blockNum > DISK_BLOCKS) {
        return E_OUTOFBOUND;
    }

    for(int i = 0; i < BUFFER_CAPACITY; i++) {
        if(metainfo[i].free == false) metainfo[i].timeStamp++ ;
    }

    int allocatedBuffer = -1 ;
    int timeStamp = -1 ;
    int index = -1 ; 

    for(int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) {
        if(metainfo[bufferIndex].timeStamp > timeStamp) {
            timeStamp = metainfo[bufferIndex].timeStamp;
            index = bufferIndex;
        }
        if(metainfo[bufferIndex].free == true) {
            allocatedBuffer = bufferIndex;
            break;
        }
    }

    if(allocatedBuffer == -1 && metainfo[index].dirty) {
        Disk::writeBlock(blocks[index], metainfo[index].blockNum);
        allocatedBuffer = index;
    }

    metainfo[allocatedBuffer].free = false;
    metainfo[allocatedBuffer].blockNum = blockNum;
    metainfo[allocatedBuffer].dirty = false ;
    metainfo[allocatedBuffer].timeStamp = 0 ;

    return allocatedBuffer;
}

/* Get the buffer index where a particular block is stored
   or E_BLOCKNOTINBUFFER otherwise
*/
int StaticBuffer::getBufferNum(int blockNum) {
    if(blockNum < 0 || blockNum > DISK_BLOCKS) {
        return E_OUTOFBOUND ;
    }

    // find and return the bufferIndex which corresponds to blockNum (check metainfo)
    for(int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++ ) {
        if(metainfo[bufferIndex].blockNum == blockNum) {
            return bufferIndex ;
        }
    }

    return E_BLOCKNOTINBUFFER;
}

int StaticBuffer::setDirtyBit(int blockNum) {
    int bufferNum = StaticBuffer::getBufferNum(blockNum);

    if(bufferNum == E_BLOCKNOTINBUFFER) {
        return E_BLOCKNOTINBUFFER ;
    }

    if(bufferNum == E_OUTOFBOUND) {
        return E_OUTOFBOUND ;
    } else {
        metainfo[bufferNum].dirty = true ;
    }
    return SUCCESS ;
}