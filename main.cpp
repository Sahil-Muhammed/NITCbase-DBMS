#include <iostream>
#include <cstring>
#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"

void Stage1(){
  unsigned char buffer[2048];
  Disk::readBlock(buffer, 0);

  for (int i = 0; i < 2048; ++i){
    printf("%u ", buffer[i]);
  }
}
void DisplayRecord(){
  // create objects for the relation catalog and attribute catalog
  RecBuffer relCatBuffer(RELCAT_BLOCK);
  RecBuffer attrCatBuffer(ATTRCAT_BLOCK);

  HeadInfo relCatHeader;
  HeadInfo attrCatHeader;

  // load the headers of both the blocks into relCatHeader and attrCatHeader.
  // (we will implement these functions later)
  relCatBuffer.getHeader(&relCatHeader);
  attrCatBuffer.getHeader(&attrCatHeader);

  for (int i = 0, slotIndex = 0; i < relCatHeader.numEntries; ++i) {

    Attribute relCatRecord[RELCAT_NO_ATTRS]; // will store the record from the relation catalog

    relCatBuffer.getRecord(relCatRecord, i);

    printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);

    for (int j = 0; j < relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal; ++j, ++slotIndex) {

      // declare attrCatRecord and load the attribute catalog entry into it
      Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

      attrCatBuffer.getRecord(attrCatRecord, slotIndex);

      if (strcmp(relCatRecord[RELCAT_REL_NAME_INDEX].sVal, attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal) == 0) {
        const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR";
        printf("  %s: %s\n", attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrType);
      }

      if (slotIndex == attrCatHeader.numSlots - 1){
        slotIndex = -1;
        attrCatBuffer = RecBuffer(attrCatHeader.rblock);
        attrCatBuffer.getHeader(&attrCatHeader);
      }
    }
    printf("\n");
  }
}

void ChangeAttributeName(const char* relationName, const char* prevAttrName, const char* newAttrName){
  RecBuffer attrCatBuffer(ATTRCAT_BLOCK);

  HeadInfo attrCatHeader;

  attrCatBuffer.getHeader(&attrCatHeader);

  for (int i = 0; i < attrCatHeader.numEntries; ++i){

    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

    attrCatBuffer.getRecord(attrCatRecord, i);

    if (strcmp(relationName, attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal) == 0 && strcmp(prevAttrName, attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal) == 0){
      strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newAttrName);
      attrCatBuffer.setRecord(attrCatRecord, i);
      printf("Updation done.\n");
      break;
    }

    if (i == attrCatHeader.numSlots - 1){
      i = -1;
      attrCatBuffer = RecBuffer(attrCatHeader.rblock);
      attrCatBuffer.getHeader(&attrCatHeader);
    }
  }
}

/*void Stage_3(int highRelId){
    for (int i = 0; i < highRelId; ++i){
      RelCatEntry relCatBuf;
      RelCacheTable::getRelCatEntry(i, &relCatBuf);
      printf("Relation name: %s\n", relCatBuf.relName);

      for (int j = 0; j < relCatBuf.numAttrs; ++j){
        AttrCatEntry attrCatBuf;
        AttrCacheTable::getAttrCatEntry(i, j, &attrCatBuf);
        const char* attrType = attrCatBuf.attrType == NUMBER ? "NUM" : "STR";
        printf(" %s: %s\n", attrCatBuf.attrName, attrType);
      }
      printf("\n");
    }
}*/
int main(int argc, char *argv[]) {
  Disk disk_run;
  StaticBuffer buffer;
  OpenRelTable cache;

  return FrontendInterface::handleFrontend(argc, argv);
  //Stage1();

  //DisplayRecord();

  //ChangeAttributeName("Students", "RollNumber", "SrNo");

  //DisplayRecord();
  //Stage_3(6);
  return 0;
}