#include "Schema.h"

#include <cmath>
#include <cstring>
#include <cstdio>

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

int Schema::createRel(char relName[], int nAttrs, char attrs[][ATTR_SIZE], int attrType[]){

    Attribute relNameAsAttribute;
    strcpy(relNameAsAttribute.sVal, relName);
    printf("Inside Schema::createRel\n");
    RecId targetRelId = {-1, -1};
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    printf("before BlockAccess::linearSearch()\n");
    targetRelId = BlockAccess::linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, relNameAsAttribute, EQ);
    printf("after linearSearch()\n");
    if (targetRelId.block != -1 && targetRelId.slot != -1){
        return E_RELEXIST;
    }

    for (int i = 0; i < nAttrs; ++i){
        for (int j = i+1; j < nAttrs; ++j){
            if (!strcmp(attrs[i], attrs[j])){
                return E_DUPLICATEATTR;
            }
        }
    }
    printf("before initializing reCatRecord\n");
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    strcpy(relCatRecord[RELCAT_REL_NAME_INDEX].sVal, relName);
    relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal = nAttrs;
    relCatRecord[RELCAT_NO_RECORDS_INDEX].nVal = 0;
    relCatRecord[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal = floor(2016/((16 * nAttrs) + 1));
    relCatRecord[RELCAT_FIRST_BLOCK_INDEX].nVal = -1;
    relCatRecord[RELCAT_LAST_BLOCK_INDEX].nVal = -1;
    printf("before BlockAccess::insert()\n");
    int retVal = BlockAccess::insert(RELCAT_RELID, relCatRecord);
    printf("after BlockAccess::insert()\n");
    if (retVal != SUCCESS){
        return retVal;
    }
    printf("before for loop of Schema::createRel\n");
    for (int i = 0; i < nAttrs; ++i){
        printf("inside %d th iteration of for loop\n", i);
        Attribute attrCatRecord[6];
        strcpy(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relName);
        strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrs[i]);
        attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal = attrType[i];
        attrCatRecord[ATTRCAT_PRIMARY_FLAG_INDEX].nVal = -1;
        attrCatRecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal = -1;
        attrCatRecord[ATTRCAT_OFFSET_INDEX].nVal = i;
        printf("before insert() in for loop of createRel\n");
        int ret = BlockAccess::insert(ATTRCAT_RELID, attrCatRecord);
        printf("after insert(); working\n");
        if (ret != SUCCESS){
            Schema::deleteRel(relName);
            printf("Schema::deleteRel working\n");
            return E_DISKFULL;
        }
    }

    return SUCCESS;
}

int Schema::deleteRel(char* relName){
    if (!strcmp(relName, RELCAT_RELNAME) || !strcmp(relName, ATTRCAT_RELNAME)){
        return E_NOTPERMITTED;
    }
    printf("inside deleteRel with relName is %s\n", relName);
    int relId = OpenRelTable::getRelId(relName);

    if (relId != E_RELNOTOPEN){
        return E_RELOPEN;
    }

    int ret = BlockAccess::deleteRelation(relName);
    printf("BlockAccess::deleteRelation() working.\n");
    if (ret != SUCCESS){
        printf("Some problems.\n");
        return ret;
    }

    return ret;
}