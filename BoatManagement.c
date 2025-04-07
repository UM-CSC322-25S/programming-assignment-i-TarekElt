#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_BOATS 120
#define MAX_BOAT_NAME 128
#define MAX_LINE_LENGTH 256

//-------------------------------------------------------------------------------------------------
//---- Place Type Enum and Conversion Functions

typedef enum {
    slip,
    land,
    trailor,
    storage,
    no_place
} PlaceType;

char *PlaceToString(PlaceType Place) {
    switch (Place) {
        case slip:
            return "slip";
        case land:
            return "land";
        case trailor:
            return "trailor";
        case storage:
            return "storage";
        case no_place:
            return "no_place";
        default:
            printf("How the faaark did I get here?\n");
            exit(EXIT_FAILURE);
    }
}

PlaceType StringToPlaceType(char *PlaceString) {
    if (!strcasecmp(PlaceString, "slip")) {
        return slip;
    }
    if (!strcasecmp(PlaceString, "land")) {
        return land;
    }
    if (!strcasecmp(PlaceString, "trailor")) {
        return trailor;
    }
    if (!strcasecmp(PlaceString, "storage")) {
        return storage;
    }
    return no_place;
}

//-------------------------------------------------------------------------------------------------
//---- Extra Info Union and Boat Structure

typedef union {
    int slipNumber;           // For boats in a slip (1-85)
    char bayLetter;           // For boats on land (A-Z)
    char trailorLicense[16];  // For boats on trailors
    int storageNumber;        // For boats in storage (1-50)
} ExtraInfo;

typedef struct {
    char name[MAX_BOAT_NAME]; // Boat name (up to 127 characters)
    int length;               // Boat length in feet (max 100)
    PlaceType place;          // Where the boat is located
    ExtraInfo extra;          // Extra info based on the place type
    double amountOwed;        // Amount owed to the marina
} Boat;

// Global array of pointers to Boat structs and boat count
Boat* boats[MAX_BOATS];
int boatCount = 0;

//-------------------------------------------------------------------------------------------------
//---- Function Prototypes

void loadData(const char *filename);
void saveData(const char *filename);
void printInventory(void);
void addBoatCSV(char *csvLine);
void removeBoat(void);
void processPayment(void);
void updateMonth(void);
int compareBoats(const void *a, const void *b);
Boat* findBoatByName(const char *name);

//-------------------------------------------------------------------------------------------------
//---- Main Function

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s BoatData.csv\n", argv[0]);
        return 1;
    }
    const char *filename = argv[1];

    // Initialize boat pointers to NULL
    for (int i = 0; i < MAX_BOATS; i++) {
        boats[i] = NULL;
    }

    // Load boat data from CSV file
    loadData(filename);

    printf("Welcome to the Boat Management System\n");
    printf("-------------------------------------\n\n");

    char option[10];
    int running = 1;
    while (running) {
        printf("(I)nventory, (A)dd, (R)emove, (P)ayment, (M)onth, e(X)it : ");
        if (scanf("%s", option) != 1)
            break;
        char choice = tolower(option[0]);
        switch (choice) {
            case 'i':
                printInventory();
                break;
            case 'a': {
                printf("Please enter the boat data in CSV format                 : ");
                char buffer[MAX_LINE_LENGTH];
                // Clear leftover newline before reading full line
                fgets(buffer, MAX_LINE_LENGTH, stdin);
                if (strlen(buffer) <= 1) {
                    fgets(buffer, MAX_LINE_LENGTH, stdin);
                }
                buffer[strcspn(buffer, "\n")] = 0; // Remove newline
                addBoatCSV(buffer);
                break;
            }
            case 'r':
                removeBoat();
                break;
            case 'p':
                processPayment();
                break;
            case 'm':
                updateMonth();
                break;
            case 'x':
                running = 0;
                break;
            default:
                printf("Invalid option %s\n", option);
                break;
        }
    }

    printf("\nExiting the Boat Management System\n");

    // Save boat data back to CSV file
    saveData(filename);

    // Free dynamically allocated memory
    for (int i = 0; i < boatCount; i++) {
        free(boats[i]);
    }
    return 0;
}

//-------------------------------------------------------------------------------------------------
//---- Function Implementations

// Load boat data from CSV file; one boat per line.
void loadData(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Could not open file %s for reading. Starting with empty database.\n", filename);
        return;
    }
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;  // Remove newline
        if (strlen(line) == 0)
            continue;
        addBoatCSV(line);
    }
    fclose(fp);
}

// Save the current boat data back to a CSV file.
void saveData(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Error opening file %s for writing.\n", filename);
        return;
    }
    for (int i = 0; i < boatCount; i++) {
        Boat *b = boats[i];
        // Write in CSV format: name,length,place,extra,amountOwed
        if (b->place == slip) {
            fprintf(fp, "%s,%d,%s,%d,%.2f\n", b->name, b->length, PlaceToString(b->place), b->extra.slipNumber, b->amountOwed);
        } else if (b->place == land) {
            fprintf(fp, "%s,%d,%s,%c,%.2f\n", b->name, b->length, PlaceToString(b->place), b->extra.bayLetter, b->amountOwed);
        } else if (b->place == trailor) {
            fprintf(fp, "%s,%d,%s,%s,%.2f\n", b->name, b->length, PlaceToString(b->place), b->extra.trailorLicense, b->amountOwed);
        } else if (b->place == storage) {
            fprintf(fp, "%s,%d,%s,%d,%.2f\n", b->name, b->length, PlaceToString(b->place), b->extra.storageNumber, b->amountOwed);
        }
    }
    fclose(fp);
}

