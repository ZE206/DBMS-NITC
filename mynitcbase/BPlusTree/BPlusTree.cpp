#include "BPlusTree.h"

#include <cstring>
extern int g_comparisonCount;

RecId BPlusTree::bPlusSearch(int relId, char attrName[ATTR_SIZE], Attribute attrVal, int op)
{
    IndexId searchIndex;
    AttrCacheTable::getSearchIndex(relId, attrName, &searchIndex);

    AttrCatEntry attrCatEntry;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

    int block, index;

    if (searchIndex.block == -1 && searchIndex.index == -1)
    {
        // first time search - start from root
        block = attrCatEntry.rootBlock;
        index = 0;

        if (block == -1)
        {
            return RecId{-1, -1};
        }
    }
    else
    {
        // resume from next entry after previous search hit
        block = searchIndex.block;
        index = searchIndex.index + 1;

        IndLeaf leaf(block);
        HeadInfo leafHead;
        leaf.getHeader(&leafHead);

        if (index >= leafHead.numEntries)
        {
            // move to next leaf block
            block = leafHead.rblock;
            index = 0;

            if (block == -1)
            {
                return RecId{-1, -1};
            }
        }
    }

    /****** Traverse internal nodes to reach the correct leaf ******/

    while (StaticBuffer::getStaticBlockType(block) == IND_INTERNAL)
    {
        IndInternal internalBlk(block);
        HeadInfo intHead;
        internalBlk.getHeader(&intHead);

        InternalEntry intEntry;

        if (op == NE || op == LT || op == LE)
        {
            // always go to leftmost child
            internalBlk.getEntry(&intEntry, 0);
            block = intEntry.lChild;
        }
        else
        {
            // EQ, GT, GE: find first entry where attrVal <= intEntry.attrVal (GE/EQ)
            // or attrVal < intEntry.attrVal (GT)
            bool found = false;

            for (int i = 0; i < intHead.numEntries; i++)
            {
                internalBlk.getEntry(&intEntry, i);
                g_comparisonCount++;
                int cmpVal = compareAttrs(intEntry.attrVal, attrVal, attrCatEntry.attrType);

                if (
                    (op == EQ || op == GE) && cmpVal >= 0 ||
                    (op == GT && cmpVal > 0))
                {
                    block = intEntry.lChild;
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                // go to rightmost child
                internalBlk.getEntry(&intEntry, intHead.numEntries - 1);
                block = intEntry.rChild;
            }
        }
    }

    /****** Search through leaf blocks ******/

    while (block != -1)
    {
        IndLeaf leafBlk(block);
        HeadInfo leafHead;
        leafBlk.getHeader(&leafHead);

        Index leafEntry;

        while (index < leafHead.numEntries)
        {
            leafBlk.getEntry(&leafEntry, index);
            g_comparisonCount++;
            int cmpVal = compareAttrs(leafEntry.attrVal, attrVal, attrCatEntry.attrType);

            if (
                (op == EQ && cmpVal == 0) ||
                (op == LE && cmpVal <= 0) ||
                (op == LT && cmpVal < 0) ||
                (op == GT && cmpVal > 0) ||
                (op == GE && cmpVal >= 0) ||
                (op == NE && cmpVal != 0))
            {
                // found a match - update search index and return
                IndexId newSearchIndex = {block, index};
                AttrCacheTable::setSearchIndex(relId, attrName, &newSearchIndex);
                return RecId{leafEntry.block, leafEntry.slot};
            }
            else if ((op == EQ || op == LE || op == LT) && cmpVal > 0)
            {
                // no future entries will satisfy condition
                return RecId{-1, -1};
            }

            ++index;
        }

        if (op == EQ || op == LT || op == LE)
        {
            break;
        }

        // move to next leaf block
        block = leafHead.rblock;
        index = 0;
    }

    return RecId{-1, -1};
}

int BPlusTree::bPlusCreate(int relId, char attrName[ATTR_SIZE])
{
    if (relId == RELCAT_RELID || relId == ATTRCAT_RELID)
    {
        return E_NOTPERMITTED;
    }
    AttrCatEntry attrCatEntry;
    int ret = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);
    if (ret != SUCCESS)
    {
        return ret;
    }

    if (attrCatEntry.rootBlock != -1)
    {
        return SUCCESS;
    }

    IndLeaf rootBlockBuf;
    int rootBlock = rootBlockBuf.getBlockNum();
    if (rootBlock == E_DISKFULL)
    {
        return E_DISKFULL;
    }

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);

    int block = relCatEntry.firstBlk;

    while (block != -1)
    {
        RecBuffer recBuffer(block);

        unsigned char slotMap[relCatEntry.numSlotsPerBlk];
        recBuffer.getSlotMap(slotMap);

        for (int slot = 0; slot < relCatEntry.numSlotsPerBlk; slot++)
        {

            if (slotMap[slot] == SLOT_UNOCCUPIED)
            {
                continue;
            }

            Attribute record[relCatEntry.numAttrs];
            recBuffer.getRecord(record, slot);

            RecId recId{block, slot};

            int retVal = bPlusInsert(relId, attrName,
                                     record[attrCatEntry.offset], recId);

            if (retVal == E_DISKFULL)
            {
                return E_DISKFULL;
            }
        }

        HeadInfo header;
        recBuffer.getHeader(&header);
        block = header.rblock;
    }

    return SUCCESS;
}

