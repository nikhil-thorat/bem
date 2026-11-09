#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static int process_message(const char *input, char *output, size_t output_size)
{
    if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0)
    {
        snprintf(output, output_size, "quit");
        return 0;
    }

    char operation[16];
    char extra;
    long long left, right, result;

    if (sscanf(input, " %15s %lld %lld %c", operation, &left, &right, &extra) == 3)
    {
        if (strcmp(operation, "add") == 0)
            result = left + right;
        else if (strcmp(operation, "sub") == 0)
            result = left - right;
        else if (strcmp(operation, "mul") == 0)
            result = left * right;
        else if (strcmp(operation, "div") == 0)
        {
            if (right == 0)
            {
                snprintf(output, output_size, "Error: division by zero\n");
                return 0;
            }
            snprintf(output, output_size, "%g\n", (double)left / (double)right);
            return 0;
        }
        else
        {
            snprintf(output, output_size, "Error: unknown operation '%s'\n", operation);
            return 0;
        }
        snprintf(output, output_size, "%lld\n", result);
        return 0;
    }

    /* A single number is already a result; forward it unchanged. */
    long long number;
    if (sscanf(input, " %lld %c", &number, &extra) == 1)
    {
        snprintf(output, output_size, "%lld\n", number);
        return 0;
    }

    snprintf(output, output_size, "Error: expected 'add|sub|mul|div number number' or a number\n");
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Usage : %s <listen_on_port> <send_to_port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    /*
     * This directory contains the files used by
     * brain, ear and mouth for communication.
     */
    char directory[64];
    snprintf(directory, sizeof(directory), "system_%s", argv[1]);

    if (access(directory, F_OK) == -1)
    {
        mkdir(directory, 0777);
    }

    /*
     * We create these semaphores in brain itself, so
     * that other 2 can just attach to it, preventing
     * race conditions.
     */
    char eb_sem_name[64];
    snprintf(eb_sem_name, sizeof(eb_sem_name), "%s_ebsem", directory);

    sem_t *eb_sem = sem_open(eb_sem_name, O_CREAT, 0664, 0);
    if (eb_sem == SEM_FAILED)
    {
        perror("brain sem_open system call failed");
        return EXIT_FAILURE;
    }

    char bm_sem_name[64];
    snprintf(bm_sem_name, sizeof(bm_sem_name), "%s_bmsem", directory);

    sem_t *bm_sem = sem_open(bm_sem_name, O_CREAT, 0664, 0);
    if (bm_sem == SEM_FAILED)
    {
        perror("brain sem_open system call failed");
        return EXIT_FAILURE;
    }

    /*
     * Once, communication directory and semaphores are
     * created successfuly we fork() the ear and mouth
     * processes.
     */
    int ear_pid = fork();
    if (ear_pid == 0)
    {
        char *args[] = {"./ear", argv[1], directory, NULL};
        execv("./ear", args);
    }

    int mouth_pid = fork();
    if (mouth_pid == 0)
    {
        char *args[] = {"./mouth", argv[2], directory, NULL};
        execv("./mouth", args);
    }

    /*
     * Opening communication files for file based communication.
     */
    char filepath[64];
    snprintf(filepath, sizeof(filepath), "%s/brain_to_mouth", directory);

    FILE *ear_brain;
    FILE *brain_mouth = fopen(filepath, "wb+");
    if (brain_mouth == NULL)
    {
        perror("brain fopen system call failed");
        return EXIT_FAILURE;
    }

    int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];

    snprintf(filepath, sizeof(filepath), "%s/ear_to_brain", directory);

    while (1)
    {
        /*
         * Blocking call, brain waits until eb_sem notifies it to continue
         * it's execution, so that we prevent race conditions, where brain
         * reads before ear writes anything to the file.
         */
        sem_wait(eb_sem);

        ear_brain = fopen(filepath, "rb");
        if (ear_brain == NULL)
        {
            perror("brain fopen system call failed");
            return EXIT_FAILURE;
        }

        /*
         * Reading message from ear_to_brain file into the buffer.
         */
        int bytes_read = (int)fread(buffer, sizeof(char), BUFFER_SIZE - 1, ear_brain);
        if (ferror(ear_brain))
        {
            perror("brain fread system call failed");
            return EXIT_FAILURE;
        }
        buffer[bytes_read] = '\0';

        printf("[BRAIN] : READ %d BYTES\n", bytes_read);

        char result[BUFFER_SIZE];
        process_message(buffer, result, sizeof(result));
        size_t result_length = strlen(result);
        /*
         * Need this because we need to overwrite the file, else subsequent writes
         * would have appended the messages in the file.
         */
        rewind(brain_mouth);
        int bytes_written = (int)fwrite(result, sizeof(char), result_length, brain_mouth);

        /*
         * Truncates unnecessary bytes from the file, and flushes the fstream
         * so we write data immediately to the file.
         */
        fflush(brain_mouth);
        ftruncate(fileno(brain_mouth), bytes_written);

        if ((size_t)bytes_written != result_length)
        {
            perror("brain fwrite system call failed");
            return EXIT_FAILURE;
        }

        printf("[BRAIN] : WROTE %d BYTES\n", bytes_written);

        /*
         * Signals bm_sem that we have done writing to the file
         * so that mouth process can now read the message.
         */
        sem_post(bm_sem);

        fclose(ear_brain);
        if (strcmp(buffer, "quit") == 0 || strcmp(buffer, "exit") == 0)
        {
            break;
        }
    }

    /*
     * Unlinking and closing the semaphores.
     */
    sem_unlink(eb_sem_name);
    sem_unlink(bm_sem_name);
    sem_close(bm_sem);
    sem_close(eb_sem);

    wait(NULL);
    wait(NULL);

    puts("[BRAIN] : QUITTING");

    return EXIT_SUCCESS;
}
