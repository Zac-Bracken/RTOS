// required for time strptime func
#define _XOPEN_SOURCE
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <libgen.h>
#include <utime.h>

// Constants
#define FGET_BUFFER_SIZE 1024
#define MAX_COMMAND_LEN 4096
#define MAX_ARGS 100
// linux standard
#define MAX_PATH_LEN 4096
#define FGET_BUFFER_SIZE 1024

// for tracking current and previous directory for 'cd -' command
char previous_directory[MAX_PATH_LEN] = "";
char current_directory[MAX_PATH_LEN] = "";

void cd(char **args)
/* cd FUNCTIONALITY
 */
{
    // initialise for tracking current and previous directory

    if (args[1] == NULL)
    {
        printf("Try: cd <path>\n");
        return;
    }
    int value;
    // for checking if user entered -
    value = strcmp(args[1], "-");
    if (value == 0)
    {
        // if user inputs cd - change to the previous working directory
        if (chdir(previous_directory) != 0)
        {
            printf("No previous directory\n");
        }
        else
        {
            char temp[MAX_PATH_LEN];
            strcpy(temp, previous_directory);
            // gets previous directory as current_directory is still the previous directory
            strcpy(previous_directory, current_directory);
            // then update current_directory
            strcpy(current_directory, temp);
        }
    }
    else
    {
        // get the users current directory and store in previous_directory, error if null.
        if (getcwd(previous_directory, MAX_PATH_LEN) == NULL)
        {
            perror("getcwd");
            return;
        }
        // change to the directory specified by the user
        // printf(args[1]);
        if (chdir(args[1]) != 0)
        {
            perror("chdir");
        }
    }
}

void ls(char **args)
{
    /* LS FUNCTIONALITY */

    struct dirent *dir;
    DIR *directory;
    char current_directory[MAX_PATH_LEN];
    bool print_hidden_files = false;
    // for ls in current directory
    if (args[1] == NULL)
    {
        getcwd(current_directory, MAX_PATH_LEN);
        directory = opendir(current_directory);
    }
    else
    {
        // ls -a
        if (strcmp(args[1], "-a") == 0)
        {
            // set flag to print hidden files
            print_hidden_files = true;
            // for files in current directory
            getcwd(current_directory, MAX_PATH_LEN);
            directory = opendir(current_directory);
        }
        else
        {
            // ls <directory>
            directory = opendir(args[1]);
            // if the ls directory doesn't exist
            if (directory == NULL)
            {
                printf("No Such Directory '%s'\n", args[1]);
                return;
            }
        }
    }
    // set flag to print hidden files
    // ls <directory> -a
    if (args[2] != NULL && strcmp(args[2], "-a") == 0)
    {
        print_hidden_files = true;
    }
    // loop for printing files
    while ((dir = readdir(directory)) != NULL)
    {
        if (dir->d_name[0] == '.')
        {
            if (print_hidden_files)
            {
                // print files beginning with .
                printf("%s\n", dir->d_name);
            }
        }
        else
        {
            printf("%s\n", dir->d_name);
        }
    }
    // close directory stream
    closedir(directory);
}

void mkdir_cmd(char **args)
{
    /* mkdir Functionality
     */
    if (args[1] == NULL)
    {
        printf("Try: mkdir <directory name> [target directory]\n");
        return;
    }

    if (mkdir(args[1], 0700) == -1)
    {
        perror("Error");
        return;
    }
    printf("Directory '%s' has been successfully created\n", args[1]);
}

void rmdir_cmd(char **args)
{
    /* rmdir Functionality */

    if (args[1] == NULL)
    {
        printf("Try: rmdir <directory name>\n");
        return;
    }

    if (rmdir(args[1]) == -1)
    {
        perror("Error");
        return;
    }
    printf("Directory '%s' has been successfully removed\n", args[1]);
}

