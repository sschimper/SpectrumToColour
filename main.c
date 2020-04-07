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
#include <math.h>
#include <time.h>

// ========================================================
// Definition of the sizes of the tables used later
// Stuct for representing wavelength and intensity values
// ========================================================
#define TABLE_SIZE 401
#define VISIBLE_SPECTRUM_LOWER_BOUND 380
#define VISIBLE_SPECTRUM_UPPER_BOUND 780
#define NOT_IN_TABLE -10000000000
#define EMEMENT_COUNT_MAX 30
#define PI 3.14159265

// struct holding single wavelength-intensity-pair
typedef struct node{
    float wavelength;
    double intensity;
    struct node *next;
} node;

typedef struct linkedList {
    struct node *head;
    int count;
} linkedList;


// luminaire data containers
struct linkedList *cie_incandescent = NULL;
struct linkedList *cie_daylight = NULL;
struct linkedList *f11 = NULL;

// reflectance data containers
struct linkedList *xrite_e2 = NULL;
struct linkedList *xrite_f4 = NULL;
struct linkedList *xrite_g4 = NULL;
struct linkedList *xrite_h4 = NULL;
struct linkedList *xrite_j4 = NULL;
struct linkedList *xrite_a1 = NULL;

// cie matching functions container
struct linkedList *cie_x = NULL;
struct linkedList *cie_y = NULL;
struct linkedList *cie_z = NULL;

