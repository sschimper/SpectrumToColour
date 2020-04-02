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
#include <assert.h>

// ========================================================
// Definition of the sizes of the tables used later
// Stuct for representing wavelength and intensity values
// ========================================================
#define TABLE_SIZE 400
#define VISIBLE_SPECTRUM_LOWER_BOUND 380
#define VISIBLE_SPECTRUM_UPPER_BOUND 780
#define EMEMENT_COUNT_MAX 30
#define PI 3.14159265

// struct holding single wavelength-intensity-pair
typedef struct node{
    float wavelength;
    double intensity;
    struct node *next;
} node;

typedef struct hash {
    struct node *head;
    int count;
} hash;


// luminaire data containers
struct hash *cie_incandescent = NULL;
struct hash *cie_daylight = NULL;
struct hash *f11 = NULL;

// reflectance data containers
struct hash *xrite_e2 = NULL;
struct hash *xrite_f4 = NULL;
struct hash *xrite_g4 = NULL;
struct hash *xrite_h4 = NULL;
struct hash *xrite_j4 = NULL;
struct hash *xrite_a1 = NULL;

// cie matching functions container
struct hash *cie_x = NULL;
struct hash *cie_y = NULL;
struct hash *cie_z = NULL;

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

// ========================================================
// DICTIONARY DATA STRUCTURE stuff
// ========================================================
// construct hash value
// simply convert float to int
unsigned int getHashIndex(const float wl) {
    int wl_int = wl;
    assert(wl_int >= VISIBLE_SPECTRUM_LOWER_BOUND && wl_int <= VISIBLE_SPECTRUM_UPPER_BOUND);
    return wl_int - VISIBLE_SPECTRUM_LOWER_BOUND;
}

// create one node
struct node *createNode(const float wl, const double in) {
    struct node *newNode;
    newNode = (struct node *)malloc(sizeof(struct node));
    newNode->wavelength = wl;
    newNode->intensity = in;
    newNode->next = NULL;

    return newNode;
}

// partially stolen from here: https://www.geeksforgeeks.org/insertion-sort-for-singly-linked-list/
void addNodeToTable(struct hash *table, const float wl, const double in) {
    unsigned int slot = getHashIndex(wl);
    struct node* newNode = createNode(wl, in);

    // check head of bucket list
    if(!table[slot].head) {
        table[slot].head = newNode;
        table[slot].count = 1;
        return;
    }

    // add new node to the list and update head
    struct node **head_ref = &table[slot].head;
    struct node *current;
    /* Special case for the head end */
    if ((*head_ref) == NULL || (*head_ref)->wavelength >= newNode->wavelength)
    {
        newNode->next = head_ref;
        *head_ref = newNode;
    }
    else
    {
        /* Locate the node before the point of insertion */
        current = *head_ref;
        while (current->next!=NULL &&
               current->next->wavelength < newNode->wavelength)
        {
            current = current->next;
        }
        newNode->next = current->next;
        current->next = newNode;
    }
    table[slot].count++;
}

// look up a wavelength and get the corresponding intensity
// takes value and container as input arguments
bool lookup(struct hash *table, const float wl) {
    unsigned int slot = getHashIndex(wl);
    struct node *wantedNode;
    wantedNode = table[slot].head;

    if(!wantedNode) {
        printf("Search element unavailable in hash table\n");
        return false;
    }

    while(wantedNode != NULL) {
        if(wantedNode->wavelength == wl) {
            return true;
        }
        wantedNode = wantedNode->next;
    }

    return false;
}

// stolen from here: http://see-programming.blogspot.com/2013/05/chain-hashing-separate-chaining-with.html
void deleteFromHash(struct hash *table, const float wl) {
    int flag = 0;
    unsigned int slot = getHashIndex(wl);
    struct node *temp, *currentNode;
    /* get the list head from current bucket */
    currentNode = table[slot].head;
    if (!currentNode) {
        printf("Given data is not present in hash Table!!\n");
        return;
    }
    temp = currentNode;
    while (currentNode != NULL) {
        /* delete the node with given key */
        if (currentNode->wavelength == wl) {
            flag = 1;
            if (currentNode == table[slot].head)
                table[slot].head = currentNode->next;
            else
                temp->next = currentNode->next;

            table[slot].count--;
            free(currentNode);
            break;
        }
        temp = currentNode;
        currentNode = currentNode->next;
    }
    if (flag)
        printf("Data with WL %.3f deleted successfully from Hash Table\n", wl);
    else
        printf("Given data with WL %.3f is not present in hash Table!!!!\n", wl);
    return;
}

// delete whole table
void deleteTable(struct hash *table) {
    struct node *currentNode;
    int i;
    for (i = VISIBLE_SPECTRUM_LOWER_BOUND; i <= VISIBLE_SPECTRUM_UPPER_BOUND; i++) {
        const unsigned int slot = getHashIndex(i);
        if (table[slot].count == 0)
            continue;
        currentNode = table[slot].head;
        if (!currentNode)
            continue;
        while (currentNode != NULL) {
            deleteFromHash(table, currentNode->wavelength);
            currentNode = currentNode->next;
        }
    }
    return;
}

