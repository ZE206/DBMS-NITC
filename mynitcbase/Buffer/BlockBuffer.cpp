#include "BlockBuffer.h"

#include <cstdlib>
#include <cstring>
#include <stdio.h>

BlockBuffer::BlockBuffer(int blockNum) {
    this->blockNum = blockNum ;
}

BlockBuffer::BlockBuffer(char blockType) {
    int type; 

    if(blockType == 'R') {
        type = REC ;
    } else if(blockType == 'I') {
        type = IND_INTERNAL ;
    } else {
        type = IND_LEAF ;
    }

    int ret = this->getFreeBlock(type);

    this->blockNum = ret ; 
}

//Constructor Initializer
RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}

RecBuffer::RecBuffer() : BlockBuffer('R') {}


int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr) {
    int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

    if(bufferNum != E_BLOCKNOTINBUFFER) {
        for(int i = 0; i < BUFFER_CAPACITY; i++) {
            if(i == bufferNum) {
                StaticBuffer::metainfo[i].timeStamp = 0;
            } else {
                StaticBuffer::metainfo[bufferNum].timeStamp += 1;
            }
        }
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

int RecBuffer::setSlotMap(unsigned char *slotMap) {
    unsigned char * bufferPtr ;

    int ret = BlockBuffer::loadBlockAndGetBufferPtr(&bufferPtr);
    if(ret != SUCCESS) {
        return ret ;
    }

    HeadInfo head ;
    this->getHeader(&head);
    int numSlots = head.numSlots ;

    memcpy(bufferPtr + HEADER_SIZE, slotMap, numSlots);

    ret = StaticBuffer::setDirtyBit(this->blockNum);

    if(ret != SUCCESS) {
        return ret ;
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

int BlockBuffer::setHeader(struct HeadInfo *head) {
    unsigned char *bufferPtr ;

    int retVal = BlockBuffer::loadBlockAndGetBufferPtr(&bufferPtr);

    if(retVal != SUCCESS) {
        return retVal ;
    }

    struct HeadInfo * bufferHeader = (struct HeadInfo *)bufferPtr ;

    bufferHeader->blockType = head->blockType ;
    bufferHeader->lblock = head->lblock ;
    bufferHeader->numAttrs = head->numAttrs ;
    bufferHeader->numEntries = head->numEntries ;
    bufferHeader->numSlots = head->numSlots ;
    bufferHeader->pblock = head->pblock ;
    bufferHeader->rblock = head->rblock ;

    int ret = StaticBuffer::setDirtyBit(this->blockNum);

    if(ret != SUCCESS) {
        return ret ;
    }

    return SUCCESS ;
}

int BlockBuffer::setBlockType(int blockType) {
    unsigned char *bufferPtr ;
    
    int ret = BlockBuffer::loadBlockAndGetBufferPtr(&bufferPtr);
    if(ret != SUCCESS) {
        return ret ;
    }

    *(int32_t*)bufferPtr = blockType ;

    StaticBuffer::blockAllocMap[this->blockNum] = blockType ;

    ret = StaticBuffer::setDirtyBit(this->blockNum);
    if(ret != SUCCESS) {
        return ret ;
    }

    return SUCCESS ;
}

int BlockBuffer::getFreeBlock(int blockType) {

    int freeBlock = -1 ;
    for(int i=0; i < DISK_BLOCKS ; i++) {
        if(StaticBuffer::blockAllocMap[i] == UNUSED_BLK) {
            freeBlock = i;
            break ;
        }
    }

    if(freeBlock == -1) return E_DISKFULL ;

    this->blockNum = freeBlock ;

    int bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);
    if(bufferNum == E_OUTOFBOUND) {
        return E_OUTOFBOUND ;
    }

    HeadInfo head ;
    head.pblock = -1 ;
    head.lblock = -1 ;
    head.rblock = -1 ;
    head.numEntries = 0 ;
    head.numAttrs = 0 ;
    head.numSlots = 0;
    
    this->setHeader(&head);
    this->setBlockType(blockType);

    return freeBlock ;
}

int BlockBuffer::getBlockNum() {
    return this->blockNum ;
}