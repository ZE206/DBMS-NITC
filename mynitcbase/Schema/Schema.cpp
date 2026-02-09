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