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

int Schema::renameRel(char oldrelName[ATTR_SIZE], char newrelName[ATTR_SIZE]){
    if (!strcmp(oldrelName, RELCAT_RELNAME) || !strcmp(newrelName, RELCAT_RELNAME) || !strcmp(oldrelName, ATTRCAT_RELNAME) || !strcmp(newrelName, ATTRCAT_RELNAME)){
        return E_NOTPERMITTED;
    }

    int ret = OpenRelTable::getRelId(oldrelName);

    if (ret != E_RELNOTOPEN)
        return E_RELOPEN;

    int retVal = BlockAccess::renameRelation(oldrelName, newrelName);
    return retVal;
}

int Schema::renameAttr(char* relName, char* oldAttrName, char* newAttrName){
    if (!strcmp(oldAttrName, RELCAT_RELNAME) || !strcmp(newAttrName, ATTRCAT_RELNAME) || !strcmp(oldAttrName, RELCAT_RELNAME) || !strcmp(newAttrName, ATTRCAT_RELNAME)){
        return E_NOTPERMITTED;
    }

    int ret = OpenRelTable::getRelId(oldAttrName);

    if (ret != E_RELNOTOPEN)
        return E_RELOPEN;
    
    int retVal = BlockAccess::renameAttribute(relName, oldAttrName, newAttrName);
    return retVal;
}

//Mistakes:
//1. incorrect arguments given in Schema::renameRelation() in strcmp