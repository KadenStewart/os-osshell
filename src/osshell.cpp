#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>
#include <filesystem>
#include <unistd.h>

bool fileExecutableExists(std::string file_path);
void splitString(std::string text, char d, std::vector<std::string>& result);
void vectorOfStringsToArrayOfCharArrays(std::vector<std::string>& list, char ***result);
void freeArrayOfCharArrays(char **array, size_t array_length);

int main (int argc, char **argv)
{
    // Get list of paths to binary executables
    std::vector<std::string> os_path_list;
    char* os_path = getenv("PATH");
    splitString(os_path, ':', os_path_list);

    // Create list to store history
    std::vector<std::string> history;

    // Create variables for storing command user types
    std::string user_command;               // to store command user types in
    std::vector<std::string> command_list;  // to store `user_command` split into its variour parameters
    char **command_list_exec;               // to store `command_list` converted to an array of character arrays

    // Welcome message
    printf("Welcome to OSShell! Please enter your commands ('exit' to quit).\n");
    while (true) { // While the user has not entered exit as a command
        std::cout << "osshell> "; //print out the prompt for the user with no newline
        if(!std::getline(std::cin, user_command)) { //get the user input for the next command
            break; //if there is an error getting the user input quit that crap.
        }
        // Handle 2 special cases.
        if (user_command == "exit") { // if user enters the command exit
            break; // quit    
        }
        if (user_command == "history"){ // if user enters history 
            for (int i = 0; i < history.size(); i++){
                std::cout << "  " << (i + 1) << " " << history[i] << std::endl; // Print the history 
            }
            continue; // skip rest of loop but dont exit. 
        }

        // Handle command history storage.
        history.push_back(user_command); // store the user command in the history vector.
        if (history.size() > 128) { // if the history is mmore than 128
            history.erase(history.begin()); // remove oldest command
        }
        // Process commands for execution.
        splitString(user_command, ' ', command_list); // split the command
        if(command_list.empty()) { // if the command is empty
            continue; // skip but dont quit
        }
        // seperate the initial command from the potential arguments. And path finding I guess.
        std::string cmd = command_list[0];
        std::string found_path = ""; //storage for path
        for (int i = 0; i < os_path_list.size(); i++) { // loop through paths
            std::string candidate = os_path_list[i] + "/" + cmd; // potential path to the command
            if (fileExecutableExists(candidate)) { // if the command is found in the path
                found_path = candidate; // store the path
                break; // stop looking for the command
            }    
        }
        if (found_path.empty()) { // if no path was found
            std::cout << cmd << ": Error command not found" << std::endl; // yell at user
            continue; // skip but dont quit
        }
        // we actually get to finally run the damn thing. 
        pid_t pid = fork(); //fork this thing
        if (pid < 0){
            perror("fork");
            freeArrayOfCharArrays(command_list_exec, command_list.size() + 1); // free the command list
            command_list_exec = nullptr; // set the command list to null
            continue; // skip but dont quit
        }
        if (pid == 0) {
            execv(found_path.c_str(), command_list_exec); // execute the command
            perror("execv"); // if execv returns, there was an error
            _exit(127); // exit with error to yell at user.
        }else {
            int status = 0;
            waitpid(pid, &status, 0); // wait for the prcess to finish
        }
        // Cleaning time
        freeArrayOfCharArrays(command_list_exec, command_list.size() + 1); // free the command list
        command_list_exec = nullptr; // set the command list to null


    }


    // /************************************************************************************
    //  *   Example code - remove in actual program                                        *
    //  ************************************************************************************/
    // // Shows how to loop over the directories in the PATH environment variable
    // int i;
    // for (i = 0; i < os_path_list.size(); i++)
    // {
    //     printf("PATH[%2d]: %s\n", i, os_path_list[i].c_str());
    // }
    // printf("------\n");
    
    // // Shows how to split a command and prepare for the execv() function
    // std::string example_command = "ls -lh";
    // splitString(example_command, ' ', command_list);
    // vectorOfStringsToArrayOfCharArrays(command_list, &command_list_exec);
    // // use `command_list_exec` in the execv() function rather than looping and printing
    // i = 0;
    // while (command_list_exec[i] != NULL)
    // {
    //     printf("CMD[%2d]: %s\n", i, command_list_exec[i]);
    //     i++;
    // }
    // // free memory for `command_list_exec`
    // freeArrayOfCharArrays(command_list_exec, command_list.size() + 1);
    // printf("------\n");

    // // Second example command - reuse the `command_list` and `command_list_exec` variables
    // example_command = "echo \"Hello world\" I am alive!";
    // splitString(example_command, ' ', command_list);
    // vectorOfStringsToArrayOfCharArrays(command_list, &command_list_exec);
    // // use `command_list_exec` in the execv() function rather than looping and printing
    // i = 0;
    // while (command_list_exec[i] != NULL)
    // {
    //     printf("CMD[%2d]: %s\n", i, command_list_exec[i]);
    //     i++;
    // }
    // // free memory for `command_list_exec`
    // freeArrayOfCharArrays(command_list_exec, command_list.size() + 1);
    // printf("------\n");
    // /************************************************************************************
    //  *   End example code                                                               *
    //  ************************************************************************************/


    // return 0;
}

/*
   file_path: path to a file
   RETURN: true/false - whether or not that file exists and is executable
*/
bool fileExecutableExists(std::string file_path)
{
    bool exists = false;
    // check if `file_path` exists
    // if so, ensure it is not a directory and that it has executable permissions
    exists = access(file_path.c_str(), X_OK);
    return exists == 0;
}

/*
   text: string to split
   d: character delimiter to split `text` on
   result: vector of strings - result will be stored here
*/
void splitString(std::string text, char d, std::vector<std::string>& result)
{
    enum states { NONE, IN_WORD, IN_STRING } state = NONE;

    int i;
    std::string token;
    result.clear();
    for (i = 0; i < text.length(); i++)
    {
        char c = text[i];
        switch (state) {
            case NONE:
                if (c != d)
                {
                    if (c == '\"')
                    {
                        state = IN_STRING;
                        token = "";
                    }
                    else
                    {
                        state = IN_WORD;
                        token = c;
                    }
                }
                break;
            case IN_WORD:
                if (c == d)
                {
                    result.push_back(token);
                    state = NONE;
                }
                else
                {
                    token += c;
                }
                break;
            case IN_STRING:
                if (c == '\"')
                {
                    result.push_back(token);
                    state = NONE;
                }
                else
                {
                    token += c;
                }
                break;
        }
    }
    if (state != NONE)
    {
        result.push_back(token);
    }
}

/*
   list: vector of strings to convert to an array of character arrays
   result: pointer to an array of character arrays when the vector of strings is copied to
*/
void vectorOfStringsToArrayOfCharArrays(std::vector<std::string>& list, char ***result)
{
    int i;
    int result_length = list.size() + 1;
    *result = new char*[result_length];
    for (i = 0; i < list.size(); i++)
    {
        (*result)[i] = new char[list[i].length() + 1];
        strcpy((*result)[i], list[i].c_str());
    }
    (*result)[list.size()] = NULL;
}

/*
   array: list of strings (array of character arrays) to be freed
   array_length: number of strings in the list to free
*/
void freeArrayOfCharArrays(char **array, size_t array_length)
{
    int i;
    for (i = 0; i < array_length; i++)
    {
        if (array[i] != NULL)
        {
            delete[] array[i];
        }
    }
    delete[] array;
}
