#include "Schema.h"
#include "../Cache/OpenRelTable.h"
#include <cmath>
#include <cstring>
#include "../define/constants.h"


int Schema::openRel(char relName[ATTR_SIZE]){
    int ret=OpenRelTable::openRel(relName);
    if(ret>0 || ret<=MAX_OPEN){
        return SUCCESS;
    }
    return  ret;
}

int Schema::closeRel(char relName[ATTR_SIZE]) {
    if(strcmp(relName,RELCAT_RELNAME) == 0 || strcmp(relName,ATTRCAT_RELNAME) == 0) {
        return E_NOTPERMITTED ;
    }

    int relId = OpenRelTable::getRelId(relName);

    if(relId == E_RELNOTOPEN) {
        return E_RELNOTOPEN ;
    }

    return OpenRelTable::closeRel(relId);
}

int Schema::renameRel(char oldRelName[ATTR_SIZE], char newRelName[ATTR_SIZE]){
    if(strcmp(oldRelName, "RELATIONCAT")==0 || strcmp(newRelName,"ATTRIBUTECAT")==0){
        return E_NOTPERMITTED;
    }
    
    int relId=OpenRelTable::getRelId(relName);

    if(relId==E_RELNOTOPEN){
        return E_RELNOTOPEN;
    }

    retVal=BlockAccess::renameRelation(oldRelName,newRelName);
    return retVal;
}

int schema::renameAttr(char *relName, char *oldAttrName, char *newAttrName){
    if(strcmp(oldRelName, "RELATIONCAT")==0 || strcmp(newRelName,"ATTRIBUTECAT")==0){
        return E_NOTPERMITTED;
    }
    
    int relId=OpenRelTable::getRelId(relName);

    if(relId==E_RELNOTOPEN){
        return E_RELNOTOPEN;
    }

    retVal=BlockAccess::renameAttribute(oldAttrName, newAttrName);
    return retVal;
}