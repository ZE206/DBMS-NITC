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
int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE])
{
    int srcRelId = OpenRelTable::getRelId(srcRel);
    if (srcRelId == E_RELNOTOPEN)
    {
        return E_RELNOTOPEN;
    }

    AttrCatEntry attrCatEntry;
    int attrCat = AttrCacheTable::getAttrCatEntry(srcRelId, attr, &attrCatEntry);
    if (attrCat != SUCCESS)
    {
        return E_ATTRNOTEXIST;
    }

    int type = attrCatEntry.attrType;
    Attribute attrVal;
    if (type == NUMBER)
    {
        bool isNumber(char *);
        if (isNumber(strVal))
        {
            attrVal.nVal = atof(strVal);
        }
        else
        {
            return E_ATTRTYPEMISMATCH;
        }
    }
    else if (type == STRING)
    {
        strcpy(attrVal.sVal, strVal);
    }

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);
    int src_nAttrs = relCatEntry.numAttrs;

    char attr_names[src_nAttrs][ATTR_SIZE];
    int attr_types[src_nAttrs];

    for (int i = 0; i < src_nAttrs; i++)
    {
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);
        strcpy(attr_names[i], attrCatEntry.attrName);
        attr_types[i] = attrCatEntry.attrType;
    }

    int ret = Schema::createRel(targetRel, src_nAttrs, attr_names, attr_types);
    if (ret < 0)
    {
        return ret;
    }

    int targetRelId = OpenRelTable::openRel(targetRel);
    if (targetRelId < 0)
    {
        Schema::deleteRel(targetRel);
        return targetRelId;
    }

    Attribute record[src_nAttrs];
    RelCacheTable::resetSearchIndex(srcRelId);
    AttrCacheTable::resetSearchIndex(srcRelId, attr);
    g_comparisonCount = 0;

    while (BlockAccess::search(srcRelId, record, attr, attrVal, op) == SUCCESS)
    {
        ret = BlockAccess::insert(targetRelId, record);

        if (ret != SUCCESS)
        {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
    }
    printf("Number of comparisons: %d\n", g_comparisonCount);
    Schema::closeRel(targetRel);
    return SUCCESS;
}
bool isNumber(char *str)
{
    char *endptr;
    errno = 0;

    strtod(str, &endptr);

    if (errno != 0)
        return false;

    while (*endptr == ' ' || *endptr == '\t')
        endptr++;

    return *endptr == '\0';
}
int Algebra::insert(char relName[ATTR_SIZE], int nAttrs, char record[][ATTR_SIZE])
{

    if (strcmp(relName, "RELATIONCAT") == 0 || strcmp(relName, "ATTRIBUTECAT") == 0)
        return E_NOTPERMITTED;

    int relId = OpenRelTable::getRelId(relName);

    if (relId == E_RELNOTOPEN)
        return E_RELNOTOPEN;

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);

    if (relCatEntry.numAttrs != nAttrs)
    {
        return E_NATTRMISMATCH;
    }

    union Attribute recordValues[nAttrs];

    for (int i = 0; i < nAttrs; i++)
    {

        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(relId, i, &attrCatEntry);

        int type = attrCatEntry.attrType;

        if (type == NUMBER)
        {
            if (isNumber(record[i]))
            {
                recordValues[i].nVal = atof(record[i]);
            }
            else
            {
                return E_ATTRTYPEMISMATCH;
            }
        }
        else if (type == STRING)
        {
            strcpy(recordValues[i].sVal, record[i]);
        }
    }

    int retVal = BlockAccess::insert(relId, recordValues);

    return retVal;
}

