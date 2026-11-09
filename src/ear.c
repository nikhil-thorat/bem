#include <fcntl.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Usage : %s <ear_port> <system_name>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int ear_port = atoi(argv[1]);

    struct sockaddr_in ear_addr;
    ear_addr.sin_family = AF_INET;
    ear_addr.sin_addr.s_addr = INADDR_ANY;
    ear_addr.sin_port = htons(ear_port);

    struct sockaddr_in peer_addr;
    socklen_t peer_addr_len = sizeof(peer_addr);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
    {
        perror("ear socket system call failed");
        return EXIT_FAILURE;
    }

    int result = bind(sockfd, (const struct sockaddr *)&ear_addr, sizeof(ear_addr));
    if (result == -1)
    {
        perror("ear bind system call failed");
        return EXIT_FAILURE;
    }

    /*
     * Opening ear_to_brain file for communication.
     */
    char filepath[64];
    snprintf(filepath, sizeof(filepath), "%s/ear_to_brain", argv[2]);

    FILE *ear_brain = fopen(filepath, "wb+");
    if (ear_brain == NULL)
    {
        perror("ear failed to create file ear_brain");
        return EXIT_FAILURE;
    }

    /*
     * Attaching to the ebsem semaphore created by
     * brain process.
     */
    char sem[64];
    snprintf(sem, sizeof(sem), "%s_ebsem", argv[2]);

    sem_t *eb_sem = sem_open(sem, 0);
    if (eb_sem == SEM_FAILED)
    {
        perror("ear sem_open system call failed");
        return EXIT_FAILURE;
    }

    int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];

    while (1)
    {
        /*
         * Blocking call, wait until we receive any
         * message on specified port.
         */
        int bytes_received =
            recvfrom(sockfd, (char *)buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&peer_addr, &peer_addr_len);
        if (bytes_received == -1)
        {
            perror("ear recvfrom system call failed");
            continue;
        }
        buffer[bytes_received] = '\0';

        printf("[EAR] : RECEIVED %d BYTES\n", bytes_received);

        /*
         * Similar to brain processes, we need
         * to overwrite the file.
         */
        rewind(ear_brain);
        int bytes_written = fwrite(buffer, sizeof(char), bytes_received, ear_brain);

        fflush(ear_brain);
        ftruncate(fileno(ear_brain), bytes_written);

        if (bytes_written != strlen(buffer))
        {
            perror("ear fwrite system call failed");
            return EXIT_FAILURE;
        }

        printf("[EAR] : WROTE %d BYTES\n", bytes_written);

        /*
         * Singals brain process to continue it's
         * execution.
         */
        sem_post(eb_sem);

        if (strcmp(buffer, "quit") == 0 || strcmp(buffer, "exit") == 0)
        {
            break;
        }
    }

    /*
     * Closing files and sockfd.
     */
    fclose(ear_brain);
    close(sockfd);

    /*
     * Unlinking and closing semaphores.
     */
    sem_unlink((const char *)eb_sem);
    sem_close(eb_sem);

    puts("[EAR] : QUITTING");

    return EXIT_SUCCESS;
}
