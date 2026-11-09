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
        printf("Usage : %s <peer_port> <system_name>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int peer_port = atoi(argv[1]);

    struct sockaddr_in peer_addr;
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_addr.s_addr = INADDR_ANY;
    peer_addr.sin_port = htons(peer_port);
    socklen_t peer_addr_len = sizeof(peer_addr);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
    {
        perror("mouth socket system call failed");
        return EXIT_FAILURE;
    }

    /*
     * Opening brain_to_mouth file for reading
     * messages.
     */
    char filepath[64];
    snprintf(filepath, sizeof(filepath), "%s/brain_to_mouth", argv[2]);

    FILE *brain_mouth;

    /*
     * Attaching to bmsem semaphore created
     * by brain process.
     */
    char sem[64];
    snprintf(sem, sizeof(sem), "%s_bmsem", argv[2]);

    sem_t *bm_sem = sem_open(sem, 0);
    if (bm_sem == SEM_FAILED)
    {
        perror("mouth sem_open system call failed");
        return EXIT_FAILURE;
    }

    int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];

    while (1)
    {
        /*
         * Blocking call, wait until brain process
         * signals to continue further execution.
         */
        sem_wait(bm_sem);

        brain_mouth = fopen(filepath, "rb");
        if (brain_mouth == NULL)
        {
            perror("mouth fopen system call failed");
            return EXIT_FAILURE;
        }

        /*
         * Reading message from brain.
         */
        int bytes_read = fread(buffer, sizeof(char), BUFFER_SIZE, brain_mouth);
        if (bytes_read == -1)
        {
            perror("mouth fread system call failed");
            return EXIT_FAILURE;
        }
        buffer[bytes_read] = '\0';

        printf("[MOUTH] : READ %d BYTES\n", bytes_read);

        /* Brain uses this internal marker to stop mouth without sending it. */
        if (strcmp(buffer, "quit") == 0 || strcmp(buffer, "exit") == 0)
        {
            fclose(brain_mouth);
            break;
        }

        printf("[MOUTH] : %s", buffer);

        /*
         * Sending message to specified
         * port.
         */
        int bytes_sent =
            sendto(sockfd, (char *)buffer, bytes_read, 0, (const struct sockaddr *)&peer_addr, peer_addr_len);
        if (bytes_sent == -1)
        {
            perror("mouth sendto system call failed");
            return EXIT_FAILURE;
        }

        printf("[MOUTH] : SENT %d BYTES\n", bytes_sent);

        fclose(brain_mouth);
    }

    close(sockfd);

    /*
     * Unlinking and closing semaphores
     */
    sem_unlink((const char *)bm_sem);
    sem_close(bm_sem);

    puts("[MOUTH] : QUITTING");

    return EXIT_SUCCESS;
}
