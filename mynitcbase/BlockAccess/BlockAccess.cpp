#include "BlockAccess.h"
#include  "../define/constants.h"
#include <cstring>
#include "../Cache/RelCacheTable.cpp"
#include "../"
RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op){
    RecId* prevSearchIndex;
    RelCacheTable::getSearchIndex(RelCacheTable::relCache[RELCAT_RELID],prevSearchIndex);
    RecId* searchIndex;

    RelCacheTable::getSearchIndex(relId, &searchIndex);

    if(prevRecId.block==-1 && prevRecId.slot==-1){
        
    }

    
}   