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

    //Main aim of this part is to store the relCacheEntries in relCat[] and use it in main()
    RecBuffer relCatBlock(RELCAT_BLOCK);
    Attribute relCatRecord[RELCAT_NO_ATTRS];

    RelCacheEntry* relCacheEntry = nullptr;
    relCacheEntry = (RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    for (int relId = RELCAT_RELID; relId <= ATTRCAT_RELID+4; ++relId){
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
    int slotNum = 0;
    for (int relId = RELCAT_RELID; relId <= ATTRCAT_RELID+4; ++relId){
        int numAttributes = RelCacheTable::relCache[relId]->relCatEntry.numAttrs;
        head = linkedListOfAttrCacheEntry(numAttributes);
        attrCacheEntry = head;
        HeadInfo attrCatHeader;
        attrCatBlock.getHeader(&attrCatHeader);
        while (attrCacheEntry){
            attrCatBlock.getRecord(attrCatRecord, slotNum);
            AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &(attrCacheEntry->attrCatEntry));
            attrCacheEntry->recId.block = ATTRCAT_BLOCK;
            attrCacheEntry->recId.slot = slotNum++;
            attrCacheEntry = attrCacheEntry->next;
            if (slotNum == attrCatHeader.numSlots){
                slotNum = 0;
                attrCatBlock = RecBuffer(attrCatHeader.rblock);
            }
        }
        AttrCacheTable::attrCache[relId] = head;
    }
}

OpenRelTable::~OpenRelTable(){
    for (int i = 0; i < MAX_OPEN; ++i) {
        if (RelCacheTable::relCache[i]) {
            free(RelCacheTable::relCache[i]);
            RelCacheTable::relCache[i] = nullptr;
        }
        AttrCacheEntry* head = AttrCacheTable::attrCache[i];
        while (head) {
            AttrCacheEntry* temp = head;
            head = head->next;
            free(temp);
        }
        AttrCacheTable::attrCache[i] = nullptr;
    }
}

int OpenRelTable::getRelId(char relName[ATTR_SIZE]){
    if (strcmp(relName, RELCAT_RELNAME) == 0)
        return RELCAT_RELID;
    if (strcmp(relName, ATTRCAT_RELNAME) == 0)
        return ATTRCAT_RELID;
    return E_RELNOTOPEN;
}