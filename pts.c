# define _DEFAULT_SOURCE
# define _XOPEN_SOURCE 600
# include <stdlib.h>
# include <unistd.h>
# include <fcntl.h>
# include <err.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <stdio.h>
# include <sys/ioctl.h>
# include <termios.h>

# define BUFF_SIZE 150

void oussh_openPt(int flags, int* fdm, int* fds) {
    
    *fdm = -1;
    *fds = -1;
    int errval = -1;

    *fdm = posix_openpt(O_RDWR);
    if (*fdm < 0) { errx(EXIT_FAILURE, "oussh_openpt : openpt failed"); }

    errval = grantpt(*fdm);
    if (errval < 0) { errx(EXIT_FAILURE, "oussh_openpt : grantpt failed"); }

    errval = unlockpt(*fdm);
    if (errval < 0) { errx(EXIT_FAILURE, "oussh_openpt : unlockpt failed"); }

    *fds = open(ptsname(*fdm), flags);
    if (*fds < 0) { errx(EXIT_FAILURE, "oussh_openpt : open fds failed"); }
}

int main() {

    int fdm = -1; // master's file descriptor
    int fds = -1; // slave's file descriptor

    oussh_openPt(O_RDWR, &fdm, &fds);    

    pid_t pid = fork();

    if (pid < 0) {

        errx(EXIT_FAILURE, "main : fork failed"); 
    }
    else if (pid != 0) {
        
        // FATHER - MASTER

        char buffer[BUFF_SIZE];
        ssize_t ioerr = -1;

        close(fds);

        while (1) {
            
            write(STDOUT_FILENO, "Input : ", sizeof("Input : "));

            ioerr = read(STDIN_FILENO, buffer, BUFF_SIZE);
            if (ioerr < 0) { errx(EXIT_FAILURE, "main : read failed"); }

            ioerr = write(fdm, buffer, BUFF_SIZE);
            if (ioerr < 0) { errx(EXIT_FAILURE, "main : write failed"); }

            ioerr = read(fdm, buffer, BUFF_SIZE - 1);
            if (ioerr < 0) { errx(EXIT_FAILURE, "main : read failed"); }

            buffer[BUFF_SIZE - 1] = '\0';
            fprintf(stderr,"%s", buffer);
        }
    }
    else {

        struct termios slave_orig_term_settings; // Saved terminal settings
        struct termios new_term_settings; // Current terminal settings

        // CHILD - SLAVE
        char buffer[BUFF_SIZE];
        ssize_t ioerr = -1;


        // Save the default parameters of the slave side of the PTY
        ioerr = tcgetattr(fds, &slave_orig_term_settings);

        // Set raw mode on the slave side of the PTY
        new_term_settings = slave_orig_term_settings;
        cfmakeraw (&new_term_settings);
        tcsetattr (fds, TCSANOW, &new_term_settings);

        close(fdm);

        close(0); // Close standard input (current terminal)
        close(1); // Close standard output (current terminal)
        close(2); // Close standard error (current terminal)

        dup(fds); // PTY becomes standard input (0)
        dup(fds); // PTY becomes standard output (1)
        dup(fds); // PTY becomes standard error (2)

        while (1) {

            ioerr = read(fds, buffer, BUFF_SIZE - 1);
            if (ioerr < 0) { errx(EXIT_FAILURE, "main : read failed"); }

            buffer[BUFF_SIZE - 1] = '\0';

            printf("Child receve : %s", buffer);
        }
    }

    close(fdm);
    close(fds);

    system("ls -l /dev/pts");

    return 0;
}
