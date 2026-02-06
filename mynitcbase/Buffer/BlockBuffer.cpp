#include "BlockBuffer.h"

#include <cstdlib>
#include <cstring>
#include <stdio.h>

BlockBuffer::BlockBuffer(int blockNum) {
    this->blockNum = blockNum ;
}

//Constructor Initializer
RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}


int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr) {
    int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

    if(bufferNum != E_BLOCKNOTINBUFFER) {
        for(int i = 0; i < BUFFER_CAPACITY; i++) {
            StaticBuffer::metainfo[i].timeStamp++;
        }
        StaticBuffer::metainfo[bufferNum].timeStamp = 0;
    } else  {
        bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

        if (bufferNum == E_OUTOFBOUND) {
            return E_OUTOFBOUND;
        }
        Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
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

    if (slotNum < 0 || slotNum >= head.numSlots) {
        return -1 ;
    }

    // read the block at this.blockNum into a buffer
    unsigned char * bufferPtr = nullptr ;
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

int RecBuffer::setRecord(union Attribute *rec, int slotNum) {
    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);

    if (ret != SUCCESS){
        return ret;
    }

    HeadInfo head;
    BlockBuffer::getHeader(&head);

    int attrCount = head.numAttrs;
    int slotCount = head.numSlots;

    int recordSize = ATTR_SIZE * attrCount;
    int recordStart = HEADER_SIZE + slotCount + slotNum * recordSize;
    unsigned char *start = bufferPtr + recordStart;

    memcpy(start, rec, recordSize);

    if(StaticBuffer::setDirtyBit(this->blockNum) != SUCCESS) {
        printf("Setting Dirty Failed.\n");
    }

    return SUCCESS;

}

int RecBuffer::getSlotMap(unsigned char * slotMap) {
    unsigned char * bufferPtr ;

    int ret = loadBlockAndGetBufferPtr(&bufferPtr) ;

    if(ret != SUCCESS) {
        return ret ;
    }

    struct HeadInfo head ;
    BlockBuffer::getHeader(&head);

    int slotCount = head.numSlots ;

    unsigned char * slotMapInBuffer = bufferPtr + HEADER_SIZE ;

    for(int i = 0; i < slotCount; i++) {
        slotMap[i] = slotMapInBuffer[i] ;
    }

    return SUCCESS ;
}

int compareAttrs(union Attribute attr1, union Attribute attr2, int attrType) {

    double diff;

    if(attrType == STRING) {
        diff = strcmp(attr1.sVal,attr2.sVal);
    } else {
        diff = attr1.nVal - attr2.nVal ;
    }

    if(diff > 0)  {
        return 1 ;
    } else if( diff < 0) {
        return -1 ;
    } else {
        return 0 ;
    }
}