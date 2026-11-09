#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(void)
{
    char message[1024];
    char port_input[32];
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
    {
        perror("ui socket");
        return EXIT_FAILURE;
    }

    while (1)
    {
        struct sockaddr_in destination = {0};

        printf("Message (add/sub/mul/div or a number; quit to stop UI): ");
        fflush(stdout);
        if (fgets(message, sizeof(message), stdin) == NULL)
            break;
        message[strcspn(message, "\n")] = '\0';

        printf("Destination port: ");
        fflush(stdout);
        if (fgets(port_input, sizeof(port_input), stdin) == NULL)
            break;

        char *end;
        long port = strtol(port_input, &end, 10);
        while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')
            end++;
        if (*end != '\0' || port < 1 || port > 65535)
        {
            fprintf(stderr, "Invalid port. Enter a value from 1 to 65535.\n");
            continue;
        }

        destination.sin_family = AF_INET;
        destination.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        destination.sin_port = htons((unsigned short)port);

        size_t length = strlen(message);
        ssize_t sent = sendto(sockfd, message, length, 0, (const struct sockaddr *)&destination, sizeof(destination));
        if (sent == -1)
        {
            perror("ui sendto");
            continue;
        }
        printf("Sent %zd bytes to port %ld.\n", sent, port);

        if (strcmp(message, "quit") == 0 || strcmp(message, "exit") == 0)
            break;
    }

    close(sockfd);
    return EXIT_SUCCESS;
}
