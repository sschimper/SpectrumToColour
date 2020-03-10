#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>

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
    Table *table = (Table*)malloc(sizeof(Table) * 1);

    // allocate table entries
    table->entries = (Pair**)malloc(sizeof(Pair*) * TABLE_SIZE);

    // set each entry to null
    int i;
    for(i = 0; i < TABLE_SIZE; ++i)
        table->entries[i] = NULL;

    return table;
}

// create one pair
Pair *createPair(const int wl, const double in) {
    Pair *pair = (Pair *)malloc(sizeof(pair) * 1);
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
        if(entry->wavelength == wl) {
            return;
        }
        prev = entry;
        entry = prev->next;
    }
    prev->next = createPair(wl, in);
}

// destroy a chain stored at a slot of the table
void destroyChain(struct node** head_ref)
{
    Pair* current = *head_ref;
    Pair* next;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    *head_ref = NULL;
}

// destroy whole table
void destroyTable(Table **table) {
    int i;
    for(i = 0; i < TABLE_SIZE; ++i) {
        if(table[i] != NULL) {
            destroyChain(table);
        }
    }
}

// helper function to print content of table
void printFunction(Table *table) {
    int i;
    for(i = VISIBLE_SPECTRUM_LOWER_BOUND; i <= VISIBLE_SPECTRUM_UPPER_BOUND; ++i) {
        double thisIntensity = lookup(i, table);

        if (thisIntensity != NULL_DOUBLE)
        {
            printf("%d : %0.13f\n", i, thisIntensity);
        }
    }
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
void destroyAllTables() {
    // luminaire data
    destroyTable(cie_daylight);
    destroyTable(cie_incandescent);
    destroyTable(f11);

    // reflectance data
    destroyTable(xrite_e2);
    destroyTable(xrite_f4);
    destroyTable(xrite_g4);
    destroyTable(xrite_h4);
    destroyTable(xrite_j4);
    destroyTable(xrite_a1);

    // cie matching functions
    destroyTable(cie_x);
    destroyTable(cie_y);
    destroyTable(cie_z);
}

void readFile(char* filename, Table* table) {

    FILE * data_file = fopen(filename, "r");

    if(data_file == NULL) {
        printf("File not found...\n");
        return;
    }

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
}

// do the above for all provided functions
void readAllFiles() {
    // luminaire data
    readFile("../data/luminaire data/cie_incandescent.txt", cie_incandescent);
    readFile("../data/luminaire data/f11.txt", f11);
    readFile("../data/luminaire data/cie_daylight.txt", cie_daylight);

    // reflection data
    readFile("../data/reflectance values/a1.txt", xrite_a1);
    readFile("../data/reflectance values/e2.txt", xrite_e2);
    readFile("../data/reflectance values/f4.txt", xrite_f4);
    readFile("../data/reflectance values/g4.txt", xrite_g4);
    readFile("../data/reflectance values/h4.txt", xrite_h4);
    readFile("../data/reflectance values/j4.txt", xrite_j4);

    // cie matching functions
    readFile("../data/cie/cie_x.txt", cie_x);
    readFile("../data/cie/cie_y.txt", cie_y);
    readFile("../data/cie/cie_z.txt", cie_z);
}
// ========================================================
// the actual main part of this homework assignment
// calculation and conversion from spectral information
// to colour
// ========================================================
void rndWavelengthSampling(int num_samples) {
    printf("rndWavelengthSampling was called with %d samples\n", num_samples);
}

void fxdWavelengthSampling(int num_samples) {
    printf("fxdWavelengthSampling was called with %d samples\n", num_samples);
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
// ========================================================
// menu - parsing of user input commands
// ========================================================
void printHelp() {
    // print instructions
    printf("Please start the program with one of the following arguments: \n"
           "    -r n [for [r]andom wavelength sampling, where n is the number of samples]\n"
           "    -f n [for [f]ixed wavelength sampling, where n is the number of samples]\n"
           "    -h   [for printing this exact same text]\n");
}

// menu parsing
void startMenu(int argc, char **argv) {
    printHeader();
    printLine();
    int n;
    int c;
    opterr = 0;

    // if no argument was typed, print help text and return
    if (optind >= argc) {
        printf ("Missing arguments.\n");
        printHelp();
        return;
    }

    while((c = getopt(argc, argv, "r:f:h")) != -1) {
        switch(c) {
            case 'h':
                printHelp();
                break;
            case 'f':
                n = atoi(optarg);
                fxdWavelengthSampling(n);
                break;
            case 'r':
                n = atoi(optarg);
                rndWavelengthSampling(n);
                break;
            case '?':
                if (optopt == 'c')
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint (optopt))
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf (stderr,
                             "Unknown option character `\\x%x'.\n",
                             optopt);
                return;
            default:
                abort();
        }
    }
}

// ========================================================
// main function
// ========================================================
int main(int argc, char **argv) {
    // prepossessing stuff
    initDataContainers();
    readAllFiles();

    // start menu
    startMenu(argc, argv);

    destroyAllTables();

    return 0;
}
