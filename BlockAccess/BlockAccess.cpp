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
        union Attribute Record[ATTRCAT_NO_ATTRS];
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

    RecId ret = BlockAccess::linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, relNameAttr, op);
    if (ret.block == -1 && ret.slot == -1){
        return E_RELNOTEXIST;
    }

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

int BlockAccess::insert(int relId, Attribute* record){
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);
    int blockNum = relCatEntry.lastBlk;

    RecId rec_id = {-1, -1};

    int numOfSlots = relCatEntry.numSlotsPerBlk;
    int numOfAttrs = relCatEntry.numAttrs;

    int prevBlockNumber = -1;

    while (blockNum != -1){
        RecBuffer block(blockNum);
        HeadInfo head;
        block.getHeader(&head);

        unsigned char* slotMap = (unsigned char*)malloc(sizeof(unsigned char) * numOfSlots);
        block.getSlotMap(slotMap);

        for (int i = 0; i < head.numSlots; ++i){
            if (*(slotMap+i) == SLOT_UNOCCUPIED){
                rec_id.block = blockNum;
                rec_id.slot = i;
                break;
            }
        }

        if (rec_id.block != -1 && rec_id.slot != -1){
            break;
        }

        prevBlockNumber = blockNum;
        blockNum = head.rblock;
    }
    if (rec_id.block == -1 && rec_id.slot == -1){
        if (relId == RELCAT_RELID){
            return E_MAXRELATIONS;
        }
        RecBuffer Buffer;
        blockNum = Buffer.getBlockNum();
        if (blockNum == E_DISKFULL){
            return E_DISKFULL;
        }

        rec_id.block = blockNum;
        rec_id.slot = 0;

        HeadInfo head;
        head.blockType = REC;
        head.pblock = -1;
        head.rblock = -1;
        //head.lblock = blockNum == -1 ? -1 : prevBlockNumber;
        head.lblock = prevBlockNumber;
        head.numAttrs = numOfAttrs;
        head.numEntries = 0;
        head.numSlots = numOfSlots;
        Buffer.setHeader(&head);
        unsigned char* newSlotMap = (unsigned char *)malloc(sizeof(unsigned char) * numOfSlots); //should probably allocate memory dynamically, but lets see what happens
        for (int k = 0; k < numOfSlots; ++k){
            newSlotMap[k] = SLOT_UNOCCUPIED;
        }
        Buffer.setSlotMap(newSlotMap);

        if (prevBlockNumber != -1){
            RecBuffer prevBlock(prevBlockNumber);
            HeadInfo prevHead;
            prevBlock.getHeader(&prevHead);
            prevHead.rblock = rec_id.block;
            prevBlock.setHeader(&prevHead);
        }

        else{
            relCatEntry.firstBlk = rec_id.block;
            RelCacheTable::setRelCatEntry(relId, &relCatEntry);
        }

        relCatEntry.lastBlk = rec_id.block;
        RelCacheTable::setRelCatEntry(relId, &relCatEntry);
    }
    RecBuffer NewBlock(rec_id.block);
    int res = NewBlock.setRecord(record, rec_id.slot);
    //printf("Record[0].sVal = %s\n", record[0].sVal);
    if (res != SUCCESS){
        printf("Not successful in setRecord.\n");
        exit(1);
    }

    unsigned char* slotmap = (unsigned char*)malloc(sizeof(unsigned char) * numOfSlots);
    NewBlock.getSlotMap(slotmap);
    slotmap[rec_id.slot] = SLOT_OCCUPIED;
    NewBlock.setSlotMap(slotmap);

    HeadInfo newHead;
    NewBlock.getHeader(&newHead);
    newHead.numEntries += 1;
    NewBlock.setHeader(&newHead);

    relCatEntry.numRecs += 1;
    RelCacheTable::setRelCatEntry(relId, &relCatEntry);

    int flag = SUCCESS;
    for (int k = 0; k < numOfAttrs; ++k){
        AttrCatEntry attrCatBuf;
        AttrCacheTable::getAttrCatEntry(relId, k, &attrCatBuf);

        int rootBlock = attrCatBuf.rootBlock;

        if (rootBlock != -1){
            int retVal = BPlusTree::bPlusInsert(relId, attrCatBuf.attrName, record[k], rec_id);

            if (retVal == E_DISKFULL){
                flag = E_INDEX_BLOCKS_RELEASED;
            }
        }
    }

    return flag;
    return SUCCESS;
}

//mistakes: 1. didn't allocate memory dynamically for slotMap

int BlockAccess::search(int relId, Attribute* record, char attrName[ATTR_SIZE], Attribute attrVal, int op){

    RecId recId;
    AttrCatEntry attrCatEntry;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

    if (attrCatEntry.rootBlock == -1){
        recId = linearSearch(relId, attrName, attrVal, op);
    }

    else{
        recId = BPlusTree::bPlusSearch(relId, attrName, attrVal, op);
    }

    if (recId.block == -1 && recId.slot == -1){
        return E_NOTFOUND;
    }

    RecBuffer block(recId.block);
    int res = block.getRecord(record, recId.slot);

    if (res != SUCCESS){
        return res;
    }

    return SUCCESS;
}

