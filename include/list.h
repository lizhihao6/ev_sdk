#ifndef LIST_H
#define LIST_H
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
typedef struct node
{
    void *val;
    struct node *next;
    struct node *prev;
} node;
typedef struct _list
{
    int size;
    node *front;
    node *back;
} _list;
_list *make__list();
int list_find(_list *l, void *val);
void list_insert(_list *, void *);
void free_list_contents(_list *l);
_list *get_paths(const char *filename);
char **get_labels(const char *filename);
char *fgetl(FILE *fp);
#endif