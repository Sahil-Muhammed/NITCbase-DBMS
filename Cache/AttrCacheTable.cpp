#include "AttrCacheTable.h"

#include <cstring>

AttrCacheEntry *AttrCacheTable::attrCache[MAX_OPEN];

int AttrCacheTable::getAttrCatEntry(int relId, int attrOffset, AttrCatEntry *attrCatBuf)
{
    if (relId < 0 || relId >= MAX_OPEN)
    {
        return E_OUTOFBOUND;
    }

    if (attrCache[relId] == nullptr)
    {
        return E_RELNOTOPEN;
    }

    for (AttrCacheEntry *entry = attrCache[relId]; entry != nullptr; entry = entry->next)
    {
        if (entry->attrCatEntry.offset == attrOffset)
        {
            *attrCatBuf = (entry->attrCatEntry);
            return SUCCESS;
        }
    }
    return E_ATTRNOTEXIST;
}

void AttrCacheTable::recordToAttrCatEntry(union Attribute record[ATTRCAT_NO_ATTRS],
                                          AttrCatEntry *attrCatEntry)
{
    strcpy(attrCatEntry->relName, record[ATTRCAT_REL_NAME_INDEX].sVal);
    attrCatEntry->primaryFlag = record[ATTRCAT_PRIMARY_FLAG_INDEX].nVal;
    strcpy(attrCatEntry->attrName, record[ATTRCAT_ATTR_NAME_INDEX].sVal);
    attrCatEntry->attrType = record[ATTRCAT_ATTR_TYPE_INDEX].nVal;
    attrCatEntry->rootBlock = record[ATTRCAT_ROOT_BLOCK_INDEX].nVal;
    attrCatEntry->offset = record[ATTRCAT_OFFSET_INDEX].nVal;
}

void AttrCacheTable::attrCatEntryToRecord(AttrCatEntry* attrCatEntry, union Attribute record[ATTRCAT_NO_ATTRS]){
    strcpy(record[ATTRCAT_REL_NAME_INDEX].sVal, attrCatEntry->relName);
    record[ATTRCAT_ATTR_TYPE_INDEX].nVal = attrCatEntry->attrType;
    strcpy(record[ATTRCAT_ATTR_NAME_INDEX].sVal, attrCatEntry->attrName);
    record[ATTRCAT_OFFSET_INDEX].nVal = attrCatEntry->offset;
    record[ATTRCAT_PRIMARY_FLAG_INDEX].nVal = attrCatEntry->offset;
    record[ATTRCAT_ROOT_BLOCK_INDEX].nVal = attrCatEntry->rootBlock;
}

int AttrCacheTable::getAttrCatEntry(int relId, char attrName[ATTR_SIZE], AttrCatEntry *attrCatBuffer)
{
    if (relId < 0 || relId >= MAX_OPEN)
    {
        return E_OUTOFBOUND;
    }

    if (attrCache[relId] == nullptr)
    {
        return E_RELNOTOPEN;
    }
    for (AttrCacheEntry *entry = attrCache[relId]; entry != nullptr; entry = entry->next)
    {
        if (!strcmp(attrName, entry->attrCatEntry.attrName))
        {
            *attrCatBuffer = (entry->attrCatEntry);
            return SUCCESS;
        }
    }
    return E_ATTRNOTEXIST;
}

int AttrCacheTable::getSearchIndex(int relId, char attrName[ATTR_SIZE], IndexId* searchIndex){
    if (relId < 0 || relId >= MAX_OPEN){
        return E_OUTOFBOUND;
    }

    if (attrCache[relId] == nullptr){
        return E_RELNOTOPEN;
    }

    AttrCacheEntry* attrCacheEntry = attrCache[relId];
    for (; attrCacheEntry != NULL; attrCacheEntry = attrCacheEntry->next){
        if (strcmp(attrCacheEntry->attrCatEntry.attrName, attrName) == 0){

            *searchIndex = attrCacheEntry->searchIndex;
            
            return SUCCESS;
        }
    }

    return E_ATTRNOTEXIST;
}

