#include <stdio.h>

extern int mm_init (void);
extern void *mm_malloc (size_t size);
extern void mm_free (void *ptr);
extern void *mm_realloc(void *ptr, size_t size);


/* 
 * Teams enter their team name, 
 * personal names and login IDs in a struct of this
 * type in their mm.c file.
 */
typedef struct {
    char *teamname; /* ID1+ID2 or ID1 */
    char *name1;    /* full name of first member */
    char *roll1;      /* login ID of first member */
    char *name2;    /* full name of second member (if any) */
    char *roll2;      /* login ID of second member */
    char *name3;    /* full name of first member */
    char *roll3;      /* login ID of first member */
    char *name4;    /* full name of second member (if any) */
    char *roll4;      /* login ID of second member */
    char *name5;    /* full name of first member */
    char *roll5;      /* login ID of first member */
} team_t;

extern team_t team;

