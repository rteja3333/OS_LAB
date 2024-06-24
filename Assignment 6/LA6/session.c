#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "event.h"

#define MAX_PATIENTS 25
#define MAX_REPORTERS __INT_MAX__
#define MAX_SALES_REPS 3
typedef struct {
    event e;    // Event information
    int token;  // Token
} eventinfo;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t doctor_condition = PTHREAD_COND_INITIALIZER;
pthread_mutex_t visitor_mutex = PTHREAD_MUTEX_INITIALIZER;// to mutex curr_event
pthread_mutex_t visitor_mutex1 = PTHREAD_MUTEX_INITIALIZER;// to mutex curr_time

pthread_cond_t doctor_done_with_curr_visitor = PTHREAD_COND_INITIALIZER;
pthread_mutex_t doctor_done_mutex = PTHREAD_MUTEX_INITIALIZER;



pthread_cond_t patient_condition = PTHREAD_COND_INITIALIZER;
pthread_cond_t reporter_condition = PTHREAD_COND_INITIALIZER;
pthread_cond_t sales_condition = PTHREAD_COND_INITIALIZER;


int doctor_done=0;
int no_of_patients = 0;
int no_of_reporters = 0;
int no_of_sales = 0;
int checked_patients=0;
int checked_sales=0;
int curr_time = 0; // Global variable to track current time
event curr_event;

eventQ EQ; // Event queue
eventQ asst_eventQ; // Assistant's event queue
void print_current_time(int curr_time1) {
    // pthread_mutex_lock(&visitor_mutex1);

    int adjusted_hour = ((curr_time1 + 540) / 60) % 12; // Add 540 minutes (9 hours) to handle negative values
    int minute = curr_time1 % 60;
    if (minute < 0) {
        // adjusted_hour--; // Adjust hour for negative minutes
        minute += 60; // Convert negative minutes to positive
    }
    const char *suffix = ((curr_time1 + 540) / 60) < 12 ? "AM" : "PM";

    // pthread_mutex_unlock(&visitor_mutex1);

    printf("[%02d:%02d%s]", adjusted_hour == 0 ? 12 : adjusted_hour, minute, suffix);
}
void print_time_interval(int start_time, int duration) {
    // Calculate start time components
    int start_hour = ((start_time + 540) / 60) % 12; // Add 540 minutes (9 hours) to handle negative values
    int start_minute = start_time % 60;
    if (start_minute < 0) {
        start_hour--; // Adjust hour for negative minutes
        start_minute += 60; // Convert negative minutes to positive
    }
    const char *start_suffix = ((start_time + 540) / 60) < 12 ? "AM" : "PM";

    // Calculate end time components
    int end_time = start_time + duration;
    int end_hour = ((end_time + 540) / 60) % 12; // Add 540 minutes (9 hours) to handle negative values
    int end_minute = end_time % 60;
    if (end_minute < 0) {
        end_hour--; // Adjust hour for negative minutes
        end_minute += 60; // Convert negative minutes to positive
    }
    const char *end_suffix = ((end_time + 540) / 60) < 12 ? "AM" : "PM";

    // Print the time interval
    printf("[%02d:%02d%s ‒ %02d:%02d%s]",
           start_hour == 0 ? 12 : start_hour, start_minute, start_suffix,
           end_hour == 0 ? 12 : end_hour, end_minute, end_suffix);
}

// Function to compare events based on priority
int asst_eventcmp(event e1, event e2) {
    if(e1.type==e2.type){
        if(e1.time>e2.time)return 1;
        return -1;
    }
    if (e1.type == 'R') return -1;
    if (e2.type == 'R') return 1;
    if (e1.type == 'P') return -1;
    if (e2.type == 'P') return 1;
    if (e1.type == 'S') return -1;
    if (e2.type == 'S') return 1;
    return 0;
}

// Function to check if assistant's event queue is empty
int asst_emptyQ(eventQ E) {
    return (E.n == 0);
}

// Function to add event to assistant's event queue
eventQ asst_addevent(eventQ E, event e) {
    int i, p;
    event t;
    // printf("adding event in asst Q\n");
    E.Q[E.n] = e;
    i = E.n;
    // printf("1");
    while (1) {
        // printf("2\n");
        if (i == 0) break;
        p = (i - 1) / 2;
        if (asst_eventcmp(E.Q[p], E.Q[i]) < 0) break;
        t = E.Q[i]; E.Q[i] = E.Q[p]; E.Q[p] = t;
        i = p;
    }
    
    ++E.n;
    // printf("Added event in asst Q\n");
    return E;
}

