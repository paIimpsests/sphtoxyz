/*    sphtoxyz.c
 * ================
 * Parse and split .sph files into .xyz format
 * Can also generate `box.txt` file for TCC analysis so has not to lose box size
 * 
 */





//    LIBRARIES
// ===============
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <getopt.h>





//    PREPROCESSOR
// ==================





//    MACROS
// ============





//    PROTOTYPES
// ================
// structures
// ----------------
typedef struct Particle Particle;
// functions
// ----------------
int parse_input(int argc, char* argv[]);
int mygetline(char* str, FILE* f);
int init_reading(FILE* initfile);
int read_file(FILE* infile);
int write_file(FILE* outfile);
int write_box(FILE* boxfile);





//    GLOBAL VARIABLES
// ======================
// system
// ----------------------
int N;                      // number of particles
double sigma = 1.0f;        // lenght unit = max particle diameter
double boxx;                // x box size
double boxy;                // y box size
double boxz;                // z box size
Particle *particles = NULL; // table of particles data
// input/output
// ----------------------
char* input_filename;       // name of SOURCE file
char output_filename[255];  // name of OUTPUT file
char TCCbox_filename[255];  // nam of TCC box file
int split = 0;              // choice to split input movie file into multiple snapshot files
long cursor_current;        // current reading position in SOURCE file
long cursor_end;            // position of EOF in SOURCE file
int snap_count = 0;         // snapshots count
int TCCbox = 0;             // choice to write `box.txt` file for TCC analysis





//    STRUCTURES
// ================
struct Particle {
/* Structure: PARTICLE
 * -------------------
 * Describe a particle in the system
 */
    double x;
    double y;
    double z;
    double r;
    char type;
    int index;
};




//    FUNCTIONS
// ===============
int parse_input(int argc, char* argv[]) {
/* Function:    PARSE_INPUT
 * ------------------------
 * Parser for command line execution prompt
 */
    struct option longopt[] = {
        {"split",no_argument,NULL,'s'},
        {"TCCbox",no_argument,NULL,'b'},
        {"help",no_argument,NULL,'h'},
        {0,0,0,0},
    };
    int c;
    while ((c = getopt_long(argc, argv, "sbh", longopt, NULL)) != -1) {
        switch (c) {
            case 's':
                        split = 1;
                        break;
            case 'b':
                        TCCbox = 1;
                        break;
            case 'h':
                        printf("[sphtoxyz]\n * Usage: ./sphtoxyz [OPTIONS] SOURCE\n * Description: converts a snapshot or movie file in .sph format into .xyz format\n * Options:\n * -s: split the movie file into individual numbered snapshots\n * -b: write a 'box.txt' file for TCC analysis\n * -h: display this message and exit\n");
                        exit(0);
        }
    }
    // Wrong usage warning
    if (optind > argc-1) {
        printf("[sphtoxyz] Usage: ./sphtoxyz [OPTIONS] SOURCE\n [sphtoxyz] Help: ./sphtoxyz -h\n");
        exit(0);
    }

    input_filename = argv[optind];
    return 0;
}



int mygetline(char* str, FILE* f) {
/* Function:    MYGETLINE
 * ----------------------
 * Line parser for SOURCE file
 */
    int comment = 1;
    while(comment) {
        if (!fgets(str, 255, f)) return -1;
        if (str[0] != '#') comment = 0;
    }
    return 0;
}



int init_reading(FILE* initfile) {
/* Function:    INIT_READING
 * -------------------------
 * Initializes reading of SOURCE file
 */
    // localization of EOF
    fseek(initfile, 0, SEEK_END);
    cursor_end = ftell(initfile);
    rewind(initfile);
    cursor_current = ftell(initfile);
    return 0;
}



