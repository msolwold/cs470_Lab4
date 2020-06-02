#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>

/*
 Since I can't upload my over the top README :(

 This was developed on macOS but it has been ran on Ubuntu 18.04

 The program requires a .bin file name to be provided to run.

 Currently, the sleep time is set to 1 second. It can be adjusted via the SLEEP_TIME variable below
 The algorithms swap every 30 iterations. This can be changed via the SCHEDULE_SWAP variable below

 This program will execute on a copy of the file provided as to not ruin the original.

 Process outputs during Round Robin scheduling will be made Red.
 Outputs during Priority Scheduling will be made Blue.
 A final print out of the file contents will be done once all files have been processed
 */

/* Program Constants */

#define SCHEDULE_SWAP 30
#define SLEEP_TIME 1000000

/* Struct to hold PCB information */

struct process_control_block
{
    long process_location;
    char process_name[16];
    int32_t process_id;
    char activity_status;
    int32_t burst_time;
    int32_t base_register;
    long limit_register;
    char process_priority;
};

/* Globals set at runtime */
int activeProcesses = 0;
int totalMemory = 0;
int file_size = 0;
int quantumTime = 0;
int scheduling = 0; /* 0 = Round Robin | 1 = Priority */

/**
 * Used for preserving the original file
 * Creates a copy of the given file and saves it as
 * "fileCopy.bin"
 *
 * @param FILE * file to be copied
 */
void createFileCopy(FILE * sourceFile)
{
    FILE *file;

    file = fopen("fileCopy.bin", "w+b");

    if (!file)
        printf("Error Opening New File!");


    fseek(sourceFile, 0L, SEEK_END);
    long size = ftell(sourceFile);
    rewind(sourceFile);

    char buffer[size];
    fread(buffer, sizeof(buffer), 1, sourceFile);
    fwrite(buffer, sizeof(buffer), 1, file);

    fclose(file);
    fclose(sourceFile);
}

/**
 * Reads input from the user
 *
 * @param argc CLI Info
 * @param argv
 * @return 0 if error | 1 if success
 */
int parseCLI(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("File Name not provided. Please provide the name of the file you would like parsed.\n");
        return 0;
    }

    char * fileName = argv[1];

    FILE * file = fopen(fileName, "rb");

    if (!file)
    {
        printf("Error Opening File!");
        return 0;
    }

    printf("\nCreating a copy of %s as fileCopy.bin\n\n", fileName);

    createFileCopy(file);

    return 1;
}

/**
 * Output contents of PCB
 *
 * @param struct process_control_block * to print
 */
void printPCB(struct process_control_block *pcb)
{
    //printf("Process Position: %li\n", pcb->process_location);
    printf("Process Name: %s\n", pcb->process_name);
    printf("Process ID: %d\n", pcb->process_id);
    printf("Activity Status: %d\n", pcb->activity_status);
    printf("CPU Burst Time: %d\n", pcb->burst_time);
    printf("Base Register: %d\n", pcb->base_register);
    printf("Limit Register: %li\n", pcb->limit_register);
    printf("Priority: %d\n", pcb->process_priority);
}

/**
 * Reads a PCB from the file and stores the contents in
 * a process_control_block struct
 *
 * @param FILE * file to read from
 * @param struct process_control_block *
 */
void parsePCB(FILE *file, struct process_control_block *pcb)
{
    fread(pcb, sizeof(struct process_control_block), 1, file);
    
    /*
    pcb->process_location = ftell(file);
    fread(pcb->process_name, sizeof(char) * 16, 1, file);
    fread(&(pcb->process_id), sizeof(int32_t), 1, file);
    fread(&(pcb->activity_status), sizeof(char), 1, file);
    fread(&(pcb->burst_time), sizeof(int32_t), 1, file);
    fread(&(pcb->base_register), sizeof(int32_t), 1, file);
    fread(&(pcb->limit_register), sizeof(long), 1, file);
    fread(&(pcb->process_priority), sizeof(char), 1, file);
    */
}