int BPlusTree::bPlusDestroy(int rootBlockNum)
{
    if (rootBlockNum < 0 || rootBlockNum >= DISK_BLOCKS)
    {
        return E_OUTOFBOUND;
    }

    int type = StaticBuffer::getStaticBlockType(rootBlockNum);

    if (type == IND_LEAF)
    {
        IndLeaf leafBlock(rootBlockNum);
        leafBlock.releaseBlock();
        return SUCCESS;
    }
    else if (type == IND_INTERNAL)
    {
        IndInternal internalBlk(rootBlockNum);

        HeadInfo header;
        internalBlk.getHeader(&header);

        // destroy lChild of the first entry
        InternalEntry firstEntry;
        internalBlk.getEntry(&firstEntry, 0);
        bPlusDestroy(firstEntry.lChild);

        // destroy rChild of all entries
        for (int i = 0; i < header.numEntries; i++)
        {
            InternalEntry entry;
            internalBlk.getEntry(&entry, i);
            bPlusDestroy(entry.rChild);
        }

        internalBlk.releaseBlock();
        return SUCCESS;
    }
    else
    {
        return E_INVALIDBLOCK;
    }
}

int BPlusTree::bPlusInsert(int relId, char attrName[ATTR_SIZE], Attribute attrVal, RecId recId)
{
    AttrCatEntry attrCatEntry;
    int ret = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);
    if (ret != SUCCESS)
    {
        return ret;
    }

    int blockNum = attrCatEntry.rootBlock;

    if (blockNum == -1)
    {
        return E_NOINDEX;
    }

    int leafBlkNum = findLeafToInsert(blockNum, attrVal, attrCatEntry.attrType);

    Index entry;
    entry.attrVal = attrVal;
    entry.block = recId.block;
    entry.slot = recId.slot;

    int retVal = insertIntoLeaf(relId, attrName, leafBlkNum, entry);

    if (retVal == E_DISKFULL)
    {
        bPlusDestroy(blockNum);

        attrCatEntry.rootBlock = -1;
        AttrCacheTable::setAttrCatEntry(relId, attrName, &attrCatEntry);

        return E_DISKFULL;
    }

    return SUCCESS;
}
int BPlusTree::findLeafToInsert(int rootBlock, Attribute attrVal, int attrType)
{
    int blockNum = rootBlock;

    while (StaticBuffer::getStaticBlockType(blockNum) != IND_LEAF)
    {
        IndInternal internalBlk(blockNum);

        HeadInfo header;
        internalBlk.getHeader(&header);

        // find the first entry with attrVal >= search value
        int nextBlock = -1;
        for (int i = 0; i < header.numEntries; i++)
        {
            InternalEntry entry;
            internalBlk.getEntry(&entry, i);

            if (compareAttrs(attrVal, entry.attrVal, attrType) <= 0)
            {
                nextBlock = entry.lChild;
                break;
            }

            // if last entry and still not found, go to rChild
            if (i == header.numEntries - 1)
            {
                nextBlock = entry.rChild;
            }
        }

        blockNum = nextBlock;
    }

    return blockNum;
}

