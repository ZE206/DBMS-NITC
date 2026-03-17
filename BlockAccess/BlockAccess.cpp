#include "BlockAccess.h"
#include <cstring>
#include <stdio.h>

RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op)
{
    RecId prevRecId;
    int ret = RelCacheTable::getSearchIndex(relId, &prevRecId);

    int block = prevRecId.block;
    int slot = prevRecId.slot;

    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        RelCatEntry relCatEntry;
        RelCacheTable::getRelCatEntry(relId, &relCatEntry);

        block = relCatEntry.firstBlk;
        slot = 0;
    }
    else
    {
        block = prevRecId.block;
        slot = prevRecId.slot + 1;
    }

    while (block != -1)
    {
        // printf("values -> %d %d\n", block, slot);
        RecBuffer relCatBuffer(block);

        HeadInfo head;
        relCatBuffer.getHeader(&head);

        if (slot >= head.numSlots)
        {
            block = head.rblock;
            slot = 0;
            continue;
        }

        unsigned char slotMap[head.numSlots];
        relCatBuffer.getSlotMap(slotMap);

        if (slotMap[slot] == SLOT_UNOCCUPIED)
        {
            slot++;
            continue;
        }

        RelCatEntry relCatEntry;
        RelCacheTable::getRelCatEntry(relId, &relCatEntry);

        int numAttrs = relCatEntry.numAttrs;

        Attribute recordAttr[numAttrs];
        relCatBuffer.getRecord(recordAttr, slot);

        AttrCatEntry attrCatEntry;

        int ret = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

        int offset = attrCatEntry.offset;
        Attribute recordVal = recordAttr[offset];

        int cmpVal = 0;

        if (attrCatEntry.attrType == NUMBER)
        {
            if (recordVal.nVal < attrVal.nVal)
            {
                cmpVal = -1;
            }
            else if (recordVal.nVal > attrVal.nVal)
            {
                cmpVal = 1;
            }
            else
            {
                cmpVal = 0;
            }
        }
        else
        {
            cmpVal = strcmp(recordVal.sVal, attrVal.sVal);
        }

        if (
            (op == NE && cmpVal != 0) || // if op is "not equal to"
            (op == LT && cmpVal < 0) ||  // if op is "less than"
            (op == LE && cmpVal <= 0) || // if op is "less than or equal to"
            (op == EQ && cmpVal == 0) || // if op is "equal to"
            (op == GT && cmpVal > 0) ||  // if op is "greater than"
            (op == GE && cmpVal >= 0)    // if op is "greater than or equal to"
        )
        {
            RecId newRecId = {block, slot};
            RelCacheTable::setSearchIndex(relId, &newRecId);

            return newRecId;
        }
        slot++;
    }
    return RecId{-1, -1};
}

int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE])
{
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute newRelationName;
    strcpy(newRelationName.sVal, newName);

    RecId retRecId = BlockAccess::linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, newRelationName, EQ);

    if (retRecId.block != -1 && retRecId.slot != -1)
    {
        return E_RELEXIST;
    }

    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute oldRelationName;
    strcpy(oldRelationName.sVal, oldName);

    RecId searchIndex = BlockAccess::linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, oldRelationName, EQ);

    if (searchIndex.block == -1 && searchIndex.slot == -1)
    {
        return E_RELNOTEXIST;
    }

    RecBuffer relCatBuffer(searchIndex.block);
    Attribute oldRec[RELCAT_NO_ATTRS];

    relCatBuffer.getRecord(oldRec, searchIndex.slot);
    strcpy(oldRec[RELCAT_REL_NAME_INDEX].sVal, newName);
    relCatBuffer.setRecord(oldRec, searchIndex.slot);

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    for (int i = 0; i < oldRec[RELCAT_NO_ATTRIBUTES_INDEX].nVal; i++)
    {
        searchIndex = linearSearch(ATTRCAT_RELID, (char *)ATTRCAT_ATTR_RELNAME, oldRelationName, EQ);

        RecBuffer attrBlock(searchIndex.block);
        Attribute attrRecord[ATTRCAT_NO_ATTRS];
        attrBlock.getRecord(attrRecord, searchIndex.slot);
        strcpy(attrRecord[ATTRCAT_REL_NAME_INDEX].sVal, newName);
        attrBlock.setRecord(attrRecord, searchIndex.slot);
    }
    return SUCCESS;
}

