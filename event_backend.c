#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_EVENTS 20
#define MAX_HISTORY 50

// ============================================================================
// DATA STRUCTURE DEFINITIONS
// ============================================================================

// 1. SINGLY LINKED LIST NODE: Each participant is a node in a linked list.
typedef struct Participant {
    char id[40], name[80], department[80];
    struct Participant *next;
} Participant;

// 2. EVENT STRUCTURE: Holds pointers to confirmed list and waiting queue.
typedef struct {
    int id, capacity, count;
    char name[80], category[40];
    Participant *registered; // Head of Singly Linked List (Confirmed)
    Participant *front;      // Front of FIFO Queue (Waitlist)
    Participant *rear;       // Rear of FIFO Queue (Waitlist)
} Event;

// 3. ARRAY: Fixed-size storage for college events
Event events[MAX_EVENTS];
int eventCount = 0;

// 4. ARRAY-BASED STACK: Fixed-size LIFO storage for recent activity history
typedef struct {
    char action[256];
} Activity;

Activity historyStack[MAX_HISTORY];
int stackTop = -1;

void copyText(char *dest, const char *source, int size) {
    snprintf(dest, size, "%s", source);
}

// ============================================================================
// 4. ARRAY-BASED STACK IMPLEMENTATION (LIFO)
// ============================================================================
int isEmpty(void) {
    return stackTop == -1;
}

int isFull(void) {
    return stackTop >= MAX_HISTORY - 1;
}

int size(void) {
    return stackTop + 1;
}

int push(const char *action) {
    if (isFull()) return 0;
    stackTop++;
    copyText(historyStack[stackTop].action, action, sizeof(historyStack[stackTop].action));
    return 1;
}

char *pop(void) {
    if (isEmpty()) return NULL;
    return historyStack[stackTop--].action;
}

const char *peek(void) {
    if (isEmpty()) return NULL;
    return historyStack[stackTop].action;
}

void display(void) {
    int i;
    if (isEmpty()) {
        printf("Activity History: No activities recorded yet.\n");
        return;
    }
    printf("Activity History (Latest first):\n");
    for (i = stackTop; i >= 0; i--) {
        printf("  [%d] %s\n", stackTop - i + 1, historyStack[i].action);
    }
}

// Internal helper for recording app activities, shifting oldest if full
void recordActivity(const char *action) {
    if (isFull()) {
        int i;
        for (i = 0; i < MAX_HISTORY - 1; i++) {
            historyStack[i] = historyStack[i + 1];
        }
        stackTop = MAX_HISTORY - 2;
    }
    push(action);
}

// ============================================================================
// 2. SINGLY LINKED LIST IMPLEMENTATION (Confirmed Participants)
// ============================================================================
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

int contains(Participant *p, const char *id) {
    while (p != NULL) {
        if (strcmp(p->id, id) == 0) return 1;
        p = p->next;
    }
    return 0;
}

// ============================================================================
// 3. FIFO QUEUE IMPLEMENTATION (Waiting List)
// ============================================================================
// Enqueue: add a participant at the rear of the waiting queue.
void enqueue(Event *e, Participant *p) {
    p->next = NULL;
    if (e->rear == NULL) e->front = p;
    else e->rear->next = p;
    e->rear = p;
}

// Dequeue: remove and return the participant at the front of the waiting queue.
Participant *dequeue(Event *e) {
    if (e->front == NULL) return NULL;
    Participant *p = e->front;
    e->front = p->next;
    if (e->front == NULL) e->rear = NULL;
    p->next = NULL;
    return p;
}

// ============================================================================
// 1. ARRAY OPERATIONS & SEARCH (College Events)
// ============================================================================
// Linear search through the event array.
Event *findEvent(int id) {
    int i;
    for (i = 0; i < eventCount; i++)
        if (events[i].id == id) return &events[i];
    return NULL;
}

int caseInsensitiveSubstring(const char *str, const char *sub) {
    if (!str || !sub) return 0;
    if (*sub == '\0') return 1;
    int i, j;
    for (i = 0; str[i] != '\0'; i++) {
        for (j = 0; sub[j] != '\0'; j++) {
            if (str[i + j] == '\0') break;
            if (tolower((unsigned char)str[i + j]) != tolower((unsigned char)sub[j])) break;
        }
        if (sub[j] == '\0') return 1;
    }
    return 0;
}