// Function to delete event from assistant's event queue
eventQ asst_delevent(eventQ E) {
    event t;
    int i, l, r, m;

    if (E.n > 0) {
        E.Q[0] = E.Q[E.n - 1];
        --E.n;
        i = 0;
        while (1) {
            l = 2 * i + 1; r = 2* i + 2;
            if (l >= E.n) break;
            m = ( (r == E.n) || (asst_eventcmp(E.Q[l],E.Q[r]) < 0) ) ? l : r;
            if (asst_eventcmp(E.Q[i], E.Q[m]) < 0) break;
            t = E.Q[i]; E.Q[i] = E.Q[m]; E.Q[m] = t;
            i = m;
        }
    }
    return E;
}

// Function to get next event from assistant's event queue
event asst_nextevent(eventQ E) {
    if (E.n == 0) return (event){'E', 0, 0};
    return E.Q[0];
}

int event_cmp(event e1, event e2) {
    if (e1.type != e2.type || e1.time != e2.time || e1.duration != e2.duration) {
        return 1; // Events are not equal
    }
    // printf("equal");
    return 0; // Events are equal
}


// Visitor thread function for patients
void *patient(void *arg) {
    eventinfo *info = (eventinfo *)arg;
    // int arrival_time = info->e.time;

    while (1) {
        pthread_mutex_lock(&doctor_done_mutex);
                // printf("patient %d waiting for the doctor\n",info->token);

        pthread_cond_wait(&patient_condition, &doctor_done_mutex); // Wait for signal from doctor
        pthread_mutex_unlock(&doctor_done_mutex);


            // Check if the current event matches the event info supplied to the thread
                    pthread_mutex_lock(&visitor_mutex);
        // printf("visitor mutex locked in patient,cuurent event is %c\n",curr_event.type);


        if (event_cmp(curr_event, info->e) == 0) {
            // printf("event equal in patient\n");
                    pthread_mutex_unlock(&visitor_mutex);
                            // pthread_mutex_lock(&visitor_mutex1);

            print_time_interval(curr_time,info->e.duration);
        // pthread_mutex_unlock(&visitor_mutex1);

            printf(" Patient %d is in doctors chamber\n",info->token);

            break; // Break out of the loop if visiting is done
        }
                pthread_mutex_unlock(&visitor_mutex);

    }

    return NULL;
}

void *reporter(void *arg) {
    eventinfo *info = (eventinfo *)arg;
    // int arrival_time = info->e.time;

    while (1) {
        pthread_mutex_lock(&doctor_done_mutex);
                // printf("reporter %d waiting for the doctor\n",info->token);

        pthread_cond_wait(&reporter_condition, &doctor_done_mutex); // Wait for signal from doctor
        pthread_mutex_unlock(&doctor_done_mutex);


            // Check if the current event matches the event info supplied to the thread
                    pthread_mutex_lock(&visitor_mutex);
        // printf("visitor mutex locked in reporter,cuurent event is %c\n",curr_event.type);


        if (event_cmp(curr_event, info->e) == 0) {
            // printf("event equal in reporter\n");
                    pthread_mutex_unlock(&visitor_mutex);
                            // pthread_mutex_lock(&visitor_mutex1);

            print_time_interval(curr_time,info->e.duration);
        // pthread_mutex_unlock(&visitor_mutex1);

            printf(" Reporter %d is in doctors chamber\n",info->token);

            break; // Break out of the loop if visiting is done
        }
                pthread_mutex_unlock(&visitor_mutex);

    }

    return NULL;
}

void *sales(void *arg) {
    eventinfo *info = (eventinfo *)arg;
    // int arrival_time = info->e.time;

    while (1) {
        pthread_mutex_lock(&doctor_done_mutex);
                        // printf("sales representative %d waiting for the doctor\n",info->token);

        pthread_cond_wait(&sales_condition, &doctor_done_mutex); // Wait for signal from doctor
        pthread_mutex_unlock(&doctor_done_mutex);

        // Check if the current event matches the event info supplied to the thread
                pthread_mutex_lock(&visitor_mutex);
        if (event_cmp(curr_event, info->e) == 0) {
                    pthread_mutex_unlock(&visitor_mutex);
                            // pthread_mutex_lock(&visitor_mutex1);

            print_time_interval(curr_time,info->e.duration);
                    // pthread_mutex_unlock(&visitor_mutex1);

            printf(" Sales representative %d is in doctors chamber\n",info->token);

            // printf("Sales representative visiting done.\n");
            break; // Break out of the loop if visiting is done
        }
                pthread_mutex_unlock(&visitor_mutex);
    }

    return NULL;
}


