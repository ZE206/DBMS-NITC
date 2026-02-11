#include "Algebra.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <iostream>

bool isNumber(char * str) {
    int len ;
    float ignore ;

    int ret = sscanf(str, "%f %n", &ignore, &len);
    return ret == 1 && len == (int)strlen(str);
}

int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE]) {
    int srcRelId = OpenRelTable::getRelId(srcRel) ;
    if(srcRelId == E_RELNOTOPEN) {
        return srcRelId ;
    }

    AttrCatEntry attrCatEntry;
    int ret = AttrCacheTable::getAttrCatEntry(srcRelId,attr,&attrCatEntry);
    if(ret != SUCCESS) {
        return ret ;
    }

    int type =  attrCatEntry.attrType ;
    Attribute attrVal;

    if (type == NUMBER) {
        if (isNumber(strVal)) {       
            attrVal.nVal = atof(strVal);
        } else {
            return E_ATTRTYPEMISMATCH;
        }
    } else if (type == STRING) {
        strcpy(attrVal.sVal, strVal);
    }
    RelCacheTable::resetSearchIndex(srcRelId);

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId,&relCatEntry);

    printf("|");
    for (int i = 0; i < relCatEntry.numAttrs; ++i) {
        AttrCatEntry attrCatEntry;
        // get attrCatEntry at offset i using AttrCacheTable::getAttrCatEntry()
        AttrCacheTable::getAttrCatEntry(srcRelId,i,&attrCatEntry);

        printf(" %s |", attrCatEntry.attrName);
    }
    printf("\n");

    int numAttrs = relCatEntry.numAttrs ;
    HeadInfo head ;
    

    while(true) {
        RecId searchRes = BlockAccess::linearSearch(srcRelId, attr, attrVal, op);

        if (searchRes.block != -1 && searchRes.slot != -1) {
            
            Attribute record[numAttrs];
            RecBuffer buff(searchRes.block);
            buff.getRecord(record, searchRes.slot);

            printf("|");
            for (int i = 0; i < numAttrs; i++) {
                AttrCatEntry fieldAttrEntry;
                AttrCacheTable::getAttrCatEntry(srcRelId, i, &fieldAttrEntry);

                if (fieldAttrEntry.attrType == NUMBER) {
                    printf(" %f |", record[i].nVal);
                } else {
                    printf(" %s |", record[i].sVal);
                }
            }
            printf("\n");
        } else {
            break ;
        }
    }
    return SUCCESS ;
}

int Algebra::insert(char relName[ATTR_SIZE], int nAttrs, char record[][ATTR_SIZE]) {
    if(strcmp(relName,"RELATIONCAT") == 0 || strcmp(relName,"ATTRCAT") == 0) {
        return E_NOTPERMITTED ;
    }

    int relId = OpenRelTable::getRelId(relName);

    if(relId == E_RELNOTOPEN) {
        return E_RELNOTOPEN ;
    }

    RelCatEntry relCatEntry ;
    RelCacheTable::getRelCatEntry(relId,&relCatEntry);

    if(relCatEntry.numAttrs != nAttrs) {
        return E_NATTRMISMATCH ;
    }

    Attribute recordValues[nAttrs];

    for( int i = 0; i < nAttrs ; i++) {
        AttrCatEntry* attrCatEntry = (AttrCatEntry*)malloc(sizeof(AttrCatEntry));

        AttrCacheTable::getAttrCatEntry(relId,i,attrCatEntry);

        int type = attrCatEntry->attrType ;

        if(type == NUMBER) {
            if(isNumber(record[i])) {
                recordValues[i].nVal = atof(record[i]);
            } else {
                return E_ATTRTYPEMISMATCH;
            }
        } else if(type == STRING) {
            strcpy(recordValues[i].sVal,record[i]);
        }
    }
    return BlockAccess::insert(relId,recordValues);
}