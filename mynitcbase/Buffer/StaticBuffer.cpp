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
        metainfo[i].dirty=false;
        metainfo[i].timestamp=-1;
        metainfo[i].blockNum=-1;
    }
}

StaticBuffer::~StaticBuffer(){
    for(int i=0;i<BUFFER_CAPACITY;i++){
        if(!metainfo[i].free && metainfo[i].dirty){
            Disk::writeBlock();
        }
    }
    
}

int StaticBuffer::getFreeBuffer(int blockNum){
    if(blockNum <0 || blockNum > DISK_BLOCKS){
        return E_OUTOFBOUND;
    }

     for(int i=0;i<BUFFER_CAPACITY;i++){
        if(!metainfo[i].free){
            metainfo[i].timestamp++;
        }
    }

    int allocatedBuffer=-1;
    int timestamp=-1;
    int index=-1;

    for(int bufferidx =0;bufferidx<BUFFER_CAPACITY;bufferidx++){
        
        if(metainfo[bufferidx].timestamp>timestamp){
            timestamp=metainfo[bufferIdx].timestamp;
            index=bufferIdx;
        }

        if(metainfo[bufferidx].free){
            allocatedBuffer = bufferidx;
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

int StaticBuffer::setDirtyBit(int blockNum){
    int bufferNum=StaticBufffer::getBUfferNum(blockNum);

    if(bufferNum==E_BLOCKNOTINBUFFER){
        return E_BLOCKNOTINBUFFER;
    }

    elseif(bufferNum==E_OUTOFBOUND){
        return E_OUTOFBOUND;
    }

    else{
        metaInfo[bufferNum].dirty=true;
    }
    return SUCCESS;
}