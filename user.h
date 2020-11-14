#ifndef USER_H
#define USER_H

#define NAMESZ 64

typedef struct _node node;
typedef node* nodePointer;

nodePointer createNode(int fd, char name[], int color);
int isEmpty(nodePointer list);
void insertCircularList(nodePointer list, nodePointer node);
void deleteCircularList(nodePointer node);

#endif