// Doctor thread function
void *doctor(void *arg) {
    // printf("doctor process started\n");
    while (1) {
        pthread_mutex_lock(&doctor_done_mutex);
        if(doctor_done && asst_eventQ.n==0)break;
        pthread_cond_signal(&doctor_done_with_curr_visitor);
        // printf("doctor signalled im free\n");
        pthread_cond_wait(&doctor_condition, &doctor_done_mutex); // Wait for signal indicating current visitor is done
        pthread_mutex_unlock(&doctor_done_mutex);

        // Check the type of current event and broadcast signal accordingly
        
        pthread_mutex_lock(&visitor_mutex);
        if (curr_event.type == 'P') {

            // printf("doctor broadcasted for patient thread\n");
            pthread_cond_broadcast(&patient_condition);
            checked_patients++;
            pthread_mutex_unlock(&visitor_mutex);

        } else if (curr_event.type == 'R') {

                        // printf("doctor broadcasted for reporter thread\n");

            pthread_cond_broadcast(&reporter_condition);
            pthread_mutex_unlock(&visitor_mutex);
        } else if (curr_event.type == 'S') {
                        // printf("doctor broadcasted for sales thread\n");

            pthread_cond_broadcast(&sales_condition);
            checked_sales++;
            pthread_mutex_unlock(&visitor_mutex);
        }
        pthread_mutex_unlock(&visitor_mutex);
        sleep(2);
        if(doctor_done && asst_eventQ.n==0)break;

        // printf("Meeting done.\n");
    }
    // print_current_time(curr_time);
    printf("!!!!!!!!!!!!!!Doctor closed\n");
    return NULL;
}