// used functions
struct linkedList *l_func;
struct linkedList *r_func;

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
// DICTIONARY DATA STRUCTURE stuff
// ========================================================
// construct linkedList value
// simply convert float to int
unsigned int getlinkedListIndex(const float wl) {
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
void addNodeToTable(struct linkedList *table, const float wl, const double in) {
    unsigned int slot = getlinkedListIndex(wl);
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
        newNode->next = *head_ref;
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

void addNodeToFixedTable(struct linkedList *table, const int index, const float wl, const double in) {
    unsigned int slot = index;
    struct node* newNode = createNode(wl, in);

    // check head of bucket list
    if(!table[slot].head) {
        table[slot].head = newNode;
        table[slot].count = 1;
        return;
    }
    else
        printf("Not added.. Wl -> %.1f  In -> %.05f\n", wl, in);
}

// look up a wavelength and get the corresponding intensity
// takes value and container as input arguments
double lookupAtWl(struct linkedList *table, const float wl) {
    unsigned int slot = getlinkedListIndex(wl);
    struct node *wantedNode;
    wantedNode = table[slot].head;

    if(!wantedNode) {
        // printf("Search element with wl %.0f unavailable in linkedList table\n", wl);
        return NOT_IN_TABLE;
    }

    while(wantedNode != NULL) {
        if(wantedNode->wavelength == wl) {
            return wantedNode->intensity;
        }
        wantedNode = wantedNode->next;
    }
}

double lookupAtIndex(struct linkedList *table, const int i) {
    unsigned int slot = i;
    struct node *wantedNode;
    wantedNode = table[slot].head;

    if(!wantedNode) {
        // printf("Search element with wl %.0f unavailable in linkedList table\n", wl);
        return NOT_IN_TABLE;
    }
    else {
        return wantedNode->intensity;
    }
}

// stolen from here: http://see-programming.blogspot.com/2013/05/chain-linkedListing-separate-chaining-with.html
void deleteFromlinkedList(struct linkedList *table, const float wl) {
    int flag = 0;
    unsigned int slot = getlinkedListIndex(wl);
    struct node *temp, *currentNode;
    /* get the list head from current bucket */
    currentNode = table[slot].head;
    if (!currentNode) {
        printf("Given data is not present in linkedList Table!!\n");
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
    if(!flag)
        printf("Given data with WL %.3f is not present in linkedList Table.\n", wl);
    return;
}

// delete whole table
void deleteTable(struct linkedList *table) {
    struct node *currentNode;
    int i;
    for (i = VISIBLE_SPECTRUM_LOWER_BOUND; i <= VISIBLE_SPECTRUM_UPPER_BOUND; i++) {
        const unsigned int slot = getlinkedListIndex(i);
        if (table[slot].count == 0)
            continue;
        currentNode = table[slot].head;
        if (!currentNode)
            continue;
        while (currentNode != NULL) {
            deleteFromlinkedList(table, currentNode->wavelength);
            currentNode = currentNode->next;
        }
    }
    return;
}

// helper function to print content of table
void printFunction(struct linkedList *table) {
    struct node *myNode;
    int i;
    for (i = VISIBLE_SPECTRUM_LOWER_BOUND; i <= VISIBLE_SPECTRUM_UPPER_BOUND; i++) {
        const unsigned int slot = getlinkedListIndex(i);
        if (table[slot].count == 0)
            continue;
        myNode = table[slot].head;
        if (!myNode)
            continue;
        printf("\nData at index %d in linkedList Table:\n", i);
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

void printIntFunc(struct linkedList *table, int count, char *name) {
    printf("%s \n ========= \n", name);
    for(int i = 0; i < count; i++) {
        struct linkedList *thisNode = &table[i];
        printf("%d %.0f %.6f\n", i, thisNode->head->wavelength, thisNode->head->intensity);
    }
}

// helper function to write a table to a txt file
void printFunctionToFile(char* filename, struct linkedList *table, int count) {

    FILE * data_file = fopen(filename, "r");
    if(data_file != NULL) {
        remove(data_file);
    }

    data_file = fopen(filename, "a");
    if(data_file == NULL) {
        printf("Error: No file was found, and a new file couldn't be created.\n");
        return;
    }

    if(count == 0) {
        for(int i = VISIBLE_SPECTRUM_LOWER_BOUND; i <= VISIBLE_SPECTRUM_UPPER_BOUND; i++) {
            double lookedUpIn;
            lookedUpIn = lookupAtWl(table, i);

            if(lookedUpIn != NOT_IN_TABLE) {
                fprintf(data_file, "%3i %11f\n", i, lookedUpIn);
            }
        }
    }
    else {
        for(int i = 0; i < count; i++) {
            double lookedUpIn;
            linkedList *thisNode = &table[i];

            if(thisNode != NULL) {
                float wl = thisNode->head->wavelength;
                double in = thisNode->head->intensity;
                fprintf(data_file, "%f %.6f\n", wl, in);
            }
        }
    }

    fclose(data_file);
}
// ========================================================
// interpolation stuff
// ========================================================
// http://paulbourke.net/miscellaneous/interpolation/
double cosineInterpolate(
        double y1,double y2,
        double mu)
{
    double mu2;

    mu2 = (1-cos(mu*PI))/2;
    return(y1*(1-mu2)+y2*mu2);
}

void interpolateTableInt(linkedList *table) {
    int x1, x2, j;

    if(lookupAtWl(table, VISIBLE_SPECTRUM_UPPER_BOUND) == NOT_IN_TABLE) {
        addNodeToTable(table, VISIBLE_SPECTRUM_UPPER_BOUND, 0.0);
    }

    for(int i = VISIBLE_SPECTRUM_LOWER_BOUND; i < VISIBLE_SPECTRUM_UPPER_BOUND; i++) {
        if(lookupAtWl(table, i) == NOT_IN_TABLE) {
            x1 = i - 1;
            j = i + 1;

            while(j < VISIBLE_SPECTRUM_UPPER_BOUND && lookupAtWl(table, j) == NOT_IN_TABLE) {
                j++;
            }
            x2 = j;

            float d = x2 - x1;
            double y1 = lookupAtWl(table, x1);
            double y2 = lookupAtWl(table, x2);

            float p = 1.0;
            for(int k = x1 + 1; k < x2; k++) {
                double result = cosineInterpolate(y1, y2, (p/d));
                addNodeToTable(table, k, result);
                p = p + 1.0;
            }
            i = j;
        }
    }
}
// ========================================================
// integration
// ========================================================
float integrate_uniform(linkedList *table, const int count, const int delta) {

    float result = 0;
    for(int i = 0; i < count - 1; i++) {
        float currentIn = table[i].head->intensity;
        float nextIn = table[i+1].head->intensity;
        result += ((currentIn + nextIn) / 2) * delta;
    }
    return result;
}

float integrate_nonuniform(linkedList *table, const int count) {

    float result = 0;
    for(int i = 0; i < count - 1; i++) {
        float currentWl = table[i].head->wavelength;
        float nextWl = table[i+1].head->wavelength;
        float currentIn = table[i].head->intensity;
        float nextIn = table[i+1].head->intensity;
        result += ((currentIn + nextIn) / 2) * (nextWl - currentWl);
    }
    return result;
}


// ========================================================
// multiplication
// ========================================================
void convertToRgb(float cie_vec[3], float res[3])
{
    int i, j;
    for (i = 0; i < 3; i++)
    {
        res[i] = 0;
        for (j = 0; j < 3; j++)
        {
            res[i] += transformation_matrix[i][j] * cie_vec[j];
        }
    }
}
// ========================================================
// randomness
// ========================================================
int getRandomNumber() {
    int r = random() % 400 + 1;
    return r + VISIBLE_SPECTRUM_LOWER_BOUND;
}

// ========================================================
// sorting
// ========================================================
void swap(linkedList *xp, linkedList *yp)
{
    linkedList temp = *xp;
    *xp = *yp;
    *yp = temp;
}

// Function to perform Selection Sort
void selectionSort(linkedList *table, int n)
{
    int i, j, min_idx;

    // One by one move boundary of unsorted subarray
    for (i = 0; i < n - 1; i++) {

        // Find the minimum element in unsorted array
        min_idx = i;
        for (j = i + 1; j < n; j++)
            if (table[j].head->wavelength < table[min_idx].head->wavelength)
                min_idx = j;

        // Swap the found minimum element
        // with the first element
        swap(&table[min_idx], &table[i]);
    }
}

// ========================================================
// point-wise multiplication
// ========================================================
struct linkedList *pointwiseMultipication(const linkedList *a, const linkedList *b, const int count) {

    size_t n = count;

    linkedList *res = (struct linkedList *)calloc(n, sizeof (struct linkedList));

    for(int i = 0; i < n; i++) {
        assert(a[i].head->wavelength == b[i].head->wavelength);
        double mult_res = a[i].head->intensity * b[i].head->intensity;
        addNodeToFixedTable(res, i, a[i].head->wavelength, mult_res);
    }
    return res;
}

// ========================================================
// prepossessing stuff: init tables and
// read the pairs of wavelength and intensity
// takes the filename and the container the files
// shall be placed in
// ========================================================
// initialize all containers
void initDataContainers(void) {
    // luminaire data
    cie_incandescent = (struct linkedList *)calloc(TABLE_SIZE, sizeof (struct linkedList));
    cie_daylight = (struct linkedList *)calloc(TABLE_SIZE, sizeof (struct linkedList));
    f11 = (struct linkedList *)calloc(TABLE_SIZE, sizeof (struct linkedList));

    // reflectance data containers
    xrite_e2 = (struct linkedList *)calloc(TABLE_SIZE, sizeof (struct linkedList));
    xrite_f4 = (struct linkedList *)calloc(TABLE_SIZE, sizeof (struct linkedList));
    xrite_g4 = (struct linkedList *)calloc(TABLE_SIZE, sizeof (struct linkedList));
    xrite_h4 = (struct linkedList *)calloc(TABLE_SIZE, sizeof (struct linkedList));
    xrite_j4 = (struct linkedList *)calloc(TABLE_SIZE, sizeof (struct linkedList));
    xrite_a1 = (struct linkedList *)calloc(TABLE_SIZE, sizeof (struct linkedList));

    // cie matching functions container
    cie_x = (struct linkedList *)calloc(TABLE_SIZE, sizeof (struct linkedList));
    cie_y = (struct linkedList *)calloc(TABLE_SIZE, sizeof (struct linkedList));
    cie_z = (struct linkedList *)calloc(TABLE_SIZE, sizeof (struct linkedList));

}

// read filenames
void readFile(char* filename, struct linkedList* table) {

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

            // assign
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
    readFile("../data/luminaire data/cie_d65.txt", cie_daylight);
    readFile("../data/luminaire data/f11.txt", f11);

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

    // interpolate
    interpolateTableInt(cie_x);
    interpolateTableInt(cie_y);
    interpolateTableInt(cie_z);

}

void deleteAllTables(void) {
    deleteTable(cie_incandescent);
    deleteTable(cie_daylight);
    deleteTable(f11);

    deleteTable(cie_incandescent);
    deleteTable(cie_incandescent);
    deleteTable(cie_incandescent);
    deleteTable(cie_incandescent);
    deleteTable(cie_incandescent);
    deleteTable(cie_incandescent);

    deleteTable(cie_incandescent);
    deleteTable(cie_incandescent);
    deleteTable(cie_incandescent);
}
// ========================================================
// the actual main part of this homework assignment
// calculation and conversion from spectral information
// to colour
// ========================================================
void setFunctionsFromInput(char* l_func_s, char* r_func_s) {

    // set l function
    if(strcmp(l_func_s,"ciea") == 0)
        l_func = cie_incandescent;
    else if(strcmp(l_func_s,"cied") == 0)
        l_func = cie_daylight;
    else if(strcmp(l_func_s,"f11") == 0)
        l_func = f11;
    else {
        printf("Couldn't find l_function: Wrong arguments...\n");
        return;
    }

    // set r function
    if(strcmp(r_func_s,"a1") == 0)
        r_func = xrite_a1;
    else if(strcmp(r_func_s,"e2") == 0)
        r_func = xrite_e2;
    else if(strcmp(r_func_s,"f4") == 0)
        r_func = xrite_f4;
    else if(strcmp(r_func_s,"g4") == 0)
        r_func = xrite_g4;
    else if(strcmp(r_func_s,"h4") == 0)
        r_func = xrite_h4;
    else if(strcmp(r_func_s,"j4") == 0)
        r_func = xrite_j4;
    else {
        printf("Couldn't find r_function: Wrong arguments...\n");
        return;
    }
}

void setUpFunctions(char* l_func_s, char* r_func_s) {
    setFunctionsFromInput(l_func_s, r_func_s);
    interpolateTableInt(l_func);
    interpolateTableInt(r_func);
}

void heroWavelengthSampling(int num_samples, linkedList* l_func, linkedList* r_func) {

    linkedList *l_heroBuckets = (struct linkedList *)calloc(num_samples, sizeof (struct linkedList));
    linkedList *r_heroBuckets = (struct linkedList *)calloc(num_samples, sizeof (struct linkedList));
    linkedList *cie_x_hero = (struct linkedList *)calloc(num_samples, sizeof (struct linkedList));
    linkedList *cie_y_hero = (struct linkedList *)calloc(num_samples, sizeof (struct linkedList));
    linkedList *cie_z_hero = (struct linkedList *)calloc(num_samples, sizeof (struct linkedList));

    srand(time(0));
    int delta = (int)(VISIBLE_SPECTRUM_UPPER_BOUND - VISIBLE_SPECTRUM_LOWER_BOUND) / num_samples;
    int heroWavelength = getRandomNumber();

    addNodeToFixedTable(l_heroBuckets, 0, heroWavelength, lookupAtWl(l_func, heroWavelength));
    addNodeToFixedTable(r_heroBuckets, 0, heroWavelength, lookupAtWl(r_func, heroWavelength));
    addNodeToFixedTable(cie_x_hero, 0, heroWavelength, lookupAtWl(cie_x, heroWavelength));
    addNodeToFixedTable(cie_y_hero, 0, heroWavelength, lookupAtWl(cie_y, heroWavelength));
    addNodeToFixedTable(cie_z_hero, 0, heroWavelength, lookupAtWl(cie_z, heroWavelength));

    for(int j = 1; j < num_samples; j++) {
        int wl = heroWavelength + (j * delta);
        if(wl > VISIBLE_SPECTRUM_UPPER_BOUND) {
            wl = wl%VISIBLE_SPECTRUM_UPPER_BOUND + VISIBLE_SPECTRUM_LOWER_BOUND;
        }
        addNodeToFixedTable(l_heroBuckets, j, wl, lookupAtWl(l_func, wl));
        addNodeToFixedTable(r_heroBuckets, j, wl, lookupAtWl(r_func, wl));
        addNodeToFixedTable(cie_x_hero, j, wl, lookupAtWl(cie_x, wl));
        addNodeToFixedTable(cie_y_hero, j, wl, lookupAtWl(cie_y, wl));
        addNodeToFixedTable(cie_z_hero, j, wl, lookupAtWl(cie_z, wl));
    }

    selectionSort(l_heroBuckets, num_samples);
    selectionSort(r_heroBuckets, num_samples);
    selectionSort(cie_x_hero, num_samples);
    selectionSort(cie_y_hero, num_samples);
    selectionSort(cie_z_hero, num_samples);

    linkedList* res_spec = pointwiseMultipication(l_heroBuckets, r_heroBuckets, num_samples);

    printFunctionToFile("../data/intermediate results/rnd_hero_l_func_res.txt", l_heroBuckets, num_samples);
    printFunctionToFile("../data/intermediate results/rnd_hero_r_func_res.txt", r_heroBuckets, num_samples);
    printFunctionToFile("../data/intermediate results/rnd_hero_res_spec.txt", res_spec, num_samples);

    printFunctionToFile("../data/intermediate results/res_cie_x.txt", cie_x_hero, num_samples);
    printFunctionToFile("../data/intermediate results/res__cie_y.txt", cie_y_hero, num_samples);
    printFunctionToFile("../data/intermediate results/res_cie_z.txt", cie_z_hero, num_samples);

    cie_x_hero = pointwiseMultipication(cie_x_hero, res_spec, num_samples);
    cie_y_hero = pointwiseMultipication(cie_y_hero, res_spec, num_samples);
    cie_z_hero = pointwiseMultipication(cie_z_hero, res_spec, num_samples);

    double cie_x_value = integrate_nonuniform(cie_x_hero, num_samples);
    double cie_y_value = integrate_nonuniform(cie_y_hero, num_samples);
    double cie_z_value = integrate_nonuniform(cie_z_hero, num_samples);

    float cie[3] = {cie_x_value, cie_y_value, cie_z_value};
    float rgb[3];

    convertToRgb(cie, rgb);

    printLine();
    printf("Result of hero WL sampling: R(%.5f) G(%.5f) B(%.5f)\n", rgb[0], rgb[1], rgb[2]);
    printLine();

    free(res_spec);
    free(l_heroBuckets);
    free(r_heroBuckets);
    free(cie_x_hero);
    free(cie_y_hero);
    free(cie_z_hero);
}

void rndWavelengthSampling(int num_samples, char* l_func_s, char* r_func_s) {
    printf("Random wavelength (incl. hero) sampling with %d samples,\n"
           "luminare function %s,\n"
           "and reflectance function %s...\n", num_samples, l_func_s, r_func_s);

    setUpFunctions(l_func_s, r_func_s);

    linkedList *l_rndBuckets = (struct linkedList *)calloc(num_samples, sizeof (struct linkedList));
    linkedList *r_rndBuckets = (struct linkedList *)calloc(num_samples, sizeof (struct linkedList));
    linkedList *cie_x_rnd = (struct linkedList *)calloc(num_samples, sizeof (struct linkedList));
    linkedList *cie_y_rnd = (struct linkedList *)calloc(num_samples, sizeof (struct linkedList));
    linkedList *cie_z_rnd = (struct linkedList *)calloc(num_samples, sizeof (struct linkedList));

    srand(time(0));
    for(int i = 0; i < num_samples; i++) {
        int rndWl = getRandomNumber();
        addNodeToFixedTable(l_rndBuckets, i, rndWl, lookupAtWl(l_func, rndWl));
        addNodeToFixedTable(r_rndBuckets, i, rndWl, lookupAtWl(r_func, rndWl));
        addNodeToFixedTable(cie_x_rnd, i, rndWl, lookupAtWl(cie_x, rndWl));
        addNodeToFixedTable(cie_y_rnd, i, rndWl, lookupAtWl(cie_y, rndWl));
        addNodeToFixedTable(cie_z_rnd, i, rndWl, lookupAtWl(cie_z, rndWl));
    }

    selectionSort(l_rndBuckets, num_samples);
    selectionSort(r_rndBuckets, num_samples);
    selectionSort(cie_x_rnd, num_samples);
    selectionSort(cie_y_rnd, num_samples);
    selectionSort(cie_z_rnd, num_samples);

    linkedList* res_spec = pointwiseMultipication(l_rndBuckets, r_rndBuckets, num_samples);

    printFunctionToFile("../data/intermediate results/rnd_l_func_res.txt", l_rndBuckets, num_samples);
    printFunctionToFile("../data/intermediate results/rnd_r_func_res.txt", r_rndBuckets, num_samples);
    printFunctionToFile("../data/intermediate results/rnd_res_spec.txt", res_spec, num_samples);

    cie_x_rnd = pointwiseMultipication(cie_x_rnd, res_spec, num_samples);
    cie_y_rnd = pointwiseMultipication(cie_y_rnd, res_spec, num_samples);
    cie_z_rnd = pointwiseMultipication(cie_z_rnd, res_spec, num_samples);

    printFunctionToFile("../data/intermediate results/ciex_res.txt", cie_x_rnd, num_samples);
    printFunctionToFile("../data/intermediate results/ciey_res.txt", cie_y_rnd, num_samples);
    printFunctionToFile("../data/intermediate results/ciez_res.txt", cie_z_rnd, num_samples);

    float cie_x_value = integrate_nonuniform(cie_x_rnd, num_samples);
    float cie_y_value = integrate_nonuniform(cie_y_rnd, num_samples);
    float cie_z_value = integrate_nonuniform(cie_z_rnd, num_samples);

    float cie[3] = {cie_x_value, cie_y_value, cie_z_value};
    float rgb[3];

    convertToRgb(cie, rgb);

    printLine();
    printf("Result of random WL sampling: R(%.5f) G(%.5f) B(%.5f)\n", rgb[0], rgb[1], rgb[2]);
    printLine();

    free(res_spec);
    free(l_rndBuckets);
    free(r_rndBuckets);
    free(cie_x_rnd);
    free(cie_y_rnd);
    free(cie_z_rnd);

    heroWavelengthSampling(num_samples, l_func, r_func);
}

void fxdWavelengthSampling(int num_samples, char* l_func_s, char* r_func_s) {
    printf("Fixed wavelength sampling with %d samples,\n"
           "luminare function %s,\n"
           "and reflectance function %s...\n", num_samples, l_func_s, r_func_s);

    if(num_samples > 50) {
        printf("Number of samples for fixed wavelength sampling is too high. Please choose a value smaller than 50.\n");
        exit(0);
    }

    setUpFunctions(l_func_s, r_func_s);

    int delta = (int)(VISIBLE_SPECTRUM_UPPER_BOUND - VISIBLE_SPECTRUM_LOWER_BOUND) / (num_samples - 1);

    linkedList* l_func_fxd = (struct linkedList *)calloc(num_samples, sizeof (struct linkedList));
    linkedList* r_func_fxd = (struct linkedList *)calloc(num_samples, sizeof (struct linkedList));
    linkedList* cie_x_fxd = (struct linkedList *)calloc(num_samples, sizeof (struct linkedList));
    linkedList* cie_y_fxd = (struct linkedList *)calloc(num_samples, sizeof (struct linkedList));
    linkedList* cie_z_fxd = (struct linkedList *)calloc(num_samples, sizeof (struct linkedList));

    for(int j = 0; j < num_samples; j++) {
        int wl = VISIBLE_SPECTRUM_LOWER_BOUND + (j * delta);
        addNodeToFixedTable(l_func_fxd, j, wl, lookupAtWl(l_func, wl));
        addNodeToFixedTable(r_func_fxd, j, wl, lookupAtWl(r_func, wl));
        addNodeToFixedTable(cie_x_fxd, j, wl, lookupAtWl(cie_x, wl));
        addNodeToFixedTable(cie_y_fxd, j, wl, lookupAtWl(cie_y, wl));
        addNodeToFixedTable(cie_z_fxd, j, wl, lookupAtWl(cie_z, wl));
    }

    linkedList* res_spec = pointwiseMultipication(l_func_fxd, r_func_fxd, num_samples);
    printFunctionToFile("../data/intermediate results/fxd_l_func.txt", l_func_fxd, num_samples);
    printFunctionToFile("../data/intermediate results/fxd_r_func.txt", r_func_fxd, num_samples);
    printFunctionToFile("../data/intermediate results/fxd_res_spec.txt", res_spec, num_samples);

    cie_x_fxd = pointwiseMultipication(cie_x_fxd, res_spec, num_samples);
    cie_y_fxd = pointwiseMultipication(cie_y_fxd, res_spec, num_samples);
    cie_z_fxd = pointwiseMultipication(cie_z_fxd, res_spec, num_samples);

    double cie_x_value = integrate_uniform(cie_x_fxd, num_samples, delta);
    double cie_y_value = integrate_uniform(cie_y_fxd, num_samples, delta);
    double cie_z_value = integrate_uniform(cie_z_fxd, num_samples, delta);

    float cie[3] = {cie_x_value, cie_y_value, cie_z_value};
    float rgb[3];

    convertToRgb(cie, rgb);

    printLine();
    printf("Result of fixed WL sampling: R(%.5f) G(%.5f) B(%.5f)\n", rgb[0], rgb[1], rgb[2]);
    printLine();

    free(res_spec);
    free(l_func_fxd);
    free(r_func_fxd);
    free(cie_x_fxd);
    free(cie_y_fxd);
    free(cie_z_fxd);
}

void cmpWavelengthSampling(int num_samples, char* l_func_s, char* r_func_s) {
    fxdWavelengthSampling(num_samples, l_func_s, r_func_s);
    rndWavelengthSampling(num_samples, l_func_s, r_func_s);
}
// ========================================================
// menu - parsing of user input commands
// ========================================================
void printHelp(void) {
    // print instructions
    printf("Available Options: \n"
           "    --random n                 (for [r]andom wavelength sampling, where n is the number of samples)\n"
           "    --fixed n                  (for [f]ixed wavelength sampling, where n is the number of samples)\n"
           "    --compare n                (uses both random- and wavelength sampling and compares the results. n is the number of samples)\n"
           "    -l [ciea/cied/f11]         (for [l]uminaire data, default = ciea)\n"
           "    -r [a1/e2/f4/g4/h4/j4]     (for [r]eflectance data, default = a1)\n"
           "    -h                         (for printing this exact same text)\n");
}

// menu parsing
void startMenu(int argc, char **argv) {
    //printHeader();

    // if this flag is set, do random wavelength sampling
    // else, do fixed bucket sampling
    static int rnd_flag;
    static int cmp_flag = 0;

    char refl_function_name[30] = "";
    char lum_function_name[30] = "";
    int n;
    int c;

    // start menu
    while(1) {
        static struct option long_options[] =
                {
                        /* These options set a flag. */
                        {"random", required_argument,     &rnd_flag, 1},
                        {"fixed", required_argument,     &rnd_flag, 0},
                        {"compare", required_argument,     &cmp_flag, 1},
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

    if(rnd_flag == 0 && cmp_flag == 0) {
        printLine();
        fxdWavelengthSampling(n, lum_function_name, refl_function_name);
    }
    else if(rnd_flag == 1 && cmp_flag == 0) {
        printLine();
        rndWavelengthSampling(n, lum_function_name, refl_function_name);
    }
    else if(cmp_flag == 1) {
        printLine();
        cmpWavelengthSampling(n, lum_function_name, refl_function_name);
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

    //clean up
    deleteAllTables();

    return 0;
}
