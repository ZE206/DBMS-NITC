#include "OpenRelTable.h"
#include "../Buffer/BlockBuffer.h"
#include "../Cache/RelCacheTable.h"
#include "../Cache/AttrCacheTable.h"
#include "../define/constants.h"
#include <cstdlib>
#include <cstring>

// initialising Caches
OpenRelTable::OpenRelTable() {

  for (int i = 0; i < MAX_OPEN; i++) {
    RelCacheTable::relCache[i] = nullptr;
    AttrCacheTable::attrCache[i] = nullptr;
  }

// for Relation Record in Relation Catalog Block
  RecBuffer relCatBlock(RELCAT_BLOCK);


  Attribute relCatRecord[RELCAT_NO_ATTRS];
  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);

  RelCacheEntry relCatCacheEntry;
  RelCacheTable::recordToRelCatEntry(relCatRecord,
                                    &relCatCacheEntry.relCatEntry);
  relCatCacheEntry.recId.block = RELCAT_BLOCK;
  relCatCacheEntry.recId.slot  = RELCAT_SLOTNUM_FOR_RELCAT;
  relCatCacheEntry.dirty = false;
  relCatCacheEntry.searchIndex = {-1, -1};

  RelCacheTable::relCache[RELCAT_RELID] =
      (RelCacheEntry*) malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[RELCAT_RELID]) = relCatCacheEntry;

// for Attribute Records in Relation Catalog Block
  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);

  RelCacheEntry attrCatCacheEntry;
  RelCacheTable::recordToRelCatEntry(relCatRecord,
                                    &attrCatCacheEntry.relCatEntry);
  attrCatCacheEntry.recId.block = RELCAT_BLOCK;
  attrCatCacheEntry.recId.slot  = RELCAT_SLOTNUM_FOR_ATTRCAT;
  attrCatCacheEntry.dirty = false;
  attrCatCacheEntry.searchIndex = {-1, -1};

  RelCacheTable::relCache[ATTRCAT_RELID] =
      (RelCacheEntry*) malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[ATTRCAT_RELID]) = attrCatCacheEntry;

// for Students Records in Relation Catalog Block
  const int STUDENT_RELID = 2;

    RecBuffer relcatBuffer(RELCAT_BLOCK);
    HeadInfo relHead;
    relcatBuffer.getHeader(&relHead);

    for(int i = 0; i < relHead.numEntries; i++) {
        Attribute record[RELCAT_NO_ATTRS];
        relcatBuffer.getRecord(record, i);

        if(strcmp(record[RELCAT_REL_NAME_INDEX].sVal, "Students") == 0) {
            RelCacheEntry *entry =
                (RelCacheEntry*)malloc(sizeof(RelCacheEntry));

            RelCacheTable::recordToRelCatEntry(
                record, &entry->relCatEntry);

            entry->recId.block = RELCAT_BLOCK;
            entry->recId.slot = i;
            entry->dirty = false;

            RelCacheTable::relCache[STUDENT_RELID] = entry;
            break;
        }
    }

// Now Moving to attribute catalog for attributes
  RecBuffer attrCatBlock(ATTRCAT_BLOCK);
  Attribute attrCatRecord[ATTRCAT_NO_ATTRS];


  AttrCacheEntry *head = nullptr;
  AttrCacheEntry *prev = nullptr;

  for (int i = 0; i < RELCAT_NO_ATTRS; i++) {

    attrCatBlock.getRecord(attrCatRecord, i);

    AttrCacheEntry *entry =
        (AttrCacheEntry*) malloc(sizeof(AttrCacheEntry));

    AttrCacheTable::recordToAttrCatEntry(attrCatRecord,
                                         &entry->attrCatEntry);
    entry->recId.block = ATTRCAT_BLOCK;
    entry->recId.slot  = i;
    entry->dirty = false;
    entry->searchIndex = {-1, -1};
    entry->next = nullptr;

    if (head == nullptr)
      head = entry;
    else
      prev->next = entry;

    prev = entry;
  }

  AttrCacheTable::attrCache[RELCAT_RELID] = head;


  head = nullptr;
  prev = nullptr;

  for (int i = 0; i < ATTRCAT_NO_ATTRS; i++) {

    attrCatBlock.getRecord(attrCatRecord,
                           i + RELCAT_NO_ATTRS);

    AttrCacheEntry *entry =
        (AttrCacheEntry*) malloc(sizeof(AttrCacheEntry));

    AttrCacheTable::recordToAttrCatEntry(attrCatRecord,
                                         &entry->attrCatEntry);
    entry->recId.block = ATTRCAT_BLOCK;
    entry->recId.slot  = i + RELCAT_NO_ATTRS;
    entry->dirty = false;
    entry->searchIndex = {-1, -1};
    entry->next = nullptr;

    if (head == nullptr)
      head = entry;
    else
      prev->next = entry;

    prev = entry;
  }

  AttrCacheTable::attrCache[ATTRCAT_RELID] = head;

    


    AttrCacheEntry *student_head = nullptr, *student_prev = nullptr;
    int block = ATTRCAT_BLOCK;

    while(block != -1) {
        RecBuffer buf(block);
        HeadInfo h;
        buf.getHeader(&h);

        for(int i = 0; i < h.numEntries; i++) {
            Attribute record[ATTRCAT_NO_ATTRS];
            buf.getRecord(record, i);

            if(strcmp(record[ATTRCAT_REL_NAME_INDEX].sVal, "Students") == 0) {
                AttrCacheEntry *e =
                    (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));

                AttrCacheTable::recordToAttrCatEntry(
                    record, &e->attrCatEntry);

                e->recId.block = block;
                e->recId.slot = i;
                e->dirty = false;
                e->next = nullptr;

                if(!student_head) student_head = e;
                else student_prev->next = e;
                student_prev = e;
            }
        }
        block = h.rblock;
    }

    AttrCacheTable::attrCache[STUDENT_RELID] = student_head;

    

}


OpenRelTable::~OpenRelTable() {

  for (int i = 0; i < MAX_OPEN; i++) {

    if (RelCacheTable::relCache[i] != nullptr) {
      free(RelCacheTable::relCache[i]);
      RelCacheTable::relCache[i] = nullptr;
    }

    AttrCacheEntry *curr = AttrCacheTable::attrCache[i];
    while (curr != nullptr) {
      AttrCacheEntry *next = curr->next;
      free(curr);
      curr = next;
    }
    AttrCacheTable::attrCache[i] = nullptr;
  }
}