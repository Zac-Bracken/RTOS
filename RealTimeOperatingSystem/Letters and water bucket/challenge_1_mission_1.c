#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

/* Programme to calculate the number of letters in an 2D array of random alphabets */

// thread processing data
struct AlphabetThreadData
{
    // number of letters in alphabet
    int number_of_letters[26];
    int string_num;
    int thread_starting_index;
    int thread_ending_index;
    char **alphabet_array;
};

char **create_random_alphabet_array(int total_strings, int length_of_string)
{

    /* Function for creating the 2D array containg the random alphabet letters */

    // pointer to pointer array
    char **alphabet_array;
    // dynamically allocate memory for how many strings
    alphabet_array = malloc(total_strings * sizeof(char *));
    // for seeding the random generator
    srand(time(NULL));
    // iterate through 2d array
    for (int string_index = 0; string_index < total_strings; string_index++)
    {
        // dynamically allocate memory for strings
        alphabet_array[string_index] = malloc((length_of_string + 1) * sizeof(char));
        // iterate through strings characters
        for (int character_index = 0; character_index < length_of_string; character_index++)
        {
            // generate a random Uppercase Alphabet letter for each character within each string
            alphabet_array[string_index][character_index] = 'A' + rand() % 26;
            // check generated characters
            // printf("Character is %c\n",  alphabet_array[string_index][character_index]);
        }
        // Add the null terminator for indicating the end of a string
        alphabet_array[string_index][length_of_string] = '\0';
    }
    // return the newly created alphabet array containing random alphabet values
    return alphabet_array;
}

void *count_alphabet(void *arg)
{

    /* Count the number of each letter in the 2D array from each threads starting and end index*/

    struct AlphabetThreadData *alphabet_data = (struct AlphabetThreadData *)arg;
    // access struct start and end index's
    for (int string_index = alphabet_data->thread_starting_index; string_index < alphabet_data->thread_ending_index; string_index++)
    {
        char *string_point = alphabet_data->alphabet_array[string_index];
        for (int character_index = 0; character_index < strlen(string_point); character_index++)
        {
            // increments the found letters count in the number of letters array based on ASCI values
            alphabet_data->number_of_letters[toupper(string_point[character_index]) - 'A']++;
        }
    }
    return 0;
}

int main()
{

    /* Main function */

    bool programme_run = true;
    while (programme_run)
    {
        // initialise local vars
        int number_of_strings;
        int length_of_string;
        int number_threads;
        // ask user for number of strings and length of each string to create 2D array
        printf("Please enter the number of strings within the array: ");
        scanf("%d", &number_of_strings);
        printf("Please enter the number of characters within each string: ");
        scanf("%d", &length_of_string);
        printf("Enter the number of threads to use: ");
        scanf("%d", &number_threads);
        // create array with user specified values
        char **alphabet_array;
        alphabet_array = create_random_alphabet_array(number_of_strings, length_of_string);
        // sets the number of threads to be used
        // array of the thread data struct
        struct AlphabetThreadData alphabet_thred_data[number_threads];
        // distribute between the number of threads available
        int strings_per_thread = number_of_strings / number_threads;
        pthread_t threads[number_threads];
        // iterate through the total amount of threads
        for (int thread_index = 0; thread_index < number_threads; thread_index++)
        {
            // set up struct data for thread
            alphabet_thred_data[thread_index].alphabet_array = alphabet_array;
            // For testing memory address
            // printf("Thread %d alphabet_array address: %p\n", i, (void*)alphabet_thred_data[i].alphabet_array);
            alphabet_thred_data[thread_index].string_num = number_of_strings;
            // ensures the threads start and end at their correct index so no overlapping occurs.
            alphabet_thred_data[thread_index].thread_starting_index = thread_index * strings_per_thread;
            // if strings_per_thread was 5 then thread 0 end index would be 5, 1 would be 10, 2 would be 15 etc.
            alphabet_thred_data[thread_index].thread_ending_index = (thread_index + 1) * strings_per_thread;
            // check if the last thread, ending index set to total number of strings to process remaining strings when an odd number
            if (thread_index == number_threads - 1)
            {
                alphabet_thred_data[thread_index].thread_ending_index = number_of_strings;
            }
            // ensure memory values of number_of_letters array are set to 0
            memset(alphabet_thred_data[thread_index].number_of_letters, 0, sizeof(alphabet_thred_data[thread_index].number_of_letters));
            // creates thread and start to run the count alphabet function
            pthread_create(&threads[thread_index], NULL, count_alphabet, &alphabet_thred_data[thread_index]);
        }

        // for scalability and next run through
        int total_number_of_letters[26] = {0};
        // iterate through threads
        for (int thread_index = 0; thread_index < number_threads; thread_index++)
        {
            // join all threads
            pthread_join(threads[thread_index], NULL);
            for (int character_index = 0; character_index < 26; character_index++)
            {
                // get total number of letters from each thread process
                total_number_of_letters[character_index] += alphabet_thred_data[thread_index].number_of_letters[character_index];
            }
        }
        // for checking programme producing correct output
        int total = 0;
        int expected_total = 0;
        // print the total counts
        for (int alphabet_index = 0; alphabet_index < 26; alphabet_index++)
        {
            // print letters and total number of letters A: 0 B: 0 C: 0 etc, If alphabet index = 0, 0 + ASCII 65 is 65 so Letter A:
            // if alphabet index = 1, 1 + a ASCII 65 == 66 so Letter B:
            printf("%c: %d\n", alphabet_index + 'A', total_number_of_letters[alphabet_index]);
            // for tracking the total
            total += total_number_of_letters[alphabet_index];
        }

        expected_total = number_of_strings * length_of_string;
        // Print expected and total
        printf("Expected Total Letters: %d\n", expected_total);
        printf("Actual Total Letters: %d\n", total);

        // Again for scalability free used memory after it's not in use.
        // because memory was dynamically allocated for each string in array it needs to be looped and freed
        for (int string_index = 0; string_index < number_of_strings; string_index++)
        {
            // free dynamic memory
            free(alphabet_array[string_index]);
        }
        // free dynamic memory to prevent memory leaks
        free(alphabet_array);
        // for user prompt
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
