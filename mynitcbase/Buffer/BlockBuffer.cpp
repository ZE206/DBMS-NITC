#include <cstdlib>
#include <cstring>
#include "BlockBuffer.h"
#include "../Disk_Class/Disk.h"
#include "../define/constants.h"
#include "../Cache/RelCacheTable.h"

BlockBuffer::BlockBuffer(int blockNum)
{
    this->blockNum = blockNum;
}

BlockBuffer::BlockBuffer(char blockType){
    int type;
    if(blockType=='R'){
        type=REC;
    } else if(blockType=='I'){
        type=IND_INTERNAL;
    } else {
        type=IND_LEAF;
    }

    int ret=this->getFreeBlock(blockType);
    
    this->blockNum=ret;

}
RecBuffer::RecBuffer(int blockNum) : BlockBuffer(blockNum) {}
RecBuffer::RecBuffer():BlockBuffer('R'){}
int BlockBuffer::getHeader(struct HeadInfo *head)
{
    unsigned char *bufferPtr;

    int ret = loadBlockAndGetBufferPtr(&bufferPtr);

    if (ret != SUCCESS)
    {
        return ret;
    }

    memcpy(&head->blockType, bufferPtr + 0, 4);
    memcpy(&head->pblock, bufferPtr + 4, 4);
    memcpy(&head->lblock, bufferPtr + 8, 4);
    memcpy(&head->rblock, bufferPtr + 12, 4);
    memcpy(&head->numEntries, bufferPtr + 16, 4);
    memcpy(&head->numAttrs, bufferPtr + 20, 4);
    memcpy(&head->numSlots, bufferPtr + 24, 4);

    return SUCCESS;
}

int RecBuffer::getRecord(union Attribute *rec, int slotNum)
{
    unsigned char *bufferPtr;

    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS)
    {
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

int RecBuffer::setRecord(union Attribute *rec, int slotNum)
{
    struct HeadInfo head;
    unsigned char buffer[BLOCK_SIZE];

    this->getHeader(&head);

    int attrCount = head.numAttrs;
    int slotCount = head.numSlots;

    Disk::readBlock(buffer, this->blockNum);

    int recordSize = attrCount * ATTR_SIZE;
    int offset = HEADER_SIZE + slotCount + (slotNum * recordSize);

    unsigned char *slotPointer = buffer + offset;
    memcpy(slotPointer, rec, recordSize);

    Disk::writeBlock(buffer, this->blockNum);

    return SUCCESS;
}

int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **bufferPtr)
{
    int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

    if (bufferNum == E_BLOCKNOTINBUFFER)
    {
        bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

        if (bufferNum == E_OUTOFBOUND)
        {
            return E_OUTOFBOUND;
        }

        Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
    }

    *bufferPtr = StaticBuffer::blocks[bufferNum];

    return SUCCESS;
}

int RecBuffer::getSlotMap(unsigned char *slotMap)
{
    unsigned char *bufferPtr;

    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS)
    {
        return ret;
    }

    struct HeadInfo head;
    this->getHeader(&head);
    int slotCount = head.numSlots;

    unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;

    for (int i = 0; i < slotCount; i++)
    {
        slotMap[i] = slotMapInBuffer[i];
    }

    return SUCCESS;
}
int compareAttrs(union Attribute attr1, union Attribute attr2, int attrType)
{

    double diff;

    if (attrType == STRING)
    {
        diff = strcmp(attr1.sVal, attr2.sVal);
    }
    else
    {
        diff = attr1.nVal - attr2.nVal;
    }

    if (diff > 0)
    {
        return 1;
    }
    else if (diff < 0)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

int BlockBuffer::setHeader(struct HeadInfo *head)
{
    unsigned char *bufferPtr;
    int ret = BlockBuffer::loadBlockAndGetBufferPtr(&bufferPtr);

    if (ret != SUCCESS)
    {
        return ret;
    }

    struct HeadInfo *bufferHeader = (struct HeadInfo *)bufferPtr;

    bufferHeader->blockType = head->blockType;
    bufferHeader->pblock = head->pblock;
    bufferHeader->lblock = head->lblock;
    bufferHeader->rblock = head->rblock;
    bufferHeader->numEntries = head->numEntries;
    bufferHeader->numAttrs = head->numAttrs;
    bufferHeader->numSlots = head->numSlots;

    ret = StaticBuffer::setDirtyBit(this->blockNum);
    if (ret != SUCCESS)
    {
        return ret;
    }
    return SUCCESS;
}

int BlockBuffer::setBlockType(int blockType)
{
    unsigned char *bufferPtr;
    int ret = BlockBuffer::loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS)
    {
        return ret;
    }

    *((int32_t *)bufferPtr) = blockType;

    StaticBuffer::blockAllocMap[this->blockNum] = blockType;

    ret = StaticBuffer::setDirtyBit(this->blockNum);
    if (ret != SUCCESS)
    {
        return ret;
    }

    return SUCCESS;
}

int BlockBuffer::getFreeBlock(int blockType)
{
    int freeBlock = -1;
    for (int i = 0; i < DISK_BLOCKS; i++)
    {
        if (StaticBuffer::blockAllocMap[i] == UNUSED_BLK)
        {
            freeBlock = i;
            break;
        }
    }

    if (freeBlock == -1)
        return E_DISKFULL;

    this->blockNum = freeBlock;
    int bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

    if (bufferNum == E_OUTOFBOUND)
    {
        return E_OUTOFBOUND;
    }

    HeadInfo head;
    head.pblock = -1;
    head.lblock = -1;
    head.rblock = -1;
    head.numEntries = 0;
    head.numAttrs = 0;
    head.numSlots = 0;
    this->setHeader(&head);
    
    BlockBuffer::setBlockType(blockType);

    return freeBlock;
}