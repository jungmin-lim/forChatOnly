#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "user.h"

struct _node{
    int fd;
    char name[NAMESZ];
    int color;
    int state;
    nodePointer next;
};

nodePointer createNode(int fd, char name[], int color){
    nodePointer newNode;
    
    newNode = (nodePointer)malloc(sizeof(*newNode));
    if(newNode == NULL){
        fprintf(stderr, "error while allocating memory\n");
        exit(EXIT_FAILURE);
    }

    newNode->fd = fd;
    strcpy(newNode->name, name); 
    newNode->color = color;
    newNode->state = 0;
    newNode->next = newNode;
    
    return newNode;
}

int isEmpty(nodePointer list){
    if( (!list) || (list->next == list)){
        return 1;
    }
    return 0;
}

void insertCircularList(nodePointer list, nodePointer node){
    node->next = list->next;
    list->next = node;

    return;
}

void deleteCircularList(nodePointer node){
    nodePointer temp;
    temp = node;

    if(isEmpty(node)) return;
    do{
        temp = temp->next;
    }while(temp != node);

    temp->next = node->next;
    free(node);

    return;
}

