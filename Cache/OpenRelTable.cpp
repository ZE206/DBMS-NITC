#include "OpenRelTable.h"
#include <cstring>
#include <stdlib.h>
#include <iostream>

OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN] ;

void freeLinkedList(AttrCacheEntry **head) {
    if(!head || !*head) return ;

    AttrCacheEntry* current = *head;

    while(current) {
        AttrCacheEntry * nextNode = current->next ;
        free(current) ;
        current = nextNode ;
    }
    *head = nullptr ;
}

OpenRelTable::OpenRelTable() {

    if (tableMetaInfo[RELCAT_RELID].free == false && 
        strcmp(tableMetaInfo[RELCAT_RELID].relName, "RELATIONCAT") == 0) {
        return; 
    }

    // 1. Initialize ALL MetaInfo slots to free
    for (int i = 0; i < MAX_OPEN; ++i) {
        tableMetaInfo[i].free = true;
        strcpy(tableMetaInfo[i].relName, "");
    }

    // 2. Initialize relCache and attrCache with nullptr
    for (int i = 0; i < MAX_OPEN; ++i) {
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
    }

    
    RecBuffer relCatBuffer(RELCAT_BLOCK);
    Attribute relCatRecord[RELCAT_NO_ATTRS];

    HeadInfo relCatHeader;
    relCatBuffer.getHeader(&relCatHeader);

    for(int i = 0; i <= ATTRCAT_RELID; i++) {
        relCatBuffer.getRecord(relCatRecord, i); 
        RelCacheEntry relCacheEntry;
        RelCacheTable::relCache[i] = (RelCacheEntry*)malloc(sizeof(RelCacheEntry));
        RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
        relCacheEntry.recId.block = RELCAT_BLOCK;
        relCacheEntry.recId.slot = i;
        relCacheEntry.searchIndex = {-1, -1};

        *(RelCacheTable::relCache[i]) = relCacheEntry;
    }

    RecBuffer attrCatBuffer(ATTRCAT_BLOCK);
    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

    AttrCacheEntry* attrCacheEntry = nullptr;
    AttrCacheEntry* head = nullptr;
    AttrCacheEntry* prev = nullptr;

    for(int i = RELCAT_RELID, attrSlotIndex = 0; i <= ATTRCAT_RELID ; i++) {
        RelCatEntry relCatEntry;
        HeadInfo attrCatHeader;
        attrCatBuffer.getHeader(&attrCatHeader);

        relCatEntry = RelCacheTable::relCache[i]->relCatEntry;
        int noOfAttributes = relCatEntry.numAttrs;

        for(int j = 0; j < noOfAttributes; j++, attrSlotIndex++) {
            attrCatBuffer.getRecord(attrCatRecord, attrSlotIndex);
            attrCacheEntry = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
            AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &(attrCacheEntry->attrCatEntry));
            attrCacheEntry->recId.block = ATTRCAT_BLOCK;
            attrCacheEntry->recId.slot = attrSlotIndex; 

            if(!head) {
                head = attrCacheEntry;
                prev = attrCacheEntry; 
                continue;
            }
            if(attrSlotIndex == attrCatHeader.numSlots - 1) {
                attrSlotIndex = -1;
                HeadInfo attrCatHeader; 
                attrCatBuffer.getHeader(&attrCatHeader);
                attrCatBuffer = RecBuffer(attrCatHeader.rblock);
            }
            prev->next = attrCacheEntry;
            prev = attrCacheEntry;
        }
        attrCacheEntry->next = nullptr;
        AttrCacheTable::attrCache[i] = head;
        head = nullptr;
        prev = nullptr;
   }

    tableMetaInfo[RELCAT_RELID].free = false ;
    tableMetaInfo[ATTRCAT_RELID].free = false ;

    strcpy(tableMetaInfo[RELCAT_RELID].relName,"RELATIONCAT") ;
    strcpy(tableMetaInfo[ATTRCAT_RELID].relName,"ATTRIBUTECAT") ;
}

OpenRelTable::~OpenRelTable() {
    // free all the memory that you allocated in the constructor
    for(int i = 2; i< MAX_OPEN; i++) {
        if(!tableMetaInfo[i].free) {
            OpenRelTable::closeRel(i);
        }
    }
    
    if(RelCacheTable::relCache[ATTRCAT_RELID]->dirty){
        RelCatEntry relCatEntry;
        relCatEntry= RelCacheTable::relCache[ATTRCAT_RELID]->relCatEntry;
        Attribute relCatRecord[RELCAT_NO_ATTRS];
        RelCacheTable::relCatEntryToRecord(&relCatEntry, relCatRecord);
        RecId recId=RelCacheTable::relCache[ATTRCAT_RELID]->recId;
        RecBuffer relCatBlock(recId.block);

        relCatBlock.setRecord(relCatRecord, recId.slot);


    }

    if(RelCacheTable::relCache[RELCAT_RELID]->dirty){
        
        RelCatEntry relCatEntry;
        relCatEntry= RelCacheTable::relCache[RELCAT_RELID]->relCatEntry;
        Attribute relCatRecord[RELCAT_NO_ATTRS];
        RelCacheTable::relCatEntryToRecord(&relCatEntry, relCatRecord);
        RecId recId=RelCacheTable::relCache[RELCAT_RELID]->recId;
        RecBuffer relCatBlock(recId.block);

        relCatBlock.setRecord(relCatRecord, recId.slot);
    }
    for (int i = 0; i < MAX_OPEN; ++i) {
        if (RelCacheTable::relCache[i]) {
            free(RelCacheTable::relCache[i]) ;
            RelCacheTable::relCache[i] = nullptr ; 
        }
        if(AttrCacheTable::attrCache[i]) {
            freeLinkedList(&AttrCacheTable::attrCache[i]);
            AttrCacheTable::attrCache[i] = nullptr ;
        }
    }
    
}

