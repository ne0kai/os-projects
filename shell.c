#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "libmem.h"

#define LINE_SIZE 256 // lines of up to 256 characters
#define COMMAND_NUM 10 // number of commands
#define BYTE_MAX 255 // max value of a byte

int argc = 0;
char **argv;
struct commandEntry {
    char *name;
    int (*functionp)(int argc, char *argv[]);
};


// Data for use in parse_time function
const char months[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const char leap_months[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
char* month_strs[] = {"January", "Feburary", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

/*
 * Input timestamp (seconds after unix Epoch), microsecond, and time string
 * The program will convert the timestamp into human-readable string and store in time
 */
void parse_time(long int timestamp, long int microsecond, char* time){
    // Leap year flag
    int isleap = 0;

    int second = timestamp % 60;
    timestamp /= 60;

    int minute = timestamp % 60;
    timestamp /= 60;

    int hour = timestamp % 24;
    timestamp /= 24;

    int year;
    int four_years = timestamp / 1461; // Every four years has 1461 days
    timestamp %= 1461;
    if (timestamp < 730){
        year = 1970 + 4 * four_years + timestamp / 365;
        timestamp %= 365;
    }
    else if (730 <= timestamp && timestamp < 1096){
        isleap = 1;
        year = 1970 + 4 * four_years + 2;
        timestamp -= 730;
    }
    else{
        year = 1970 + 4 * four_years + 3;
        timestamp -= 1096;
    }

    int month;
    if (isleap){
        for (month = 0; leap_months[month] < timestamp; month++){
            timestamp -= leap_months[month];
        }
    }
    else{
        for (month = 0; months[month] < timestamp; month++){
            timestamp -= months[month];
        }
    }
    // Convert month number to string
    char* month_str = month_strs[month];

    int day = timestamp + 1;

    sprintf(time, "%s %02d, %d %02d:%02d:%02d.%06ld",
            month_str, day, year, hour, minute, second, microsecond);
}

// Print out (human-readable) current time
int cmd_date(int argc, char *argv[]){
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);

    long int seconds = currentTime.tv_sec; 
    long int microseconds = currentTime.tv_usec;

    char time[50];
    parse_time(seconds, microseconds, time);
    fprintf(stdout, "%s\n", time);
    return 0; 
}

// Echo the following string to stdout
int cmd_echo(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        fprintf(stdout, "%s ", argv[i]);
    }
    fprintf(stdout, "\n");
    return 0;
}

// Exit the shell program
int cmd_exit(int argc, char *argv[]){
    free(currentPCB);
    free(pool);
    exit(0);
    return 0; 
}

// Print out help messages
int cmd_help(int argc, char *argv[]){
    fprintf(stdout, "Type \"date\" to print out current time.\n\
Use \"echo\" to echo the words.\n\
Type \"exit\" to exit the shell.\n\
Type \"help\" to print out this help.\n\
Use \"clockdate\" to convert timestamp to time string.\n\
Use \"malloc\" to allocate memory block.\n\
Use \"free\" to free memory.\n\
Type \"memorymap\" to print out current memory map.\n\
Ues \"memset\" to set memory block to specified value.\n\
Use \"memchk\" to validate if memory block is specified value.\n");
    return 0; 
}

// Convert UNIX timestamp to human-readable time string
int cmd_clockdate(int argc, char *argv[]){
    if (argc != 2){
        fprintf(stderr, "%s: must accept one argument\n", argv[0]);
        return 1;
    }
    char *endptr;

    long seconds = strtol(argv[1], &endptr, 10);

    if (*endptr != '\0') {
        fprintf(stderr, "%s: %s is not a valid integer\n", argv[0], argv[1]);
        return 1;
    }

    if (seconds < 0){
        fprintf(stderr, "%s: timestamp must be >= 0\n", argv[0]);
        return 1;
    }

    char time[50];
    parse_time(seconds, 0, time);
    fprintf(stdout, "%s\n", time);
    return 0; 
}


/*
 * Helper function to turn a string into an long pointed by ulptr.
 * Automatically detect base and call strtol accordingly.
 * Return integer indicates success or failure.
 */
int multi_strtol(char *string, long *ulptr){
    char *endptr;
    if (string[0] == '0'){
        if (string[1] == 'X' || string[1] == 'x'){
            *ulptr = strtol(string, &endptr, 16);
        }
        else{
            *ulptr = strtol(string, &endptr, 8);
        }
    }
    else{
        *ulptr = strtol(string, &endptr, 10);
    }

    if (*endptr != '\0') {
        fprintf(stderr, "Error: Not a valid integer.\n");
        return 1;
    }
    return 0;
}

/*
 * malloc accepts a single argument, and allocate that much bytes of memory.
 * The argument can be specified in decimal, hexademal, or octal format.
 */

int cmd_malloc(int argc, char *argv[]){
    if (argc != 2){
        fprintf(stderr, "%s: must accept one argument\n", argv[0]);
        return 1;
    }

    // Parse string into long integer
    long memory_bytes;
    if (multi_strtol(argv[1], &memory_bytes))
        return 1;
  
    if (memory_bytes <= 0){
        fprintf(stderr, "%s: memory bytes must be > 0\n", argv[0]);
        return 1;
    }

    // Call to myMalloc
    void *p = myMalloc(memory_bytes);
    if (p == NULL){
        fprintf(stderr, "%s: call to myMalloc failed\n", argv[0]);
        return 1;
    }
    fprintf(stdout, "%p\n", p);
    return 0;
}

/*
 * free accepts a single argument, and free up the memory block pointed by
 * the argument. The argument can be specified in decimal, hexadecimal, or
 * octal format. It prints out the return code of myFreeErrorCode().
 */

int cmd_free(int argc, char *argv[]){
    if (argc != 2){
        fprintf(stderr, "%s: must accept one argument\n", argv[0]);
        return 1;
    }

    // Parse string into long integer
    long address;
    if (multi_strtol(argv[1], &address))
        return 1;

    if (address <= 0){
        fprintf(stderr, "%s: address must be > 0\n", argv[0]);
        return 1;
    }
 
    // Call to myFreeErrorCode
    fprintf(stdout, "%d\n", myFreeErrorCode((void *)address));
    return 0;
}

/*
 * memorymap prints out the current memory map
 * by calling memoryMap(). It accepts no argument.
 */
int cmd_memorymap(int argc, char *argv[]){
    if (argc != 1){
        fprintf(stderr, "%s: accept no argument\n", argv[0]);
        return 1;
    }

    memoryMap();
    return 0;
}

/*
 * memset initializes every byte in a range of memory addresses to a
 * specified value. The first argument is the beginning address of an
 * allocated area of memory, the second is the specified value, and the
 * third is the length (in bytes) of the specified memory. All arguments
 * can be specified in decimal, octal, or hexadecimal format.
 */

int cmd_memset(int argc, char *argv[]){
    if (argc != 4){
        fprintf(stderr, "%s: must accept three arguments\n", argv[0]);
        return 1;
    }

    // Parse and check first argument
    long beg_l;
    if (multi_strtol(argv[1], &beg_l))
        return 1;
    uint8_t *beg = (uint8_t *)beg_l;
    if (beg < (uint8_t *)pool || ((uint8_t*)pool + POOL_SIZE) <= beg){
        fprintf(stderr, "%s: %s is not within memory scope\n", argv[0], argv[1]);
        return 1;
    }

    // Parse and check second argument
    long val_l;
    if (multi_strtol(argv[2], &val_l))
        return 1;
    if (val_l > BYTE_MAX){
        fprintf(stderr, "%s: %s is not a byte value\n", argv[0], argv[2]);
        return 1;
    }
    uint8_t val = (uint8_t)val_l;
    
    // Parse and check third argument
    long len;
    if (multi_strtol(argv[3], &len))
        return 1;
    if (((uint8_t*)pool + POOL_SIZE) <= beg + len){
        fprintf(stderr, "%s: %s exceeds memory scope\n", argv[0], argv[3]);
        return 1;
    }

    // Set value
    while (len > 0){
        len--;
        *(beg + len) = val;
    }
    return 0;
}

/*
 * memchk validates whether avery byte in a range of memory address is
 * set to a specified value. The first argument is the beginning address
 * of an allocated area of memory, the second is the specified value, and
 * the third is the length (in bytes) of the specified memory. All arguments
 * can be specified in decimal, octal, or hexadecimal format.
 * It outputs "memchk sucessful" or "memchk failed".
 */

int cmd_memchk(int argc, char *argv[]){
    if (argc != 4){
        fprintf(stderr, "%s: must accept three arguments\n", argv[0]);
        return 1;
    }

    // Parse and check first argument
    long beg_l;
    if (multi_strtol(argv[1], &beg_l))
        return 1;
    uint8_t *beg = (uint8_t *)beg_l;
    if (beg < (uint8_t *)pool || ((uint8_t*)pool + POOL_SIZE) <= beg){
        fprintf(stderr, "%s: %s is not within memory scope\n", argv[0], argv[1]);
        return 1;
    }

    // Parse and check second argument
    long val_l;
    if (multi_strtol(argv[2], &val_l))
        return 1;
    if (val_l > BYTE_MAX){
        fprintf(stderr, "%s: %s is not a byte value\n", argv[0], argv[2]);
        return 1;
    }
    uint8_t val = (uint8_t)val_l;
    
    // Parse and check third argument
    long len;
    if (multi_strtol(argv[3], &len))
        return 1;
    if (((uint8_t*)pool + POOL_SIZE) <= beg + len){
        fprintf(stderr, "%s: %s exceeds memory scope\n", argv[0], argv[3]);
        return 1;
    }

    // Check value
    while (len > 0){
        len--;
        if (*(beg + len) != val){
            fprintf(stdout, "memchk failed\n"); 
            break;
        }
    }
    fprintf(stdout, "memchk successful\n"); 
    return 0;
}

struct commandEntry commands[] = {{"date", cmd_date},
                                  {"echo", cmd_echo},
                                  {"exit", cmd_exit},
                                  {"help", cmd_help},
                                  {"clockdate", cmd_clockdate},
                                  {"malloc", cmd_malloc},
                                  {"free", cmd_free},
                                  {"memorymap", cmd_memorymap},
                                  {"memset", cmd_memset},
                                  {"memchk", cmd_memchk}
};

/*
 * Parse a line into space separated fields.
 * Store the count of fields into global variable argc.
 * Store the fields into argv.
 * Return 0 if succeed, return 1 if fail.
 */
int parse_line(char* line){

    // Get argc
    if (line[0] != ' ' && line[0] != '\0')
        argc++;
        
    for (int i = 1; line[i] != '\0'; i++){
        if (line[i - 1] == ' ' && line[i] != ' ')
            argc++;
    }

    argv = (char **)malloc((argc + 1) * sizeof(char *));
    if (argv == NULL){
	fprintf(stderr, "Error: Memory allocation for argv failed.\n");
	return 1;
    }
    
    // Get argv
    argc = 0;
    int i = 0;
    int j = 0;
        
    if (line[0] != ' ' && line[0] != '\0')
        argc++;

    while (line[i] != '\0'){
        i++;
        // Begin of a field
        if (line[i] != '\0' && line[i - 1] == ' ' && line[i] != ' '){
            argc++;
            j = i;
        }
        // End of a field
        // Including the edge case of end of line
        else if ((line[i] == '\0' || (line[i] == ' ')) && (line[i - 1] != ' ')){
            argv[argc - 1] = (char *)malloc((i - j) * sizeof(char));
	    if (argv[argc - 1] == NULL){
		fprintf(stderr, "Error: Memory allocation for argv[%d] failed.\n", argc - 1);
		return 1;
	    }
            strncpy(argv[argc - 1], line + j, i - j);
            argv[argc - 1][i - j] = '\0';
        }
    }

    argv[argc] = NULL;
    return 0;
}

int main(){
    char line[LINE_SIZE + 1];

    // Initialize 128MB memory pool and currentPCB.
    myInitializeMemory();

    while(1){
        fputs("$ ", stdout);
        
        // Get a line of input
        int i = 0;
        char c;
        while (i <= LINE_SIZE && (c = fgetc(stdin)) != EOF && c != '\n') {
            line[i] = c;
            i++;
        }
        line[i] = '\0';

        // Parse that line
        if (parse_line(line)){
	    fprintf(stderr, "Error: Line parse failed.\n");
	    return 1;
	}

        // If there is input
        if (argc){
        // Execute the corresponding command
            int i = 0;
	    while (i < COMMAND_NUM){
                if (!strcmp(argv[0], commands[i].name)){
                    commands[i].functionp(argc, argv);
                    break;
                }
		i++;
            }

            // If the command is not found
            if (i == COMMAND_NUM){
                fprintf(stderr, "shell: %s: command not found\n", argv[0]);
            }

            // Free memory
            for (int i = 0; i < argc; i++){
                free(argv[i]);
            }
            free(argv);
            argc = 0;
        }
    }
}
