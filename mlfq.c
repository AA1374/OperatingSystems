/*
    COMP3511 Fall 2022 
    Simplified Multi-Level Feedback Queue (MLFQ)


*/

// Note: Necessary header files are included
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Define MAX_NUM_PROCESS
// For simplicity, assume that we have at most 10 processes

#define MAX_NUM_PROCESS 10
#define MAX_QUEUE_SIZE 10
#define MAX_PROCESS_NAME 5
#define MAX_GANTT_CHART 300

// Keywords (to be used when parsing the input)
#define KEYWORD_TQ0 "tq0"
#define KEYWORD_TQ1 "tq1"
#define KEYWORD_PROCESS_TABLE_SIZE "process_table_size"
#define KEYWORD_PROCESS_TABLE "process_table"

// Assume that we only need to support 2 types of space characters: 
// " " (space), "\t" (tab)
#define SPACE_CHARS " \t"

int total_time = 0;

// Process data structure
// Helper functions:
//  process_init: initialize a process entry
//  process_table_print: Display the process table
struct Process {
    char name[MAX_PROCESS_NAME];
    int arrival_time ;
    int burst_time;
    int remain_time; // remain_time is needed in the intermediate steps of MLFQ 
};
void process_init(struct Process* p, char name[MAX_PROCESS_NAME], int arrival_time, int burst_time) {
    strcpy(p->name, name);
    p->arrival_time = arrival_time;
    p->burst_time = burst_time;
    p->remain_time = 0;
}
void process_table_print(struct Process* p, int size) {
    int i;
    printf("Process\tArrival\tBurst\n");
    for (i=0; i<size; i++) {
        printf("%s\t%d\t%d\n", p[i].name, p[i].arrival_time, p[i].burst_time);
        total_time += p[i].burst_time;
        //p[i].remain_time = p[i].burst_time;
    }
}

// A simple integer queue implementation using a fixed-size array
// Helper functions:
//   queue_init: initialize the queue
//   queue_is_empty: return true if the queue is empty, otherwise false
//   queue_is_full: return true if the queue is full, otherwise false
//   queue_peek: return the current front element of the queue
//   queue_enqueue: insert one item at the end of the queue
//   queue_dequeue: remove one item from the beginning of the queue
//   queue_print: display the queue content, it is useful for debugging
struct Queue {
    int values[MAX_QUEUE_SIZE];
    int front, rear, count;
};
void queue_init(struct Queue* q) {
    q->count = 0;
    q->front = 0;
    q->rear = -1;
}
int queue_is_empty(struct Queue* q) {
    return q->count == 0;
}
int queue_is_full(struct Queue* q) {
    return q->count == MAX_QUEUE_SIZE;
}

int queue_peek(struct Queue* q) {
    return q->values[q->front];
}
void queue_enqueue(struct Queue* q, int new_value) {
    if (!queue_is_full(q)) {
        if ( q->rear == MAX_QUEUE_SIZE -1)
            q->rear = -1;
        q->values[++q->rear] = new_value;
        q->count++;
    }
}
void queue_dequeue(struct Queue* q) {
    q->front++;
    if (q->front == MAX_QUEUE_SIZE)
        q->front = 0;
    q->count--;
}
void queue_print(struct Queue* q) {
    int c = q->count;
    printf("size = %d\n", c);
    int cur = q->front;
    printf("values = ");
    while ( c > 0 ) {
        if ( cur == MAX_QUEUE_SIZE )
            cur = 0;
        printf("%d ", q->values[cur]);
        cur++;
        c--;
    }
    printf("\n");
}

// A simple GanttChart structure
// Helper functions:
//   gantt_chart_update: append one item to the end of the chart (or update the last item if the new item is the same as the last item)
//   gantt_chart_print: display the current chart
struct GanttChartItem {
    char name[MAX_PROCESS_NAME];
    int duration;
};
void gantt_chart_update(struct GanttChartItem chart[MAX_GANTT_CHART], int* n, char name[MAX_PROCESS_NAME], int duration) {
    int i;
    i = *n;
    // The new item is the same as the last item
    if ( i > 0 && strcmp(chart[i-1].name, name) == 0) 
    {
        chart[i-1].duration += duration; // update duration
    } 
    else
    {
        strcpy(chart[i].name, name);
        chart[i].duration = duration;
        *n = i+1;
    }
}
void gantt_chart_print(struct GanttChartItem chart[MAX_GANTT_CHART], int n) {
    int t = 0;
    int i = 0;
    printf("Gantt Chart = ");
    printf("%d ", t);
    for (i=0; i<n; i++) {
        t = t + chart[i].duration;     
        printf("%s %d ", chart[i].name, t);
    }
    printf("\n");
}

// Global variables
int tq0 = 0, tq1 = 0;
int process_table_size = 0;
struct Process process_table[MAX_NUM_PROCESS];



// Helper function: Check whether the line is a blank line (for input parsing)
int is_blank(char *line) {
    char *ch = line;
    while ( *ch != '\0' ) {
        if ( !isspace(*ch) )
            return 0;
        ch++;
    }
    return 1;
}
// Helper function: Check whether the input line should be skipped
int is_skip(char *line) {
    if ( is_blank(line) )
        return 1;
    char *ch = line ;
    while ( *ch != '\0' ) {
        if ( !isspace(*ch) && *ch == '#')
            return 1;
        ch++;
    }
    return 0;
}
// Helper: parse_tokens function
void parse_tokens(char **argv, char *line, int *numTokens, char *delimiter) {
    int argc = 0;
    char *token = strtok(line, delimiter);
    while (token != NULL)
    {
        argv[argc++] = token;
        token = strtok(NULL, delimiter);
    }
    *numTokens = argc;
}

