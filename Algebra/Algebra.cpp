#include "Algebra.h"

#include <cstring>
#include <stdlib.h>
#include <stdio.h>

// will return if a string can be parsed as a floating point number
bool isNumber(char *str) {
    int len;
    float ignore;
    /*
        sscanf returns the number of elements read, so if there is no float matching
        the first %f, ret will be 0, else it'll be 1

        %n gets the number of characters read. this scanf sequence will read the
        first float ignoring all the whitespace before and after. and the number of
        characters read that far will be stored in len. if len == strlen(str), then
        the string only contains a float with/without whitespace. else, there's other
        characters.
    */
    int ret = sscanf(str, "%f %n", &ignore, &len);
    return ret == 1 && len == strlen(str);
}
/* used to select all the records that satisfy a condition.
the arguments of the function are
- srcRel - the source relation we want to select from
- targetRel - the relation we want to select into. (ignore for now)
- attr - the attribute that the condition is checking
- op - the operator of the condition
- strVal - the value that we want to compare against (represented as a string)
*/
int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE]) {
    int srcRelId = OpenRelTable::getRelId(srcRel);      // we'll implement this later
    if (srcRelId == E_RELNOTOPEN) {
        return E_RELNOTOPEN;
    }

    AttrCatEntry attrCatEntry;
    
    // get the attribute catalog entry for attr, using AttrCacheTable::getAttrcatEntry()
    //    return E_ATTRNOTEXIST if it returns the error
    int result = AttrCacheTable::getAttrCatEntry(srcRelId, attr, &attrCatEntry);

    if (result != SUCCESS)
        return E_ATTRNOTEXIST;

    /*** Convert strVal (string) to an attribute of data type NUMBER or STRING ***/
    int type = attrCatEntry.attrType;
    Attribute attrVal;
    if (type == NUMBER) {
        if (isNumber(strVal)) {       // the isNumber() function is implemented below
            attrVal.nVal = atof(strVal);
        } else {
            return E_ATTRTYPEMISMATCH;
        }
    } else if (type == STRING) {
        strcpy(attrVal.sVal, strVal);
    }

    /*** Selecting records from the source relation ***/

      // Before calling the search function, reset the search to start from the first hit
    // using RelCacheTable::resetSearchIndex()
    //RelCacheTable::resetSearchIndex(srcRelId);

    RelCatEntry relCatEntry;
    // get relCatEntry using RelCacheTable::getRelCatEntry()
    RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);
    int src_nAttrs = relCatEntry.numAttrs;

    char attr_names[src_nAttrs][ATTR_SIZE];
    int attr_types[src_nAttrs];

    AttrCatEntry attrCatBuf;
    for (int i = 0; i < src_nAttrs; ++i){
        AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatBuf);
        strcpy(attr_names[i], attrCatBuf.attrName);
        attr_types[i] = attrCatBuf.attrType;
    }

    int retValCreateRel = Schema::createRel(targetRel, src_nAttrs, attr_names, attr_types);

    if (retValCreateRel != SUCCESS){
        return retValCreateRel;
    }

    int retValopenRel = OpenRelTable::openRel(targetRel);

    if (retValopenRel < 0){
        Schema::deleteRel(targetRel);
        return retValopenRel;
    }

    int targetRelId = OpenRelTable::getRelId(targetRel);
    RelCacheTable::resetSearchIndex(targetRelId);

    Attribute record[src_nAttrs];

    RelCacheTable::resetSearchIndex(targetRelId);
    
    while (BlockAccess::search(srcRelId, record, attr, attrVal, op) == SUCCESS){
        int retValinsert = BlockAccess::insert(targetRelId, record);

        if (retValinsert != SUCCESS){
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return retValinsert;
        }
    }

    Schema::closeRel(targetRel);

    return SUCCESS;
    /************************
    The following code prints the contents of a relation directly to the output
    console. Direct console output is not permitted by the actual the NITCbase
    specification and the output can only be inserted into a new relation. We will
    be modifying it in the later stages to match the specification.
    ************************/

    // printf("|");
    // for (int i = 0; i < relCatEntry.numAttrs; ++i) {
    //     AttrCatEntry attrCatEntry;
    //     // get attrCatEntry at offset i using AttrCacheTable::getAttrCatEntry()
    //     AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);
    //     printf(" %s |", attrCatEntry.attrName);
    // }
    // printf("\n");

    // while (true) {
    //     RecId searchRes = BlockAccess::linearSearch(srcRelId, attr, attrVal, op);

    //     if (searchRes.block != -1 && searchRes.slot != -1) {

    //       // get the record at searchRes using BlockBuffer.getRecord
    //         RecBuffer BlockBuffer(searchRes.block);

    //         struct HeadInfo head;
    //         BlockBuffer.getHeader(&head);
    //         Attribute recordBuffer[head.numAttrs];
    //         BlockBuffer.getRecord(recordBuffer, searchRes.slot);

    //         // print the attribute values in the same format as above
    //         AttrCatEntry attrCatEntry;
    //         for (int i = 0; i < head.numAttrs; ++i){
    //             AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);
    //             if (attrCatEntry.attrType == NUMBER){
    //                 printf(" %d |", (int)recordBuffer[i].nVal);
    //             }

    //             else{
    //                 printf(" %s |", recordBuffer[i].sVal);
    //             }
    //         }
    //         printf("\n");
    //     } else {
    //         // (all records over)
    //         break;
    //     }
    // }

    // return SUCCESS;
}

//few errors that I made:
//1. incorrect use of malloc: something like malloc(sizeof() * sizeof()). should have been malloc(sizeof() * <some variable>)
//2. using a single getAttrCatEntry method. Instead, two methods of the same name but different arguments should have been made
//3. Using the wrong argument for the method getAttrCatEntry. Got confused because of same method name

