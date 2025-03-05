#include "BPlusTree.h"

#include <cstring>
#include <cstdio>

RecId BPlusTree::bPlusSearch(int relId, char attrName[ATTR_SIZE], Attribute attrVal, int op){
    IndexId searchIndex;
    
    AttrCacheTable::getSearchIndex(relId, attrName, &searchIndex);

    AttrCatEntry attrCatEntry;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

    int block, index;

    if (searchIndex.block == -1 && searchIndex.index == -1){

        block = attrCatEntry.rootBlock;
        index = 0;

        if (block == -1){
            return RecId{-1, -1};
        }
    }

    else{

        block = searchIndex.block;
        index = searchIndex.index + 1;

        IndLeaf leaf(block);

        HeadInfo leafHead;
        leaf.getHeader(&leafHead);

        if (index >= leafHead.numEntries){

            block = leafHead.rblock;  //could be leaf = leaf(leafHead.rblock)
            index = 0;

            if (block == -1){
                return RecId{-1, -1};
            }
        }

    }

    while (StaticBuffer::getStaticBlockType(block) == IND_INTERNAL){  //condition could be checking ASCII value of I
        IndInternal internalBlk(block);

        HeadInfo inthead;

        internalBlk.getHeader(&inthead);

        InternalEntry internalEntry;

        if (op == NE || op == LE || op == LT){

            internalBlk.getEntry(&internalEntry, 0);
            block = internalEntry.lChild;

        }

        else{
                int flag = -1;
                for (int i = 0; i < inthead.numEntries; ++i){
                    internalBlk.getEntry(&internalEntry, i);
                    int cmpAns = compareAttrs(internalEntry.attrVal, attrVal, attrCatEntry.attrType);
                    if ((cmpAns >= 0 && op == GE) || (op == EQ && cmpAns == 0) || (cmpAns > 0 && op == GT)){
                        flag = 0;
                        break;
                    }
                }
                if (flag == 0){
                    block = internalEntry.lChild;
                }
                else{
                    internalBlk.getEntry(&internalEntry, inthead.numEntries - 1);
                    block = internalEntry.rChild;
                }

        }
        index = 0;
    }

    while (block != -1){
        IndLeaf leafBlk(block);
        HeadInfo leafHead;
        leafBlk.getHeader(&leafHead);

        Index leafEntry;

        while (index < leafHead.numEntries){
            leafBlk.getEntry(&leafEntry, index);

            int cmpVal = compareAttrs(leafEntry.attrVal, attrVal, attrCatEntry.attrType);

            if (cmpVal == 0 && op == EQ ||
                cmpVal >= 0 && op == GE ||
                cmpVal <= 0 && op == LE ||
                cmpVal < 0 && op == LT ||
                cmpVal > 0 && op == GT ||
                cmpVal != 0 && op == NE){
                    searchIndex = {block, index};
                    AttrCacheTable::setSearchIndex(relId, attrName, &searchIndex);
                    return RecId{leafEntry.block, leafEntry.slot};
                }

            else if ((op == EQ || op == LE || op == LT) && cmpVal > 0){
                return RecId{-1, -1};
            }
            index++;
        }

        if (op != NE){
            break;
        }

        block = leafHead.rblock;
        index = 0;
    }
    return RecId{-1, -1};
}

int BPlusTree::bPlusCreate(int relId, char attrName[ATTR_SIZE]){
    if (relId == RELCAT_RELID || relId == ATTRCAT_RELID){
        return E_NOTPERMITTED;
    }

    AttrCatEntry attrCatBuf;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatBuf);

    if (attrCatBuf.rootBlock != -1){
        return SUCCESS;
    }

    IndLeaf rootBlockBuf;

    int rootBlock = rootBlockBuf.getBlockNum();

    attrCatBuf.rootBlock = rootBlock;
    AttrCacheTable::setAttrCatEntry(relId, attrName, &attrCatBuf);

    if (rootBlock == E_DISKFULL){
        return E_DISKFULL;
    }

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);

    int block = relCatEntry.firstBlk;

    while (block != -1){
        RecBuffer bPlus(block);

        unsigned char slotMap[relCatEntry.numSlotsPerBlk];
        bPlus.getSlotMap(slotMap);

        for (int i = 0; i < relCatEntry.numSlotsPerBlk; ++i){

            if (slotMap[i] == SLOT_OCCUPIED){

                Attribute record[relCatEntry.numAttrs];
                bPlus.getRecord(record, i);

                RecId recId{block, i};

                int retVal = bPlusInsert(relId, attrName, record[attrCatBuf.offset], recId);

                if (retVal == E_DISKFULL){
                    return E_DISKFULL;
                }
            }
        }
        HeadInfo head;
        bPlus.getHeader(&head);
        block = head.rblock;
    }
    return SUCCESS;
}