/**
 * "Removes" process after CPU Burst is reduced to 0
 * Sets:
 *      Activity Status = 0
 *      CPU Burst = 0
 *      Priority = CHAR_MAX (127)
 *
 * @param FILE * file to write to
 * @param struct process_control_block * that is being removed
 */
void removeProcess(FILE *file, struct process_control_block *pcb)
{
    char newActivityStatus = 0;
    int32_t newCPUBurst = 0;
    char newPriority = CHAR_MAX;

    /* Update Binary File */
    fseek(file, pcb->process_location + 20, SEEK_SET);
    fwrite(&newActivityStatus, sizeof(char), 1, file);
    fwrite(&newCPUBurst, sizeof(int32_t), 1, file);
    fseek(file, pcb->process_location + 37, SEEK_SET);
    fwrite(&newPriority, sizeof(char), 1, file);

    /* Update Struct */
    pcb->activity_status = newActivityStatus;
    pcb->burst_time = newCPUBurst;
    pcb->process_priority = newPriority;

    activeProcesses--;
}

/**
 * Execute a process by decrementing CPU Burst
 *
 * @param FILE * file to write to
 * @param struct process_control_block * that is being executed
 */
void executeProcess(FILE *file, struct process_control_block *pcb)
{
    long endOfProcess = ftell(file);
    fseek(file, pcb->process_location + 21, SEEK_SET);

    int32_t newBurstTime = pcb->burst_time - 1;

    if (newBurstTime <= 0)
        removeProcess(file, pcb);
    else
    {
        fwrite(&newBurstTime, sizeof(int32_t), 1, file);
        pcb->burst_time = newBurstTime;
    }

    fseek(file, endOfProcess, SEEK_SET);
}

/**
 * Create a PCB struct then gets a PCB from file
 *
 * @param FILE * file to read from
 * @return struct process_control_block *
 */
struct process_control_block *getPCB(FILE *file)
{
    struct process_control_block *pcb;
    pcb = calloc(1, sizeof(struct process_control_block));

    parsePCB(file, pcb);

    return pcb;
}

/**
 * Age all processes except for current process being executed
 * Aging decrements the priority value of a process
 *
 * @param FILE * file to write to
 * @param int32_t process_id of process being executed
 */
void ageProcesses(FILE *file, int32_t currentProcessID)
{
    fseek(file, 0L, SEEK_END);
    long size = ftell(file);
    rewind(file);

    while (ftell(file) != size)
    {
        struct process_control_block *pcb;

        pcb = getPCB(file);

        long endOfProcess = ftell(file);

        if (pcb->activity_status == 0 || pcb->process_id == currentProcessID)
            continue;

        fseek(file, pcb->process_location + 37, SEEK_SET);

        char newPriority = pcb->process_priority > 0 ? (pcb->process_priority) - 1 : 0;
        fwrite(&newPriority, sizeof(char), 1, file);

        free(pcb);

        fseek(file, endOfProcess, SEEK_SET);
    }
}

/**
 * Parses entire file. Will either print or not print info
 * based on flags provided
 *
 * @param FILE * file to read from
 * @param flag 0 to not print | 1 to print
 */
void parseFile(FILE *file, int flag)
{
    fseek(file, 0L, SEEK_END);
    long size = ftell(file);
    rewind(file);

    int numActiveProcesses = 0;
    int totalProcessMemory = 0;

    while (ftell(file) != size)
    {
        struct process_control_block *pcb;

        pcb = getPCB(file);

        if (flag == 1)
        {
            printPCB(pcb);
            printf("\n");
        }


        if (pcb->activity_status > 0)
            numActiveProcesses++;

        totalProcessMemory += (pcb->limit_register - pcb->base_register);
        free(pcb);
    }

    if (flag == 1)
    {
        printf("Total number of active processes: %d\n", numActiveProcesses);
        printf("Total memory used by active processes: %d\n", totalProcessMemory);
    }


    activeProcesses = numActiveProcesses;
    totalMemory = totalProcessMemory;
    file_size = size;

    rewind(file);
}