int AttrCacheTable::getSearchIndex(int relId, int attrOffset, IndexId* searchIndex){
    if (relId < 0 || relId >= MAX_OPEN){
        return E_OUTOFBOUND;
    }

    if (attrCache[relId] == nullptr){
        return E_RELNOTOPEN;
    }

    AttrCacheEntry* attrCacheEntry = attrCache[relId];
    for (; attrCacheEntry != NULL; attrCacheEntry = attrCacheEntry->next){
        if (attrCacheEntry->attrCatEntry.offset == attrOffset){

            *searchIndex = attrCacheEntry->searchIndex;
            
            return SUCCESS;
        }
    }

    return E_ATTRNOTEXIST;
}

int AttrCacheTable::setSearchIndex(int relId, char attrName[ATTR_SIZE], IndexId* searchIndex){
    if (relId < 0 || relId >= MAX_OPEN){
        return E_OUTOFBOUND;
    }

    if (attrCache[relId] == nullptr){
        return E_RELNOTOPEN;
    }

    AttrCacheEntry* attrCacheEntry = attrCache[relId];
    for (; attrCacheEntry != NULL; attrCacheEntry = attrCacheEntry->next){
        if (strcmp(attrCacheEntry->attrCatEntry.attrName, attrName) == 0){

            attrCacheEntry->searchIndex = *searchIndex;
            
            return SUCCESS;
        }
    }

    return E_ATTRNOTEXIST;
}

int AttrCacheTable::setSearchIndex(int relId, int attrOffset, IndexId* searchIndex){
    if (relId < 0 || relId >= MAX_OPEN){
        return E_OUTOFBOUND;
    }

    if (attrCache[relId] == nullptr){
        return E_RELNOTOPEN;
    }

    AttrCacheEntry* attrCacheEntry = AttrCacheTable::attrCache[relId];
    for (; attrCacheEntry != NULL; attrCacheEntry = attrCacheEntry->next){
        if (attrCacheEntry->attrCatEntry.offset == attrOffset){

            attrCacheEntry->searchIndex = *searchIndex;
            
            return SUCCESS;
        }
    }

    return E_ATTRNOTEXIST;
}

int AttrCacheTable::resetSearchIndex(int relId, char attrName[ATTR_SIZE]){
    IndexId indexid = {-1, -1};
    int res = setSearchIndex(relId, attrName, &indexid);
    return res;
}

int AttrCacheTable::resetSearchIndex(int relId, int attrOffset){
    IndexId indexid = {-1, -1};
    int res = setSearchIndex(relId, attrOffset, &indexid);
    return res;
}

int AttrCacheTable::setAttrCatEntry(int relId, char attrName[ATTR_SIZE], AttrCatEntry* attrCatEntry){
    if (relId < 0 || relId >= MAX_OPEN){
        return E_OUTOFBOUND;
    }

    if (attrCache[relId] == nullptr)
    {
        return E_RELNOTOPEN;
    }

    for (AttrCacheEntry *entry = attrCache[relId]; entry != nullptr; entry = entry->next)
    {
        if (strcmp(entry->attrCatEntry.attrName, attrName) == 0)
        {
            memcpy(&(entry->attrCatEntry), attrCatEntry, sizeof(AttrCatEntry));
            entry->dirty = true;
            return SUCCESS;
        }
    }
    return E_ATTRNOTEXIST;
}

int AttrCacheTable::setAttrCatEntry(int relId, int offset, AttrCatEntry* attrCatEntry){
    if (relId < 0 || relId >= MAX_OPEN){
        return E_OUTOFBOUND;
    }

    if (attrCache[relId] == nullptr){
        return E_RELNOTOPEN;
    }

    for (AttrCacheEntry* entry = attrCache[relId]; entry != nullptr; entry = entry->next){
        if (entry->attrCatEntry.offset == offset){
            memcpy(&(entry->attrCatEntry), attrCatEntry, sizeof(AttrCatEntry));
            entry->dirty = true;
            return SUCCESS;
        }
    }

    return E_ATTRNOTEXIST;
}