int BPlusTree::bPlusDestroy(int rootBlockNum){
    if (rootBlockNum < 0 || rootBlockNum >= DISK_BLOCKS){
        return E_OUTOFBOUND;
    }

    int type = StaticBuffer::getStaticBlockType(rootBlockNum);

    if (type == IND_LEAF){
        IndLeaf leaf(rootBlockNum);

        leaf.releaseBlock();
        return SUCCESS;
    }

    else if (type == IND_INTERNAL){
        IndInternal Internal(rootBlockNum);

        HeadInfo head;
        Internal.getHeader(&head);

        InternalEntry internalEntry;
        Internal.getEntry(&internalEntry, 0);
        bPlusDestroy(internalEntry.lChild);

        for (int i = 0; i < head.numEntries; ++i){
            InternalEntry internalBlk;
            Internal.getEntry(&internalBlk, i);
            bPlusDestroy(internalBlk.rChild);
        }

        Internal.releaseBlock();

        return SUCCESS;
    }

    else{
        return E_INVALIDBLOCK;
    }
}

int BPlusTree::bPlusInsert(int relId, char attrName[ATTR_SIZE], Attribute attrVal, RecId recid){
    AttrCatEntry attrCatBuf;
    int ret = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatBuf);

    if (ret != SUCCESS){
        return ret;
    }

    int blockNum = attrCatBuf.rootBlock;

    if (blockNum == -1){
        return E_NOINDEX;
    }

    int leafBlkNum = findLeafToInsert(blockNum, attrVal, attrCatBuf.attrType);

    Index index;
    index.attrVal = attrVal;
    index.block = recid.block;
    index.slot = recid.slot;

    if (insertIntoLeaf(relId, attrName, leafBlkNum, index) == E_DISKFULL){
        bPlusDestroy(blockNum);
        attrCatBuf.rootBlock = -1;
        AttrCacheTable::setAttrCatEntry(relId, attrName, &attrCatBuf);

        return E_DISKFULL;
    }

    return SUCCESS;
}

int BPlusTree::findLeafToInsert(int rootBlock, Attribute attrVal, int attrType){
    int blockNum = rootBlock;

    while (StaticBuffer::getStaticBlockType(blockNum) != IND_LEAF){
        IndInternal block(blockNum);
        HeadInfo head;
        block.getHeader(&head);

        int i = 0;
        
        for(; i < head.numEntries; ++i){
            InternalEntry entry;
            block.getEntry(&entry, i);
            if (compareAttrs(entry.attrVal, attrVal, NUMBER) > 0){
                break;
            }
        }

        if (i == head.numEntries){
            InternalEntry entr;
            block.getEntry(&entr, head.numEntries - 1);
            blockNum = entr.rChild;
        }
        else{
            InternalEntry ent;
            block.getEntry(&ent, i);
            blockNum = ent.lChild;
        }
    }

    return blockNum;
}