/**
 * Parses the file and returns the process
 * with the highest priority
 *
 * @param FILE * file to read from
 * @return struct process_control_block * process with highest priority
 */
struct process_control_block *getHighestPriority(FILE *file)
{
    fseek(file, 0L, SEEK_END);
    long size = ftell(file);
    rewind(file);

    struct process_control_block *highestPriorityPCB = getPCB(file);

    while (ftell(file) != size)
    {
        struct process_control_block *pcb;

        pcb = getPCB(file);

        if (pcb->activity_status == 1 && pcb->process_priority < highestPriorityPCB->process_priority)
        {
            free(highestPriorityPCB);
            highestPriorityPCB = pcb;
        }
    }

    return highestPriorityPCB;
}

/**
 * Algorithm for Priority Scheduling
 * Will run for SCHEDULE_SWAP number of iterations
 *
 * Gets the process with highest priority and processes it
 * Every 2 iterations, age all processes
 *
 * @param FILE * file to read from
 */
void priorityScheduling(FILE *file)
{
    struct process_control_block *priorityPCB;

    priorityPCB = getHighestPriority(file);

    printf("\033[0m");
    printf("\n-----------------------\n");
    printf("\nPriority Scheduling\n\n");
    printf("-----------------------\n\n");
    printf("\033[0;36m");

    while (quantumTime % SCHEDULE_SWAP != 0 && activeProcesses > 0)
    {
        if (quantumTime % 2 == 0)
        {
            printf("Aging all processes...\n\n");
            ageProcesses(file, priorityPCB->process_id);
            free(priorityPCB);
            priorityPCB = getHighestPriority(file);
        }

        printPCB(priorityPCB);
        printf("\n");
        executeProcess(file, priorityPCB);

        if (priorityPCB->activity_status == 0)
        {
            free(priorityPCB);
            priorityPCB = getHighestPriority(file);
        }

        quantumTime++;
        usleep(SLEEP_TIME);
    }
}

/**
 * Algorithm for Round Robin Scheduling
 *
 * Starting at the beginning of the file,
 * execute processes one by one.
 * Performs a total of SCHEDULE_SWAP number of executions
 *
 * @param FILE * file to read from
 */
void rrScheduling(FILE *file)
{
    printf("\033[0m");
    printf("\n-----------------------\n");
    printf("\nRound Robin Scheduling\n\n");
    printf("-----------------------\n\n");
    printf("\033[0;31m");

    rewind(file);

    while (quantumTime % SCHEDULE_SWAP != 0 && activeProcesses > 0)
    {
        struct process_control_block *pcb = getPCB(file);

        while (pcb->activity_status == 0)
        {
            if (ftell(file) >= file_size)
                rewind(file);
            free(pcb);
            pcb = getPCB(file);
        }

        printPCB(pcb);
        printf("\n");
        executeProcess(file, pcb);
        free(pcb);

        quantumTime++;
        usleep(SLEEP_TIME);
    }
}

/**
 * Driver for Scheduling Algorithms.
 * scheduling flag determines which algorithm is called
 * Runs until all processes are finished
 *
 * @param FILE * file to read from
 */
void runSchedulers(FILE *file)
{
    printf("Running Scheduler...\n");
    do
    {
        quantumTime++;
        if (scheduling == 0)
        {
            rrScheduling(file);
            scheduling = 1;
        }
        else
        {
            priorityScheduling(file);
            scheduling = 0;
        }
    } while (activeProcesses > 0);
}

int main(int argc, char *argv[])
{
    printf("\033[0m");

    int isValid = parseCLI(argc, argv);
    if (isValid == 0)
        return 1;

    FILE *file;

    file = fopen("fileCopy.bin", "r+b");

    if (!file)
    {
        printf("Error Error Error!");
    }

    parseFile(file, 0);

    printf("This file has %d processes...\n\n", activeProcesses);
    printf("Total memory used by active processes: %d\n\n", totalMemory);

    runSchedulers(file);

    printf("\033[0;33m");
    parseFile(file, 1);

    printf("\033[0m");
    return 0;
}