void touch_cmd(char **args)
{

    /* touch Functionality */

    if (args[1] == NULL)
    {
        printf("Try: touch <filename>\n");
        return;
    }

    // Check if the user specified the time option (-t)
    if (strcmp(args[1], "-t") == 0 && args[2] != NULL)
    {
        // put into time struct
        struct tm input_time_info;
        // options found here https://man7.org/linux/man-pages/man3/strptime.3.html (reference in bibliography)
        // input 203801191205.09 would be Tuesday 19 Jan 2038 12∶05∶09 GMT, this format is the linux format.
        strptime(args[2], "%Y%m%d%H%M.%S", &input_time_info);
        // set access and modification
        struct utimbuf times;
        times.actime = mktime(&input_time_info);
        times.modtime = mktime(&input_time_info);
        if (utime(args[3], &times) == -1)
        {
            perror("Error");
            return;
        }
        printf("Time changed successfully\n");
        return;
    }
    if (args[1] == NULL)
    {
        printf("Try: touch <filename>\n");
        return;
    }
    // file created if it does not exist
    fopen(args[1], "a");
}

void cp_cmd(char **args)
{
    /* cp Functionality */

    int i_option_value;
    // File pointer
    FILE *cp_fptr1, *cp_fptr2;
    char filename[100];
    char character_var;

    if (args[1] == NULL || args[2] == NULL)
    {
        printf("Try: cp <source file> <target file>\n");
        return;
    }

    // interactive mode cp -i <source file> <target file>
    i_option_value = strcmp(args[1], "-i");
    if (i_option_value == 0)
    {
        // If file already exists
        if (access(args[3], F_OK) == 0)
        {
            // verify with the user
            printf("Would you like to overwrite the existing file? ");
            // 5 because null character is added
            char input[5];
            fgets(input, 5, stdin);
            // if user says yes file will be copied and overwritten
            if (strncmp(input, "yes", 3) == 0)
            {
                // read mode
                cp_fptr1 = fopen(args[2], "r");
                // write mode
                cp_fptr2 = fopen(args[3], "w");
                // check file exists
                if (cp_fptr1 == NULL)
                {
                    printf("Cannot open file %s \n", basename(args[2]));
                    return;
                }
                if (cp_fptr2 == NULL)
                {
                    printf("Cannot open file %s \n", basename(args[2]));
                    return;
                }
                // get character from file stream
                character_var = fgetc(cp_fptr1);
                // until end of file
                while (character_var != EOF)
                {
                    // put characters in file
                    fputc(character_var, cp_fptr2);
                    // get characters from file 1
                    character_var = fgetc(cp_fptr1);
                }
                // release memory
                fclose(cp_fptr1);
                fclose(cp_fptr2);
                return;
            }
            // if user says no file will not be copied
            else if (strncmp(input, "no", 2) == 0)
            {
                return;
            }
            else
            {
                printf("only yes or no is accepted\n");
                return;
            }
        }
        // if file doesn't already exist
        else
        {
            // open in reading mode
            cp_fptr1 = fopen(args[2], "r");
            if (cp_fptr1 == NULL)
            {
                printf("Cannot open file %s \n", basename(args[2]));
                return;
            }
            // open in writing mode
            cp_fptr2 = fopen(args[3], "w");
            if (cp_fptr2 == NULL)
            {
                printf("Cannot open file %s \n", basename(args[3]));
                return;
            }
            //
            character_var = fgetc(cp_fptr1);
            while (character_var != EOF)
            {
                // put characters in
                fputc(character_var, cp_fptr2);
                character_var = fgetc(cp_fptr1);
            }
            // release memory
            fclose(cp_fptr1);
            fclose(cp_fptr2);
            return;
        }
    }
    // -i not specified run this

    // open first input file for reading
    cp_fptr1 = fopen(args[1], "r");
    if (cp_fptr1 == NULL)
    {
        printf("Cannot open file %s \n", basename(args[1]));
        return;
    }
    // open second file for writing
    cp_fptr2 = fopen(args[2], "w");
    if (cp_fptr2 == NULL)
    {
        printf("Cannot open file %s \n", basename(args[2]));
        return;
    }
    character_var = fgetc(cp_fptr1);
    while (character_var != EOF)
    {
        fputc(character_var, cp_fptr2);
        character_var = fgetc(cp_fptr1);
    }
    // release memory
    fclose(cp_fptr1);
    fclose(cp_fptr2);
}

