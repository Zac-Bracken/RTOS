#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>

/* Programme for using threads to fill a water tank and signalling to a waiting thread and displaying the thread that overfilled
the water tank to the user */

// pthread mutex
pthread_mutex_t mutex;

// pthread signal
pthread_cond_t signal;

// to hold thread ids
struct ThreadData
{
    int bucket_thread_id;
    // pointer to water tank struct for allowing threads modification of the water tank shared resources
    struct WaterTank *water_tank;
};

// hold water tank information
struct WaterTank
{
    bool tank_overflow;
    int water_level;
    int max_water_level;
    int first_exceed_bucket_thread_id;
};

void *add_water(void *args)
{

    /* Provides functionality for adding a random number to the water tank */

    struct ThreadData *thread_data = (struct ThreadData *)args;
    // access water tank through the passed in thread data
    struct WaterTank *water_tank = thread_data->water_tank;
    int randomnumber;
    // get each threads unique id
    int bucket_thread_id = thread_data->bucket_thread_id;
    // change random seed
    srand(time(NULL));
    // terminate when a thread has exceeded the max water level
    while (!water_tank->tank_overflow)
    {
        // lock mutex to protect shared resource
        pthread_mutex_lock(&mutex);
        // generate a random number between 1 and 9 to add to the bucket
        randomnumber = rand() % 9 + 1;
        // increase water level by a random number between 1 and 9
        water_tank->water_level += randomnumber;
        printf("Bucket Thread %d added %d litres of water, tank water level is: %d\n", bucket_thread_id, randomnumber, water_tank->water_level);
        // if the water in the tank goes above the max water level set the first thread that crossed it
        if (water_tank->water_level > water_tank->max_water_level)
        {
            // store the thread that went over the limit
            water_tank->first_exceed_bucket_thread_id = bucket_thread_id;
            // set the tank to overflow
            water_tank->tank_overflow = true;
            // signal the waiting thread
            pthread_cond_signal(&signal);
        }
        // unlock mutex
        pthread_mutex_unlock(&mutex);
        sleep(2);
    }
    return 0;
}

void *waiting_bucket_thread_signal(void *arg)
{

    /* Provides functionality for the waiting thread */

    // for accessing the signal condition stores in the water tank struct
    struct WaterTank *water_tank = (struct WaterTank *)arg;
    // lock the mutex to protect shared resource
    pthread_mutex_lock(&mutex);
    // Wait for a signal from one of the add_water threads
    pthread_cond_wait(&signal, &mutex);
    // Tell the user what thread ID exceeded the maximum water level
    printf("Thread %d exceeded the maximum water level first!\n",
           water_tank->first_exceed_bucket_thread_id);
    // unlock the mutex
    pthread_mutex_unlock(&mutex);
}

int main()
{

    /* Main function */

    bool programme_run = true;
    // Prompt the user to enter the maximum water level
    while (programme_run)
    {
        int max_water_level;
        printf("Enter the maximum water level: ");
        scanf("%d", &max_water_level);

        // Prompt the user to enter the number of threads to use
        int number_threads;
        printf("Enter the number of threads to use: ");
        scanf("%d", &number_threads);

        // Initialize the tank struct
        struct WaterTank tank = {
            .tank_overflow = false,
            .water_level = 0,
            .max_water_level = max_water_level,
            .first_exceed_bucket_thread_id = 0};
        // initialise mutex
        pthread_mutex_init(&mutex, NULL);
        // condition variable
        pthread_cond_init(&signal, NULL);
        // array of threads
        pthread_t water_threads[number_threads];
        // array of thread structures
        struct ThreadData thread_data[number_threads];
        // iterate through threads
        for (int i = 0; i < number_threads; i++)
        {
            // printf("%d",number_threads);
            //  give each thread a unique ID starting from 1
            thread_data[i].bucket_thread_id = i + 1;
            thread_data[i].water_tank = &tank;
            // create threads with the add water function to be executed by the threads
            pthread_create(&water_threads[i], NULL, add_water, &thread_data[i]);
            sleep(1);
        }
        // Create the wait_signal thread
        pthread_t wait_signal_thread;
        pthread_create(&wait_signal_thread, NULL, waiting_bucket_thread_signal, &tank);
        // allow time for thread to start after being created
        sleep(1);

        // Join threads to wait for threads to terminate
        for (int i = 0; i < number_threads; i++)
        {
            pthread_join(water_threads[i], NULL);
        }
        pthread_join(wait_signal_thread, NULL);

        // run again if user specifies
        char run_again;
        // prompt user
        printf("Y to run again type anything else to exit: ");
        scanf(" %c", &run_again);
        // change user input to uppercase and compare
        if (toupper(run_again) == 'Y')
        {
        }
        else
        {
            // Set the flag to false to exit the loop
            programme_run = false;
        }
    }
    return 0;
}