void printEventCard(const Event *e) {
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

void searchEvents(const char *query) {
    int i, found = 0;
    int queryId = atoi(query);
    printf("Search results for '%s':\n\n", query);
    for (i = 0; i < eventCount; i++) {
        Event *e = &events[i];
        if (caseInsensitiveSubstring(e->name, query) ||
            caseInsensitiveSubstring(e->category, query) ||
            (queryId > 0 && e->id == queryId)) {
            printEventCard(e);
            found++;
        }
    }
    if (found == 0) {
        printf("No matching events found.\n");
    } else {
        printf("Found %d matching event(s).\n", found);
    }
}

// ============================================================================
// TEMPORARY IN-MEMORY STATE TRANSFER (IPC via stdin / stderr)
// ============================================================================
// The Python server sends the current temporary data through standard input.
// Fields are separated by |; the server checks the input first.
void loadData(void) {
    FILE *file = stdin;
    char line[512], kind[8], name[80], category[40], id[40], department[80];
    int eventId, capacity;
    Event *e;
    while (fgets(line, sizeof(line), file) != NULL) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }
        if (len == 0) continue;

        if (sscanf(line, "%7[^|]|%d|%79[^|]|%39[^|]|%d", kind, &eventId, name, category, &capacity) == 5
            && strcmp(kind, "E") == 0 && eventCount < MAX_EVENTS) {
            e = &events[eventCount++];
            e->id = eventId; e->capacity = capacity; e->count = 0;
            copyText(e->name, name, sizeof(e->name));
            copyText(e->category, category, sizeof(e->category));
            e->registered = e->front = e->rear = NULL;
        } else if (sscanf(line, "%7[^|]|%d|%39[^|]|%79[^|]|%79[^\r\n]", kind, &eventId, id, name, department) == 5) {
            e = findEvent(eventId);
            if (e != NULL && strcmp(kind, "R") == 0) addConfirmed(e, newParticipant(id, name, department));
            else if (e != NULL && strcmp(kind, "W") == 0) enqueue(e, newParticipant(id, name, department));
        } else if (strncmp(line, "A|", 2) == 0) {
            push(line + 2);
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
    for (i = 0; i <= stackTop; i++) {
        fprintf(file, "A|%s\n", historyStack[i].action);
    }
}

void showEvents(void) {
    int i;
    if (eventCount == 0) {
        printf("No events yet. Add an event to begin.\n\n");
    } else {
        for (i = 0; i < eventCount; i++) {
            printEventCard(&events[i]);
        }
    }
    printf("----------------------------------------\n");
    display();
}

// ============================================================================
// EVENT CANCELLATION & AUTOMATIC WAITING-LIST PROMOTION
// ============================================================================
void cancel(Event *e, const char *id) {
    char act[256];
    Participant *p = e->registered, *previous = NULL;
    while (p != NULL && strcmp(p->id, id) != 0) { previous = p; p = p->next; }
    if (p != NULL) {
        char cancelledName[80];
        copyText(cancelledName, p->name, sizeof(cancelledName));
        if (previous == NULL) e->registered = p->next;
        else previous->next = p->next;
        free(p);
        e->count--;
        printf("Registration cancelled for %s (ID %s). ", cancelledName, id);
        snprintf(act, sizeof(act), "Cancelled confirmed: %s (%s) from Event %d", cancelledName, id, e->id);
        recordActivity(act);

        if (e->front != NULL) {
            // Dequeue: move the first waiting participant to the confirmed list.
            p = dequeue(e);
            char promotedName[80], promotedId[40];
            copyText(promotedName, p->name, sizeof(promotedName));
            copyText(promotedId, p->id, sizeof(promotedId));
            addConfirmed(e, p);
            printf("%s is now confirmed.", promotedName);
            snprintf(act, sizeof(act), "Promoted from waitlist: %s (%s) in Event %d", promotedName, promotedId, e->id);
            recordActivity(act);
        }
        printf("\n");
        return;
    }
    previous = NULL; p = e->front;
    while (p != NULL && strcmp(p->id, id) != 0) { previous = p; p = p->next; }
    if (p == NULL) {
        printf("Participant not found.\n");
        return;
    }
    char waitName[80];
    copyText(waitName, p->name, sizeof(waitName));
    if (previous == NULL) e->front = p->next;
    else previous->next = p->next;
    if (e->rear == p) e->rear = previous;
    free(p);
    printf("Removed %s (ID %s) from waiting list.\n", waitName, id);
    snprintf(act, sizeof(act), "Cancelled waitlist: %s (%s) from Event %d", waitName, id, e->id);
    recordActivity(act);
}

// ============================================================================
// MAIN: CLI COMMAND DISPATCHER (Invoked by Python server per request)
// ============================================================================
int main(int argc, char *argv[]) {
    Event *e;
    int capacity;
    loadData();
    if (argc == 2 && strcmp(argv[1], "list") == 0) {
        showEvents();
    } else if (argc == 5 && strcmp(argv[1], "add") == 0) {
        capacity = atoi(argv[4]);
        if (eventCount >= MAX_EVENTS) printf("Maximum 20 events reached.\n");
        else if (capacity < 1 || capacity > 100) printf("Capacity must be 1 to 100.\n");
        else {
            int maxId = 0, j;
            for (j = 0; j < eventCount; j++) {
                if (events[j].id > maxId) maxId = events[j].id;
            }
            e = &events[eventCount];
            e->id = maxId + 1;
            e->capacity = capacity;
            e->count = 0;
            copyText(e->name, argv[2], sizeof(e->name));
            copyText(e->category, argv[3], sizeof(e->category));
            e->registered = e->front = e->rear = NULL;
            eventCount++;
            printf("Event added with ID %d.\n", e->id);
            char act[256];
            snprintf(act, sizeof(act), "Added event '%s' (ID %d, Category: %s, Capacity: %d)", e->name, e->id, e->category, e->capacity);
            recordActivity(act);
        }
    } else if (argc == 6 && strcmp(argv[1], "register") == 0) {
        e = findEvent(atoi(argv[2]));
        if (e == NULL) printf("Event not found.\n");
        else if (contains(e->registered, argv[3]) || contains(e->front, argv[3])) {
            printf("Participant ID already exists in this event.\n");
        } else {
            Participant *p = newParticipant(argv[3], argv[4], argv[5]);
            char act[256];
            if (e->count < e->capacity) {
                addConfirmed(e, p);
                printf("Registration confirmed.\n");
                snprintf(act, sizeof(act), "Registered: %s (%s) for Event %d", p->name, p->id, e->id);
                recordActivity(act);
            } else {
                enqueue(e, p);
                printf("Event is full; participant added to waiting list.\n");
                snprintf(act, sizeof(act), "Waitlisted: %s (%s) for Event %d", p->name, p->id, e->id);
                recordActivity(act);
            }
        }
    } else if (argc == 4 && strcmp(argv[1], "cancel") == 0) {
        e = findEvent(atoi(argv[2]));
        if (e == NULL) printf("Event not found.\n");
        else cancel(e, argv[3]);
    } else if (argc >= 3 && strcmp(argv[1], "search") == 0) {
        searchEvents(argv[2]);
    } else if (argc == 2 && (strcmp(argv[1], "history") == 0 || strcmp(argv[1], "stack") == 0)) {
        display();
    } else if (argc == 2 && strcmp(argv[1], "peek") == 0) {
        const char *topItem = peek();
        if (topItem != NULL) printf("Top Activity: %s\n", topItem);
        else printf("Activity History is empty.\n");
    } else if (argc == 2 && strcmp(argv[1], "pop") == 0) {
        char *popped = pop();
        if (popped != NULL) printf("Popped Activity: %s\n", popped);
        else printf("Stack underflow: Activity History is empty.\n");
    } else if (argc >= 3 && strcmp(argv[1], "push") == 0) {
        if (push(argv[2])) printf("Pushed to stack: %s\n", argv[2]);
        else printf("Stack overflow: Cannot push.\n");
    } else if (argc == 2 && strcmp(argv[1], "status") == 0) {
        printf("Stack size: %d, isEmpty: %s, isFull: %s\n", size(), isEmpty() ? "true" : "false", isFull() ? "true" : "false");
    } else printf("Invalid command.\n");
    sendDataToServer(); // Return the temporary state to Python.
    return 0;
}



