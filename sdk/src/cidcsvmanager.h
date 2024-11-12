#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <sys/time.h>
#include <stdint.h>

typedef struct __LEAFLIST
{
    char lines[5000][512];
    int count;
}LEAFLIST;

void InsertLine(char *line, LEAFLIST *leaf_list);
void SearchLeafs(char *keyword, LEAFLIST *leaf_list);
void SearchLeafsX(char *keyword, LEAFLIST *leaf_list);
void PrintFiltered(void);
void PrintLeafs(LEAFLIST *leaf_list);
void GetKeyFromLeafString(char *leaf_string, char *leaf_key, char *_prefix);
void Free(LEAFLIST *leaf_list);
void Init(void);
int ReadFile(char *fname);

#ifdef __cplusplus
}
#endif
