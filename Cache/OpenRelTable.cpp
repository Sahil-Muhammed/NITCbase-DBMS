#include "OpenRelTable.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>

AttrCacheEntry* linkedListOfAttrCacheEntry(int numAttributes){
    AttrCacheEntry* head = nullptr, *curr = nullptr;
    head = curr = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
    for (int i = 0; i < numAttributes-1; ++i){
        curr->next = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        curr = curr->next;
    }
    curr->next = nullptr;
    return head;
}

OpenRelTable::OpenRelTable(){
    for (int i = 0; i < MAX_OPEN; ++i){
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
    }

    /************ Setting up Relation Cache entries ************/
  // (we need to populate relation cache with entries for the relation catalog
  //  and attribute catalog.)

  /**** setting up Relation Catalog relation in the Relation Cache Table****/
    RecBuffer relCatBlock(RELCAT_BLOCK);
    Attribute relCatRecord[RELCAT_NO_ATTRS];

    RelCacheEntry* relCacheEntry = nullptr;
    relCacheEntry = (RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    for (int relId = RELCAT_RELID; relId <= ATTRCAT_RELID+1; ++relId){
        relCatBlock.getRecord(relCatRecord, relId);
        RelCacheTable::recordToRelCatEntry(relCatRecord, &(relCacheEntry->relCatEntry));
        relCacheEntry->recId.block = RELCAT_BLOCK;
        relCacheEntry->recId.slot = relId;

        RelCacheTable::relCache[relId] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
        *(RelCacheTable::relCache[relId]) = *relCacheEntry;
    }
    /**** setting up Attribute Catalog relation in the Relation Cache Table ****/

  // set up the relation cache entry for the attribute catalog similarly
  // from the record at RELCAT_SLOTNUM_FOR_ATTRCAT

  // set the value at RelCacheTable::relCache[ATTRCAT_RELID]

    /************ Setting up Attribute cache entries ************/
  // (we need to populate attribute cache with entries for the relation catalog
  //  and attribute catalog.)

  /**** setting up Relation Catalog relation in the Attribute Cache Table ****/
    RecBuffer attrCatBlock(ATTRCAT_BLOCK);

    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

    AttrCacheEntry* attrCacheEntry = nullptr, *head = nullptr;
    for (int relId = RELCAT_RELID, slotNum = 0; relId < ATTRCAT_RELID+2; ++relId){
        int numAttributes = RelCacheTable::relCache[relId]->relCatEntry.numAttrs;
        head = linkedListOfAttrCacheEntry(numAttributes);
        attrCacheEntry = head;
        while (numAttributes--){
            attrCatBlock.getRecord(attrCatRecord, slotNum);
            AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &(attrCacheEntry->attrCatEntry));
            attrCacheEntry->recId.block = ATTRCAT_BLOCK;
            attrCacheEntry->recId.slot = slotNum++;
            attrCacheEntry = attrCacheEntry->next;
        }
        AttrCacheTable::attrCache[relId] = head;
    }
}

OpenRelTable::~OpenRelTable(){

}