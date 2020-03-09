#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// ========================================================
// Definition of the sizes of the tables used later
// Stuct for representing wavelength and intensity values
// ========================================================
#define TABLE_SIZE 80
#define VISIBLE_SPECTRUM_LOWER_BOUND 380
#define VISIBLE_SPECTRUM_UPPER_BOUND 775
#define NULL_DOUBLE -10000.0000

// struct holding single wavelength-intensity-pair
typedef struct Pair {
    int wavelength;
    double intensity;
    struct Pair *next;
} Pair;

// struct holding multiple pairs
typedef struct {
    Pair **entries;
} Table;

// ========================================================
// GLOBAL VARIABLES
// - transformation matrix and containers for data
// ========================================================

// used for conversion from CIE XYZ -> sRGB
const float transformation_matrix[3][3] = {
        {3.2410, -1.5374, -0.4986},
        {-0.9692, 1.8760, 0.0416},
        {0.0556, -0.2050, 1.0570}
};

// luminaire data containers
static Table *cie_incandescent;
static Table *cie_daylight;
static Table *f11;

// reflectance data containers
static Table *xrite_e2;
static Table *xrite_f4;
static Table *xrite_g4;
static Table *xrite_h4;
static Table *xrite_j4;
static Table *xrite_a1;

// cie matching functions container
static Table *cie_x;
static Table *cie_y;
static Table *cie_z;

// ========================================================
// DICTIONARY DATA STRUCTURE stuff
// got a little help from here: https://www.youtube.com/watch?v=wg8hZxMRwcw&t=466s
// ========================================================
// construct hash value
unsigned int hash(const int wl) {
    int hash_value = 0;
    int i;
    hash_value = wl * TABLE_SIZE + wl;
    return hash_value % TABLE_SIZE;
}

// create the table
Table *createTable(void) {
    // allocate
    Table *table = malloc(sizeof(table) * 1);

    // allocate table entries
    table->entries = malloc(sizeof(Pair*) * TABLE_SIZE);

    // set each entry to null
    int i;
    for(i = 0; i < TABLE_SIZE; ++i)
        table->entries[i] = NULL;

    return table;
}

// create one pair
Pair *createPair(const int wl, const double in) {
    Pair *pair = malloc(sizeof(pair) * 1);
    pair->wavelength = wl;
    pair->intensity = in; // NO MEMORY ALLOC !!!!

    return pair;
}

// look up a wavelength and get the corresponding intensity
// takes value and container as input arguments
double lookup(const int wl, Table *table) {
    unsigned int slot = hash(wl);
    Pair *entry = table->entries[slot];

    if(entry == NULL) {
        return NULL_DOUBLE;
    }

    while(entry != NULL) {
        if(entry->wavelength == wl) {
            return entry->intensity;
        }
        entry = entry->next;
    }
    return NULL_DOUBLE;
}

// add a pair struct to the container
void addPairToTable(Table *table, const int wl, const double in)
{
    unsigned int slot = hash(wl);
    Pair *entry = table->entries[slot];

    if(entry == NULL) {
        table->entries[slot] = createPair(wl, in);
        return;
    }
    Pair *prev;
    while(entry != NULL) {
        // check key

        if(entry->wavelength == wl) {
            // match found, replace values
            /*
            free(entry->wavelength); //MAYBE BUG HERE
            free(entry->intensity);
             */
            return; // for now
        }

        prev = entry;
        entry = prev->next;
    }

    prev->next = createPair(wl, in);
}

// free memory
// takes a container as input argument
void destoryTable(Pair *Pair) {
    // TODO
}

// helper function to print content of table
void printTable(Table *table) {
    int i;
    for(i = 0; i < TABLE_SIZE; ++i) {
        Pair *entry = table->entries[i];

        if(entry == NULL)
            continue;

        for(;;) {
            printf("%d %f\n", entry->wavelength, entry->intensity);

            if(entry->next == NULL)
                break;

            entry = entry->next;
        }
    }
}

// helper function to print content of table
void printFunction(Table *table) {
    int i;
    for(i = VISIBLE_SPECTRUM_LOWER_BOUND; i <= VISIBLE_SPECTRUM_UPPER_BOUND; ++i) {
        double thisIntensity = lookup(i, table);

        if(thisIntensity != NULL_DOUBLE) {
            printf("%d : %f\n", i, thisIntensity);
        }
    }
}

// ========================================================
// print a nice header
// print a line when needed
// ========================================================
void printHeader() {
    FILE *welcome_txt_file;
    int c;
    welcome_txt_file = fopen("../welcome.txt", "r");

    if(welcome_txt_file == NULL) {
        printf("Text file could not be loaded.\n");
        return;
    }

    while((c = fgetc(welcome_txt_file)) != EOF) {
        putchar(c);
    }

    fclose(welcome_txt_file);
}

void printLine() {
    printf("-------------------------------------------------------------------------------------\n");
}

void printInstructions() {
    printf("Please type 'h', 'H', 'help' or 'Help' to see a list of available "
           "functions and \ncommands with an appropriate description.\n");
}

// ========================================================
// prepossessing stuff: init tables and
// read the pairs of wavelength and intensity
// takes the filename and the container the files
// shall be placed in
// ========================================================
// initialize all containers
void initDataContainers() {
    cie_incandescent = createTable();
    cie_daylight = createTable();
    f11 = createTable();

// reflectance data containers
    xrite_e2 = createTable();
    xrite_f4 = createTable();
    xrite_g4 = createTable();
    xrite_h4 = createTable();
    xrite_j4 = createTable();
    xrite_a1 = createTable();

// cie matching functions container
    cie_x  = createTable();
    cie_y = createTable();
    cie_z = createTable();
}

// destroy all containers
void destroyAllContainers() {

}


void readFile(char* filename, Table* table) {

    FILE * data_file = fopen(filename, "r");
    char ch;

    char int_c[3] = "";
    char float_c[11] = "";

    int int_count = 0;
    int float_count = 0;
    int slot_index = 0;

    bool column_crossed = false;
    while((ch = fgetc(data_file)) != EOF) {
        if(ch == ',')
            column_crossed = true;
        else if(ch == '\n') {
            column_crossed = false;

            // get int value
            int wl = atoi(int_c) / 10; // don't know why, but works

            // get float value
            double in = atof(float_c);

            // assign and reset
            addPairToTable(table, wl, in);
            slot_index++;
            int_count = 0;
            float_count = 0;
        }
        else if(isdigit(ch) && !column_crossed) {
            int_c[int_count] = ch;
            int_count++;
        }
        else if((isdigit(ch) || ch == '.') && column_crossed) {
            float_c[float_count] = ch;
            float_count++;
        }
    }
    fclose(data_file);
    printFunction(cie_daylight);
} // EVERY TIME I LEAVE THE FUNCTION I GET "*** stack smashing detected ***: <unknown> terminated". I DUNNO WHAT'S WRONG...

// do the above for all provided functions
void readAllFiles() {
    readFile("../data/luminaire data/cie_daylight.txt", cie_daylight);

}

// ========================================================
// menu - parsing of user input commands
// ========================================================
void startMenu() {

}

// ========================================================
// main function
// ========================================================
int main(int argc, char **argv) {
    // prepossessing stuff
    initDataContainers();
    readAllFiles();


    printHeader();
    printLine();
    printInstructions();



    return 0;
}