int BPlusTree::insertIntoLeaf(int relId, char attrName[ATTR_SIZE], int blockNum, Index indexEntry){
    AttrCatEntry attrCatBuf;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatBuf);

    IndLeaf leaf(blockNum);

    HeadInfo head;
    leaf.getHeader(&head);

    Index indices[head.numEntries + 1];

    int flag = 0;

    for (int i = 0; i < head.numEntries; ++i){
        Index entry;
        leaf.getEntry(&entry, i);

        if (compareAttrs(entry.attrVal, indexEntry.attrVal, attrCatBuf.attrType) <= 0){
            indices[i] = entry;
        }
        else{
            indices[i] = indexEntry;
            flag = 1;

            for (i += 1; i <= head.numEntries; ++i){
                leaf.getEntry(&entry, i);
                indices[i] = entry;
            }
            break;
        }
    }

    if (flag == 0){
        indices[head.numEntries] = indexEntry;
    }

    if (head.numEntries < MAX_KEYS_LEAF){
        head.numEntries += 1;
        leaf.setHeader(&head);

        for (int i = 0; i < head.numEntries; ++i){
            leaf.setEntry(&indices[i], i);
        }

        return SUCCESS;
    }

    int newBlk = splitLeaf(blockNum, indices);

    if (newBlk == E_DISKFULL){
        return E_DISKFULL;
    }

    if (head.pblock != -1){
        InternalEntry middle;
        middle.attrVal = indices[MIDDLE_INDEX_LEAF].attrVal;
        middle.lChild = blockNum;
        middle.rChild = newBlk;
        return insertIntoInternal(relId, attrName, head.pblock, middle);
    }

    else{
        if (createNewRoot(relId, attrName, indices[MIDDLE_INDEX_LEAF].attrVal, blockNum, newBlk) == E_DISKFULL){
            return E_DISKFULL;
        }
    }

    return SUCCESS;
}

int BPlusTree::splitLeaf(int leafBlockNum, Index indices[]){
    IndLeaf rightBlk;
    IndLeaf leftBlk(leafBlockNum);

    int rightBlkNum = rightBlk.getBlockNum();
    int leftBlkNum = leftBlk.getBlockNum();

    if (rightBlkNum == E_DISKFULL){
        return E_DISKFULL;
    }

    HeadInfo leftBlkHeader, rightBlkHeader;

    rightBlk.getHeader(&rightBlkHeader);
    leftBlk.getHeader(&leftBlkHeader);

    rightBlkHeader.numEntries = (MAX_KEYS_LEAF + 1) / 2;
    rightBlkHeader.pblock = leftBlkHeader.pblock;
    rightBlkHeader.lblock = leafBlockNum;
    rightBlkHeader.rblock = leftBlkHeader.rblock;

    rightBlk.setHeader(&rightBlkHeader);

    leftBlkHeader.numEntries = (MAX_KEYS_LEAF + 1) / 2;
    leftBlkHeader.rblock = rightBlkNum;
    leftBlk.setHeader(&leftBlkHeader);

    int i = 0;
    for (; i < (MAX_KEYS_LEAF + 1) / 2; ++i){
        leftBlk.setEntry(&indices[i], i);
    }

    for(; i < MAX_KEYS_LEAF + 1; ++i){
        rightBlk.setEntry(&indices[i], i-32);
    }

    return rightBlkNum;

}

int BPlusTree::insertIntoInternal(int relId, char attrName[ATTR_SIZE], int intBlockNum, InternalEntry intEntry){
    AttrCatEntry attrCatBuf;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatBuf);

    IndInternal intBlk(intBlockNum);

    HeadInfo blockHeader;
    intBlk.getHeader(&blockHeader);

    InternalEntry internalEntries[blockHeader.numEntries + 1];

    int i = 0;
    int indexPos = -1; 
    for (; i < blockHeader.numEntries; ++i){
        InternalEntry entry;
        intBlk.getEntry(&entry, i);
        if (compareAttrs(entry.attrVal, intEntry.attrVal, attrCatBuf.attrType) <= 0){
            internalEntries[i] = entry;
        }
        else{
            internalEntries[i] = intEntry;
            indexPos = i;
            i++;
            while (i <= blockHeader.numEntries){
                InternalEntry entry;
                intBlk.getEntry(&entry, i);
                internalEntries[i] = entry;
                i++;
            }

            break;
        }
    }

    if (indexPos == -1){
        internalEntries[blockHeader.numEntries] = intEntry;
        indexPos = blockHeader.numEntries;
    }

    if (indexPos > 0){
        internalEntries[indexPos - 1].rChild = intEntry.lChild; 
    }

    if (indexPos < blockHeader.numEntries){
        internalEntries[indexPos + 1].lChild = intEntry.rChild;
    }

    if (blockHeader.numEntries != MAX_KEYS_INTERNAL){
        blockHeader.numEntries++;
        intBlk.setHeader(&blockHeader);

        for (int j = 0; j <= blockHeader.numEntries; ++j){
            intBlk.setEntry(&internalEntries[j], j);
        }

        return SUCCESS;
    }

    int newRightBlk = splitInternal(intBlockNum, internalEntries);

    if (newRightBlk == E_DISKFULL){
        bPlusDestroy(intEntry.rChild);
        return E_DISKFULL;
    }

    if (blockHeader.pblock != -1){
        InternalEntry rightInt;
        
        rightInt.attrVal = internalEntries[MIDDLE_INDEX_INTERNAL].attrVal;
        rightInt.lChild = intBlockNum;
        rightInt.rChild = newRightBlk;
        
        return insertIntoInternal(relId, attrName, blockHeader.pblock, rightInt);
    }

    else{
        createNewRoot(relId, attrName, internalEntries[MIDDLE_INDEX_INTERNAL].attrVal, intBlockNum, newRightBlk);
    }

    return SUCCESS;
}