// Helper: parse the input file
void parse_input() {
    FILE *fp = stdin;
    char *line = NULL;
    ssize_t nread;
    size_t len = 0;

    char *two_tokens[2]; // buffer for 2 tokens
    char *three_tokens[3]; // buffer for 3 tokens
    int numTokens = 0, n=0, i=0;
    char equal_plus_spaces_delimiters[5] = "";

    char process_name[MAX_PROCESS_NAME];
    int process_arrival_time = 0;
    int process_burst_time = 0;

    strcpy(equal_plus_spaces_delimiters, "=");
    strcat(equal_plus_spaces_delimiters,SPACE_CHARS);

    while ( (nread = getline(&line, &len, fp)) != -1 ) {
        if ( is_skip(line) == 0)  {
            line = strtok(line,"\n");

            if (strstr(line, KEYWORD_TQ0)) {
                // parse tq0
                parse_tokens(two_tokens, line, &numTokens, equal_plus_spaces_delimiters);
                if (numTokens == 2) {
                    sscanf(two_tokens[1], "%d", &tq0);
                }
            } 
            else if (strstr(line, KEYWORD_TQ1)) {
                // parse tq0
                parse_tokens(two_tokens, line, &numTokens, equal_plus_spaces_delimiters);
                if (numTokens == 2) {
                    sscanf(two_tokens[1], "%d", &tq1);
                }
            }
            else if (strstr(line, KEYWORD_PROCESS_TABLE_SIZE)) {
                // parse process_table_size
                parse_tokens(two_tokens, line, &numTokens, equal_plus_spaces_delimiters);
                if (numTokens == 2) {
                    sscanf(two_tokens[1], "%d", &process_table_size);
                }
            } 
            else if (strstr(line, KEYWORD_PROCESS_TABLE)) {

                // parse process_table
                for (i=0; i<process_table_size; i++) {

                    getline(&line, &len, fp);
                    line = strtok(line,"\n");  

                    sscanf(line, "%s %d %d", process_name, &process_arrival_time, &process_burst_time);
                    process_init(&process_table[i], process_name, process_arrival_time, process_burst_time);

                }
            }

        }
        
    }
}
// Helper: Display the parsed values
void print_parsed_values() {
    printf("%s = %d\n", KEYWORD_TQ0, tq0);
    printf("%s = %d\n", KEYWORD_TQ1, tq1);
    printf("%s = \n", KEYWORD_PROCESS_TABLE);
    process_table_print(process_table, process_table_size);
}

// TODO: Implementation of MLFQ algorithm
void mlfq() {

    // Initialize the gantt chart
    struct GanttChartItem chart[MAX_GANTT_CHART];
    int sz_chart = 0;
    
    // TODO: implement your MLFQ algorithm here
    struct Queue q0[MAX_QUEUE_SIZE], q1[MAX_QUEUE_SIZE], q2[MAX_QUEUE_SIZE];
    queue_init(q0);
    queue_init(q1);
    queue_init(q2);

    int time_elapsed_q0 = 0, time_elapsed_q1 = 0, time_elapsed_q2 = 0;

    int initial_process_value = 0;
    for(int i = 0; i < total_time; i++){
        if(initial_process_value < process_table_size){
            if(i == process_table[initial_process_value].arrival_time){
                queue_enqueue(q0, initial_process_value);
                process_table[initial_process_value].remain_time = tq0;
                initial_process_value++;
            }
        }
        //printf("Queue 0\n");
        //queue_print(q0);
        //printf("Queue 1\n");
        //queue_print(q1);
        //printf("Queue 2\n");
        //queue_print(q2);

        if(!queue_is_empty(q0)){    //Queue 0
            int q0_value = queue_peek(q0);
            if(process_table[q0_value].burst_time != 0 && process_table[q0_value].remain_time != 0){
                process_table[q0_value].burst_time -= 1;
                process_table[q0_value].remain_time -= 1;
                //time_elapsed_q0++;
                gantt_chart_update(chart, &sz_chart, process_table[q0_value].name, 1);

                if(process_table[q0_value].burst_time == 0){
                    queue_dequeue(q0);
                }
                else if(process_table[q0_value].remain_time == 0){
                    queue_enqueue(q1, q0_value);
                    queue_dequeue(q0);
                    process_table[q0_value].remain_time = tq1;
                }
            }
        }
        else if(!queue_is_empty(q1)){ //Queue 1
            int q1_value = queue_peek(q1);
            if(process_table[q1_value].burst_time != 0 && process_table[q1_value].remain_time != 0){
                process_table[q1_value].burst_time -= 1;
                process_table[q1_value].remain_time -= 1;
                gantt_chart_update(chart, &sz_chart, process_table[q1_value].name, 1);

                if(process_table[q1_value].burst_time == 0){
                    queue_dequeue(q1);
                }
                else if(process_table[q1_value].remain_time == 0){
                    queue_enqueue(q2, q1_value);
                    queue_dequeue(q1);
                    process_table[q1_value].remain_time = process_table[q1_value].burst_time;
                }
            }
        }
        else if(!queue_is_empty(q2)){
            int q2_value = queue_peek(q2);
            if(process_table[q2_value].burst_time != 0 && process_table[q2_value].remain_time != 0){
                process_table[q2_value].burst_time -= 1;
                process_table[q2_value].remain_time -= 1;
                gantt_chart_update(chart, &sz_chart, process_table[q2_value].name, 1);

                if(process_table[q2_value].burst_time == 0){
                    queue_dequeue(q2);
                }
            }
        }
    }

    // At the end, uncomment this line to display the final Gantt chart
    gantt_chart_print(chart, sz_chart);   
}


int main() {
    parse_input();
    print_parsed_values();
    mlfq();
    return 0;
}