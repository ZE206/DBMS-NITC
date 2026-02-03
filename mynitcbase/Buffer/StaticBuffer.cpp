#include "StaticBuffer.h"
#include <cstdlib>
#include <cstring>
#include "../Disk_Class/Disk.h"
#include "../define/constants.h"

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

StaticBuffer::StaticBuffer(){
    for(int i=0;i<BUFFER_CAPACITY;i++){
        metainfo[i].free = true;
    }
}

StaticBuffer::~StaticBuffer(){}

int StaticBuffer::getFreeBuffer(int blockNum){
    if(blockNum <0 || blockNum > DISK_BLOCKS){
        return E_OUTOFBOUND;
    }
    int allocatedBuffer;
    for(int bufferidx =0;bufferidx<BUFFER_CAPACITY;bufferidx++){
        if(metainfo[bufferidx].free){
            allocatedBuffer = bufferidx;
            break;
        }
    }
    metainfo[allocatedBuffer].free = false;
    metainfo[allocatedBuffer].blockNum = blockNum;

    return allocatedBuffer;
}

int StaticBuffer::getBufferNum(int blockNum){
    if(blockNum <0 || blockNum > DISK_BLOCKS){
        return E_OUTOFBOUND;
    }
    for(int bufferidx =0;bufferidx<BUFFER_CAPACITY;bufferidx++){
        if(!metainfo[bufferidx].free && metainfo[bufferidx].blockNum == blockNum){
            return bufferidx;
        }
    }

    return E_BLOCKNOTINBUFFER;
}