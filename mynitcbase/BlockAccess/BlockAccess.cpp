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


int BlockAccess::renameRelation(char oldName[ATRR_SIZE], char newName[ATTR_SIZE]){
    
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute newRelationName;
    strcpy(newRelationName.sVal, newName);

    RecId retRecId=BlockAccess::linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, newRelationName,EQ);

    if(retRecId.block!=-1 && retRecId.slot!=-1){
        return E_RELEXIST;
    }

    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute oldRelationName;
    strcpy(oldRelationName.sVal, oldRelationName);

    RecId retRecId=BlockAccess::linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, oldRelationName, EQ);

    if(retRecId.block==-1  && retRecId.slot==-1){
        return E_RELNOTEXIST;
    }

    Recbuffer relCatBuffer(retRecId.block);
    Attribute oldRec[RELCAT_NO_ATTRS];

    relCatBuffer.getRecord(oldRec, retRecId.slot);
    strcpy(oldRec[RELCAT_REL_NAME_INDEX].sVal, newName);
    relCatBuffer.setRecord(oldRec,retRecId.slot);
    
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    for(int i=0;i<oldRec[RELCAT_NO_ATTRIBUTES_INDEX].nVal;i++){
        searchIndex=linearSearch(ATTRCAT_RELID, (char  *)ATTRCAT_RELID_RELNAME,oldRelationName, EQ);

        RecBuffer attrBlock(searchIndex.block);
        Attribute attrRecord[ATTRCAT_NO_ATTRS];
        attrBlock.getRecord(attrRecord, searchIndex.slot);
        strcpy(attrRecord[ATTRCAT_REL_NAME_INDEX].sVal, newName);
        attrBlock.setRecord(attrRecord, searchIndex.slot);

    }

    return SUCCESS;
}   


int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE]) {
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr;
    strcpy(relNameAttr.sVal,relName);

    RecId searchIndex = BlockAccess::linearSearch(RELCAT_RELID,(char*)RELCAT_ATTR_RELNAME,relNameAttr,EQ);

    if(searchIndex.block == -1 && searchIndex.slot == -1) {
        return E_RELNOTEXIST ;
    }
    
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    RecId attrToRenameRecId{-1, -1};
    Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];

    while(true) {
        searchIndex = BlockAccess::linearSearch(ATTRCAT_RELID, (char*)ATTRCAT_ATTR_RELNAME, relNameAttr, EQ);

        if(searchIndex.block == -1 && searchIndex.slot == -1) {
            break ;
        }

        RecBuffer attrBuffer(searchIndex.block);
        Attribute attrRecord[ATTRCAT_NO_ATTRS];

        attrBuffer.getRecord(attrRecord,searchIndex.slot);

        if(strcmp(attrRecord[ ATTRCAT_ATTR_NAME_INDEX].sVal,oldName) == 0) {
            attrToRenameRecId = searchIndex;
            break;
        }

        if(strcmp(attrRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName) == 0) {
            return E_ATTREXIST;
        }
    }

    if(attrToRenameRecId.block == -1 && attrToRenameRecId.slot == -1) {
        return E_ATTRNOTEXIST;
    }

    RecBuffer attrBuffer(attrToRenameRecId.block);
    Attribute attrRecord[ATTRCAT_NO_ATTRS];
    attrBuffer.getRecord(attrRecord, attrToRenameRecId.slot);
    strcpy(attrRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName);
    attrBuffer.setRecord(attrRecord, attrToRenameRecId.slot);

    return SUCCESS ;
}