int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE])
{
    int srcRelId = OpenRelTable::getRelId(srcRel);
    if (srcRelId == E_RELNOTOPEN)
    {
        return E_RELNOTOPEN;
    }

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);
    int nAttrs = relCatEntry.numAttrs;

    char attrNames[nAttrs][ATTR_SIZE];
    int attrTypes[nAttrs];
    for (int i = 0; i < nAttrs; i++)
    {
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);
        strcpy(attrNames[i], attrCatEntry.attrName);
        attrTypes[i] = attrCatEntry.attrType;
    }

    int ret = Schema::createRel(targetRel, nAttrs, attrNames, attrTypes);
    if (ret < 0)
    {
        return ret;
    }
    int targetRelId = OpenRelTable::openRel(targetRel);
    if (targetRelId < 0)
    {
        Schema::deleteRel(targetRel);
        return targetRelId;
    }

    RelCacheTable::resetSearchIndex(srcRelId);
    Attribute record[nAttrs];

    while (BlockAccess::project(srcRelId, record) == SUCCESS)
    {
        ret = BlockAccess::insert(targetRelId, record);
        if (ret != SUCCESS)
        {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
    }
    Schema::closeRel(targetRel);

    return SUCCESS;
}
int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], int tar_nAttrs, char tar_Attrs[][ATTR_SIZE])
{

    int srcRelId = OpenRelTable::getRelId(srcRel);
    if (srcRelId == E_RELNOTOPEN)
    {
        return E_RELNOTOPEN;
    }

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);
    int src_nAttrs = relCatEntry.numAttrs;

    int attr_offset[tar_nAttrs];
    int attr_types[tar_nAttrs];

    for (int i = 0; i < tar_nAttrs; i++)
    {
        AttrCatEntry attrCatEntry;
        if (AttrCacheTable::getAttrCatEntry(srcRelId, tar_Attrs[i], &attrCatEntry) != SUCCESS)
        {
            return E_ATTRNOTEXIST;
        }
        attr_offset[i] = attrCatEntry.offset;
        attr_types[i] = attrCatEntry.attrType;
    }

    int ret = Schema::createRel(targetRel, tar_nAttrs, tar_Attrs, attr_types);
    if (ret < 0)
    {
        return ret;
    }
    int targetRelId = OpenRelTable::openRel(targetRel);
    if (targetRelId < 0)
    {
        Schema::deleteRel(targetRel);
        return targetRelId;
    }
    RelCacheTable::resetSearchIndex(srcRelId);
    Attribute record[src_nAttrs];

    while (BlockAccess::project(srcRelId, record) == SUCCESS)
    {
        Attribute proj_record[tar_nAttrs];
        for (int i = 0; i < tar_nAttrs; i++)
        {
            proj_record[i] = record[attr_offset[i]];
        }

        ret = BlockAccess::insert(targetRelId, proj_record);
        if (ret != SUCCESS)
        {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
    }
    Schema::closeRel(targetRel);

    return SUCCESS;
}
int Algebra::join(char srcRelation1[ATTR_SIZE], char srcRelation2[ATTR_SIZE], char targetRelation[ATTR_SIZE], char attribute1[ATTR_SIZE], char attribute2[ATTR_SIZE])
{

    // get relation1's and relation2's relId
    int relId1 = OpenRelTable::getRelId(srcRelation1);
    int relId2 = OpenRelTable::getRelId(srcRelation2);

    if (relId1 < 0 || relId1 >= MAX_OPEN || relId2 < 0 || relId2 >= MAX_OPEN)
    {
        return E_RELNOTOPEN;
    }

    // get attribute catalog entries for the source relations corresponding to their give attributes.
    AttrCatEntry attrCatEntry1, attrCatEntry2;
    if (AttrCacheTable::getAttrCatEntry(relId1, attribute1, &attrCatEntry1) == E_ATTRNOTEXIST)
    {
        return E_ATTRNOTEXIST;
    }
    if (AttrCacheTable::getAttrCatEntry(relId2, attribute2, &attrCatEntry2) == E_ATTRNOTEXIST)
    {
        return E_ATTRNOTEXIST;
    }

    // if attribute1 and attribute2 are of different types, return error
    if (attrCatEntry1.attrType != attrCatEntry2.attrType)
    {
        return E_ATTRTYPEMISMATCH;
    }

    /*
        iterate through all the attributes in both the source relation and check if there are
        any other pair of attributes other than join attributes

        if yes, then return error
    */
    RelCatEntry relCatEntry1, relCatEntry2;
    RelCacheTable::getRelCatEntry(relId1, &relCatEntry1);
    RelCacheTable::getRelCatEntry(relId2, &relCatEntry2);

    AttrCatEntry temp1, temp2;
    for (int j = 0; j < relCatEntry2.numAttrs; j++)
    {
        if (j == attrCatEntry2.offset)
        {
            continue;
        }

        AttrCacheTable::getAttrCatEntry(relId2, j, &temp2);

        for (int i = 0; i < relCatEntry1.numAttrs; i++)
        {
            AttrCacheTable::getAttrCatEntry(relId1, i, &temp1);

            if (strcmp(temp2.attrName, temp1.attrName) == 0)
            {

                return E_DUPLICATEATTR;
            }
        }
    }

    int numOfAttributes1 = relCatEntry1.numAttrs;
    int numOfAttributes2 = relCatEntry2.numAttrs;

    /*
        If srcRelation2 doesn't have an index on attribute2, create index

        This will reduce time complexity from O(m.n) -> O(mlogn + n)
        where m = no. of records in relation_1, n = no. of records in relation_2
    */
    if (attrCatEntry2.rootBlock == -1)
    {
        int ret = BPlusTree::bPlusCreate(relId2, attribute2);
        if (ret != SUCCESS)
        {
            return E_DISKFULL;
        }
    }

    int numOfAttributesInTarget = numOfAttributes1 + numOfAttributes2 - 1;

    // declaring the following arrays to store the details of the target relation
    char targetRelAttrNames[numOfAttributesInTarget][ATTR_SIZE];
    int targetRelAttrTypes[numOfAttributesInTarget];

    /*
        Iterate through all the attributes in both the source relations and update
        targetRelAttrNames[], targetRelAttrTypes[] arrays excluding attribute2
    */
    for (int i = 0; i < numOfAttributes1; i++)
    {
        AttrCacheTable::getAttrCatEntry(relId1, i, &temp1);
        strcpy(targetRelAttrNames[i], temp1.attrName);
        targetRelAttrTypes[i] = temp1.attrType;
    }
    for (int j = 0; j < attrCatEntry2.offset; j++)
    {
        AttrCacheTable::getAttrCatEntry(relId2, j, &temp2);
        strcpy(targetRelAttrNames[numOfAttributes1 + j], temp2.attrName);
        targetRelAttrTypes[numOfAttributes1 + j] = temp2.attrType;
    }
    for (int j = attrCatEntry2.offset + 1; j < numOfAttributes2; j++)
    {
        AttrCacheTable::getAttrCatEntry(relId2, j, &temp2);
        strcpy(targetRelAttrNames[numOfAttributes1 + j - 1], temp2.attrName);
        targetRelAttrTypes[numOfAttributes1 + j - 1] = temp2.attrType;
    }

    // create the target relation
    int ret = Schema::createRel(targetRelation, numOfAttributesInTarget, targetRelAttrNames, targetRelAttrTypes);
    if (ret != SUCCESS)
    {
        return ret;
    }

    // open and get targetRelation id
    int targetRelId = OpenRelTable::openRel(targetRelation);
    if (targetRelId < 0 || targetRelId >= MAX_OPEN)
    {
        // delete the relation
        Schema::deleteRel(targetRelation);
        return targetRelId;
    }

    Attribute record1[numOfAttributes1];
    Attribute record2[numOfAttributes2];
    Attribute targetRecord[numOfAttributesInTarget];

    RelCacheTable::resetSearchIndex(relId1);

    // outer while loop to get all the records from srcRelation1 one by one
    while (BlockAccess::project(relId1, record1) == SUCCESS)
    {

        // reset search index for srcRelation2, cause for every record in srcRelation1, we start from beginning
        RelCacheTable::resetSearchIndex(relId2);

        // reset search index for attribute2, cause we want record with whose attribute2 matches with attribute1
        AttrCacheTable::resetSearchIndex(relId2, attribute2);

        /*
            inner loop will get every record of srcRelation2 which satisfy the following condition:
            record1.attribute1 = record2.attribute2
        */
        while (BlockAccess::search(relId2, record2, attribute2, record1[attrCatEntry1.offset], EQ) == SUCCESS)
        {
            /*
                copy srcRelation1's anf srcRelation2's attribute values (except for attribute2 in rel2) from record1
                and record2 to targetRecord
            */
            for (int i = 0; i < numOfAttributes1; i++)
            {
                targetRecord[i] = record1[i];
            }
            for (int i = 0; i < attrCatEntry2.offset; i++)
            {
                targetRecord[numOfAttributes1 + i] = record2[i];
            }
            for (int i = attrCatEntry2.offset + 1; i < numOfAttributes2; i++)
            {
                targetRecord[numOfAttributes1 + i - 1] = record2[i];
            }

            // insert the currnt record into the target relation
            ret = BlockAccess::insert(targetRelId, targetRecord);
            if (ret != SUCCESS)
            {
                OpenRelTable::closeRel(targetRelId);
                Schema::deleteRel(targetRelation);
                return E_DISKFULL;
            }
        }
    }

    OpenRelTable::closeRel(targetRelId);
    return SUCCESS;
}
