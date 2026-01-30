#include "BlockAccess.h"
#include <cstring>
#include <stdio.h>

RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op) {
    RecId prevRecId ;
    int ret = RelCacheTable::getSearchIndex(relId,&prevRecId);

    int block = prevRecId.block ;
    int slot = prevRecId.slot ;

    if(prevRecId.block == -1 && prevRecId.slot == -1) {
        RelCatEntry relCatEntry ;
        RelCacheTable::getRelCatEntry(relId,&relCatEntry);

        block = relCatEntry.firstBlk ;
        slot = 0 ;

    } else {
        block = prevRecId.block ;
        slot = prevRecId.slot + 1 ;
    }

    while(block != -1) {
        //printf("values -> %d %d\n", block, slot);
        RecBuffer relCatBuffer(block);

        HeadInfo head ;
        relCatBuffer.getHeader(&head);

        if(slot >= head.numSlots) {
            block = head.rblock ;
            slot = 0;
            continue ;
        }
        
        unsigned char slotMap[head.numSlots] ;
        relCatBuffer.getSlotMap(slotMap);

        if(slotMap[slot] == SLOT_UNOCCUPIED) {
            slot++ ;
            continue ;
        }

        RelCatEntry relCatEntry ;
        RelCacheTable::getRelCatEntry(relId,&relCatEntry);

        int numAttrs = relCatEntry.numAttrs ;

        Attribute recordAttr[numAttrs];
        relCatBuffer.getRecord(recordAttr, slot);

        AttrCatEntry attrCatEntry ;

        int ret = AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatEntry);

        int offset = attrCatEntry.offset ;
        Attribute recordVal = recordAttr[offset];

        int cmpVal = 0;

        if(attrCatEntry.attrType == NUMBER) {
            if(recordVal.nVal < attrVal.nVal) {
                cmpVal = -1; 
            } else if(recordVal.nVal > attrVal.nVal) {
                cmpVal = 1;
            } else {
                cmpVal = 0 ;
            }
        } else {
            cmpVal = strcmp(recordVal.sVal,attrVal.sVal);
        }

        if(
            (op == NE && cmpVal != 0) ||    // if op is "not equal to"
            (op == LT && cmpVal < 0) ||     // if op is "less than"
            (op == LE && cmpVal <= 0) ||    // if op is "less than or equal to"
            (op == EQ && cmpVal == 0) ||    // if op is "equal to"
            (op == GT && cmpVal > 0) ||     // if op is "greater than"
            (op == GE && cmpVal >= 0)       // if op is "greater than or equal to"
        ) {
            RecId newRecId = {block,slot} ;
            RelCacheTable::setSearchIndex(relId,&newRecId);

            return newRecId ;
        }   
        slot++ ;
    }
    return RecId{-1,-1} ;
}