int BlockAccess::deleteRelation(char relName[ATTR_SIZE]){
    if (!strcmp(relName, RELCAT_RELNAME) || !strcmp(relName, ATTRCAT_RELNAME)){
        return E_NOTPERMITTED;
    }

    RelCacheTable::resetSearchIndex(RELCAT_RELID); //used to reset search index of given relation

    Attribute relNameAttr;
    strcpy(relNameAttr.sVal, relName);

    int op = 0;
    RecId recId = BlockAccess::linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, relNameAttr, op);
    if (recId.block == -1 && recId.slot == -1){
        return E_RELNOTEXIST;
    }

    RecBuffer buffer(recId.block);
    Attribute relCatEntryRecord[RELCAT_NO_ATTRS];
    buffer.getRecord(relCatEntryRecord, recId.slot);

    int firstBlk = relCatEntryRecord[RELCAT_FIRST_BLOCK_INDEX].nVal;
    int numAttrs = relCatEntryRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal; //array index was given earlier as RELCAT_NO_RECORDS_INDEX

    int current = firstBlk;
    int next = firstBlk;

    while (next != -1){
        RecBuffer Block(current);
        HeadInfo head;
        Block.getHeader(&head);
        next = head.rblock;
        current = next;
        Block.releaseBlock();
    }

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    int numberofAttributesDeleted = 0;

    while (true){

        RecId attrCatRecId = linearSearch(ATTRCAT_RELID, (char *)RELCAT_ATTR_RELNAME, relNameAttr, op);

        if (attrCatRecId.block == -1 && attrCatRecId.slot == -1){
            break;
        }

        numberofAttributesDeleted++;

        RecBuffer delBlock(attrCatRecId.block);
        struct HeadInfo head;
        delBlock.getHeader(&head);
        int attrs = head.numAttrs;
        int slots = head.numSlots;
        Attribute record[ATTRCAT_NO_ATTRS];
        delBlock.getRecord(record, attrCatRecId.slot);

        int rootBlock = record[ATTRCAT_ROOT_BLOCK_INDEX].nVal;

        unsigned char* slotMap = (unsigned char*)malloc(sizeof(unsigned char) * slots);
        delBlock.getSlotMap(slotMap);
        slotMap[attrCatRecId.slot] = SLOT_UNOCCUPIED;
        delBlock.setSlotMap(slotMap);

        head.numEntries -= 1;
        delBlock.setHeader(&head);

        if (head.numEntries == 0){
            RecBuffer updateBlock(head.lblock);
            HeadInfo leftHead;
            updateBlock.getHeader(&leftHead);
            leftHead.lblock = head.rblock;
            updateBlock.setHeader(&leftHead);

            if (head.rblock != -1){
                RecBuffer rightBlock(head.rblock);
                HeadInfo rightHead;
                rightBlock.getHeader(&rightHead);
                rightHead.lblock = head.lblock;
                rightBlock.setHeader(&rightHead);
            }

            else{
                RelCatEntry relCatEntryBuf;
                RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &relCatEntryBuf);
                relCatEntryBuf.lastBlk = head.lblock;
            }

            delBlock.releaseBlock();
        }

        if (rootBlock != -1){
            BPlusTree::bPlusDestroy(rootBlock);
        }
    }

    HeadInfo relCatHead;
    buffer.getHeader(&relCatHead);

    relCatHead.numEntries -= 1;
    buffer.setHeader(&relCatHead);
    int slots = relCatHead.numSlots;

    unsigned char* slotMap = (unsigned char*)malloc(sizeof(unsigned char) * slots);
    buffer.getSlotMap(slotMap);
    slotMap[recId.slot] = SLOT_UNOCCUPIED;
    buffer.setSlotMap(slotMap);

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(RELCAT_RELID, &relCatEntry);
    relCatEntry.numRecs -= 1;
    RelCacheTable::setRelCatEntry(RELCAT_RELID, &relCatEntry);

    RelCatEntry attrCat;
    RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &attrCat);
    attrCat.numRecs -= numberofAttributesDeleted;
    RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &attrCat);
    return SUCCESS;
}

int BlockAccess::project(int relId, Attribute* record){
    RecId prevSearchIndex;
    RelCacheTable::getSearchIndex(relId, &prevSearchIndex);

    int block = prevSearchIndex.block, slot = prevSearchIndex.slot;

    RelCatEntry relCatBuffer;

    if (prevSearchIndex.block == -1 && prevSearchIndex.slot == -1){

        RelCacheTable::getRelCatEntry(relId, &relCatBuffer);

        block = relCatBuffer.firstBlk;
        slot = 0;
    }

    else{
        slot++;
    }

    while (block != -1){

        RecBuffer Block(block);

        HeadInfo header;
        Block.getHeader(&header);
        int numSlots = header.numSlots;
        unsigned char slotMap[numSlots];
        Block.getSlotMap(slotMap); //didn't call this earlier

        if (slot >= numSlots){
            block = header.rblock;
            slot = 0;
            continue;
        }

        else if (slotMap[slot] == SLOT_UNOCCUPIED){
            slot++;
            continue;
        }

        else{
            break;
        }
    }

    if (block == -1){
        return E_NOTFOUND;
    }

    RecId nextRecId = {block, slot};
    RelCacheTable::setSearchIndex(relId, &nextRecId);

    RecBuffer CopyBlock(nextRecId.block);
    return CopyBlock.getRecord(record, slot);
}