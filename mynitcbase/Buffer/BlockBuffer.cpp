#include "BlockBuffer.h"

#include <cstdlib>
#include <cstring>

BlockBuffer::BlockBuffer(int blockNum) {
    this->blockNum = blockNum ;
}

//Constructor Initializer
RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}


int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr) {
    int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

    if (bufferNum == E_BLOCKNOTINBUFFER) {
        bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

        if (bufferNum == E_OUTOFBOUND) {
            return E_OUTOFBOUND;
        }
        //Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
        //No need since block will be read from static buffer 
    }

    *buffPtr = StaticBuffer::blocks[bufferNum] ;

    return SUCCESS ;
}


// load the block header into the argument pointer
int BlockBuffer::getHeader(struct HeadInfo *head) {

    unsigned char * bufferPtr ;

    int ret = loadBlockAndGetBufferPtr(&bufferPtr);

    if(ret != SUCCESS) {
        return ret ;
    }

    unsigned char buffer[BLOCK_SIZE];

    // read the block at this.blockNum into the buffer
    //Disk::readBlock(buffer,this->blockNum);

    // populate the numEntries, numAttrs and numSlots fields in *head
    memcpy(&head->numSlots, bufferPtr + 24, 4);
    memcpy(&head->numAttrs, bufferPtr + 20, 4);
    memcpy(&head->numEntries, bufferPtr + 16, 4);
    memcpy(&head->rblock, bufferPtr + 12, 4);
    memcpy(&head->lblock, bufferPtr + 8, 4);
    memcpy(&head->pblock, bufferPtr + 4, 4);

    return SUCCESS;
}

// load the record at slotNum into the argument pointer
int RecBuffer::getRecord(union Attribute *rec, int slotNum) {
    struct HeadInfo head;

    BlockBuffer::getHeader(&head);
    
    int attrCount = head.numAttrs;
    int slotCount = head.numSlots;

    // read the block at this.blockNum into a buffer
    unsigned char buffer[BLOCK_SIZE] ;
    unsigned char * bufferPtr ;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);

    if(ret != SUCCESS) {
        return ret ;
    }

    /* record at slotNum will be at offset HEADER_SIZE + slotMapSize + (recordSize * slotNum)
        - each record will have size attrCount * ATTR_SIZE
        - slotMap will be of size slotCount
    */
    int recordStart = HEADER_SIZE + slotCount + slotNum * (ATTR_SIZE * attrCount);
    int recordSize = attrCount * ATTR_SIZE;
    unsigned char *slotPointer = bufferPtr + recordStart ;

    // load the record into the rec data structure
    memcpy(rec, slotPointer, recordSize);

    return SUCCESS;
}