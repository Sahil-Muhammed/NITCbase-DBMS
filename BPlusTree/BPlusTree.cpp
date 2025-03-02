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
                    int cmpAns = compareAttrs(internalEntry.attrVal, attrVal, NUMBER);
                    if ((cmpAns >= 0 && op == GE) || (op == EQ && cmpAns == 0) || (cmpAns > 0 && op == GT)){
                        flag = 0;
                        break;
                    }
                }
                if (flag == 0){
                    block = internalEntry.lChild;
                }
                else{
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

            int cmpVal = compareAttrs(leafEntry.attrVal, attrVal, NUMBER);

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