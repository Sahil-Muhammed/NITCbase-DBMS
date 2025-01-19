#include "BlockAccess.h"

#include <cstring>
#include <stdlib.h>

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

        if (slot >= relCatBuffer.numSlotsPerBlk){
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
