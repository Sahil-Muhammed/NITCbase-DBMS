#include "BlockAccess.h"

#include <cstring>
#include <stdlib.h>
#include <cstdio>

RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op){
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId, &prevRecId);

    int block = -1;
    int slot = -1;

    if (prevRecId.block == -1 && prevRecId.slot == -1){
        RelCatEntry relCatBuf;
        RelCacheTable::getRelCatEntry(relId, &relCatBuf);
        
        block = relCatBuf.firstBlk;
        slot = 0;
    }
    else{
        block = prevRecId.block;
        slot = prevRecId.slot+1;
    }

    RelCatEntry relCatBuffer;
    RelCacheTable::getRelCatEntry(relId, &relCatBuffer);

    while (block != -1){
        RecBuffer Buffer(block);

        Attribute Record[ATTRCAT_NO_ATTRS];
        HeadInfo head;
        Buffer.getRecord(Record, slot);
        Buffer.getHeader(&head);
        unsigned char* slotmap = (unsigned char *)malloc(sizeof(unsigned char) * head.numSlots);

        Buffer.getSlotMap(slotmap);

        if (slot >= head.numSlots){
            block = head.rblock;
            slot = 0;
            continue;
        }
        if (slotmap[slot] == SLOT_UNOCCUPIED){
            slot += 1;
            continue;
        }

        AttrCatEntry attrCatBuf;
        AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatBuf);

        int offset = attrCatBuf.offset;
        Attribute* attrValCheck = (Attribute*)malloc(sizeof(Attribute) * head.numAttrs);
        Buffer.getRecord(attrValCheck, slot);

        int cmpVal = compareAttrs(attrValCheck[offset], attrVal, attrCatBuf.attrType);

        if (
            (op == NE && cmpVal != 0) ||    // if op is "not equal to"
            (op == LT && cmpVal < 0) ||     // if op is "less than"
            (op == LE && cmpVal <= 0) ||    // if op is "less than or equal to"
            (op == EQ && cmpVal == 0) ||    // if op is "equal to"
            (op == GT && cmpVal > 0) ||     // if op is "greater than"
            (op == GE && cmpVal >= 0)       // if op is "greater than or equal to"
        ) {
            /*
            set the search index in the relation cache as
            the record id of the record that satisfies the given condition
            (use RelCacheTable::setSearchIndex function)
            */
            RecId rec;
            rec.block = block;
            rec.slot = slot;
            RelCacheTable::setSearchIndex(relId, &rec);
            return RecId{block, slot};
        }

        slot++;
    }

    return RecId{-1, -1};
}

int BlockAccess::renameRelation(char oldRelName[ATTR_SIZE], char newRelName[ATTR_SIZE]){
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute newRelationName;
    strcpy(newRelationName.sVal, newRelName);
    int op = 0;

    RecId resultForNew = linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, newRelationName, op);
    if (resultForNew.block != -1 && resultForNew.slot != -1){
        return E_RELEXIST;
    }

    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute oldRelationName;
    strcpy(oldRelationName.sVal, oldRelName);
    
    RecId resultForOld = linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, oldRelationName, op);
    if (resultForOld.block == -1 && resultForOld.slot == -1){
        return E_RELNOTEXIST;
    }

    RecBuffer Buffer(resultForOld.block);
    Attribute record[RELCAT_NO_ATTRS];
    Buffer.getRecord(record, resultForOld.slot);
    strcpy(record[RELCAT_REL_NAME_INDEX].sVal, newRelName);
    Buffer.setRecord(record, resultForOld.slot);

    int numAttrs = record[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    // for (int i = 0; i < head.numEntries; ++i){
    //     Buffer.getRecord(record, i);
    //     if (!strcmp(record[RELCAT_REL_NAME_INDEX].sVal, oldRelName)){
    //         int numAttrs = record[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    //         Attribute recordAttribute[numAttrs];
    //         RecBuffer AttrBuffer(ATTRCAT_BLOCK);
    //         for (int j = 0; j < record[RELCAT_NO_RECORDS_INDEX].nVal; ++j){
    //             AttrBuffer.getRecord(recordAttribute, );
    //         }
    //     }
    //     else{
    //         continue;
    //     }
    // }
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    for (int i = 0; i < numAttrs; ++i){
        RecId result = linearSearch(ATTRCAT_RELID, (char *)ATTRCAT_ATTR_RELNAME, oldRelationName, op);
        if (result.block != -1 && result.slot != -1){
            RecBuffer attrBuffer(result.block);
            attrBuffer.getRecord(record, i);  //Can use another array of union Attribute, but checking if this will work
            strcpy(record[ATTRCAT_REL_NAME_INDEX].sVal, newRelName);
            attrBuffer.setRecord(record, i);
        }

        else{
            printf("Error. Old Relation not found.\n");
        }
    }

    return SUCCESS;
}

int BlockAccess::renameAttribute(char relname[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE]){

    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr;
    int op = 0;
    strcpy(relNameAttr.sVal, relname);

    printf("Before first linearSearch in renameAttribute.\n");
    RecId ret = BlockAccess::linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, relNameAttr, op);
    printf("After first linearSearch in renameAttribute.\n");
    if (ret.block == -1 && ret.slot == -1){
        return E_RELNOTEXIST;
    }
    printf("Did not go through if condition.\n");

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    RecId attrToRenameRecId{-1, -1};
    Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];

    while (true){
        RecId result = linearSearch(ATTRCAT_RELID, (char *)ATTRCAT_ATTR_RELNAME, relNameAttr, op);
        if (result.block == -1 && result.slot == -1){
            break;
        }

        RecBuffer Buffer(result.block);
        Buffer.getRecord(attrCatEntryRecord, result.slot);

        if (!strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, oldName)){
            attrToRenameRecId.block = result.block;
            attrToRenameRecId.slot = result.slot;
            break;
        }

        if (!strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName)){
            return E_ATTREXIST;
        }
    }

    if (attrToRenameRecId.block == -1 && attrToRenameRecId.slot == -1){
        return E_ATTRNOTEXIST;
    }

    Attribute record[ATTRCAT_NO_ATTRS];
    RecBuffer AttrBuffer(attrToRenameRecId.block);
    AttrBuffer.getRecord(record, attrToRenameRecId.slot);
    strcpy(record[ATTRCAT_ATTR_NAME_INDEX].sVal, newName);
    AttrBuffer.setRecord(record, attrToRenameRecId.slot);

    return SUCCESS;
}