void mv_cmd(char **args)
{
    /* mv functionality
     */
    bool verbose_mode = false;
    int file_index;
    int new_index;
    if (args[1] == NULL || args[2] == NULL)
    {
        printf("Try: mv <directory name> <new directory name>\n");
        printf("Try: mv <filename> <new filename>\n");
        return;
    }

    int verbose_option_value = strcmp(args[1], "-v");
    // for handling the verbose option
    if (verbose_option_value == 0)
    {
        // set verbose mode and the correct index for the users input
        verbose_mode = true;
        file_index = 2;
        new_index = 3;
    }
    else
    {
        file_index = 1;
        new_index = 2;
    }

    int mv_result = rename(args[file_index], args[new_index]);
    if (mv_result == 0)
    {
        // verbose mode will log what has happened
        if (verbose_mode)
        {
            printf(" '%s' -> '%s'\n", args[file_index], args[new_index]);
            return;
        }
        else
        {
            return;
        }
    }
    else
    {
        // check source file or directory exists
        FILE *mv_fptr1 = fopen(args[file_index], "r");
        if (mv_fptr1 == NULL)
        {
            printf("Cannot open file %s\n", basename(args[file_index]));
            return;
        }

        // check target directory exists
        DIR *dir = opendir(args[new_index]);
        if (dir)
        {
            // if the second argument is a directory, move the file to that directory
            char new_mv_path[MAX_PATH_LEN];
            strcpy(new_mv_path, args[new_index]);

            // add a slash at the end of the directory name, if it's missing
            if (new_mv_path[strlen(new_mv_path) - 1] != '/')
            {
                strcat(new_mv_path, "/");
            }
            // join the path the user wants the file moved to with the specified filename
            strcat(new_mv_path, basename(args[file_index]));

            // move file with rename
            int mvResult2 = rename(args[file_index], new_mv_path);
            if (mvResult2 == 0)
            {
                if (verbose_mode)
                {
                    printf(" '%s' -> '%s' \n", args[file_index], new_mv_path);
                }
                else
                {
                    return;
                }
            }
            else
            {
                printf("Could not move file - Check permissions\n");
            }
        }
        else
        {
            printf("Directory does not exist\n");
        }
    }
}

void rm_cmd(char **args)
{
    /* rm functionality */

    // for tracking whether the user specifies -r
    bool rm_directory_mode = false;
    if (args[1] == NULL)
    {
        printf("Try: rm <filename> \n");
        return;
    }
    int rm_directory_option_value = strcmp(args[1], "-r");
    if (rm_directory_option_value == 0)
    {
        rm_directory_mode = true;
    }

    // loop starts at 2 if -r is specified and 1 if not because 'rm -r <directory>' puts the directory at 2
    for (int arg_index = rm_directory_mode ? 2 : 1; args[arg_index] != NULL; arg_index++)
    {
        // checks if a directory
        DIR *check_if_directory = opendir(args[arg_index]);
        // if the user tries to remove a directory and they haven't specified -r
        if (check_if_directory != NULL && !rm_directory_mode)
        {
            printf("Use -r to remove a directory\n");
            return;
        }
        // remove file or directory
        int rm_result = remove(args[arg_index]);
        if (rm_result == 0)
        {
            printf("Successfully removed %s\n", args[arg_index]);
        }
        else
        {
            printf("Cannot Remove, is the file empty? %s\n", args[arg_index]);
        }
    }
}

void cat_cmd(char **args)
{
    /* cat Functionality */

    bool line_mode = false;
    int input_file = 1;
    int line_number_counter = 1;
    int character;
    FILE *cat_file;
    int line_mode_option_value = strcmp(args[1], "-n");
    // if user specifies -n
    if (line_mode_option_value == 0)
    {
        line_mode = true;
        input_file = 2;
    }
    // openfile
    cat_file = fopen(args[input_file], "r");
    // if the cat_file exists
    if (cat_file)
    {
        // if the user specified the line option print 1: for line 1
        if (line_number_counter == 1 && line_mode)
        {
            printf("1: ");
        }
        // run until end of file
        while ((character = getc(cat_file)) != EOF)
        {
            // put each character to console
            putchar(character);
            // at the end of each line print number if the user specified the line option
            if (character == '\n' && line_mode)
            {
                line_number_counter++;
                printf("%d: ", line_number_counter);
            }
        }
        putchar('\n');
        // close file
        fclose(cat_file);
    }
    else
    {
        printf("Cannot read file \n");
    }
}

