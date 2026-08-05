#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Usage : %s <listen_on_port> <send_to_port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char directory[64];
    snprintf(directory, sizeof(directory), "system_%s", argv[1]);

    if (access(directory, F_OK) == -1)
    {
        mkdir(directory, 0777);
    }

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

    char filepath[64];
    snprintf(filepath, sizeof(filepath), "%s/brain_to_mouth", directory);

    FILE *ear_brain;
    FILE *brain_mouth = fopen(filepath, "wb+");
    if (brain_mouth == NULL)
    {
        perror("brain fopen system call failed");
        return EXIT_FAILURE;
    }

    char sem[64];
    snprintf(sem, sizeof(sem), "%s_ebsem", directory);

    sem_t *eb_sem = sem_open(sem, 0);
    if (eb_sem == NULL)
    {
        perror("brain sem_open system call failed");
        return EXIT_FAILURE;
    }

    snprintf(sem, sizeof(sem), "%s_bmsem", directory);

    sem_t *bm_sem = sem_open(sem, O_CREAT, 0664, 0);
    if (bm_sem == SEM_FAILED)
    {
        perror("brain sem_open system call failed");
        return EXIT_FAILURE;
    }

    int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];

    snprintf(filepath, sizeof(filepath), "%s/ear_to_brain", directory);

    while (1)
    {
        sem_wait(eb_sem);

        ear_brain = fopen(filepath, "rb");
        if (ear_brain == NULL)
        {
            perror("brain fopen system call failed");
            return EXIT_FAILURE;
        }

        int bytes_read = fread(buffer, sizeof(char), BUFFER_SIZE, ear_brain);
        if (bytes_read == -1)
        {
            perror("brain fread system call failed");
            return EXIT_FAILURE;
        }

        printf("[BRAIN] : READ %d BYTES\n", bytes_read);

        rewind(brain_mouth);
        int bytes_written = fwrite(buffer, sizeof(char), bytes_read, brain_mouth);

        ftruncate(fileno(brain_mouth), bytes_written);
        fflush(brain_mouth);

        if (bytes_written != bytes_read)
        {
            perror("brain fwrite system call failed");
            return EXIT_FAILURE;
        }

        printf("[BRAIN] : WROTE %d BYTES\n", bytes_written);

        sem_post(bm_sem);

        fclose(ear_brain);
        if (strncmp(buffer, "exit", 4) == 0)
        {
            break;
        }
    }

    sem_unlink((const char *)eb_sem);
    sem_unlink((const char *)bm_sem);

    wait(NULL);
    wait(NULL);

    puts("[BRAIN] : QUITTING");

    return EXIT_SUCCESS;
}
