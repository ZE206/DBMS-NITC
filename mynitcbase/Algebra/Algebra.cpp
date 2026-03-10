#include "Algebra.h"
#include "../BlockAccess/BlockAccess.h"
#include "../Cache/OpenRelTable.h"
#include "../Cache/RelCacheTable.h"
#include "../Cache/AttrCacheTable.h"
#include "../Buffer/BlockBuffer.h"
#include "../define/constants.h"

#include <cstdio>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include <cerrno>


bool isNumber(char *str) {
    char *endptr;
    errno = 0;

    strtod(str, &endptr);

    if (errno != 0)
        return false;

    while (*endptr == ' ' || *endptr == '\t')
        endptr++;

    return *endptr == '\0';
}
/*
 * SELECT operation
 */
int Algebra::select(char srcRel[ATTR_SIZE],
                    char targetRel[ATTR_SIZE],
                    char attr[ATTR_SIZE],
                    int op,
                    char strVal[ATTR_SIZE]) {

    int srcRelId = OpenRelTable::getRelId(srcRel);
    if (srcRelId == E_RELNOTOPEN)
        return E_RELNOTOPEN;

    AttrCatEntry attrCatEntry;
    if (AttrCacheTable::getAttrCatEntry(srcRelId, attr, &attrCatEntry) != SUCCESS)
        return E_ATTRNOTEXIST;

    Attribute attrVal;
    if (attrCatEntry.attrType == NUMBER) {
        if (!isNumber(strVal))
            return E_ATTRTYPEMISMATCH;
        attrVal.nVal = atof(strVal);
    } else {
        strcpy(attrVal.sVal, strVal);
    }

    // ✅ reset search
    RelCacheTable::resetSearchIndex(srcRelId);

    // ✅ FETCH relCatEntry ONCE (THIS FIXES YOUR ERROR)
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);

    // ✅ print header
    printf("|");
    for (int i = 0; i < relCatEntry.numAttrs; i++) {
        AttrCatEntry col;
        AttrCacheTable::getAttrCatEntry(srcRelId, i, &col);
        printf(" %s |", col.attrName);
    }
    printf("\n");
 //////////////
   
    // ✅ print tuples
    while (true) {

        RecId searchRes =
            BlockAccess::linearSearch(srcRelId, attr, attrVal, op);

        if (searchRes.block == -1 && searchRes.slot == -1)
            break;

        RecBuffer recBuf(searchRes.block);

        Attribute record[relCatEntry.numAttrs];
        recBuf.getRecord(record, searchRes.slot);

        printf("|");
        for (int i = 0; i < relCatEntry.numAttrs; i++) {
            AttrCatEntry col;
            AttrCacheTable::getAttrCatEntry(srcRelId, i, &col);

            if (col.attrType == NUMBER)
                printf(" %g |", record[i].nVal);
            else
                printf(" %s |", record[i].sVal);
        }
        printf("\n");
    }

    return SUCCESS;
}








