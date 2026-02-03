#include <cstdlib>
#include <cstring>
#include "BlockBuffer.h"
#include "../Disk_Class/Disk.h"
#include "../define/constants.h"
#include "../Cache/RelCacheTable.h"


BlockBuffer::BlockBuffer(int blockNum) {
    this->blockNum = blockNum;
}


RecBuffer::RecBuffer(int blockNum) : BlockBuffer(blockNum) {}


int BlockBuffer::getHeader(struct HeadInfo *head) {
    unsigned char *bufferPtr;

    int ret = loadBlockAndGetBufferPtr(&bufferPtr);

    if(ret != SUCCESS){
        return ret;
    }


    memcpy(&head->blockType,  bufferPtr + 0,  4);
    memcpy(&head->pblock,     bufferPtr + 4,  4);
    memcpy(&head->lblock,     bufferPtr + 8,  4);
    memcpy(&head->rblock,     bufferPtr + 12, 4);
    memcpy(&head->numEntries, bufferPtr + 16, 4);
    memcpy(&head->numAttrs,   bufferPtr + 20, 4);
    memcpy(&head->numSlots,   bufferPtr + 24, 4);

    return SUCCESS;
}


int RecBuffer::getRecord(union Attribute *rec, int slotNum) {
    unsigned char *bufferPtr;

    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if(ret != SUCCESS){
        return ret;
    }


    struct HeadInfo head;
    this->getHeader(&head);

    int attrCount = head.numAttrs;
    int slotCount = head.numSlots;


    int recordSize = attrCount * ATTR_SIZE;
    int offset = HEADER_SIZE + slotCount + (slotNum * recordSize);

    unsigned char *slotPointer = bufferPtr + offset;
    memcpy(rec, slotPointer, recordSize);

    return SUCCESS;
}

int RecBuffer::setRecord(union Attribute *rec,int slotNum){
    struct HeadInfo head;
    unsigned char buffer[BLOCK_SIZE];

    this->getHeader(&head);

    int attrCount = head.numAttrs;
    int slotCount = head.numSlots;


    Disk::readBlock(buffer, this->blockNum);

    int recordSize = attrCount * ATTR_SIZE;
    int offset = HEADER_SIZE + slotCount + (slotNum * recordSize);

    unsigned char *slotPointer = buffer + offset;
    memcpy(slotPointer,rec, recordSize);

    Disk::writeBlock(buffer,this->blockNum);

    return SUCCESS;
}

int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **bufferPtr){
    int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

    if(bufferNum == E_BLOCKNOTINBUFFER){
        bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

        if(bufferNum == E_OUTOFBOUND){
            return E_OUTOFBOUND;
        }

        Disk::readBlock(StaticBuffer::blocks[bufferNum],this->blockNum);
    }

    *bufferPtr = StaticBuffer::blocks[bufferNum];



    return SUCCESS;
}

int RecBuffer::getSlotMap(unsigned char *slotMap){
    unsigned char * bufferPtr;

    int ret=loadBlockAndGetBufferPtr(&bufferPtr);
    if(ret!=SUCCESS){
        return ret;
    }

    struct HeadInfo head;
    this->getHeader(&head);
    int slotCount=head.numSlots;

    unsigned char * slotMapInBuffer=bufferPtr+HEADER_SIZE;

    strcpy(*slotMap, *slotMapInBuffer);

    return SUCCESS;

}

int compareAttrs(union Attribute attr1, union Attribute attr2, int attrType){

    double diff;
    if(attrType==STRING){
        diff=strcmp(attr1.sval, attr2.sval);
    } else{
        diff=attr1.nval-attr2.nval;
    }

    if(diff>0) return 1;
    if(diff<0) return -1;
    if(diff==0) return 0;
}

