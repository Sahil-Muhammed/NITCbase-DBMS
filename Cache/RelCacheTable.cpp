#include "RelCacheTable.h"

#include <cstring>
#include <cstdio>

RelCacheEntry* RelCacheTable::relCache[MAX_OPEN];

/*
Get the relation catalog entry for the relation with rel-id `relId` from the cache
NOTE: this function expects the caller to allocate memory for `*relCatBuf`
*/

int RelCacheTable::getRelCatEntry(int relId, RelCatEntry* relCatBuf){
    //check whether relation ID is out of bounds
    if (relId < 0 || relId > MAX_OPEN){
        printf("out of bound\n");
        return E_OUTOFBOUND;
    }

    //check whether the corresponding relation catalog entry in relation cache is empty
    if (relCache[relId] == nullptr){
        printf("relation not open.\n");
        return E_RELNOTOPEN;
    }

    //copy the entry from cache to buffer
    *relCatBuf = relCache[relId]->relCatEntry;
    return SUCCESS;
}

/* Converts a relation catalog record to RelCatEntry struct
    We get the record as Attribute[] from the BlockBuffer.getRecord() function.
    This function will convert that to a struct RelCatEntry type.
NOTE: this function expects the caller to allocate memory for `*relCatEntry`
*/

void RelCacheTable::recordToRelCatEntry(union Attribute record[RELCAT_NO_ATTRS], RelCatEntry* relCatEntry){
    strcpy(relCatEntry->relName, record[RELCAT_REL_NAME_INDEX].sVal);
    relCatEntry->numAttrs = (int)record[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    relCatEntry->numRecs = (int)record[RELCAT_NO_RECORDS_INDEX].nVal;
    relCatEntry->firstBlk = (int)record[RELCAT_FIRST_BLOCK_INDEX].nVal;
    relCatEntry->lastBlk = (int)record[RELCAT_LAST_BLOCK_INDEX].nVal;
    relCatEntry->numSlotsPerBlk = (int)record[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal;
}

int RelCacheTable::getSearchIndex(int relId, RecId* searchIndex){
    if (relId < 0 || relId >= MAX_OPEN){
        return E_OUTOFBOUND;
    }
    if (relCache[relId] == nullptr)
        return E_RELNOTOPEN;
    *searchIndex = relCache[relId]->searchIndex;
    return SUCCESS;
}

int RelCacheTable::setSearchIndex(int relId, RecId* searchIndex){
    if (relId < 0 || relId >= MAX_OPEN){
        return E_OUTOFBOUND;
    }
    if (relCache[relId] == nullptr)
        return E_RELNOTOPEN;
    relCache[relId]->searchIndex = *searchIndex;
    return SUCCESS;
}

int RelCacheTable::resetSearchIndex(int relId){
    if (relId < 0 || relId >= MAX_OPEN){
        return E_OUTOFBOUND;
    }
    if (relCache[relId] == nullptr)
        return E_RELNOTOPEN;
    RecId searchIndex;
    searchIndex.block = -1;
    searchIndex.slot = -1;
    RelCacheTable::setSearchIndex(relId, &searchIndex);
    return SUCCESS;
}

int RelCacheTable::setRelCatEntry(int relId, RelCatEntry* relCatBuf){

    if (relId < 0 || relId >= MAX_OPEN){
        return E_OUTOFBOUND;
    }

    if (relCache[relId] == nullptr){
        return E_RELNOTOPEN;
    }

    memcpy(&(relCache[relId]->relCatEntry), relCatBuf, sizeof(RelCatEntry));

    relCache[relId]->dirty = true;

    return SUCCESS;
}

void RelCacheTable::relCatEntryToRecord(RelCatEntry* relCatEntry, union Attribute record[RELCAT_NO_ATTRS]){
    strcpy(record[RELCAT_REL_NAME_INDEX].sVal, relCatEntry->relName);
    record[RELCAT_NO_ATTRIBUTES_INDEX].nVal = relCatEntry->numAttrs;
    record[RELCAT_NO_RECORDS_INDEX].nVal = relCatEntry->numRecs;
    record[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal = relCatEntry->numSlotsPerBlk;
    record[RELCAT_FIRST_BLOCK_INDEX].nVal = relCatEntry->firstBlk;
    record[RELCAT_LAST_BLOCK_INDEX].nVal = relCatEntry->lastBlk;
}