#include <stdio.h>
#include <stdlib.h>


struct listelement {    //listelement consists of value and pointer on next listelement
    int value;
    struct listelement *next;
};

/*I----> +--------------------------------------------------------------------+
         | FYI: Wenn eine Variable als `static' deklariert wird, ist es nicht |
         | notwendig = NULL anzugeben.                                        |
         +-------------------------------------------------------------------*/
static struct listelement *head = NULL;    // head element pointer

int insertElement(int value) {

    if (value < 0){
            return -1;
    }

    if (head == NULL){    // initialize head when function is called for the first time
        head = (struct listelement*) malloc(sizeof(struct listelement));
        if (head == NULL) return -1;    // error occured in malloc
        head->value = value;
        head->next = NULL;

        return value;
    }

    // head element does already exist
    struct listelement *element = head;


    while (element->next != NULL){    // iterate list and check all elements
        if (element->value == value){
            return -1;
        }
        element = element->next;
    }
    if (element->value == value) return -1;  // check last element, too

    // value not in list -> add
    struct listelement *next = (struct listelement*) malloc(sizeof(struct listelement));
    if (next == NULL) return -1; // error handling malloc

    next->value = value;
    next->next = NULL;

    element->next = next;

    return value;

}

int removeElement(void) {
    if (head == NULL) return -1; // if no value has been added yet

    int value = head->value;  //save value in order to make free possible
    struct listelement *tmp = head; // for saving the head listelement to free it later on
    head = head->next;
    free(tmp);
    return value;
}

int main (int argc, char* argv[]) {
        printf("insert 47: %d\n", insertElement(47));
    printf("insert 11: %d\n", insertElement(11));
    printf("insert 23: %d\n", insertElement(23));
    printf("insert 11: %d\n", insertElement(11));
    printf("insert 13: %d\n", insertElement(13));
    printf("insert 13: %d\n", insertElement(13));
    printf("insert -10: %d\n", insertElement(-10));
    printf("insert -10: %d\n", insertElement(-10));
    printf("insert 0: %d\n", insertElement(0));

    printf("remove: %d\n", removeElement());
    printf("remove: %d\n", removeElement());


    exit(EXIT_SUCCESS);
}

/*I----> +--------------------------------------------------------------------+
         | Es sieht alles gut umgesetzt aus, gut gemacht!                     |
         +-------------------------------------------------------------------*/
/*P----> +--------------------------------------------------------------------+
         | Punkatbzug in dieser Datei: 0.0 Punkte                             |
         +-------------------------------------------------------------------*/