int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE])
{
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr;
    strcpy(relNameAttr.sVal, relName);

    RecId searchIndex = BlockAccess::linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, relNameAttr, EQ);

    if (searchIndex.block == -1 && searchIndex.slot == -1)
    {
        return E_RELNOTEXIST;
    }

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    RecId attrToRenameRecId{-1, -1};
    Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];

    while (true)
    {
        searchIndex = BlockAccess::linearSearch(ATTRCAT_RELID, (char *)ATTRCAT_ATTR_RELNAME, relNameAttr, EQ);

        if (searchIndex.block == -1 && searchIndex.slot == -1)
        {
            break;
        }

        RecBuffer attrBuffer(searchIndex.block);
        Attribute attrRecord[ATTRCAT_NO_ATTRS];

        attrBuffer.getRecord(attrRecord, searchIndex.slot);

        if (strcmp(attrRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, oldName) == 0)
        {
            attrToRenameRecId = searchIndex;
            break;
        }

        if (strcmp(attrRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName) == 0)
        {
            return E_ATTREXIST;
        }
    }

    if (attrToRenameRecId.block == -1 && attrToRenameRecId.slot == -1)
    {
        return E_ATTRNOTEXIST;
    }

    RecBuffer attrBuffer(attrToRenameRecId.block);
    Attribute attrRecord[ATTRCAT_NO_ATTRS];
    attrBuffer.getRecord(attrRecord, attrToRenameRecId.slot);
    strcpy(attrRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName);
    attrBuffer.setRecord(attrRecord, attrToRenameRecId.slot);

    return SUCCESS;
}

int BlockAccess::insert(int relId, Attribute *record)
{
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);

    int currBlockNum = relCatEntry.firstBlk;

    RecId rec_id = {-1, -1};
    int numOfSlots = relCatEntry.numSlotsPerBlk;
    int numOfAttributes = relCatEntry.numAttrs;
    int prevBlockNum = -1;

    while (currBlockNum != -1)
    {
        RecBuffer recBuffer(currBlockNum);

        HeadInfo head;
        unsigned char slotMap[numOfSlots];
        recBuffer.getHeader(&head);
        recBuffer.getSlotMap(slotMap);

        for (int i = 0; i < head.numSlots; i++)
        {
            if (slotMap[i] == SLOT_UNOCCUPIED)
            {
                rec_id.block = currBlockNum;
                rec_id.slot = i;
                break;
            }
        }

        if (rec_id.block != -1 && rec_id.slot != -1)
        {
            break;
        }

        prevBlockNum = currBlockNum;
        currBlockNum = head.rblock;
    }

    if (rec_id.block == -1 && rec_id.slot == -1)
    {
        if (strcmp(relCatEntry.relName, "RELATIONCAT") == 0)
        {
            return E_MAXRELATIONS;
        }

        RecBuffer newBlockBuffer;
        int newBlockNum = newBlockBuffer.getBlockNum();

        if (newBlockNum == E_DISKFULL)
        {
            return E_DISKFULL;
        }

        rec_id.block = newBlockNum;
        rec_id.slot = 0;

        HeadInfo head;
        head.blockType = REC;
        head.pblock = -1;
        head.rblock = -1;
        head.numEntries = 0;
        head.numSlots = relCatEntry.numSlotsPerBlk;
        head.numAttrs = relCatEntry.numAttrs;
        head.lblock = (relCatEntry.numRecs == 0) ? -1 : prevBlockNum;

        newBlockBuffer.setHeader(&head);

        unsigned char slotMap[numOfSlots];

        for (int i = 0; i < relCatEntry.numSlotsPerBlk; i++)
        {
            slotMap[i] = SLOT_UNOCCUPIED;
        }

        newBlockBuffer.setSlotMap(slotMap);

        if (prevBlockNum != -1)
        {
            RecBuffer prevRecordBuffer(prevBlockNum);
            HeadInfo prevHead;
            prevRecordBuffer.getHeader(&prevHead);

            prevHead.rblock = newBlockNum;
            prevRecordBuffer.setHeader(&prevHead);
        }
        else
        {
            relCatEntry.firstBlk = newBlockNum;
            RelCacheTable::setRelCatEntry(relId, &relCatEntry);
        }

        relCatEntry.lastBlk = newBlockNum;
        RelCacheTable::setRelCatEntry(relId, &relCatEntry);
    }

    RecBuffer recBuffer(rec_id.block);
    recBuffer.setRecord(record, rec_id.slot);

    unsigned char slotMap[numOfSlots];

    recBuffer.getSlotMap(slotMap);
    slotMap[rec_id.slot] = SLOT_OCCUPIED;
    recBuffer.setSlotMap(slotMap);

    HeadInfo head;
    recBuffer.getHeader(&head);
    head.numEntries++;
    recBuffer.setHeader(&head);

    relCatEntry.numRecs++;
    RelCacheTable::setRelCatEntry(relId, &relCatEntry);

    return SUCCESS;
}

