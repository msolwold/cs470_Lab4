# CS470 Lab4

## Introduction

This README is for a program that simulates two process scheduling algorithms; Priority Scheduling and Round Robin Scheduling. 

This program accepts a .bin file filled with example Process Control Blocks (PCB's) as a command line argument. The structure of the PCB is described below. The program will parse the .bin file and alternate between the two types of scheduing algorithms. The program will exit once all of the processes are executed.

During simulation, processes that are executed with Round Robin Scheduling will be made red and processes executed with Priority Scheduling will be made blue.

The *processes.bin* file in this repository was created by Dr. Szilard Vajda as an example file for this lab. It holds 100 processes.

## Requirements

This program was developed on a **macOS** system. It has also been ran on Ubuntu 18.04.

## Instructions

To run this program, open a terminal and navigate to the directory where the program exists. After compiling main.c on a macOS system, run the program from the command line with a valid path to a .bin file that you wish to parse. e.g *./a.out processes.bin*

## PCB Structure

Each PCB in the file is of the following structure:

| Offset | Type       | Description     |
|--------|------------|-----------------|
| 0000   | 16 char    | Process Name    |
| 0016   | 32 bit int | Process ID      |
| 0020   | char       | Activity Status |
| 0021   | 32 bit int | CPU Burst Time  |
| 0025   | 32 bit int | Base Register   |
| 0029   | 64 bit long| Limit Register  |
| 0037   | char       | Priority        |

## Customization

Per the assignment description, 1 process is executed per "time unit". This can be adjusted via the SLEEP_TIME constant. By default, it is one second for demonstation purposes and will take upwards of 10+ minutes to finish processing the provided .bin file. To see a full run of this program, adjust the SLEEP_TIME constant to .1 seconds or lower (100000 nano seconds).

Similarly, this program swaps Scheduling Algorithms every 30 executions. This can be adjusted via the SCHEDULE_SWAP constant.

## Error Handling

This program will exit if an invalid or missing file path is given.
