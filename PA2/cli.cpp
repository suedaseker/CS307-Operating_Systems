#include <string.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <cstdio>

using namespace std;

pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;
vector<pthread_t> thread_list;
vector<int> rclist;

struct Command
{
    string cmd_name;
    string input;
    string option;
    string symbol;
    string file_name;
    bool isBackGround = false;
    int arg = 0;
};

void *ThreadFunc(void *arg)
{
    pthread_mutex_lock(&lock1);
    
    cout << "---- " << pthread_self() << endl;
    
    int myfd = *(int*)arg;
    FILE *file = fdopen(myfd, "r");
    
    char buffer[1024];
    while(!feof(file) && !ferror(file) && fgets(buffer,sizeof(buffer), file) != NULL)
    {
            fputs(buffer,stdout);
    }
    
    cout << "---- " << pthread_self() << endl;
    
    pthread_mutex_unlock(&lock1);
    return NULL;
}

void parse(vector<Command>& list)
{
    //Open the file containing the commands
    ifstream file("commands.txt");
    if (!file.is_open()){
        cout << "Can not open file.\n";
        file.close();
    }

    //Read each line of the file and execute the command in a separate process
    string line;
    while (getline(file, line)) //parse each line
    {
        istringstream input(line);
        Command command;
        int counter = 0;
        bool isPrevSymbol = false;
        string word;
        while(input >> word)
        {
            if (counter == 0){
                command.cmd_name = word;
                command.arg++;
                if (word == "wait")
                    break;
            }
            else{
                if (word[0] == '-'){
                    command.option = word;
                    command.arg++;
                }
                else if (word[0] == '<' || word[0] == '>'){
                    command.symbol = word;
                    isPrevSymbol = true;
                    command.arg++;
                }
                else if (word[0] == '&'){
                    command.isBackGround = true;
                }
                else if (isPrevSymbol == true){
                    command.file_name = word;
                    command.arg++;
                }
                else{
                    command.input = word;
                }
            }
            counter ++;
        }
        //done parse line
        list.push_back(command); //push the struct
    }
    file.close();

    ofstream outfile;
    outfile.open("parse.txt");
    for (int i = 0; i < list.size(); i++)
    {
        outfile << "----------\n";
        outfile << "Command: " << list[i].cmd_name << "\n";
        outfile << "Inputs: " << list[i].input << "\n";
        outfile << "Options: " << list[i].option << "\n";
        outfile << "Redirection: " << list[i].symbol << "\n";
        if (list[i].isBackGround == true) {
            outfile << "Background Job: y\n";
        }
        else if (list[i].isBackGround == false) {
            outfile << "Background Job: n\n";
        }
        outfile << "----------\n";
    }
    outfile.close();
}

int main()
{
    vector<Command> commands;
    parse(commands);
    
    for (int i = 0; i < commands.size(); i++) //iterate over command lines
    {
        if (commands[i].cmd_name == "wait")
        {
            for (int i = 0; i < thread_list.size(); i++)
            {
                pthread_join(thread_list[i], NULL);
            }
            for (int i=0; i < rclist.size(); i++)
            {
                int status;
                waitpid(rclist[i], &status, 0);
            }
            continue;
        }

        if (commands[i].symbol != ">")
        {
            int *pfd = (int*)malloc(sizeof(int)*2); //(0 read, 1 write)
            pipe(pfd);
            pthread_t thread_id;
            
            int rc = fork();
            
            if (rc < 0) {
                //fork failed
                fprintf(stderr, "fork failed\n");
                exit(1);
            }
            else if (rc == 0) {
                //child process
                
                //there is no redirection for output, do pipeline
                close(pfd[0]);
                dup2(pfd[1], STDOUT_FILENO); //instead of write to console, write to pfd[1]
                
                int arg_length = commands[i].arg + 1;
                int j = 0;
                char *myargs[arg_length];
                
                myargs[j] = strdup(commands[i].cmd_name.c_str());
                j++;
                if (commands[i].input != "")
                {
                    myargs[j] = strdup(commands[i].input.c_str());
                    j++;
                }
                if (commands[i].option != "")
                {
                    myargs[j] = strdup(commands[i].option.c_str());
                    j++;
                }
                if (commands[i].symbol == "<")
                {
                    int fd = open(commands[i].file_name.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                    dup2(fd, STDIN_FILENO);
                }

                myargs[j] = NULL;
                execvp(myargs[0], myargs);
            }
            else {
                //parent
                rclist.push_back(rc);
                
                close(pfd[1]);
                pthread_create(&thread_id, NULL, ThreadFunc, &pfd[0]);
                thread_list.push_back(thread_id);
                
                if (!commands[i].isBackGround) //if not background job, should wait
                {
                    int status;
                    waitpid(rc, &status, 0);
                }
            }
        }
        else {
            int rc = fork();
            
            if (rc < 0) {
                //fork failed
                fprintf(stderr, "fork failed\n");
                exit(1);
            }
            else if (rc == 0) {
                //child process
                int arg_length = commands[i].arg + 1;
                int j = 0;
                char *myargs[arg_length];
                
                myargs[j] = strdup(commands[i].cmd_name.c_str());
                j++;
                if (commands[i].input != "")
                {
                    myargs[j] = strdup(commands[i].input.c_str());
                    j++;
                }
                if (commands[i].option != "")
                {
                    myargs[j] = strdup(commands[i].option.c_str());
                    j++;
                }
                
                int fd = open(commands[i].file_name.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                dup2(fd, STDOUT_FILENO);
                
                myargs[j] = NULL;
                execvp(myargs[0], myargs);
            }
            else {
                //parent
                rclist.push_back(rc);
                
                if (!commands[i].isBackGround) //if not background job, should wait
                {
                    int status;
                    waitpid(rc, &status, 0);
                }
            }
        }
    }
    
    //wait for threads
    for (int i = 0; i < thread_list.size(); i++)
    {
        pthread_join(thread_list[i], NULL);
    }
    thread_list.clear();
    
    //wait for child processes
    for (int i=0; i < rclist.size(); i++)
    {
        int status;
        waitpid(rclist[i], &status, 0);
    }
    
    return 0;
}