int read_file(FILE* infile) {
/* Function:    READ_FILE
 * ----------------------
 * Reads and stores info for one snapshot from SOURCE file in .sph format
 */
    char buffer[255];
    int ftmp;
    double rtemp = 0.0f;
    Particle* p;

    mygetline(buffer, infile);
    ftmp = sscanf(buffer, "%d\n", &N);
    if (ftmp != 1) {printf("[sphtoxyz] ERROR: Could not parse N: %s\n", buffer); exit(3);}
    particles = malloc(N * sizeof(*particles));
    mygetline(buffer, infile);
    ftmp = sscanf(buffer, "%lf %lf %lf\n", &boxx, &boxy, &boxz);
    if (ftmp != 3) {printf("[sphtoxyz] ERROR: Could not parse box size: %s\n", buffer); exit(3);}
    for (int i = 0; i < N; i++) {
        p = &(particles[i]);
        mygetline(buffer, infile);
        ftmp = sscanf(buffer, "%c %lf %lf %lf %lf\n", &(p->type), &(p->x), &(p->y), &(p->z), &(p->r));
        if (ftmp != 5) {printf("[sphtoxyz] ERROR: Could not parse particle %d: %s\n", i, buffer); exit(3);}
        p->index = i;
        if (p->r > rtemp) rtemp = p->r;
    }
    boxx /= (2.0f * rtemp);
    boxy /= (2.0f * rtemp);
    boxz /= (2.0f * rtemp);
    for (int i = 0; i < N; i++) {
        p = &(particles[i]);
        p->x /= (2.0f * rtemp);
        p->x = fmod(p->x + 2.0f * boxx, boxx);
        p->y /= (2.0f * rtemp);
        p->y = fmod(p->y + 2.0f * boxy, boxy);
        p->z /= (2.0f * rtemp);
        p->z = fmod(p->z + 2.0f * boxz, boxz);
        p->r /= (2.0f * rtemp);
    }

    return 0;
}



int write_file(FILE* outfile) {
/* Function:    WRITE_FILE
 * -----------------------
 * Writes one snapshot into OUTPUT file in .xyz format
 */
    Particle* p;

    fprintf(outfile, "%d\n", N);
    fprintf(outfile, "snapshot %d box %.12f %.12f %.12f\n", snap_count, boxx, boxy, boxz);
    for (int i = 0; i < N; i++) {
        p = &(particles[i]);
        fprintf(outfile, "C %.12f %.12f %.12f\n", p->x, p->y, p->z);
    }
    return 0;
}



int write_box(FILE* boxfile) {
/* Function:    WRITE_BOX
 * ----------------------
 * Writes a box file in format intended for TCC analysis
 */
    int static first = 1;
    if (first) {
        first = 0;
        fprintf(boxfile, "# %s box size\n", input_filename);
    }
    fprintf(boxfile, "%d %.12f %.12f %.12f\n", snap_count, boxx, boxy, boxz);
    return 0;
}


//    MAIN
// ==========
int main(int argc, char* argv[]) {
    parse_input(argc, argv);
    printf("[sphtoxyz] SOURCE: %s\n", input_filename);
    FILE* inputfile = fopen(input_filename, "r");
    FILE* outputfile = NULL;
    FILE* boxfile = NULL;

    if (!split) {
        strcpy(output_filename, input_filename);
        char *end = strstr(output_filename, ".sph");
        *end = '\0';
        strcat(output_filename, ".xyz");
        outputfile = fopen(output_filename, "w");
        if (!outputfile) {printf("[sphtoxyz] ERROR: Could not open OUTPUT file\n"); exit(3);}
    }

    if (TCCbox) {
        sprintf(TCCbox_filename, "box.txt");
        boxfile = fopen(TCCbox_filename, "w");
        if (!boxfile) {printf("[sphtoxyz] ERROR: Could not open BOX file\n"); exit(3);}
    }

    if (inputfile != NULL) {
        init_reading(inputfile);
        while (cursor_current < cursor_end) {
            read_file(inputfile);
            if (split) {
                strcpy(output_filename, input_filename);
                char* end = strstr(output_filename, ".sph");
                *end = '\0';
                char temp[255];
                sprintf(temp, "_%.3d.xyz", snap_count);
                strcat(output_filename, temp);
                outputfile = fopen(output_filename, "w");
                if (!outputfile) {printf("[sphtoxyz] ERROR: Could not open OUTPUT file\n"); exit(3);}
            }
            write_file(outputfile);
            if (TCCbox) write_box(boxfile);
            if (split) fclose(outputfile);
            printf("[sphtoxyz] Snapshot %.3d done\n", snap_count);
            snap_count++;
            cursor_current = ftell(inputfile);
            free(particles);
        }
        fclose(inputfile);
        if (!split) fclose(outputfile);
        if (TCCbox) fclose(boxfile);
    }

    if (split) {
        char temp_filename[255];
        strcpy(temp_filename, input_filename);
        char* end = strstr(temp_filename, ".sph");
        *end = '\0';
        strcat(temp_filename, "_i.xyz");
        printf("[sphtoxyz] OUTPUT: %s, for i in range %d\n", temp_filename, snap_count);
    }
    else printf("[sphtoxyz] OUTPUT: %s\n", output_filename);
    printf("[sphtoxyz] Done.\n");
    return 0;
}
