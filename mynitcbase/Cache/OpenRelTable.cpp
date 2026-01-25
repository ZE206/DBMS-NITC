#include "OpenRelTable.h"
#include <cstring>
#include <stdlib.h>

OpenRelTable::OpenRelTable() {

    // initialize relCache and attrCache with nullptr
    for (int i = 0; i < MAX_OPEN; ++i) {
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
    }

    /************ Setting up Relation Cache entries ************/
    // (we need to populate relation cache with entries for the relation catalog
    //  and attribute catalog.)

    /**** setting up Relation Catalog relation in the Relation Cache Table****/
    RecBuffer relCatBlock(RELCAT_BLOCK);

    Attribute relCatRecord[RELCAT_NO_ATTRS];
    relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);

    struct RelCacheEntry relCacheEntry;
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
    relCacheEntry.recId.block = RELCAT_BLOCK;
    relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;

    // allocate this on the heap because we want it to persist outside this function
    RelCacheTable::relCache[RELCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;

    /**** setting up Attribute Catalog relation in the Relation Cache Table ****/

    // set up the relation cache entry for the attribute catalog similarly
    // from the record at RELCAT_SLOTNUM_FOR_ATTRCAT

    Attribute attrCatRecord[RELCAT_NO_ATTRS];
    relCatBlock.getRecord(attrCatRecord,RELCAT_SLOTNUM_FOR_ATTRCAT);

    RelCacheTable::recordToRelCatEntry(attrCatRecord,&relCacheEntry.relCatEntry) ;
    relCacheEntry.recId.block = RELCAT_BLOCK ;
    relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT ;

    // set the value at RelCacheTable::relCache[ATTRCAT_RELID]
    RelCacheTable::relCache[ATTRCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[ATTRCAT_RELID]) = relCacheEntry ;

    // Exercise 1 : Loading student table to relCacheTable 
    relCatBlock.getRecord(relCatRecord,2);

    RelCacheTable::recordToRelCatEntry(relCatRecord,&relCacheEntry.relCatEntry);
    relCacheEntry.recId.block = RELCAT_BLOCK ;
    relCacheEntry.recId.slot = 2 ;

    RelCacheTable::relCache[2] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[2]) = relCacheEntry ;

    /************ Setting up Attribute cache entries ************/
    // (we need to populate attribute cache with entries for the relation catalog
    //  and attribute catalog.)

    /**** setting up Relation Catalog relation in the Attribute Cache Table ****/
    RecBuffer attrCatBlock(ATTRCAT_BLOCK);

    attrCatBlock.getRecord(attrCatRecord,RELCAT_SLOTNUM_FOR_ATTRCAT);

    struct AttrCacheEntry attrCacheEntry;
    AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&attrCacheEntry.attrCatEntry);


    struct AttrCacheEntry * head = nullptr ;
    struct AttrCacheEntry * curr = nullptr ;

    for(int i=0; i < 6 ; i++) {
        struct AttrCacheEntry * entry = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));

        attrCatBlock.getRecord(attrCatRecord,i);
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&entry->attrCatEntry);

        entry->recId.block = ATTRCAT_BLOCK;
        entry->recId.slot = i ;
        entry->next = nullptr ;

        if(head == nullptr) {
            head = entry ;
            curr = entry ;
        } else {
            curr->next = entry ;
            curr = entry ;
        }
    }

    AttrCacheTable::attrCache[RELCAT_RELID] = head;

    /**** setting up Attribute Catalog relation in the Attribute Cache Table ****/

    head = nullptr; 
    curr = nullptr;

    for(int i = 6; i <= 11; i++) {
        struct AttrCacheEntry * entry = (struct AttrCacheEntry *) (malloc(sizeof(AttrCacheEntry)));

        attrCatBlock.getRecord(attrCatRecord,i);
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&entry->attrCatEntry);

        entry->recId.block = ATTRCAT_BLOCK ;
        entry->recId.slot = i;
        entry->next = nullptr ;

        if(head == nullptr) {
            head = entry ;
            curr = entry ;
        } else {
            curr->next = entry ;
            curr = entry ;
        }

    }

    // set the value at AttrCacheTable::attrCache[ATTRCAT_RELID]
    AttrCacheTable::attrCache[ATTRCAT_RELID] = head;

    // set up the attributes of the attribute cache similarly.
    // read slots 6-11 from attrCatBlock and initialise recId appropriately

    head = nullptr ;
    curr = nullptr ;

    for(int i=12;i<=15;i++) {
        struct AttrCacheEntry * entry = (struct AttrCacheEntry *) (malloc(sizeof(AttrCacheEntry)));

        attrCatBlock.getRecord(attrCatRecord,i);
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&entry->attrCatEntry);

        entry->recId.block = ATTRCAT_BLOCK ;
        entry->recId.slot = i;
        entry->next = nullptr ;

        if(head == nullptr) {
            head = entry ;
            curr = entry ;
        } else {
            curr->next = entry ;
            curr = entry ;
        }
    }   

    AttrCacheTable::attrCache[2] = head ;

}

OpenRelTable::~OpenRelTable() {
    // free all the memory that you allocated in the constructor

    for (int i = 0; i < MAX_OPEN; ++i) {
        if (RelCacheTable::relCache[i]) {
            free(RelCacheTable::relCache[i]);
            RelCacheTable::relCache[i] = nullptr; 
        }
    }
    
    for (int i = 0; i < MAX_OPEN; ++i) {
        struct AttrCacheEntry* head = AttrCacheTable::attrCache[i];
        
        while (head != nullptr) {
            struct AttrCacheEntry* curr = head;
            head = head->next; 
            free(curr);
        }
        AttrCacheTable::attrCache[i] = nullptr;
    }
}
