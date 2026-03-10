#include "BlockAccess.h"          
#include "../Cache/RelCacheTable.h"  
#include "../Cache/AttrCacheTable.h" 
#include "../Buffer/BlockBuffer.h"   
#include "../define/constants.h"    
#include <cstring>               
#include <stdio.h>

RecId BlockAccess::linearSearch(int relId,
                                char attrName[ATTR_SIZE],
                                union Attribute attrVal,
                                int op) {

    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId, &prevRecId);

    int block, slot;

    
    if (prevRecId.block == -1 && prevRecId.slot == -1) {

        RelCatEntry relCatEntry;
        RelCacheTable::getRelCatEntry(relId, &relCatEntry);

        block = relCatEntry.firstBlk;
        slot = 0;
        
    } else {
        block = prevRecId.block;
        slot  = prevRecId.slot + 1;
    }


    AttrCatEntry attrCatEntry;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);
    int attrOffset = attrCatEntry.offset;
    int attrType   = attrCatEntry.attrType;

    while (block != -1) {

        RecBuffer recBuf(block);

        HeadInfo head;
        recBuf.getHeader(&head);

        unsigned char slotMap[head.numSlots];
        recBuf.getSlotMap(slotMap);

        if (slot >= head.numSlots) {
            block = head.rblock;
            slot = 0;
            continue;
        }

       
        if (slotMap[slot] == SLOT_UNOCCUPIED) {
            slot++;
            continue;
        }

     
        Attribute record[head.numAttrs];
        recBuf.getRecord(record, slot);

  
        int cmpVal = compareAttrs(record[attrOffset], attrVal, attrType);

       
        if (
            (op == NE && cmpVal != 0) ||
            (op == LT && cmpVal < 0)  ||
            (op == LE && cmpVal <= 0) ||
            (op == EQ && cmpVal == 0) ||
            (op == GT && cmpVal > 0)  ||
            (op == GE && cmpVal >= 0)
        ) {
            RecId found = {block, slot};
            RelCacheTable::setSearchIndex(relId, &found);
            return found;
        }

        slot++;
    }

    return RecId{-1, -1};
}