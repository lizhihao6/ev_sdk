#include <stdlib.h> 
#include <string.h> 
#include "list.h" 
list *make_list() { 
    list *l = malloc(sizeof(list)); 
    l->size = 0; 
    l->front = 0; 
    l->back = 0; 
    return l; 
} 
   
void *list_pop(list *l){ 
    if(!l->back) return 0; 
    node *b = l->back; 
    void *val = b->val; 
    l->back = b->prev; 
    if(l->back) l->back->next = 0; 
    free(b); 
    --l->size; 
   return val; 
 } 

void list_insert(list *l, void *val) { 
    node *new = malloc(sizeof(node)); 
    new->val = val; 
    new->next = 0; 
    if(!l->back){ 
        l->front = new; 
        new->prev = 0; 
    }else{ 
        l->back->next = new; 
        new->prev = l->back; 
    } 
    l->back = new; 
    ++l->size; 
 } 

void free_node(node *n) { 
    node *next; 
    while(n) { 
    next = n->next; 
    free(n); 
    n = next; 
    } 
} 

void free_list(list *l) { 
    free_node(l->front); 
    free(l); 
 } 

void free_list_contents(list *l) 
{ 
    node *n = l->front; 
    while(n){ 
        free(n->val); 
        n = n->next; 
    } 
 } 

void **list_to_array(list *l) 
{ 
    void **a = calloc(l->size+1, sizeof(void*)); 
    int count = 0; 
    node *n = l->front; 
    while(n){ 
        a[count++] = n->val; 
        n = n->next; 
    } 
    a[count++] = NULL;
    return a; 
} 

list *get_paths(char *filename) 
{ 
    char *path; 
    FILE *file = fopen(filename, "r"); 
    // if(!file) file_error(filename); 
    list *lines = make_list(); 
    while((path=fgetl(file))){ 
        list_insert(lines, path); 
    } 
    fclose(file); 
    return lines; 
} 

char **get_labels(char *filename){ 
    list *plist = get_paths(filename); 
    char **labels = (char **)list_to_array(plist);
    free_list(plist); 
    return labels; 
} 

char *fgetl(FILE *fp){
    if(feof(fp)) return 0;
    size_t size = 512;
    char *line = malloc(size*sizeof(char));
    if(!fgets(line, size, fp)){
        free(line);
        return 0;
    }

    size_t curr = strlen(line);

    while((line[curr-1] != '\n') && !feof(fp)){
        if(curr == size-1){
            size *= 2;
            line = realloc(line, size*sizeof(char));
            if(!line) {
                printf("%ld\n", size);
                // malloc_error();
            }
        }
        size_t readsize = size-curr;
        if(readsize > INT_MAX) readsize = INT_MAX-1;
        fgets(&line[curr], readsize, fp);
        curr = strlen(line);
    }
    if(line[curr-1] == '\n') line[curr-1] = '\0';

    return line;
}