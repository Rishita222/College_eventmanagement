#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Each participant is a node in a linked list.
typedef struct Participant {
    char id[40], name[80], department[80];
    struct Participant *next;
} Participant;

// The waiting list is a queue: front leaves first, rear joins last.
typedef struct {
    int id, capacity, count;
    char name[80], category[40];
    Participant *registered, *front, *rear;
} Event;

Event events[20];
int eventCount = 0;

void copyText(char *dest, const char *source, int size) {
    snprintf(dest, size, "%s", source);
}

Participant *newParticipant(const char *id, const char *name, const char *department) {
    Participant *p = malloc(sizeof(Participant));
    if (p == NULL) { printf("Memory allocation failed.\n"); exit(1); }
    copyText(p->id, id, sizeof(p->id));
    copyText(p->name, name, sizeof(p->name));
    copyText(p->department, department, sizeof(p->department));
    p->next = NULL;
    return p;
}

// Append a confirmed participant to the linked list.
void addConfirmed(Event *e, Participant *p) {
    Participant *last = e->registered;
    if (last == NULL) e->registered = p;
    else {
        while (last->next != NULL) last = last->next;
        last->next = p;
    }
    p->next = NULL;
    e->count++;
}

// Enqueue: add a participant at the rear of the waiting queue.
void enqueue(Event *e, Participant *p) {
    p->next = NULL;
    if (e->rear == NULL) e->front = p;
    else e->rear->next = p;
    e->rear = p;
}

// Linear search through the event array.
Event *findEvent(int id) {
    int i;
    for (i = 0; i < eventCount; i++)
        if (events[i].id == id) return &events[i];
    return NULL;
}

int contains(Participant *p, const char *id) {
    while (p != NULL) {
        if (strcmp(p->id, id) == 0) return 1;
        p = p->next;
    }
    return 0;
}

// The Python server sends the current temporary data through standard input.
// Fields are separated by |; the server checks the input first.
void loadData(void) {
    FILE *file = stdin;
    char line[512], kind[8], name[80], category[40], id[40], department[80];
    int eventId, capacity;
    Event *e;
    while (fgets(line, sizeof(line), file) != NULL) {
        if (sscanf(line, "%7[^|]|%d|%79[^|]|%39[^|]|%d", kind, &eventId, name, category, &capacity) == 5
            && strcmp(kind, "E") == 0 && eventCount < 20) {
            e = &events[eventCount++];
            e->id = eventId; e->capacity = capacity; e->count = 0;
            copyText(e->name, name, sizeof(e->name));
            copyText(e->category, category, sizeof(e->category));
            e->registered = e->front = e->rear = NULL;
        } else if (sscanf(line, "%7[^|]|%d|%39[^|]|%79[^|]|%79[^\n]", kind, &eventId, id, name, department) == 5) {
            e = findEvent(eventId);
            if (e != NULL && strcmp(kind, "R") == 0) addConfirmed(e, newParticipant(id, name, department));
            else if (e != NULL && strcmp(kind, "W") == 0) enqueue(e, newParticipant(id, name, department));
        }
    }
}

void sendDataToServer(void) {
    FILE *file = stderr; // The server keeps this data in memory for the next request.
    int i;
    Participant *p;
    for (i = 0; i < eventCount; i++) {
        Event *e = &events[i];
        fprintf(file, "E|%d|%s|%s|%d\n", e->id, e->name, e->category, e->capacity);
        for (p = e->registered; p != NULL; p = p->next)
            fprintf(file, "R|%d|%s|%s|%s\n", e->id, p->id, p->name, p->department);
        for (p = e->front; p != NULL; p = p->next)
            fprintf(file, "W|%d|%s|%s|%s\n", e->id, p->id, p->name, p->department);
    }
}

void showEvents(void) {
    int i;
    if (eventCount == 0) printf("No events yet. Add an event to begin.\n");
    for (i = 0; i < eventCount; i++) {
        Event *e = &events[i];
        Participant *p;
        printf("Event %d: %s (%s)\nCapacity: %d | Registered: %d | Available: %d\n",
               e->id, e->name, e->category, e->capacity, e->count, e->capacity - e->count);
        printf("Confirmed participants:\n");
        if (e->registered == NULL) printf("  None\n");
        for (p = e->registered; p != NULL; p = p->next)
            printf("  %s | %s | %s\n", p->id, p->name, p->department);
        printf("Waiting list (first to last):\n");
        if (e->front == NULL) printf("  None\n");
        for (p = e->front; p != NULL; p = p->next)
            printf("  %s | %s | %s\n", p->id, p->name, p->department);
        printf("\n");
    }
}

void cancel(Event *e, const char *id) {
    Participant *p = e->registered, *previous = NULL;
    while (p != NULL && strcmp(p->id, id) != 0) { previous = p; p = p->next; }
    if (p != NULL) {
        if (previous == NULL) e->registered = p->next;
        else previous->next = p->next;
        free(p); e->count--;
        printf("Registration cancelled. ");
        if (e->front != NULL) {
            // Dequeue: move the first waiting participant to the confirmed list.
            p = e->front; e->front = p->next;
            if (e->front == NULL) e->rear = NULL;
            addConfirmed(e, p);
            printf("%s is now confirmed.", p->name);
        }
        printf("\n");
        return;
    }
    previous = NULL; p = e->front;
    while (p != NULL && strcmp(p->id, id) != 0) { previous = p; p = p->next; }
    if (p == NULL) { printf("Participant not found.\n"); return; }
    if (previous == NULL) e->front = p->next;
    else previous->next = p->next;
    if (e->rear == p) e->rear = previous;
    free(p);
    printf("Removed from waiting list.\n");
}

// The local server passes one command at a time to this C program.
int main(int argc, char *argv[]) {
    Event *e;
    int capacity;
    loadData();
    if (argc == 2 && strcmp(argv[1], "list") == 0) showEvents();
    else if (argc == 5 && strcmp(argv[1], "add") == 0) {
        capacity = atoi(argv[4]);
        if (eventCount == 20) printf("Maximum 20 events reached.\n");
        else if (capacity < 1 || capacity > 100) printf("Capacity must be 1 to 100.\n");
        else {
            e = &events[eventCount]; e->id = eventCount + 1; e->capacity = capacity; e->count = 0;
            copyText(e->name, argv[2], sizeof(e->name)); copyText(e->category, argv[3], sizeof(e->category));
            e->registered = e->front = e->rear = NULL; eventCount++;
            printf("Event added with ID %d.\n", e->id);
        }
    } else if (argc == 6 && strcmp(argv[1], "register") == 0) {
        e = findEvent(atoi(argv[2]));
        if (e == NULL) printf("Event not found.\n");
        else if (contains(e->registered, argv[3]) || contains(e->front, argv[3])) printf("Participant ID already exists in this event.\n");
        else {
            Participant *p = newParticipant(argv[3], argv[4], argv[5]);
            if (e->count < e->capacity) { addConfirmed(e, p); printf("Registration confirmed.\n"); }
            else { enqueue(e, p); printf("Event is full; participant added to waiting list.\n"); }
        }
    } else if (argc == 4 && strcmp(argv[1], "cancel") == 0) {
        e = findEvent(atoi(argv[2]));
        if (e == NULL) printf("Event not found.\n");
        else cancel(e, argv[3]);
    } else printf("Invalid command.\n");
    sendDataToServer(); // Return the temporary state to Python.
    return 0;
}