int BPlusTree::insertIntoLeaf(int relId, char attrName[ATTR_SIZE], int blockNum, Index indexEntry)
{
    AttrCatEntry attrCatEntry;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

    IndLeaf leafBlock(blockNum);

    HeadInfo blockHeader;
    leafBlock.getHeader(&blockHeader);

    Index indices[blockHeader.numEntries + 1];

    // copy existing entries and insert new entry in sorted order
    bool inserted = false;
    for (int i = 0; i < blockHeader.numEntries; i++)
    {
        leafBlock.getEntry(&indices[i], i);

        if (!inserted && compareAttrs(indices[i].attrVal, indexEntry.attrVal,
                                      attrCatEntry.attrType) >= 0)
        {
            // shift current entry right and insert indexEntry here
            indices[i + 1] = indices[i];
            indices[i] = indexEntry;
            inserted = true;

            // copy remaining entries shifted by one
            for (int j = i + 1; j < blockHeader.numEntries; j++)
            {
                leafBlock.getEntry(&indices[j + 1], j);
            }
            break;
        }
    }
    if (!inserted)
    {
        indices[blockHeader.numEntries] = indexEntry;
    }

    if (blockHeader.numEntries != MAX_KEYS_LEAF)
    {
        blockHeader.numEntries++;
        leafBlock.setHeader(&blockHeader);

        for (int i = 0; i < blockHeader.numEntries; i++)
        {
            leafBlock.setEntry(&indices[i], i);
        }
        return SUCCESS;
    }

    int newRightBlk = splitLeaf(blockNum, indices);

    if (newRightBlk == E_DISKFULL)
    {
        return E_DISKFULL;
    }

    int retVal;
    if (blockHeader.pblock != -1)
    {
        InternalEntry newEntry;
        newEntry.attrVal = indices[MIDDLE_INDEX_LEAF].attrVal;
        newEntry.lChild = blockNum;
        newEntry.rChild = newRightBlk;

        retVal = insertIntoInternal(relId, attrName, blockHeader.pblock, newEntry);
    }
    else
    {
        retVal = createNewRoot(relId, attrName, indices[MIDDLE_INDEX_LEAF].attrVal,
                               blockNum, newRightBlk);
    }

    if (retVal == E_DISKFULL)
    {
        return E_DISKFULL;
    }

    return SUCCESS;
}

int BPlusTree::splitLeaf(int leafBlockNum, Index indices[])
{
    IndLeaf rightBlk;
    IndLeaf leftBlk(leafBlockNum);

    int rightBlkNum = rightBlk.getBlockNum();
    int leftBlkNum = leftBlk.getBlockNum();

    if (rightBlkNum == E_DISKFULL)
    {
        return E_DISKFULL;
    }

    HeadInfo leftBlkHeader, rightBlkHeader;
    leftBlk.getHeader(&leftBlkHeader);
    rightBlk.getHeader(&rightBlkHeader);

    rightBlkHeader.numEntries = (MAX_KEYS_LEAF + 1) / 2;
    rightBlkHeader.pblock = leftBlkHeader.pblock;
    rightBlkHeader.lblock = leftBlkNum;
    rightBlkHeader.rblock = leftBlkHeader.rblock;
    rightBlk.setHeader(&rightBlkHeader);

    leftBlkHeader.numEntries = (MAX_KEYS_LEAF + 1) / 2;
    leftBlkHeader.rblock = rightBlkNum;
    leftBlk.setHeader(&leftBlkHeader);

    // first 32 entries go to left block
    for (int i = 0; i < (MAX_KEYS_LEAF + 1) / 2; i++)
    {
        leftBlk.setEntry(&indices[i], i);
    }

    // next 32 entries go to right block
    for (int i = 0; i < (MAX_KEYS_LEAF + 1) / 2; i++)
    {
        rightBlk.setEntry(&indices[(MAX_KEYS_LEAF + 1) / 2 + i], i);
    }

    return rightBlkNum;
}

int BPlusTree::insertIntoInternal(int relId, char attrName[ATTR_SIZE], int intBlockNum, InternalEntry intEntry)
{
    AttrCatEntry attrCatEntry;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

    IndInternal intBlk(intBlockNum);

    HeadInfo blockHeader;
    intBlk.getHeader(&blockHeader);

    InternalEntry internalEntries[blockHeader.numEntries + 1];

    // copy existing entries and insert new entry in sorted order
    bool inserted = false;
    for (int i = 0; i < blockHeader.numEntries; i++)
    {
        intBlk.getEntry(&internalEntries[i], i);

        if (!inserted && compareAttrs(internalEntries[i].attrVal, intEntry.attrVal,
                                      attrCatEntry.attrType) >= 0)
        {
            internalEntries[i + 1] = internalEntries[i];
            internalEntries[i] = intEntry;

            // update lChild of the next entry to rChild of the inserted entry
            internalEntries[i + 1].lChild = intEntry.rChild;
            inserted = true;

            for (int j = i + 1; j < blockHeader.numEntries; j++)
            {
                intBlk.getEntry(&internalEntries[j + 1], j);
            }
            break;
        }
    }
    if (!inserted)
    {
        internalEntries[blockHeader.numEntries] = intEntry;
    }

    if (blockHeader.numEntries != MAX_KEYS_INTERNAL)
    {
        blockHeader.numEntries++;
        intBlk.setHeader(&blockHeader);

        for (int i = 0; i < blockHeader.numEntries; i++)
        {
            intBlk.setEntry(&internalEntries[i], i);
        }
        return SUCCESS;
    }

    int newRightBlk = splitInternal(intBlockNum, internalEntries);

    if (newRightBlk == E_DISKFULL)
    {
        bPlusDestroy(intEntry.rChild);
        return E_DISKFULL;
    }

    int retVal;
    if (blockHeader.pblock != -1)
    {
        InternalEntry newEntry;
        newEntry.attrVal = internalEntries[MIDDLE_INDEX_INTERNAL].attrVal;
        newEntry.lChild = intBlockNum;
        newEntry.rChild = newRightBlk;

        retVal = insertIntoInternal(relId, attrName, blockHeader.pblock, newEntry);
    }
    else
    {
        retVal = createNewRoot(relId, attrName,
                               internalEntries[MIDDLE_INDEX_INTERNAL].attrVal,
                               intBlockNum, newRightBlk);
    }

    if (retVal == E_DISKFULL)
    {
        return E_DISKFULL;
    }

    return SUCCESS;
}