void head_cmd(char **args)
{
    /* head functionality*/

    FILE *head_file;
    char head_file_line[FGET_BUFFER_SIZE];
    int lines_printed = 0;
    int input_file = 1;
    int number_lines;

    if (args[1] == NULL)
    {
        printf("Try: head <filename> \n");
        printf("Try: head -n <number of lines> <filename> \n");
        return;
    }

    // head -n 2 file
    // head file
    int specified_num_lines_value = strcmp(args[1], "-n");
    // head -n
    if (specified_num_lines_value == 0)
    {
        if (args[3] == NULL || args[2] == NULL)
        {
            printf("Try: head -n <number of lines> <file>\n");
            return;
        }
        input_file = 3;
        // specified number of lines
        // convert to int
        number_lines = atoi(args[2]);
    }
    else
    {
        // default number of lines
        number_lines = 10;
    }
    // open users specified file
    head_file = fopen(args[input_file], "r");
    if (head_file == NULL)
    {
        printf("Cannot open file %s\n", args[input_file]);
        return;
    }

    while (lines_printed < number_lines && fgets(head_file_line, FGET_BUFFER_SIZE, head_file) != NULL)
    {
        printf("%s", head_file_line);
        lines_printed++;
    }
    // add a new line because there is no new line within a txt file of 1 line
    if (lines_printed <= 1)
    {
        // write single newline
        putchar('\n');
    }
    // release memory close file
    fclose(head_file);
}

void tail_cmd(char **args)
{
    /* tail Functionality
    Allows users to view the end of a text file with specified number of lines
     */
    FILE *tail_file;
    char tail_file_line[FGET_BUFFER_SIZE];
    int line_index;
    int file_index = 1;
    int number_lines;

    if (args[1] == NULL)
    {
        printf("Try: tail <filename> \n");
        return;
    }
    // tail -n 2 file
    // tail file
    int specified_num_lines_value = strcmp(args[1], "-n");
    if (specified_num_lines_value == 0)
    {
        if (args[3] == NULL || args[2] == NULL)
        {
            printf("Try: tail -n <number of lines> <filename>\n");
            return;
        }
        file_index = 3;
        // specified number of lines
        // convert to int
        number_lines = atoi(args[2]);
    }
    else
    {
        // default number of lines
        number_lines = 10;
    }
    tail_file = fopen(args[file_index], "r");
    if (tail_file == NULL)
    {
        printf("Cannot open file %s\n", args[file_index]);
        return;
    }

    // Count how many lines there are in the file
    int total_lines = 0;
    while (fgets(tail_file_line, FGET_BUFFER_SIZE, tail_file) != NULL)
    {
        total_lines++;
    }

    // use seek to find the last ten lines
    int seek_last_lines = total_lines - number_lines;
    fseek(tail_file, 0, SEEK_SET);
    for (line_index = 0; line_index < seek_last_lines; line_index++)
    {
        fgets(tail_file_line, FGET_BUFFER_SIZE, tail_file);
    }
    // Print the last specified number of lines
    // get the specified number of lines from specified file
    while (fgets(tail_file_line, FGET_BUFFER_SIZE, tail_file) != NULL)
    {
        printf("%s", tail_file_line);
    }
    // new line
    putchar('\n');
    // cleanup free memory
    fclose(tail_file);
}