int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {

    for(int relId = 0; relId < MAX_OPEN; relId++) {
        if(tableMetaInfo[relId].free == false && strcmp(tableMetaInfo[relId].relName, relName) == 0) {
            return relId;
        }
    }
    return E_RELNOTOPEN;
}

int OpenRelTable::getFreeOpenRelTableEntry() {
    for(int relId = 0; relId < MAX_OPEN; relId++) {
        if(tableMetaInfo[relId].free == true) {
            return relId ;
        }
    }
    return E_CACHEFULL ;
}

int OpenRelTable::openRel(char relName[ATTR_SIZE]) {

    int relCheck = getRelId(relName);
    if(relCheck != E_RELNOTOPEN) {
        return relCheck;
    }

    int emptySlot = getFreeOpenRelTableEntry();
    if(emptySlot == E_CACHEFULL) {
        return E_CACHEFULL;
    }

    int relId = emptySlot;

    Attribute attrVal;
    strcpy(attrVal.sVal, relName);

    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    RecId searchIndex = BlockAccess::linearSearch(RELCAT_RELID, (char*)RELCAT_ATTR_RELNAME, attrVal, EQ);

    if(searchIndex.block == -1 && searchIndex.slot == -1) {
        return E_RELNOTEXIST;
    }

    RecBuffer relationBuffer(searchIndex.block);
    Attribute record[RELCAT_NO_ATTRS];
    relationBuffer.getRecord(record, searchIndex.slot);

    RelCacheEntry *relCacheEntry = (RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    RelCacheTable::recordToRelCatEntry(record, &relCacheEntry->relCatEntry);

    relCacheEntry->recId = searchIndex;
    RelCacheTable::relCache[relId] = relCacheEntry;

    Attribute attrRecord[ATTRCAT_NO_ATTRS];
    AttrCacheEntry* prev = nullptr;
    AttrCacheEntry* head = nullptr;

    int numberOfAttributes = RelCacheTable::relCache[relId]->relCatEntry.numAttrs;
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    for(int i = 0; i < numberOfAttributes; i++) {
        RecId attributeSearchIndex = BlockAccess::linearSearch(ATTRCAT_RELID, (char*)RELCAT_ATTR_RELNAME, attrVal, EQ);

        RecBuffer attrCatBlock = RecBuffer(attributeSearchIndex.block);
        attrCatBlock.getRecord(attrRecord, attributeSearchIndex.slot);
        AttrCacheEntry* attrCacheEntry = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        AttrCacheTable::recordToAttrCatEntry(attrRecord, &(attrCacheEntry->attrCatEntry));
        attrCacheEntry->recId = attributeSearchIndex;
        attrCacheEntry->recId.block = attributeSearchIndex.block ;
        attrCacheEntry->recId.slot = attributeSearchIndex.slot ;
        
        if(!head) {
            head = attrCacheEntry;
        } else {
            prev->next = attrCacheEntry;
        }
        prev = attrCacheEntry;
    }

    prev->next = nullptr;
    AttrCacheTable::attrCache[relId] = head;

    tableMetaInfo[relId].free = false;
    strcpy(tableMetaInfo[relId].relName, relName);

    return relId ;
}

int OpenRelTable::closeRel(int relId) {
    //printf("DEBUG: closeRel called with ID: %d (MAX_OPEN is %d)\n", relId, MAX_OPEN);

    if(relId == RELCAT_RELID || relId == ATTRCAT_RELID) {
        return E_NOTPERMITTED ;
    }

    if( relId < 0 || relId >= MAX_OPEN) {
        return E_OUTOFBOUND ;
    }

    if(tableMetaInfo[relId].free == true) {
        return E_RELNOTOPEN;
    }


    if(RelCacheTable::relCache[relId]->dirty) {
        Attribute record[RELCAT_NO_ATTRS] ;
        RelCacheTable::relCatEntryToRecord(&(RelCacheTable::relCache[relId]->relCatEntry),record);

        Attribute attrVal ;
        strcpy(attrVal.sVal,record[RELCAT_REL_NAME_INDEX].sVal);
        RelCacheTable::resetSearchIndex(RELCAT_RELID);
        RecId recId = BlockAccess::linearSearch(RELCAT_RELID,(char*)RELCAT_ATTR_RELNAME, attrVal, EQ);
        RecBuffer relCatBlock(recId.block);

        relCatBlock.setRecord(record,recId.slot);
    }

    if(RelCacheTable::relCache[relId]) {
        free(RelCacheTable::relCache[relId]);
        RelCacheTable::relCache[relId] = nullptr ;
    }

    if (AttrCacheTable::attrCache[relId]) {
        freeLinkedList(&AttrCacheTable::attrCache[relId]);
        AttrCacheTable::attrCache[relId] = nullptr; 
    }

    tableMetaInfo[relId].free = true;
    strcpy(tableMetaInfo[relId].relName, "");
    return SUCCESS;
}