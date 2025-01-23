#include "Schema.h"

#include <cmath>
#include <cstring>

int Schema::openRel(char rel[ATTR_SIZE]){
    int ret = OpenRelTable::openRel(rel);
    
    return ret >=0 && ret < MAX_OPEN ? SUCCESS : ret;
}

int Schema::closeRel(char rel[ATTR_SIZE]){
    if (!strcmp(rel, "RELATIONCAT") || !strcmp(rel, "ATTRIBUTECAT")){
        return E_NOTPERMITTED;
    }
    int relId = OpenRelTable::getRelId(rel);
    if (relId < 0 || relId >= MAX_OPEN){
        return E_RELNOTOPEN;
    }
    return OpenRelTable::closeRel(relId);
}