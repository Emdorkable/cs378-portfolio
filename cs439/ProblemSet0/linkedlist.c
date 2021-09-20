#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
typedef struct linked_block_struct {
    int data;
    struct linked_block_struct *prev;
    struct linked_block_struct *next;
}   linked_block_t;

linked_block_t *head = NULL;
void list_sort() {
    linked_block_t *curr = head;
    if (curr->next == NULL) {
        return;
    }
    int sorted = 1;
    while (sorted) {
        while (curr->data < curr->next->data) {
            curr = curr->next;
            if (curr->next == NULL) {
                sorted = 0;
                return;
            }
        }
        int temp = curr->next->data;
        curr->next->data = curr->data;
        curr->data = temp;
    }
}
void list_add(int num) {
    linked_block_t *curr = malloc(sizeof(linked_block_t));
    int head_is_null = head == NULL;
    curr->data = num;
    curr->next = head; 
    head = curr;
    list_sort();
}
void list_remove(int num) {
    linked_block_t *curr = head;
    linked_block_t *prev_curr = NULL;
    while (curr != NULL && curr->data != num) {
        if (curr->next == NULL) {
            return;
        }
        prev_curr = curr;
        curr = curr->next;
    }
    linked_block_t *temp = curr;
    if (curr->next == NULL) {
        free(temp);
        return;
    }
    else if (curr == head) {
        curr = head->next;
        head = curr;
    }
    else {
        prev_curr->next = curr->next;
        if (curr->prev != NULL)
            curr->next->prev = curr->prev;
    }
    free(temp);
}

void print_list() {
    linked_block_t *curr = head;
    printf(" ");
     while (curr->next != NULL) {
        printf("%d", curr->data);
        printf(" ");
        curr = curr->next;
     }
     printf("%d", curr->data);
}

int main() {
    list_add(1);
    list_add(3);
    list_add(4);
    list_add(2);
    list_add(5);
    print_list();
    list_remove(1);
    print_list();
    list_remove(3);
    print_list();
    list_remove(2);
    print_list();
    list_remove(5);
    print_list();
    list_remove(4);
    
    return 0;
}