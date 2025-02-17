#include "OpenRelTable.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>

OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];

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
        OpenRelTable::tableMetaInfo[i].free = true;
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
    for (int relId = RELCAT_RELID; relId <= ATTRCAT_RELID; ++relId){
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
    for (int relId = RELCAT_RELID; relId <= ATTRCAT_RELID; ++relId){
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
                HeadInfo attrCatHead;
                attrCatBlock = RecBuffer(attrCatHeader.rblock);
            }
        }
        AttrCacheTable::attrCache[relId] = head;
    }

    OpenRelTable::tableMetaInfo[RELCAT_RELID].free = false;
    strcpy(OpenRelTable::tableMetaInfo[RELCAT_RELID].relName, "RELATIONCAT");
    OpenRelTable::tableMetaInfo[ATTRCAT_RELID].free = false;
    strcpy(OpenRelTable::tableMetaInfo[ATTRCAT_RELID].relName, "ATTRIBUTECAT");
}

OpenRelTable::~OpenRelTable(){
    /*for (int i = 2; i < MAX_OPEN; ++i) {
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
    }*/
    for (int i = 2; i < MAX_OPEN; ++i){
        if (!OpenRelTable::tableMetaInfo[i].free){
            OpenRelTable::closeRel(i);
        }
    }

    if (RelCacheTable::relCache[ATTRCAT_RELID]->dirty == true){

        Attribute record[6];
        RelCacheTable::relCatEntryToRecord(&RelCacheTable::relCache[ATTRCAT_RELID]->relCatEntry, record);
        RecId recId = RelCacheTable::relCache[ATTRCAT_RELID]->recId;
        RecBuffer relCatBlock(recId.block);
        relCatBlock.setRecord(record, recId.slot);
    }

    free(RelCacheTable::relCache[ATTRCAT_RELID]);
    RelCacheTable::relCache[ATTRCAT_RELID] = nullptr;

    if (RelCacheTable::relCache[RELCAT_RELID]->dirty == true){

        Attribute record[6];
        RelCacheTable::relCatEntryToRecord(&RelCacheTable::relCache[RELCAT_RELID]->relCatEntry, record);
        RecId recId = RelCacheTable:: relCache[RELCAT_RELID]->recId;
        RecBuffer relCatBlock(recId.block);
        relCatBlock.setRecord(record, recId.slot);
    }

    free(RelCacheTable::relCache[RELCAT_RELID]);
    RelCacheTable::relCache[RELCAT_RELID] = nullptr;

    for (int i = RELCAT_RELID; i <= ATTRCAT_RELID; ++i){
        AttrCacheEntry* current = AttrCacheTable::attrCache[i];
        AttrCacheEntry* next = nullptr;
        while (current != nullptr){
            next = current->next;
            if (current->dirty == true){
                Attribute record[6];
                AttrCacheTable::attrCatEntryToRecord(&current->attrCatEntry, record);
                RecBuffer attrCatBlock(current->recId.block);
                attrCatBlock.setRecord(record, current->recId.slot);
            }
            free(current);
            current = next;
        }
    }
}

int OpenRelTable::getRelId(char relName[ATTR_SIZE]){ //should be unsigned char according to documentation; let's see if this works; it works
    int relId = -1;
    
    for (int i = 0; i < MAX_OPEN; ++i){
        if (!strcmp(tableMetaInfo[i].relName, relName) && tableMetaInfo[i].free == false){
            relId = i;
            break;
        }
    }
    return relId >= 0 ? relId : E_RELNOTOPEN;
}

int OpenRelTable::getFreeOpenRelTableEntry(){
    int relId = -1;
    for (int i = 0; i < MAX_OPEN; ++i){
        if (tableMetaInfo[i].free){ //doubtful whether class name should be mentioned; not really needed
            relId = i;
            break;
        }
    }
    return relId >= 0 ? relId : E_CACHEFULL;
}

int OpenRelTable::openRel(char relName[ATTR_SIZE]){ //should be unsigned char according to documentation; doesn't matter
    int retRelId = OpenRelTable::getRelId((relName));
    if (retRelId >= 0 && retRelId < MAX_OPEN){
        return retRelId;
    }
    int freeRelId = getFreeOpenRelTableEntry();
    if (freeRelId == E_CACHEFULL){
        return E_CACHEFULL;
    }


    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    union Attribute attr;
    strcpy(attr.sVal, relName);
    int op = EQ;
    char attrName[ATTR_SIZE]; //doubtful whether this will work. this absolutely works
    strcpy(attrName, "RelName");
    RecId relCatRecId = BlockAccess::linearSearch(RELCAT_RELID, attrName, attr, op);
    if (relCatRecId.block == -1 && relCatRecId.slot == -1){
        return E_RELNOTEXIST;
    }
    
    RelCacheEntry* relCacheEntry = (RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    RecBuffer Buffer(relCatRecId.block);
    HeadInfo head;
    Buffer.getHeader(&head);
    Attribute record[head.numAttrs];
    Buffer.getRecord(record, relCatRecId.slot); //again, doubtful. yay, it works!
    RelCacheTable::recordToRelCatEntry(record, &(relCacheEntry->relCatEntry));
    relCacheEntry->recId.block = relCatRecId.block;
    relCacheEntry->recId.slot = relCatRecId.slot;
    RelCacheTable::relCache[freeRelId] = relCacheEntry;

    
    
    AttrCacheEntry* listHead = linkedListOfAttrCacheEntry(RelCacheTable::relCache[freeRelId]->relCatEntry.numAttrs);
    AttrCacheEntry* list = listHead;
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    AttrCacheEntry* attrCacheEntry = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
    listHead = list;
    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
    char attr1[ATTR_SIZE];
    strcpy(attr1, "RelName");
    for (int i = 0; i < RelCacheTable::relCache[freeRelId]->relCatEntry.numAttrs; i++){
        
        RecId attrcatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, attr1, attr, op);
        RecBuffer Buf(attrcatRecId.block);
        Buf.getRecord(attrCatRecord, attrcatRecId.slot);
        
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &list->attrCatEntry);
        list->recId.block = attrcatRecId.block;
        list->recId.slot = attrcatRecId.slot;
        list = list->next;
    }
    AttrCacheTable::attrCache[freeRelId] = listHead;

    tableMetaInfo[freeRelId].free = false;
    strcpy(tableMetaInfo[freeRelId].relName, relName);
    return freeRelId;
}

int OpenRelTable::closeRel(int relId){
    if (relId == 0 || relId == 1){
        return E_NOTPERMITTED;
    }
    if (relId < 0 || relId >= MAX_OPEN){
        return E_OUTOFBOUND;
    }
    if (tableMetaInfo[relId].free == true){
        return E_RELNOTOPEN;
    }
    
    //free(relCacheEntry);

    tableMetaInfo[relId].free = true;
    RelCacheTable::relCache[relId] = nullptr;
    AttrCacheTable::attrCache[relId] = nullptr;

    return SUCCESS;
}