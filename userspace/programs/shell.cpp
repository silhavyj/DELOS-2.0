#include <system.h>
#include <string.h>
#include <stdint.h>
#include <memory.h>

#define BUFFER_SIZE 256

void printHelp();

int main() {
    const char *prompt = "C:/> ";
    const char *HELP = "help";
    const char *PS = "ps";
    const char *LS = "ls";
    const char *LP = "lp";
    const char *EXEC = "./";
    const char *KILL = "kill";
    const char *TOUCH = "touch";
    const char *CAT = "cat";
    const char *RM = "rm";
    const char *CLEAR = "clear";
    const char *LAST_EXIT_CODE = "$?";
    const char *EXIT = "exit";
    const char *CP = "cp";
    const char *ECHO = "echo";

    const char *UNKNOWN_COMMAND = "(%s) is not recognized as internal or external command\n\r Use \"help\" for help.\r\n";
    const char *INVALID_PID = "Given PID (%d) is invalid!\n\r";
    const char *FILE_CREATE_ERR = "File (%s) cannot be created!\n\r";
    const char *FILE_NOT_FOUND_ERR = "File (%s) not found!\n\r";
    const char *FILE_REMOVE_ERR = "File (%s) cannot be removed!\n\r";

    const char *PRINT_DECIMAL = "%d\n\r";
    const char *PRINT_STRING = "%s\n\r";

    char buffer[BUFFER_SIZE];
    unsigned int pid = 0;
    char file1[BUFFER_SIZE];
    char file2[BUFFER_SIZE];
    uint32_t i;

    while (1) {
        color_screen_command(0x0F, 0x00);
        printf(prompt);
        read_line(buffer);

        if (strcmp(buffer, HELP) == 0) {
            printHelp();
        }
        else if (strcmp(buffer, PS) == 0) {
            ps();
        } else if (strcmp(buffer, LS) == 0) {
            ls();
        } else if (strcmp(buffer, LP) == 0) {
            lp();
        } else if (strcmp(buffer, EXEC, (uint32_t)strlen(EXEC)-1) == 0) {
            char *programName = buffer + strlen(EXEC);
            int pidChild = exec(programName);
            if (pidChild == 0){
                printf(UNKNOWN_COMMAND, programName);
            }
            else {
                wait_for_child(pidChild);
            }
        } else if (strcmp(buffer, KILL, (uint32_t)strlen(KILL)-1) == 0) {
            //TODO: Wheres kill?
            pid = atoi(buffer + strlen(KILL) + 1);
            if( pid != 0 ){
                //TODO: Add kill call
            }
            else{
                printf(INVALID_PID, pid);
            }
        } else if (strcmp(buffer, TOUCH, (uint32_t)strlen(TOUCH)-1) == 0) {
            char *fileName = buffer + strlen(TOUCH) + 1;
            if (touch(fileName) != 0){
                printf(FILE_CREATE_ERR, fileName);
            }
        } else if (strcmp(buffer, CP, (uint32_t)strlen(CP)-1) == 0) {
            memset(&file1[0], 0, sizeof(file1));
            memset(&file2[0], 0, sizeof(file2));
            char *curr = buffer + strlen(CP) + 1;
            for (i = 0; *curr && *curr != ' '; i++, curr++)
                file1[i] = *curr;
            if (*curr)
                curr++;
            for (i = 0; *curr && *curr != ' '; i++, curr++)
                file2[i] = *curr;
            cp(&file1[0], &file2[0]);
        } else if (strcmp(buffer, CAT, (uint32_t)strlen(CAT)-1) == 0) {
            char *fileName = buffer + strlen(CAT) + 1;
            if (cat(fileName) != 0){
                printf(FILE_NOT_FOUND_ERR, fileName);
            }
        } else if (strcmp(buffer, ECHO, (uint32_t)strlen(ECHO)-1) == 0) {
            // echo "some text" > file1.txt
            // echo "some text"
            memset(&file1[0], 0, sizeof(file1));
            memset(&file2[0], 0, sizeof(file2));
            char *curr = buffer + strlen(ECHO) + 1;
            if (*curr == '"') {
                for (i = 0, curr++; *curr && *curr != '"'; i++, curr++) {
                    file1[i] = *curr;
                }
                if (*curr && *curr == '"') {
                    for (curr++; *curr && *curr != '>'; curr++)
                        ;
                    if (*curr == '>') {
                        curr += 2;
                        file_append(curr, &file1[0]);
                    } else {
                        printf(PRINT_STRING, &file1[0]);
                    }
                }
            }
        } else if (strcmp(buffer, RM, (uint32_t)strlen(RM)-1) == 0) {
            char *fileName = buffer + strlen(RM) + 1;
            if (rm(fileName) != 0){
                printf(FILE_REMOVE_ERR, fileName);
            }
        } else if (strcmp(buffer, CLEAR) == 0) {
            clear_screen_command();
        } else if (strcmp(buffer, LAST_EXIT_CODE) == 0) {
            printf(PRINT_DECIMAL, get_last_process_return_value());
        } else if (strcmp(buffer, EXIT) == 0) {
            //TODO: Last shell should end OS?
            break;
        } else if (strlen(buffer) != 0) {
            printf(UNKNOWN_COMMAND, buffer);
        }
    }
    return 0;
}

void printHelp(){
    printf("Available commands: \n\r");
    printf("> help              (Prints help) \n\r");
    printf("> ps                (Prints processes) \n\r");
    printf("> ls                (Prints directory entities) \n\r");
    printf("> cp <src> <des>    (Copies file src into file des) \n\r");
    printf("> lp                (Prints available programs) \n\r");
    printf("> ./<program>       (Executes given program) \n\r");
    printf("> touch <file>      (Creates file) \n\r");
    printf("> echo \"<text>\"     (Prints text onto the screen) \n\r");
    printf("> echo \"<text>\" > <file>   (Echo into a file) \n\r");
    printf("> cat <file>        (Prints file) \n\r");
    printf("> rm <file>         (Removes file) \n\r");
    printf("> clear             (Clears screen) \n\r");
    printf("> CTR+[1-4]         (Switches terminal) \n\r");
    printf("> exit              (Xxits shell) \n\r");
}