int BPlusTree::splitInternal(int intBlockNum, InternalEntry internalEntries[])
{
    IndInternal rightBlk;
    IndInternal leftBlk(intBlockNum);

    int rightBlkNum = rightBlk.getBlockNum();
    int leftBlkNum = leftBlk.getBlockNum();

    if (rightBlkNum == E_DISKFULL)
    {
        return E_DISKFULL;
    }

    HeadInfo leftBlkHeader, rightBlkHeader;
    leftBlk.getHeader(&leftBlkHeader);
    rightBlk.getHeader(&rightBlkHeader);

    rightBlkHeader.numEntries = (MAX_KEYS_INTERNAL) / 2;
    rightBlkHeader.pblock = leftBlkHeader.pblock;
    rightBlk.setHeader(&rightBlkHeader);

    leftBlkHeader.numEntries = (MAX_KEYS_INTERNAL) / 2;
    leftBlk.setHeader(&leftBlkHeader);

    // first 50 entries go to left block (indices 0-49)
    for (int i = 0; i < (MAX_KEYS_INTERNAL) / 2; i++)
    {
        leftBlk.setEntry(&internalEntries[i], i);
    }

    // entries from index 51 to 100 go to right block (index 50 goes to parent)
    for (int i = 0; i < (MAX_KEYS_INTERNAL) / 2; i++)
    {
        rightBlk.setEntry(&internalEntries[MIDDLE_INDEX_INTERNAL + 1 + i], i);
    }

    // get block type of a child (use lChild of first entry)
    int type = StaticBuffer::getStaticBlockType(internalEntries[MIDDLE_INDEX_INTERNAL + 1].lChild);

    // update pblock of all children of the new right block
    for (int i = 0; i <= (MAX_KEYS_INTERNAL) / 2; i++)
    {
        int childBlockNum;
        if (i == 0)
        {
            childBlockNum = internalEntries[MIDDLE_INDEX_INTERNAL + 1].lChild;
        }
        else
        {
            childBlockNum = internalEntries[MIDDLE_INDEX_INTERNAL + i].rChild;
        }

        BlockBuffer childBlk(childBlockNum);
        HeadInfo childHeader;
        childBlk.getHeader(&childHeader);
        childHeader.pblock = rightBlkNum;
        childBlk.setHeader(&childHeader);
    }

    return rightBlkNum;
}

int BPlusTree::createNewRoot(int relId, char attrName[ATTR_SIZE], Attribute attrVal, int lChild, int rChild)
{
    AttrCatEntry attrCatEntry;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

    IndInternal newRootBlk;
    int newRootBlkNum = newRootBlk.getBlockNum();

    if (newRootBlkNum == E_DISKFULL)
    {
        bPlusDestroy(rChild);
        return E_DISKFULL;
    }

    HeadInfo newRootHeader;
    newRootBlk.getHeader(&newRootHeader);
    newRootHeader.numEntries = 1;
    newRootBlk.setHeader(&newRootHeader);

    InternalEntry newEntry;
    newEntry.lChild = lChild;
    newEntry.attrVal = attrVal;
    newEntry.rChild = rChild;
    newRootBlk.setEntry(&newEntry, 0);

    // update pblock of lChild
    BlockBuffer lChildBlk(lChild);
    HeadInfo lChildHeader;
    lChildBlk.getHeader(&lChildHeader);
    lChildHeader.pblock = newRootBlkNum;
    lChildBlk.setHeader(&lChildHeader);

    // update pblock of rChild
    BlockBuffer rChildBlk(rChild);
    HeadInfo rChildHeader;
    rChildBlk.getHeader(&rChildHeader);
    rChildHeader.pblock = newRootBlkNum;
    rChildBlk.setHeader(&rChildHeader);

    attrCatEntry.rootBlock = newRootBlkNum;
    AttrCacheTable::setAttrCatEntry(relId, attrName, &attrCatEntry);

    return SUCCESS;
}