// helper function to print content of table
void printFunction(struct hash *table) {
    struct node *myNode;
    int i;
		for (i = VISIBLE_SPECTRUM_LOWER_BOUND; i <= VISIBLE_SPECTRUM_UPPER_BOUND; i++) {
			  const unsigned int slot = getHashIndex(i);
        if (table[slot].count == 0)
            continue;
        myNode = table[slot].head;
        if (!myNode)
            continue;
        printf("\nData at index %d in Hash Table:\n", i);
        printf("Wavel           Intens\n");
        printf("----------------------\n");
        while (myNode != NULL) {
            printf("%.6f      ", myNode->wavelength);
            printf("%.6f\n", myNode->intensity);
            myNode = myNode->next;
        }
    }
    return;
}

/*
// helper function to write a table to a txt file
void printFunctionToFile(char* filename, struct hash *table) {

    FILE * data_file = fopen(filename, "r");
    if(data_file == NULL) {
        data_file = fopen(filename, "a");
        if(data_file == NULL) {
            printf("Error: No file was found, and a new file couldn't be created.\n");
            return;
        }
    }

    for(int i = VISIBLE_SPECTRUM_LOWER_BOUND; i <= VISIBLE_SPECTRUM_UPPER_BOUND; ++i) {
        if(lookup(i, table) != NULL_DOUBLE) {
            fprintf(data_file, "%3i %11f\n", i, lookup(i, table));
        }
    }
    fclose(data_file);
}
*/
// ========================================================
// prepossessing stuff: init tables and
// read the pairs of wavelength and intensity
// takes the filename and the container the files
// shall be placed in
// ========================================================
// initialize all containers
void initDataContainers(void) {
    cie_incandescent = (struct hash *)calloc(TABLE_SIZE, sizeof (struct hash));
    cie_daylight = (struct hash *)calloc(TABLE_SIZE, sizeof (struct hash));
    f11 = (struct hash *)calloc(TABLE_SIZE, sizeof (struct hash));
    /*

// reflectance data containers
    xrite_e2 = (struct hash *)calloc(TABLE_SIZE, sizeof (struct hash));
    xrite_f4 = (struct hash *)calloc(TABLE_SIZE, sizeof (struct hash));
    xrite_g4 = (struct hash *)calloc(TABLE_SIZE, sizeof (struct hash));
    xrite_h4 = (struct hash *)calloc(TABLE_SIZE, sizeof (struct hash));
    xrite_j4 = (struct hash *)calloc(TABLE_SIZE, sizeof (struct hash));
    xrite_a1 = (struct hash *)calloc(TABLE_SIZE, sizeof (struct hash));

// cie matching functions container
    cie_x = (struct hash *)calloc(TABLE_SIZE, sizeof (struct hash));
    cie_y = (struct hash *)calloc(TABLE_SIZE, sizeof (struct hash));
    cie_z = (struct hash *)calloc(TABLE_SIZE, sizeof (struct hash));
     */
}

// read filenames
void readFile(char* filename, struct hash* table) {

    FILE * data_file = fopen(filename, "r");
    if(data_file == NULL) {
        printf("File not found...\n");
        return;
    }

    char ch;
    char int_c[255] = "";
    char float_c[255] = "";

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
            int wl = atoi(int_c);

            // get float value
            double in = atof(float_c);

            // assign and reset
            printf("Wl: %d\n", wl);
            addNodeToTable(table, wl, in);
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

    /*
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
     */
}

void deleteAllTables(void) {
    deleteTable(cie_incandescent);
    deleteTable(cie_daylight);
    deleteTable(f11);

    /*
    deleteTable(cie_incandescent);
    deleteTable(cie_incandescent);
    deleteTable(cie_incandescent);
    deleteTable(cie_incandescent);
    deleteTable(cie_incandescent);
    deleteTable(cie_incandescent);

    deleteTable(cie_incandescent);
    deleteTable(cie_incandescent);
    deleteTable(cie_incandescent);
     */
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

    // if this flag is set, do random wavelength sampling
    // else, do fixed bucket sampling
    static int rnd_flag;

    char refl_function_name[2] = "";
    char lum_function_name[4] = "";
    int n;
    int c;

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
                strcpy(lum_function_name, optarg);
                break;

            case 'r':
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
    } // end while loop

    if (optind < argc)
    {
        printf ("non-option ARGV-elements: ");
        while (optind < argc)
            printf ("%s ", argv[optind++]);
        putchar ('\n');
    }

    if(rnd_flag == 0) {
        fxdWavelengthSampling(n, lum_function_name, refl_function_name);
    }
    else if(rnd_flag == 1) {
        rndWavelengthSampling(n, lum_function_name, refl_function_name);
    }
}

// ========================================================
// main function
// ========================================================
int main(int argc, char **argv) {
    //prepossessing stuff
    initDataContainers();
    readAllFiles();


    // start menu
    startMenu(argc, argv);
    deleteAllTables();

    return 0;
}
