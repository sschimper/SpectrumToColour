// ======================================================================== //
// SPECTRAL TO COLOUR CONVERTER                                             //
// Calculates RGB values from spectral information.                         //
//                                                                          //
// Author: Sebastian Schimper                                               //
// ======================================================================== //


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <getopt.h>
#include <math.h>

// ========================================================
// Definition of the sizes of the tables used later
// Stuct for representing wavelength and intensity values
// ========================================================
#define TABLE_SIZE 400
#define VISIBLE_SPECTRUM_LOWER_BOUND 380
#define VISIBLE_SPECTRUM_UPPER_BOUND 775
#define NULL_DOUBLE -10000.0000
#define PI 3.14159265

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

// helper function to write a table to a txt file
void printFunctionToFile(char* filename, Table *table) {

    FILE * data_file = fopen(filename, "r");
    if(data_file == NULL) {
        data_file = fopen(filename, "a");
        if(data_file == NULL) {
            printf("Error: No file was found, and a new file couldn't be created.\n");
            return;
        }
    }

    int i;
    for(i = VISIBLE_SPECTRUM_LOWER_BOUND; i <= VISIBLE_SPECTRUM_UPPER_BOUND; ++i) {
        if(lookup(i, table) != NULL_DOUBLE) {
            fprintf(data_file, "%3i %11f\n", i, lookup(i, table));
        }
    }
    fclose(data_file);
}

// interploation, taken from here:
// http://paulbourke.net/miscellaneous/interpolation/
double cosineInterpolate(
        double y1,double y2, int mu) {

    double mu2 = (1 - cos(PI*mu))/2;
    return(y1*(1-mu2)+y2*mu2);
}

void fillGap(Table* table, int x1, int x2, int middle, double y1, double y2) {
    double newValue = cosineInterpolate(y1, y2, middle);

    if(x2 - middle == 1 && middle - x1 == 1) {
        if(lookup(middle, table) == NULL_DOUBLE)
            addPairToTable(table, middle, newValue);
        return;
    }

    addPairToTable(table, middle, newValue);

    if(x2 - middle > 1) {
        int quater2 = (middle + x2) / 2;
        fillGap(table, middle, x2, quater2, newValue, y2);
    }
    if(middle - x1 > 1) {
        int quater1 = (x1 + middle) / 2;
        fillGap(table, x1, middle, quater1, y1, newValue);
    }
}