int Algebra::insert(char relName[ATTR_SIZE], int nAttrs, char record[][ATTR_SIZE]){

    printf("Entered Algebra::insert()\n");

    if (!strcmp(relName, ATTRCAT_RELNAME) || !strcmp(relName, RELCAT_RELNAME)){
        return E_NOTPERMITTED;
    }
    printf("Before getRElId in Algebra::insert()\n");
    int relId = OpenRelTable::getRelId(relName);
    printf("After getRelId\n");
    if (relId == E_RELNOTOPEN){
        return E_RELNOTOPEN;
    }

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);
    printf("just after getRElCatEntry in Algebra\n");
    if (relCatEntry.numAttrs != nAttrs){
        return E_NATTRMISMATCH;
    }

    union Attribute recordValues[nAttrs];
    printf("Before for loop in algebra\n");
    for (int i = 0; i < nAttrs; ++i){
        printf("%d th iteration\n", i);
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(relId, i, &attrCatEntry);

        int type = attrCatEntry.attrType;

        if (type == NUMBER){
            if (isNumber(record[i])){
                printf("is a Number and value is %s", record[i]);
                recordValues[i].nVal = atof(record[i]);
            }
            else{
                return E_ATTRTYPEMISMATCH;
            }
        }

        else if (type == STRING){
            printf("is a string and value is %s\n", record[i]);
            strcpy(recordValues[i].sVal, record[i]);
        }
    }
    printf("Exited for loop\n");
    int retVal = BlockAccess::insert(relId, recordValues);
    printf("All good in Algebra Insert()\n");
    return retVal;
}

int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE]){
    int srcRelId = OpenRelTable::getRelId(srcRel);      // we'll implement this later
    if (srcRelId == E_RELNOTOPEN) {
        return E_RELNOTOPEN;
    }

    RelCatEntry relCatEntry;
    
    // get the attribute catalog entry for attr, using AttrCacheTable::getAttrcatEntry()
    //    return E_ATTRNOTEXIST if it returns the error
    int result = RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);

    if (result != SUCCESS)
        return E_ATTRNOTEXIST;

    int src_nAttrs = relCatEntry.numAttrs;

    char attr_names[src_nAttrs][ATTR_SIZE];
    int attr_types[src_nAttrs];

    AttrCatEntry attrCatBuf;
    for (int i = 0; i < src_nAttrs; ++i){
        AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatBuf);
        strcpy(attr_names[i], attrCatBuf.attrName);
        attr_types[i] = attrCatBuf.attrType;
    }

    int retValCreateRel = Schema::createRel(targetRel, src_nAttrs, attr_names, attr_types);

    if (retValCreateRel != SUCCESS){
        return retValCreateRel;
    }

    int retValopenRel = OpenRelTable::openRel(targetRel);

    if (retValopenRel < 0){
        Schema::deleteRel(targetRel);
        return retValopenRel;
    }

    int targetRelId = OpenRelTable::getRelId(targetRel);
    RelCacheTable::resetSearchIndex(srcRelId);

    Attribute record[src_nAttrs];
    
    while (BlockAccess::project(srcRelId, record) == SUCCESS){
        int retValinsert = BlockAccess::insert(targetRelId, record);

        if (retValinsert != SUCCESS){
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return retValinsert;
        }
    }

    Schema::closeRel(targetRel);

    return SUCCESS;
}

int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], int target_nAttrs, char tar_Attrs[][ATTR_SIZE]){
    int srcRelId = OpenRelTable::getRelId(srcRel);      // we'll implement this later
    if (srcRelId == E_RELNOTOPEN) {
        return E_RELNOTOPEN;
    }

    /*** Selecting records from the source relation ***/

      // Before calling the search function, reset the search to start from the first hit
    // using RelCacheTable::resetSearchIndex()
    //RelCacheTable::resetSearchIndex(srcRelId);

    RelCatEntry relCatEntry;
    // get relCatEntry using RelCacheTable::getRelCatEntry()
    RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);
    int src_nAttrs = relCatEntry.numAttrs;

    int attr_offset[target_nAttrs];
    int attr_types[target_nAttrs];

    AttrCatEntry attrCatBuf;
    for (int i = 0; i < target_nAttrs; ++i){
        int retgetAttrCatEntry = AttrCacheTable::getAttrCatEntry(srcRelId, tar_Attrs[i], &attrCatBuf);
        if (retgetAttrCatEntry != SUCCESS){
            return retgetAttrCatEntry;
        }
        attr_offset[i] = attrCatBuf.offset;
        attr_types[i] = attrCatBuf.attrType;
    }

    int retValCreateRel = Schema::createRel(targetRel, src_nAttrs, tar_Attrs, attr_types);

    if (retValCreateRel != SUCCESS){
        return retValCreateRel;
    }

    int retValopenRel = OpenRelTable::openRel(targetRel);

    if (retValopenRel < 0){
        Schema::deleteRel(targetRel);
        return retValopenRel;
    }

    int targetRelId = OpenRelTable::getRelId(targetRel);
    RelCacheTable::resetSearchIndex(targetRelId);

    Attribute record[src_nAttrs];
    
    while (BlockAccess::project(srcRelId, record) == SUCCESS){

        Attribute proj_record[target_nAttrs];

        for (int j = 0; j < target_nAttrs; ++j){
            proj_record[j] = record[attr_offset[j]];
        }

        int retValinsert = BlockAccess::insert(targetRelId, proj_record);

        if (retValinsert != SUCCESS){
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return retValinsert;
        }
    }

    Schema::closeRel(targetRel);

    return SUCCESS;
}