int BPlusTree::splitInternal(int intBlockNum, InternalEntry internalEntries[]){
    IndInternal rightBlk;
    IndInternal leftBlk(intBlockNum);

    int rightBlkNum = rightBlk.getBlockNum();
    int leftBlkNum = leftBlk.getBlockNum();

    if (rightBlkNum == E_DISKFULL){
        return E_DISKFULL;
    }

    HeadInfo leftBlkHeader, rightBlkHeader;

    rightBlk.getHeader(&rightBlkHeader);
    leftBlk.getHeader(&leftBlkHeader);

    rightBlkHeader.numEntries = MAX_KEYS_INTERNAL / 2;
    rightBlkHeader.pblock = leftBlkHeader.pblock;

    rightBlk.setHeader(&rightBlkHeader);

    leftBlkHeader.numEntries = MAX_KEYS_INTERNAL / 2;
    leftBlk.setHeader(&leftBlkHeader);

    BlockBuffer block(internalEntries[MIDDLE_INDEX_INTERNAL + 1].lChild);
    HeadInfo blockHead;

    block.getHeader(&blockHead);
    blockHead.pblock = rightBlkNum;
    block.setHeader(&blockHead);

    for (int i = 0; i < MAX_KEYS_INTERNAL / 2; ++i){
        leftBlk.setEntry(&internalEntries[i], i);
        rightBlk.setEntry(&internalEntries[i+51], i);
    }

    int type = StaticBuffer::getStaticBlockType(internalEntries[0].lChild);

    for (int k = 0; k < MAX_KEYS_INTERNAL / 2; ++k){
        BlockBuffer block(internalEntries[k + MIDDLE_INDEX_INTERNAL + 1].rChild);

        block.getHeader(&blockHead);
        blockHead.pblock = rightBlkNum;
        block.setHeader(&blockHead);
    }

    return rightBlkNum;
}

int BPlusTree::createNewRoot(int relId, char attrName[ATTR_SIZE], Attribute attrVal, int lChild, int rChild){
    AttrCatEntry attrCatBuf;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatBuf);

    IndInternal newRootBlk;

    int newRootBlkNum = newRootBlk.getBlockNum();

    if (newRootBlkNum == E_DISKFULL){
        bPlusDestroy(rChild);
        return E_DISKFULL;
    }

    HeadInfo head;
    newRootBlk.getHeader(&head);
    head.numEntries = 1;
    newRootBlk.setHeader(&head);

    InternalEntry intEntry;
    intEntry.attrVal = attrVal;
    intEntry.lChild = lChild;
    intEntry.rChild = rChild;

    newRootBlk.setEntry(&intEntry, 0);

    BlockBuffer lBlock(lChild);
    BlockBuffer rBlock(rChild);
    HeadInfo lChildHead;
    HeadInfo rChildHead;
    lBlock.getHeader(&lChildHead);
    rBlock.getHeader(&rChildHead);
    lChildHead.pblock = newRootBlkNum;
    rChildHead.pblock = newRootBlkNum;

    attrCatBuf.rootBlock = newRootBlkNum;
    AttrCacheTable::setAttrCatEntry(relId, attrName, &attrCatBuf);

    return SUCCESS;
}