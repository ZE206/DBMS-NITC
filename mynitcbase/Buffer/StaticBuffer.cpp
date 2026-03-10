#include "StaticBuffer.h"
#include <cstdlib>
#include <cstring>
#include "../Disk_Class/Disk.h"
#include "../define/constants.h"

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];
unsigned char StaticBuffer::blockAllocMap[DISK_BLOCKS];

StaticBuffer::StaticBuffer()
{
    for (int i = 0, index = 0; i < 4; i++)
    {
        unsigned char buffer[BLOCK_SIZE];
        Disk::readBlock(buffer, i);

        for (int j = 0; j < BLOCK_SIZE; j++, index++)
        {
            blockAllocMap[index] = buffer[j];
        }
    }

    for (int i = 0; i < BUFFER_CAPACITY; i++)
    {
        metainfo[i].free = true;
        metainfo[i].dirty = false;
        metainfo[i].timeStamp = -1;
        metainfo[i].blockNum = -1;
    }
}

StaticBuffer::~StaticBuffer()
{
    for (int i, index; i < 4; i++)
    {
        unsigned char buffer[BLOCK_SIZE];

        for (int j = 0; j < BLOCK_SIZE; j++, index++)
        {
            buffer[j] = blockAllocMap[index];
        }
        
        Disk::writeBlock(buffer, i);
    }
    for (int i = 0; i < BUFFER_CAPACITY; i++)
    {
        if (metainfo[i].free == false)
        {
            if (metainfo[i].dirty == true)
            {
                Disk::writeBlock(blocks[i], metainfo[i].blockNum);

                metainfo[i].dirty = false;
            }
        }
    }
}

int StaticBuffer::getFreeBuffer(int blockNum)
{
    if (blockNum < 0 || blockNum > DISK_BLOCKS)
    {
        return E_OUTOFBOUND;
    }

    for (int i = 0; i < BUFFER_CAPACITY; i++)
    {
        if (!metainfo[i].free)
        {
            metainfo[i].timeStamp++;
        }
    }

    int allocatedBuffer = -1;
    int timestamp = -1;
    int index = -1;

    for (int bufferidx = 0; bufferidx < BUFFER_CAPACITY; bufferidx++)
    {

        if (metainfo[bufferidx].timeStamp > timestamp)
        {
            timestamp = metainfo[bufferidx].timeStamp;
            index = bufferidx;
        }

        if (metainfo[bufferidx].free)
        {
            allocatedBuffer = bufferidx;
            break;
        }
    }
    if (allocatedBuffer == -1 && metainfo[index].dirty)
    {
        Disk::writeBlock(blocks[index], metainfo[index].blockNum);
        allocatedBuffer = index;
    }

    metainfo[allocatedBuffer].free = false;
    metainfo[allocatedBuffer].blockNum = blockNum;
    metainfo[allocatedBuffer].dirty = false;
    metainfo[allocatedBuffer].timeStamp = 0;

    return allocatedBuffer;
}

int StaticBuffer::getBufferNum(int blockNum)
{
    if (blockNum < 0 || blockNum > DISK_BLOCKS)
    {
        return E_OUTOFBOUND;
    }
    for (int bufferidx = 0; bufferidx < BUFFER_CAPACITY; bufferidx++)
    {
        if (!metainfo[bufferidx].free && metainfo[bufferidx].blockNum == blockNum)
        {
            return bufferidx;
        }
    }

    return E_BLOCKNOTINBUFFER;
}

int StaticBuffer::setDirtyBit(int blockNum)
{
    int bufferNum = StaticBuffer::getBufferNum(blockNum);

    if (bufferNum == E_BLOCKNOTINBUFFER)
    {
        return E_BLOCKNOTINBUFFER;
    }

    else if (bufferNum == E_OUTOFBOUND)
    {
        return E_OUTOFBOUND;
    }

    else
    {
        metainfo[bufferNum].dirty = true;
    }
    return SUCCESS;
}