// Compare boats alphabetically by name (case insensitive)
int compareBoats(const void *a, const void *b) {
    Boat * const *boatA = (Boat * const *)a;
    Boat * const *boatB = (Boat * const *)b;
    return strcasecmp((*boatA)->name, (*boatB)->name);
}

// Print the inventory of boats in sorted order.
void printInventory(void) {
    // Sort the boats array by boat name
    qsort(boats, boatCount, sizeof(Boat*), compareBoats);
    for (int i = 0; i < boatCount; i++) {
        Boat *b = boats[i];
        printf("%-20s %2d' ", b->name, b->length);
        if (b->place == slip) {
            printf("   slip   # %d   ", b->extra.slipNumber);
        } else if (b->place == land) {
            printf("  land      %c   ", b->extra.bayLetter);
        } else if (b->place == trailor) {
            printf("trailor %s   ", b->extra.trailorLicense);
        } else if (b->place == storage) {
            printf("storage   # %d   ", b->extra.storageNumber);
        }
        printf("Owes $%8.2f\n", b->amountOwed);
    }
}

// Add a boat using a CSV string: name,length,place,extra,amountOwed.
void addBoatCSV(char *csvLine) {
    if (boatCount >= MAX_BOATS) {
        printf("Marina is full, cannot add more boats.\n");
        return;
    }
    // Tokenize the CSV string
    char *token;
    char *rest = csvLine;

    // Allocate memory for a new boat
    Boat *newBoat = malloc(sizeof(Boat));
    if (!newBoat) {
        fprintf(stderr, "Memory allocation failed.\n");
        return;
    }

    // Parse boat name
    token = strtok_r(rest, ",", &rest);
    if (!token) { free(newBoat); return; }
    strncpy(newBoat->name, token, MAX_BOAT_NAME-1);
    newBoat->name[MAX_BOAT_NAME-1] = '\0';

    // Parse length
    token = strtok_r(NULL, ",", &rest);
    if (!token) { free(newBoat); return; }
    newBoat->length = atoi(token);

    // Parse place type
    token = strtok_r(NULL, ",", &rest);
    if (!token) { free(newBoat); return; }
    newBoat->place = StringToPlaceType(token);

    // Parse extra information based on place type
    token = strtok_r(NULL, ",", &rest);
    if (!token) { free(newBoat); return; }
    if (newBoat->place == slip) {
        newBoat->extra.slipNumber = atoi(token);
    } else if (newBoat->place == land) {
        newBoat->extra.bayLetter = token[0];
    } else if (newBoat->place == trailor) {
        strncpy(newBoat->extra.trailorLicense, token, sizeof(newBoat->extra.trailorLicense)-1);
        newBoat->extra.trailorLicense[sizeof(newBoat->extra.trailorLicense)-1] = '\0';
    } else if (newBoat->place == storage) {
        newBoat->extra.storageNumber = atoi(token);
    }

    // Parse amount owed
    token = strtok_r(NULL, ",", &rest);
    if (!token) { free(newBoat); return; }
    newBoat->amountOwed = atof(token);

    // Add the new boat to the array and update count
    boats[boatCount++] = newBoat;

    // Re-sort the array after addition
    qsort(boats, boatCount, sizeof(Boat*), compareBoats);
}

// Find a boat by name (case insensitive)
Boat* findBoatByName(const char *name) {
    for (int i = 0; i < boatCount; i++) {
        if (strcasecmp(boats[i]->name, name) == 0) {
            return boats[i];
        }
    }
    return NULL;
}

// Remove a boat by name.
void removeBoat(void) {
    char boatName[MAX_BOAT_NAME];
    printf("Please enter the boat name                               : ");
    scanf(" %[^\n]", boatName);

    int index = -1;
    for (int i = 0; i < boatCount; i++) {
        if (strcasecmp(boats[i]->name, boatName) == 0) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        printf("No boat with that name\n");
        return;
    }
    free(boats[index]);
    // Shift remaining boats to keep array packed
    for (int i = index; i < boatCount - 1; i++) {
        boats[i] = boats[i+1];
    }
    boatCount--;
}

// Process a payment for a boat.
void processPayment(void) {
    char boatName[MAX_BOAT_NAME];
    printf("Please enter the boat name                               : ");
    scanf(" %[^\n]", boatName);

    Boat *b = findBoatByName(boatName);
    if (!b) {
        printf("No boat with that name\n");
        return;
    }
    double payment;
    printf("Please enter the amount to be paid                       : ");
    scanf("%lf", &payment);
    if (payment > b->amountOwed) {
        printf("That is more than the amount owed, $%.2f\n", b->amountOwed);
        return;
    }
    b->amountOwed -= payment;
}

// Update the amount owed for each boat for a new month.
void updateMonth(void) {
    for (int i = 0; i < boatCount; i++) {
        Boat *b = boats[i];
        double rate = 0.0;
        if (b->place == slip) {
            rate = 12.50;
        } else if (b->place == land) {
            rate = 14.00;
        } else if (b->place == trailor) {
            rate = 25.00;
        } else if (b->place == storage) {
            rate = 11.20;
        }
        b->amountOwed += b->length * rate;
    }
}
