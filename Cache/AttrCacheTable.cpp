#include "AttrCacheTable.h"

#include <cstring>

AttrCacheEntry* AttrCacheTable::attrCache[MAX_OPEN];

int AttrCacheTable::getAttrCatEntry(int relId, int attrOffset, AttrCatEntry* attrCatBuffer){
    //check whether relId is out of bounds
    if (relId < 0 || relId > MAX_OPEN){
        return E_OUTOFBOUND;
    }

    //check whether given relId is empty in attribute cache
    if (attrCache[relId] == nullptr){
        return E_RELNOTOPEN;
    }

    //search through the attribute cache for finding the offset-th attribute
    for (AttrCacheEntry* entry = attrCache[relId]; entry != nullptr; entry = entry->next){

        //if the offset matches, then copy entry->attrCatEntry to attrCatBuffer
        if (entry->attrCatEntry.offset == attrOffset){
            *attrCatBuffer = entry->attrCatEntry;
            return SUCCESS;
        }
    }

    //at given offset, attributeCatEntry is not present in attrCache
    return E_ATTRNOTEXIST;
}

/* Converts a attribute catalog record to AttrCatEntry struct
    We get the record as Attribute[] from the BlockBuffer.getRecord() function.
    This function will convert that to a struct AttrCatEntry type.
*/

//copies details from array of attribute type named record to attrCatEntry
void AttrCacheTable::recordToAttrCatEntry(union Attribute record[ATTRCAT_NO_ATTRS], AttrCatEntry* attrCatEntry){
    strcpy(attrCatEntry->relName, record[ATTRCAT_REL_NAME_INDEX].sVal);
    strcpy(attrCatEntry->attrName, record[ATTRCAT_ATTR_NAME_INDEX].sVal);
    attrCatEntry->attrType = (bool)record[ATTRCAT_ATTR_TYPE_INDEX].nVal;
    attrCatEntry->primaryFlag = (int)record[ATTRCAT_PRIMARY_FLAG_INDEX].nVal;
    attrCatEntry->rootBlock = (int)record[ATTRCAT_ROOT_BLOCK_INDEX].nVal;
    attrCatEntry->offset = (int)record[ATTRCAT_OFFSET_INDEX].nVal;
}