#include <iostream>
#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"

using namespace std ;

int main(int argc, char *argv[]) {
    /* Initialize the Run Copy of Disk */
    Disk disk_run;
    StaticBuffer buffer;
    OpenRelTable cache;

    FrontendInterface::handleFrontend( argc, argv);

    // for(int i = 0; i <= 1; i++) {
    //     RelCatEntry relCatEntry ;

    //     RelCacheTable::getRelCatEntry(i,&relCatEntry);
    //     printf("Relation : %s\n",relCatEntry.relName);

    //     for(int j = 0; j < relCatEntry.numAttrs; j++) {
    //         AttrCatEntry attrCatEntry ;

    //         AttrCacheTable::getAttrCatEntry(i,j,&attrCatEntry);
    //         const char * attrType = (attrCatEntry.attrType == NUMBER) ? "NUM" : "STR" ;

    //         printf("  %s: %s\n", attrCatEntry.attrName,attrType ) ;
    //     }
    // }

    return 0 ;

    // create objects for the relation catalog and attribute catalog
    RecBuffer relCatBuffer(RELCAT_BLOCK);

    HeadInfo relCatHeader ;

    relCatBuffer.getHeader(&relCatHeader) ;

    //cout << relCatHeader.numEntries << endl ;

    for(int i = 0; i < relCatHeader.numEntries; i++) {

        Attribute relCatRecord[RELCAT_NO_ATTRS] ;
        relCatBuffer.getRecord(relCatRecord,i);

        printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);

        int currBlock = ATTRCAT_BLOCK ;

        while(currBlock != -1) {

            RecBuffer attrCatBuffer(currBlock);
            HeadInfo attrCatHeader ;

            attrCatBuffer.getHeader(&attrCatHeader) ;
            
            for(int j=0; j < attrCatHeader.numEntries; j++) {

            Attribute attrCatRecord[ATTRCAT_NO_ATTRS] ;
            attrCatBuffer.getRecord(attrCatRecord,j);

            if(strcmp(relCatRecord[RELCAT_REL_NAME_INDEX].sVal ,attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal) == 0) {
                const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR" ;
                if(strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, "Students") == 0 &&
                strcmp(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, "Class") == 0) {
                const char * s = "Batch" ;
                
                strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,s);
                }

                printf("  %s: %s\n",attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,attrType);
            }
        }
        printf("\n");
        currBlock = attrCatHeader.rblock ;
      }
      
    }

    return 0 ;

  // unsigned char buffer[BLOCK_SIZE];
  // Disk::readBlock(buffer,7000);

  // char message[] = "hello" ;
  // memcpy(buffer + 20, message, 6);
  // Disk::writeBlock(buffer,7000);

  // unsigned char buffer2[BLOCK_SIZE];
  // char message2[6] ;

  // Disk::readBlock(buffer2,7000);
  // memcpy(message2,buffer2 + 20,6);

  // cout << message2 << "\n" ;  

  // unsigned char showBufferContent[BLOCK_SIZE] ;
  // Disk::readBlock(showBufferContent,4);

  // for(int i=0;i<BLOCK_SIZE;i++) {
  //   cout << (int)showBufferContent[i] << " " ;
  // }

  // //BMAP = 4, RECORD_BLOCK = 0, UNUSED_BLOCK = 3

  // return 0;
  // return FrontendInterface::handleFrontend(argc, argv);
}