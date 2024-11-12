#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cidcsvmanager.h"
#include "main.h"

static LEAFLIST leaflist;

LEAFLIST sbsh_leaflist[6];
LEAFLIST scbr_leaflist[6];
LEAFLIST oltc_leaflist[6];
LEAFLIST siml_leaflist[6];
LEAFLIST spdc_leaflist[6];

extern ds61850IfcInfo* g_p61850IfcInfo;

int ReadFile(char *fname)
{
    int idx = 0;
    __ssize_t read;
    size_t len;
    char line[200];
    FILE *fp = fopen(fname, "r");
    if(!fp)
    {
        return -1;
    }

    while (fgets(line, 200, fp)) 
    {
        InsertLine(line, &leaflist);
        //printf("%s\n", line);
        // idx++;
        // if(idx > 100)
        // {
        //     break;
        // }
    }

    fclose(fp);

    return 1;
}

void Init(void)
{
    leaflist.count = 0;
}

void Free(LEAFLIST *leaf_list)
{
    int idx;
    for(idx=0;idx<leaf_list->count;idx++)
    {
        free(leaf_list->lines[idx]);
    }
    leaf_list->count = 0;
}

void SearchLeafs(char *keyword, LEAFLIST *leaf_list)
{
    int idx;
    char *p;
    Free(leaf_list);
    for(idx=0;idx<leaflist.count;idx++)
    {
        p = strstr(leaflist.lines[idx], keyword);
        if(p)
        {
            InsertLine(leaflist.lines[idx], leaf_list);
        }
    }
}

void SearchLeafsX(char *keyword, LEAFLIST *leaf_list)
{
    int idx;
    char *p;
    Free(leaf_list);
    for(idx=0;idx<g_p61850IfcInfo->s32leafnum;idx++)
    {
    	if(g_p61850IfcInfo->p61850mappingtbl[idx].ps8leafname)
    	{
	        p = strstr((const char *)g_p61850IfcInfo->p61850mappingtbl[idx].ps8leafname, keyword);
	        if(p)
	        {        
	            InsertLine(g_p61850IfcInfo->p61850mappingtbl[idx].ps8leafname, leaf_list);
	        }
    	}
    }
}

void _SearchLeafsX(char *keyword, LEAFLIST *leaf_list)
{
    int idx;
    char *p;
    Free(leaf_list);
    for(idx=0;idx<g_p61850IfcInfo->s32leafnum;idx++)
    {
    	if(g_p61850IfcInfo->p61850mappingtbl[idx].ps8leafname)
    	{
	        p = strstr((const char *)g_p61850IfcInfo->p61850mappingtbl[idx].ps8leafname, keyword);
	        if(p)
	        {        
	            InsertLine(g_p61850IfcInfo->p61850mappingtbl[idx].ps8leafname, leaf_list);
	        }
    	}
    }
}

void PrintLeafs(LEAFLIST *leaf_list)
{
    int idx;
    printf("\n\n Number of Leafs %d \n\n", leaf_list->count);
    for(idx=0;idx<leaf_list->count;idx++)
    {
        printf("%s", leaf_list->lines[idx]);
        printf("\n");
    }
}

void GetKeyFromLeafString(char *leaf_string, char *leaf_key, char *_prefix)
{
    int lidxA;
    int idx2;
    int len = 0;
    int len2 = 0;
    //printf("\n\nlen = %d\n\n", len);
    len = strlen(leaf_string);
    //printf("\n\nlen = %d\n\n", len);
    if(len == 0)
    {
        return;
    }

    //printf("      In Func leaf_string size : %d - %s\n", len, leaf_string);
    sprintf(leaf_key, "%s", leaf_string);
    
}

// void GetKeyFromLeafString(char *leaf_string, char *leaf_key, char *_prefix)
// {
//     int lidxA;
//     int idx2;
//     int len = 0;
//     int len2 = 0;
//     //printf("\n\nlen = %d\n\n", len);
//     len = strlen(leaf_string);
//     //printf("\n\nlen = %d\n\n", len);
//     if(len == 0)
//     {
//         return;
//     }

//     //printf("      In Func leaf_string size : %d - %s\n", len, leaf_string);
    
//     for(lidxA=0;lidxA<len;lidxA++)
//     {
//         if(leaf_string[lidxA] == '/')
//         {
            
//             //leaf_string[len-1] = 0;
//             sprintf(leaf_key, "%s%s", _prefix, &leaf_string[lidxA]);

//             //printf("In Func leaf_key %s\n", leaf_key);

//             len2 = strlen(leaf_key);
//             for(idx2=2;idx2<len2;idx2++)
//             {
//                 if(leaf_key[idx2] == '\n')
//                 {
//                     leaf_key[idx2] = 0;
//                 }
//             }
//             return;
//         }
//     }
// }

void PrintFiltered(void)
{
    int idx;
    char *p;

    printf("\n\n Number of Leafs %d \n\n", leaflist.count);

    for(idx=0;idx<leaflist.count;idx++)
    {
        p = strstr(leaflist.lines[idx], "SBSH1");
        if(p)
        {
            printf("%s", leaflist.lines[idx]);
            printf("\n");
        }
    }
}

void InsertLine(char *line, LEAFLIST *leaf_list)
{
    strcpy(leaf_list->lines[leaf_list->count], line);
    leaf_list->count++;
}