void initialize_asst_eventQ() {
            // pthread_mutex_lock(&visitor_mutex1);

    if (emptyQ(asst_eventQ) && !emptyQ(EQ) &&  EQ.Q[0].time > curr_time) {
                // pthread_mutex_unlock(&visitor_mutex1);
        // printf("!!!case1\n");
        //case when no visitors till current time ,so doctor is fprint_curee for some time 
         event e = nextevent(EQ); // Get next event from main event queue
        printf("\t\t ");
        print_current_time(e.time);
printf(" %s %d Arrives\n",
       (e.type == 'P') ? "Patient" : ((e.type == 'R') ? "Reporter" : "Sales representative"),
       (e.type == 'P') ? no_of_patients + 1 : ((e.type == 'R') ? no_of_reporters + 1 : no_of_sales + 1));
        if (e.type == 'P') {
            if (no_of_patients >= MAX_PATIENTS) {
                 pthread_mutex_lock(&doctor_done_mutex);
                         printf("\t\t ");

                 print_current_time(e.time);
                    if(doctor_done)printf(" Patient %d leaves (session over)\n",no_of_patients+1);
                    else             printf(" Patient %d leaves (quota full)\n",no_of_patients+1);
                    no_of_patients++;
                pthread_mutex_unlock(&doctor_done_mutex);
                
            } else {
                pthread_t patient_thread;
                eventinfo *info = malloc(sizeof(eventinfo));
                info->e = e;
                info->token = no_of_patients+1;
                pthread_create(&patient_thread, NULL, patient, (void *)info);
                pthread_detach(patient_thread); // Detach thread to allow it to run independently
                no_of_patients++;
                // printf("no_of_patients:%d\n",no_of_patients);
            }
        } else if (e.type == 'R') {
            if (doctor_done) {
                 pthread_mutex_lock(&doctor_done_mutex);
                         printf("\t\t ");

                 print_current_time(e.time);
                 printf(" Reporter %d leaves (session over)\n",no_of_reporters+1);
                    no_of_reporters++;
                pthread_mutex_unlock(&doctor_done_mutex);
            } else {
                pthread_t reporter_thread;
                eventinfo *info = malloc(sizeof(eventinfo));
                info->e = e;
                info->token = no_of_reporters+1;
                pthread_create(&reporter_thread, NULL, reporter, (void *)info);
                pthread_detach(reporter_thread); // Detach thread to allow it to run independently
                no_of_reporters++;
            }
        } else if (e.type == 'S') {
            if (no_of_sales >= MAX_SALES_REPS) {
                 pthread_mutex_lock(&doctor_done_mutex);
                         printf("\t\t ");

                 print_current_time(e.time);
                    if(doctor_done)printf(" Sales representative %d leaves (session over)\n",no_of_sales+1);
                    else             printf(" Sales representative %d leaves (quota full)\n",no_of_sales+1);
                    no_of_sales++;
                pthread_mutex_unlock(&doctor_done_mutex);            
                } 
                else {
                pthread_t sales_thread;
                eventinfo *info = malloc(sizeof(eventinfo));
                info->e = e;
                info->token = no_of_sales+1;
                pthread_create(&sales_thread, NULL, sales, (void *)info);
                pthread_detach(sales_thread); // Detach thread to allow it to run independently
                no_of_sales++;
            }
        }
        // pthread_mutex_lock(&visitor_mutex1);

        curr_time=e.time;
        pthread_mutex_unlock(&visitor_mutex1);

        asst_eventQ = asst_addevent(asst_eventQ, e); // Add event to assistant's event queue
        EQ = delevent(EQ); // Delete event from main event queue   
        
    }
            // pthread_mutex_unlock(&visitor_mutex1);
        // pthread_mutex_lock(&visitor_mutex1);

    // Loop through the main event queue and add events with arrival time less than current time
    while (!emptyQ(EQ) && EQ.Q[0].time <= curr_time) {
                // pthread_mutex_unlock(&visitor_mutex1);

        event e = nextevent(EQ); // Get next event from main event queue

        // Print arrival event
        printf("\t\t");
        print_current_time(e.time);
printf(" %s %d Arrives\n",
       (e.type == 'P') ? "Patient" : ((e.type == 'R') ? "Reporter" : "Sales representative"),
       (e.type == 'P') ? no_of_patients + 1 : ((e.type == 'R') ? no_of_reporters + 1 : no_of_sales + 1));

        // Update corresponding counters
        if (e.type == 'P') {
            if (no_of_patients >= MAX_PATIENTS) {
                 pthread_mutex_lock(&doctor_done_mutex);
                 printf("\t\t");
                 print_current_time(e.time);
                    if(doctor_done)printf("Patient %d leaves (session over)\n",no_of_patients+1);
                    else             printf("Patient %d leaves (quota full)\n",no_of_patients+1);
                    no_of_patients++;
                pthread_mutex_unlock(&doctor_done_mutex);
                EQ = delevent(EQ); // Delete event from main event queue

                continue;
                
            } else {
                pthread_t patient_thread;
                eventinfo *info = malloc(sizeof(eventinfo));
                info->e = e;
                info->token = no_of_patients+1;
                pthread_create(&patient_thread, NULL, patient, (void *)info);
                pthread_detach(patient_thread); // Detach thread to allow it to run independently
                no_of_patients++;
                // printf("no_of_patients:%d\n",no_of_patients);
            }
        } else if (e.type == 'R') {
            pthread_mutex_lock(&doctor_done_mutex);

            if (doctor_done) {
                pthread_mutex_unlock(&doctor_done_mutex);

                 pthread_mutex_lock(&doctor_done_mutex);
                 printf("\t\t");
                 print_current_time(e.time);
                    printf("Reporter %d leaves (session over)\n",no_of_reporters+1);
                    no_of_reporters++;
                pthread_mutex_unlock(&doctor_done_mutex);
                EQ = delevent(EQ); // Delete event from main event queue

                continue;
            } else {
                pthread_mutex_unlock(&doctor_done_mutex);

                pthread_t reporter_thread;
                eventinfo *info = malloc(sizeof(eventinfo));
                info->e = e;
                info->token = no_of_reporters+1;
                pthread_create(&reporter_thread, NULL, reporter, (void *)info);
                pthread_detach(reporter_thread); // Detach thread to allow it to run independently
                no_of_reporters++;
            }
        } else if (e.type == 'S') {
            if (no_of_sales >= MAX_SALES_REPS) {
                 pthread_mutex_lock(&doctor_done_mutex);
                 printf("\t\t");
                 print_current_time(e.time);
                    if(doctor_done)printf("Sales representative %d leaves (session over)\n",no_of_sales+1);
                    else             printf("Sales representative %d leaves (quota full)\n",no_of_sales+1);
                    no_of_sales++;
                pthread_mutex_unlock(&doctor_done_mutex);  
                EQ = delevent(EQ); // Delete event from main event queue

                continue;          
                } 
                else {
                pthread_t sales_thread;
                eventinfo *info = malloc(sizeof(eventinfo));
                info->e = e;
                info->token = no_of_sales+1;
                pthread_create(&sales_thread, NULL, sales, (void *)info);
                pthread_detach(sales_thread); // Detach thread to allow it to run independently
                no_of_sales++;
            }
        }

        // token++; // Increment token counter
        asst_eventQ = asst_addevent(asst_eventQ, e); // Add event to assistant's event queue
        // printf("next event : %c\n",asst_eventQ.Q[0].type);
        EQ = delevent(EQ); // Delete event from main event queue

        
    }

        if(checked_patients>=MAX_PATIENTS && checked_sales>=MAX_SALES_REPS && asst_eventQ.n==0){
                    print_current_time(curr_time);
                    printf("Doctor leaves!!!\n");
                            pthread_mutex_lock(&doctor_done_mutex);
                            doctor_done=1;
                            pthread_mutex_unlock(&doctor_done_mutex);

                }
            // pthread_mutex_unlock(&visitor_mutex1);

        
}



