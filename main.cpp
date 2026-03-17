#include "Buffer/StaticBuffer.h"
#include "Buffer/BlockBuffer.h"
#include "define/constants.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>
#include <cstdio>
#include <cstring>


int stage2(int argc, char *argv[]) {
    Disk disk_run;
    RecBuffer relCatBuffer(RELCAT_BLOCK);
    HeadInfo relCatHeader;
    relCatBuffer.getHeader(&relCatHeader);

    int number_of_relations = relCatHeader.numEntries;

    for (int i = 0; i < number_of_relations; i++) {

        Attribute relCatRecord[RELCAT_NO_ATTRS];
        relCatBuffer.getRecord(relCatRecord, i);

        printf("Relation: %s\n",
               relCatRecord[RELCAT_REL_NAME_INDEX].sVal);

        int curr_block = ATTRCAT_BLOCK;

        while (curr_block != -1) {

            RecBuffer attrCatBuffer(curr_block);
            HeadInfo currHead;
            attrCatBuffer.getHeader(&currHead);

            for (int j = 0; j < currHead.numEntries; j++) {

                Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
                attrCatBuffer.getRecord(attrCatRecord, j);

                if (strcmp(relCatRecord[RELCAT_REL_NAME_INDEX].sVal,
                           attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal) == 0) {

                    const char *attrType =
                        (attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER)
                        ? "NUM" : "STR";

                    printf("  %s: %s\n",
                           attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,
                           attrType);
                }
            }

            curr_block = currHead.rblock;
        }

        printf("\n");
    }

    return 0;
}

// int main() {
//     Disk disk_run;

//     int found = 0;

//     RecBuffer relCatBuffer(RELCAT_BLOCK);
//     HeadInfo relCatHeader;
//     relCatBuffer.getHeader(&relCatHeader);

//     int num_relations = relCatHeader.numEntries;

//     for (int i = 0; i < num_relations && !found; i++) {

//         Attribute relCatRecord[RELCAT_NO_ATTRS];
//         relCatBuffer.getRecord(relCatRecord, i);

//         if (strcmp(relCatRecord[RELCAT_REL_NAME_INDEX].sVal, "Students") == 0) {

//             int curr_block = ATTRCAT_BLOCK;

//             while (curr_block != -1 && !found) {

//                 RecBuffer attrCatBuffer(curr_block);
//                 HeadInfo attrHead;
//                 attrCatBuffer.getHeader(&attrHead);


//                 for (int j = 0; j < attrHead.numEntries && !found; j++) {

//                     Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
//                     attrCatBuffer.getRecord(attrCatRecord, j);


//                     if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, "RANDOM") == 0 &&
//                         strcmp(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, "computername") == 0) {

//                         strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,"anothername");

//                         attrCatBuffer.setRecord(attrCatRecord, j);

//                         found = 1;
//                     }
//                 }

//                 curr_block = attrHead.rblock;
//             }
//         }
//     }

//     if (found)
//         printf("Attribute renamed successfully.\n");
//     else
//         printf("Attribute not found.\n");

//     return 0;
// }

int stage3_buffer(int argc, char *argv[]){
    Disk disk_run;
    StaticBuffer buffer;
    RecBuffer relCatBuffer(RELCAT_BLOCK);
    HeadInfo relCatHeader;
    relCatBuffer.getHeader(&relCatHeader);

    int number_of_relations = relCatHeader.numEntries;

    for (int i = 0; i < number_of_relations; i++) {

        Attribute relCatRecord[RELCAT_NO_ATTRS];
        relCatBuffer.getRecord(relCatRecord, i);

        printf("Relation: %s\n",
               relCatRecord[RELCAT_REL_NAME_INDEX].sVal);

        int curr_block = ATTRCAT_BLOCK;

        while (curr_block != -1) {

            RecBuffer attrCatBuffer(curr_block);
            HeadInfo currHead;
            attrCatBuffer.getHeader(&currHead);

            for (int j = 0; j < currHead.numEntries; j++) {

                Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
                attrCatBuffer.getRecord(attrCatRecord, j);

                if (strcmp(relCatRecord[RELCAT_REL_NAME_INDEX].sVal,
                           attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal) == 0) {

                    const char *attrType =
                        (attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER)
                        ? "NUM" : "STR";

                    printf("  %s: %s\n",
                           attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,
                           attrType);
                }
            }

            curr_block = currHead.rblock;
        }

        printf("\n");
    }

    return 0;
}


int stage3_cache(int argc,char *argv[]){
    Disk disk_run;
    StaticBuffer buffer;
    OpenRelTable cache;

    for(int relId = 0; relId < 3; relId++){
        RelCatEntry relCat;
        if(RelCacheTable::getRelCatEntry(relId, &relCat) != SUCCESS)
            continue;

        printf("Relation: %s\n", relCat.relName);

        for(int i = 0; i < relCat.numAttrs; i++){
            AttrCatEntry attrCat;
            if(AttrCacheTable::getAttrCatEntry(relId, i, &attrCat) != SUCCESS)
                continue;

            const char* type =
                (attrCat.attrType == 1) ? "STR" : "NUM";
            printf("  %s: %s\n", attrCat.attrName, type);
        }
        printf("\n");
    }

    return 0;

}

int main(int argc, char *argv[]){
    Disk disk_run;
    StaticBuffer buffer;
    OpenRelTable cache;

    return FrontendInterface::handleFrontend(argc,argv);
}