int BlockAccess::search(int relId, Attribute *record, char attrName[ATTR_SIZE], Attribute attrVal, int op)
{
    RecId recId;
    recId = BlockAccess::linearSearch(relId, attrName, attrVal, op);
    if (recId.block == -1 && recId.slot == -1)
    {
        return E_NOTFOUND;
    }

    RecBuffer recBuffer(recId.block);
    recBuffer.getRecord(record, recId.slot);

    return SUCCESS;
}

int BlockAccess::deleteRelation(char relName[ATTR_SIZE])
{
    if (strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0)
    {
        return E_NOTPERMITTED;
    }
    int relId;

    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr;
    strcpy(relNameAttr.sVal, relName);

    RecId recId;
    recId = BlockAccess::linearSearch(RELCAT_RELID, (char *)"RelName", relNameAttr, EQ);
    if (recId.block == -1 && recId.slot == -1)
    {
        return E_RELNOTEXIST;
    }

    Attribute relCatEntryRecord[RELCAT_NO_ATTRS];
    RecBuffer relCatBlockBuffer(recId.block);
    relCatBlockBuffer.getRecord(relCatEntryRecord, recId.slot);

    int firstBlock = (int)relCatEntryRecord[RELCAT_FIRST_BLOCK_INDEX].nVal;
    int numAttrs = (int)relCatEntryRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;

    int currentBlock = firstBlock;
    while (currentBlock != -1)
    {
        HeadInfo head;
        BlockBuffer blockBuffer(currentBlock);
        blockBuffer.getHeader(&head);
        int nextBlock = head.rblock;
        blockBuffer.releaseBlock();
        currentBlock = nextBlock;
    }

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    int numberOfAttributesDeleted = 0;

    while (true)
    {
        RecId attrCatRecId;

        attrCatRecId = linearSearch(ATTRCAT_RELID, (char *)"RelName", relNameAttr, EQ);
        if (attrCatRecId.block == -1 && attrCatRecId.slot == -1)
        {
            break;
        }
        numberOfAttributesDeleted++;

        RecBuffer attrCatBuffer(attrCatRecId.block);
        HeadInfo head;
        attrCatBuffer.getHeader(&head);

        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBuffer.getRecord(attrCatRecord, attrCatRecId.slot);

        int rootBlock = (int)attrCatRecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal;
        unsigned char slotMap[head.numSlots];
        attrCatBuffer.getSlotMap(slotMap);

        head.numEntries--;

        attrCatBuffer.setHeader(&head);

        if (head.numEntries == 0)
        {

            RecBuffer leftBlock(head.lblock);
            HeadInfo lblockHeader;
            leftBlock.getHeader(&lblockHeader);

            lblockHeader.rblock = head.rblock;

            leftBlock.setHeader(&lblockHeader);

            if (head.rblock != -1)
            {
                RecBuffer rightBlock(head.rblock);
                HeadInfo rblockHeader;
                rightBlock.getHeader(&rblockHeader);
                rblockHeader.lblock = head.lblock;
                rightBlock.setHeader(&rblockHeader);
            }
            else
            {
                RelCatEntry attrCatEntry;
                RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &attrCatEntry);
                attrCatEntry.lastBlk = head.lblock;
                RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &attrCatEntry);
            }
            attrCatBuffer.releaseBlock();
        }
        
    }

    RecBuffer relCatBuffer(recId.block);
    HeadInfo relCatHead;
    relCatBuffer.getHeader(&relCatHead);

    relCatHead.numEntries--;
    relCatBuffer.setHeader(&relCatHead);

    unsigned char relCatSlotMap[relCatHead.numSlots];
    relCatBuffer.getSlotMap(relCatSlotMap);

    relCatSlotMap[recId.slot] = SLOT_UNOCCUPIED;
    relCatBuffer.setSlotMap(relCatSlotMap);

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(RELCAT_RELID, &relCatEntry);
    relCatEntry.numRecs--;
    RelCacheTable::setRelCatEntry(RELCAT_RELID, &relCatEntry);

    RelCatEntry attrCatEntry;
    RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &attrCatEntry);
    attrCatEntry.numRecs -= numberOfAttributesDeleted;
    RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &attrCatEntry);

    return SUCCESS;
}