// interpolate the function
void interpolateFunction(Table *table) {
    int i;
    int j;
    int prevInd;
    int nextInd;

    for(i = VISIBLE_SPECTRUM_LOWER_BOUND; i <= VISIBLE_SPECTRUM_UPPER_BOUND; i++) {
        if(lookup(i, table) == NULL_DOUBLE && i != VISIBLE_SPECTRUM_LOWER_BOUND) {
            prevInd = i-1;
            j = i;
            while(lookup(j, table) == NULL_DOUBLE) {
                j++;
                if(lookup(j, table) != NULL_DOUBLE) {
                    nextInd = j;
                }
            }
            // j = i;
            double y1 = lookup(prevInd, table);
            double y2 = lookup(nextInd, table);

            int middle = (prevInd + nextInd) / 2;
            fillGap(table, prevInd, nextInd, middle, y1, y2);
            // addPairToTable(table, middle, cosineInterpolate(y1, y2, middle));
            /*
            while(j != nextInd) {
                addPairToTable(table, j, cosineInterpolate(y1, y2, j));
                j++;
            }
            */
            i = nextInd;
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
void initDataContainers(void) {
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
void destroyAllTables(void) {
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
void readAllFiles(void) {
    // luminaire data
    readFile("../data/luminaire data/cie_a.txt", cie_incandescent);
    readFile("../data/luminaire data/f11.txt", f11);
    readFile("../data/luminaire data/cie_d65.txt", cie_daylight);

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
void rndWavelengthSampling(int num_samples, char* l_func, char* r_func) {
    printf("rndWavelengthSampling was called with %d samples,\n"
           "luminare function is %s,\n"
           "refl function is %s.\n", num_samples, l_func, r_func);
}

void fxdWavelengthSampling(int num_samples, char* l_func, char* r_func) {
    printf("fxdWavelengthSampling was called with %d samples,\n"
           "luminare function is %s,\n"
           "refl function is %s.\n", num_samples, l_func, r_func);
}


// ========================================================
// print a nice header
// print a line when needed
// ========================================================
void printHeader(void) {
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

void printLine(void) {
    printf("-------------------------------------------------------------------------------------\n");
}
// ========================================================
// menu - parsing of user input commands
// ========================================================
void printHelp(void) {
    // print instructions
    printf("Available Options: \n"
           "    --rnd n                  (for [r]andom wavelength sampling, where n is the number of samples)\n"
           "    --fxd n                  (for [f]ixed wavelength sampling, where n is the number of samples)\n"
           "    -l [ciea/cied/f11]       (for [l]uminaire data, default = ciea)\n"
           "    -r [a1/e2/f4/g4/h4/j4]   (for [r]eflectance data, default = a1)\n"
           "    -h                       (for printing this exact same text)\n");
}

// menu parsing
void startMenu(int argc, char **argv) {
    printHeader();
    printLine();
    int n;
    int c;
    // opterr = 0;

    /*
    // if no argument was typed, print help text and return
    if (optind >= argc) {
        printf ("Missing arguments.\n");
        printHelp();
        return;
    }
     */

    // if this flag is set, do random wavelength sampling
    // else, do fixed bucket sampling
    static int rnd_flag;

    char refl_function_name[2] = "";
    char lum_function_name[4] = "";

    // start menu
    while(1) {

        static struct option long_options[] =
                {
                        /* These options set a flag. */
                        {"rnd", required_argument,     &rnd_flag, 1},
                        {"fxd", required_argument,     &rnd_flag, 0},
                        /* These options don’t set a flag.
                           We distinguish them by their indices. */
                        {"help",     no_argument,      0, 'h'},
                        {"liminaire",  required_argument, 0, 'l'},
                        {"reflectance",  required_argument, 0, 'r'},
                        {0, 0, 0, 0}
                };
        int option_index = 0;

        c = getopt_long (argc, argv, "hl:r:", long_options, &option_index);

        if(c == -1)
            break;

        switch (c)
        {
            case 0:
                /* If this option set a flag, do nothing else now. */
                //if (long_options[option_index].flag != 0)
                    //break;
                printf ("option %s with value `%s'\n", long_options[option_index].name, optarg);
                if (!optarg)
                    printf ("Missing Argument: Please submit an integer value for the amount of samples.\n");
                else {
                    n = atoi(optarg);
                    if(long_options[option_index].name == "rnd")
                        rnd_flag = 1;
                    else if(long_options[option_index].name == "fxd")
                        rnd_flag = 0;
                }
                break;

            case 'h':
                printHelp();
                break;

            case 'l':
                printf ("option -l with value `%s'\n", optarg);
                strcpy(lum_function_name, optarg);
                break;

            case 'r':
                printf ("option -r with value `%s'\n", optarg);
                strcpy(refl_function_name, optarg);
                break;

            case '?':
                /* getopt_long already printed an error message. */
                printf("Something went wrong ... Here's some help:\n");
                printHelp();
                break;

            default:
                abort ();
        }

        if(refl_function_name[0] == '\0') {
            strcpy(refl_function_name, "a1");
        }
        if(lum_function_name[0] == '\0') {
            strcpy(lum_function_name, "ciea");
        }

        int result = strcmp(refl_function_name, "a1");

        /*
        if(strcmp(refl_function_name, "a1") != 0 && strcmp(refl_function_name, "e2") != 0
           && strcmp(refl_function_name, "f4") != 0 && strcmp(refl_function_name, "g4") != 0
           && strcmp(refl_function_name, "h4") != 0 && strcmp(refl_function_name, "j4") != 0)
        {
            printf("Error: Provided function name for Reflectance function does not"
                   "match the ones, specified by the program.\n");
            break;
        }
        if(strcmp(refl_function_name, "ciea") != 0 && strcmp(refl_function_name, "cied") != 0
                && strcmp(refl_function_name, "f11") != 0)
        {
            printf("Error: Provided function name for Luminaire function does not"
                   "match the ones, specified by the program.\n");
            break;
        }
         */

        if(rnd_flag == 0) {
            fxdWavelengthSampling(n, lum_function_name, refl_function_name);
        }
        else if(rnd_flag == 1) {
            rndWavelengthSampling(n, lum_function_name, refl_function_name);
        }

    } // end while loop

    if (optind < argc)
    {
        printf ("non-option ARGV-elements: ");
        while (optind < argc)
            printf ("%s ", argv[optind++]);
        putchar ('\n');
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