// Assistant thread function
void *assistant(void *arg) {
    // Initialize event queue
    char *filename = "arrival.txt"; // Name of the file containing arrival schedule
    EQ = initEQ(filename);
    if (emptyQ(EQ)) {
        printf("Error: Empty event queue\n");
        exit(EXIT_FAILURE);
    }
   asst_eventQ.n = 0;
   asst_eventQ.Q = (event *)malloc(128 * sizeof(event));

    // printf("assistant thread\n");
    while (1) {
        // pthread_mutex_lock(&mutex);
        
        initialize_asst_eventQ(); // Initialize assistant's event queue
        // if (emptyQ(asst_eventQ)) {
        // printf("Hospital closed\n");
        // exit(EXIT_FAILURE);
        // }

        pthread_mutex_unlock(&doctor_done_mutex);
        // printf("111\n");
        if(EQ.n==0 && asst_eventQ.n==0)break;
        if(EQ.n!=0 && asst_eventQ.n==0){
            // printf("EQ not empty and asstQ empty\n");
            initialize_asst_eventQ();
            sleep(1);
        }

        if(doctor_done && (asst_eventQ.n!=0)){
            while(EQ.n!=0){
            // printf("still visitors coming after doctor leaves");
                    asst_eventQ = asst_delevent(asst_eventQ); // Delete event from assistant's event queue
            initialize_asst_eventQ();
            sleep(1);
            }
            break;
        }
                pthread_mutex_lock(&doctor_done_mutex);

        pthread_mutex_unlock(&doctor_done_mutex);
        // assistant waiting until doctor chamber is free
        pthread_mutex_lock(&doctor_done_mutex);
        // printf("waiting for doctor to become free\n");
        pthread_cond_wait(&doctor_done_with_curr_visitor, &doctor_done_mutex);
        pthread_mutex_unlock(&doctor_done_mutex);
        // printf("doctor is free now\n");
        curr_event = asst_nextevent(asst_eventQ); // Get next event from assistant's event queue
        print_current_time(curr_time);
        printf(" Doctor has next visitor\n");
        pthread_cond_signal(&doctor_condition); // Signal the doctor thread

        sleep(1);
        curr_time += curr_event.duration; // Update current time

        // pthread_mutex_lock(&visitor_mutex1);

        //         pthread_mutex_unlock(&visitor_mutex1);

        asst_eventQ = asst_delevent(asst_eventQ); // Delete event from assistant's event queue
        // printf("assistant scheduled a visitor\n");
        // pthread_mutex_unlock(&mutex);
        // pthread_cond_signal(&doctor_cond); // Signal the doctor thread
    }

    // Rest of the assistant's tasks
    // ...
    printf("Hospital closed !!!!\n");

    exit(EXIT_FAILURE);
}

int main() {
    pthread_t assistant_thread, doctor_thread;
    // Create assistant thread
    pthread_create(&assistant_thread, NULL, assistant, NULL);
    // Create doctor thread
    sleep(1);
    pthread_create(&doctor_thread, NULL, doctor, NULL);
    // Join threads
    pthread_join(assistant_thread, NULL);
    pthread_join(doctor_thread, NULL);

    // Destroy the mutexes
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&visitor_mutex);
    pthread_mutex_destroy(&visitor_mutex1);
    pthread_mutex_destroy(&doctor_done_mutex);

    // Destroy the condition variables
    pthread_cond_destroy(&doctor_condition);
    pthread_cond_destroy(&doctor_done_with_curr_visitor);
    pthread_cond_destroy(&patient_condition);
    pthread_cond_destroy(&reporter_condition);
    pthread_cond_destroy(&sales_condition);
    return 0;
}