void more_cmd(char **args)
{
    /* more Functionality
    Allows users to view the contents of a text file a screen at a time
    */
    FILE *more_file;
    char line[4096];
    size_t len = 0;
    int line_count = 0;
    int get_user_key_press;
    int file_index = 1;

    // dynamically get terminal lines
    int cmd_window_size = atoi(getenv("LINES")) - 1;

    if (args[1] == NULL)
    {
        printf("Try: more <file>\n");
        return;
    }
    int p_option_value = strcmp(args[1], "-p");
    if (p_option_value == 0)
    {
        printf("\033[H\033[J");
        file_index = 2;
    }
    more_file = fopen(args[file_index], "r");
    if (more_file == NULL)
    {
        printf("Error: could not open file\n");
        return;
    }
    while (fgets(line, FGET_BUFFER_SIZE, more_file) != NULL)
    {
        printf("%s", line);

        // track how many lines displayed
        line_count++;
        // when the line_count equals the maximum number of lines
        if (line_count == cmd_window_size)
        {
            printf("--More-- [Enter to continue] ");
            get_user_key_press = getchar();
            line_count = 0;
        }
    }

    // new line
    putchar('\n');
    fclose(more_file);
}

void clear_screen(char **args)
{
    /* Provides functionality to clear users command prompt */

    printf("\033[H\033[J");
}

// this defines a pointer to a function that takes in a char ** (args) and returns void.
typedef void (*cmd_function)(char **);
// custom struct for command table allowing a name and corresponding function for scalability
typedef struct
{
    const char *cmd_name;
    // custom typedef
    cmd_function cmd_func;
} wshell_commands;

wshell_commands available_commands[] = {
    /* All available commads and their functions */
    {"cd", cd},
    {"ls", ls},
    {"mkdir", mkdir_cmd},
    {"rmdir", rmdir_cmd},
    {"touch", touch_cmd},
    {"cp", cp_cmd},
    {"mv", mv_cmd},
    {"rm", rm_cmd},
    {"cat", cat_cmd},
    {"head", head_cmd},
    {"tail", tail_cmd},
    {"more", more_cmd},
    {"cls", clear_screen},
    // for tracking commands in the available commands loop, allows for exit if NULL reached
    {NULL, NULL}};

void execute_user_command(char *cmd)
/* Handles users input commands */
{
    // initialise values to 0
    char *args[MAX_ARGS] = {0};
    int args_index = 0;
    char *next_token = NULL;
    // tokens split at defined delimiters of new line and tab/space
    char *arg = strtok_r(cmd, " \t\n", &next_token);

    while (arg != NULL && args_index < MAX_ARGS - 1)
    {
        // store parts of the command in args array so if user inputs cd /home args[0] will equal cd and args[1] will equal /home
        args[args_index++] = arg;
        // keep track of positions, space delimiter of new line and tab space
        arg = strtok_r(NULL, " \t\n", &next_token);
    }

    int cmd_index = 0;
    // search whether the user entered command is available and when get to NULL in command table and print the command not found
    while (available_commands[cmd_index].cmd_name != NULL)
    {
        // args[0] will be the user entered command as its the first thing they type in
        // this checks if the command is available and if so runs it
        if (strcmp(args[0], available_commands[cmd_index].cmd_name) == 0)
        {
            available_commands[cmd_index].cmd_func(args);
            return;
        }
        // allows iteration through all commands
        cmd_index++;
    }

    // if the command is not available then print
    printf("Did you enter the correct command? Command Not Found: %s\n", args[0]);
}

int main(void)
{

    /* Main Function */

    char dir[MAX_PATH_LEN + 2];

    // get initial current directory
    if (getcwd(dir, MAX_PATH_LEN) != NULL)
    {
        // concatenate directory with $ symbol
        strcat(dir, "$ ");
    }
    else
    {
        // set dir to "$"
        strcpy(dir, "$ ");
    }
    // main loop of cmd line
    while (true)
    {
        // print prompt
        printf("%s", dir);
        // get user input
        char user_input_cmd[1024];
        if (fgets(user_input_cmd, FGET_BUFFER_SIZE, stdin) == NULL)
        {
            perror("Error");
            break;
        }
        // execute users chosen command
        execute_user_command(user_input_cmd);
        char cwd[MAX_PATH_LEN];
        // Update prompt with current directory
        if (getcwd(cwd, MAX_PATH_LEN) != NULL)
        {
            sprintf(dir, "%s$ ", cwd);
        }
        else
        {
            strcpy(dir, "$ ");
        }
    }
    return 0;
}