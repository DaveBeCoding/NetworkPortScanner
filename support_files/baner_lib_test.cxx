#include <stdio.h>
#include <fcntl.h>       // For fcntl()
#include <sys/select.h>  // For select()
#include <unistd.h>      // For read()
#include <errno.h>       // For errno

int main() {
    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe");
        return 1;
    }

    // Set the read end of the pipe to non-blocking mode using fcntl()
    int flags = fcntl(fd[0], F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return 1;
    }

    flags |= O_NONBLOCK;
    if (fcntl(fd[0], F_SETFL, flags) == -1) {
        perror("fcntl F_SETFL");
        return 1;
    } else {
        printf("fcntl() works! Set file descriptor to non-blocking mode.\n");
    }

    // Prepare for select()
    fd_set read_fds;
    struct timeval timeout;
    FD_ZERO(&read_fds);
    FD_SET(fd[0], &read_fds);

    timeout.tv_sec = 5;  // 5-second timeout
    timeout.tv_usec = 0;

    int retval = select(fd[0] + 1, &read_fds, NULL, NULL, &timeout);

    if (retval == -1) {
        perror("select()");
        return 1;
    } else if (retval) {
        printf("Data is available to read.\n");
    } else {
        printf("No data within five seconds. select() works!\n");
    }

    // Attempt to read from the pipe (should be non-blocking)
    char buffer[10];
    ssize_t bytesRead = read(fd[0], buffer, sizeof(buffer));

    if (bytesRead == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            printf("No data to read. Non-blocking read confirmed. errno works!\n");
        } else {
            perror("read");
            return 1;
        }
    } else {
        printf("Read %zd bytes.\n", bytesRead);
    }

    // Clean up
    close(fd[0]);
    close(fd[1]);

    return 0;
}

