#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

// #define NOBONG

#ifndef NOBONG
#define ASSERT(condition)                                                                          \
    ({                                                                                             \
        printf("%s:%i: %s\n", __FILE__, __LINE__, #condition);                                     \
                                                                                                   \
        if (!(condition)) {                                                                        \
            int err = errno;                                                                       \
            fprintf(stderr, "%s:%i: Assertion failed! (%s), errno = \"%s\"\n", __FILE__, __LINE__, \
                    #condition, strerror(err));                                                    \
            exit(EXIT_FAILURE);                                                                    \
        }                                                                                          \
    })
#else
#define ASSERT(condition)                                                                          \
    ({                                                                                             \
        if (!(condition)) {                                                                        \
            int err = errno;                                                                       \
            fprintf(stderr, "%s:%i: Assertion failed! (%s), errno = \"%s\"\n", __FILE__, __LINE__, \
                    #condition, strerror(err));                                                    \
            exit(EXIT_FAILURE);                                                                    \
        }                                                                                          \
    })
#endif

void test_file();
void test_inet();
void test_ctype();
void test_fb();
void test_string();
void test_pdevs();
void test_bong();
void test_signals();
void test_pthread();

#if __aex__
void test_aex();
#endif

void fault_handler(int id) {
    exit(0x80 | id);
}

int main(int argc, char* argv[]) {
    if (argc >= 2) {
        if (strcmp(argv[1], "cloexec") == 0) {
            int a = atoi(argv[2]);
            int b = atoi(argv[3]);

            ASSERT(close(a) == -1);
            ASSERT(close(b) != -1);

            return 0;
        }
        else if (strcmp(argv[1], "exit") == 0)
            return 0;
        else if (strcmp(argv[1], "pagefault") == 0) {
            struct sigaction act;

            act.sa_handler = fault_handler;
            act.sa_flags   = 0;

            sigaction(SIGSEGV, &act, NULL);

            *((int*) 0xFFFF800000000000) = 'A';
            *((int*) 0xFFFF800000000001) = 'E';
            *((int*) 0xFFFF800000000002) = 'X';
        }
    }

    nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);

#if __aex__
    test_file();
#endif

    test_inet();
    test_ctype();
    test_string();
    test_pdevs();
    test_signals();
    test_pthread();
    // test_fb();

#if __aex__
    test_aex();
#endif

    char buffer[256];
    gethostname(buffer, sizeof(buffer));

    printf("hostname: %s\n", buffer);
    printf("PATH: %s\n", getenv("PATH"));

    printf("test: %s %i %o %x %X %p %c %h% %#x %#X %8x | %0-8x %8s | %+x %+i %+u end\n", "bong",
           2137, 02137, 0x2137FF, 0x2137FF, &buffer, 65, 0x23ab, 0x23ab, 0x11, 0x11, "abcd", 0xabcd,
           23, 72);

    snprintf(buffer, 4, "%s", "abcdefgh");
    printf("%s\n", buffer);

    return 0;
}

void test_pipes();

void test_file() {
    DIR* root = opendir("/");
    ASSERT(root);
    ASSERT(closedir(root) == 0);

    int root_fd = open("/", O_RDONLY);
    ASSERT(root_fd != -1);
    ASSERT(close(root_fd) == 0);

    ASSERT(open("/", O_WRONLY) == -1);
    ASSERT(open("/", O_RDWR) == -1);

    test_pipes();

    if (fork() == 0) {
        int fdtestA = open("/", O_RDONLY);
        int fdtestB = fcntl(fdtestA, F_DUPFD_CLOEXEC);

        ASSERT(fdtestA != -1);
        ASSERT(fdtestB != -1);
        ASSERT(fcntl(fdtestB, F_GETFD) & FD_CLOEXEC);

        ASSERT(fcntl(fdtestA, F_SETFD, FD_CLOEXEC) != -1);
        ASSERT(fcntl(fdtestB, F_SETFD, FD_CLOEXEC) != -1);
        ASSERT(fcntl(fdtestB, F_SETFD, 0) != -1);

        char buffA[8];
        char buffB[8];

        sprintf(buffA, "%i", fdtestA);
        sprintf(buffB, "%i", fdtestB);

        char* const argv[5] = {
            "utest", "cloexec", buffA, buffB, NULL,
        };

        ASSERT(execve("utest", argv, NULL) != -1);
    }
    else {
        int stat;
        wait(&stat);

        ASSERT(stat == EXIT_SUCCESS);
    }

    int closetest = open("/", O_RDONLY);

    if (fork() == 0) {
        close(closetest);
        exit(EXIT_SUCCESS);
    }
    else {
        int stat;
        wait(&stat);

        ASSERT(stat == EXIT_SUCCESS);
        ASSERT(close(closetest) == 0);
    }

    FILE* utest_file = fopen("utest", "r");

    if (fork() == 0) {
        nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
        ASSERT(ftell(utest_file) == 6);
        exit(EXIT_SUCCESS);
    }
    else {
        ASSERT(fseek(utest_file, 6, SEEK_SET) == 0);

        int stat;
        wait(&stat);

        ASSERT(stat == EXIT_SUCCESS);
    }
}

void test_pipes() {
    char buff[8];
    int  des[2];
    ASSERT(pipe(des) == 0);
    // ASSERT(read(des[1], buff, 6) == -1);
    ASSERT(write(des[1], "abcdef", 6) == 6);

    ASSERT(read(des[0], buff, 6) == 6);
    // ASSERT(write(des[0], "abcdef", 6) == -1);
    ASSERT(memcmp(buff, "abcdef", 6) == 0);

    pid_t ff = fork();
    if (ff == 0) {
        ASSERT(read(des[0], buff, 6) == 2);
        ASSERT(read(des[0], buff, 6) == 4);

        exit(EXIT_SUCCESS);
    }

    ASSERT(write(des[1], buff, 2) == 2);
    nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
    ASSERT(write(des[1], buff, 4) == 4);

    int stat;
    wait(&stat);

    ASSERT(stat == EXIT_SUCCESS);

    ASSERT(close(des[0]) == 0);
    ASSERT(close(des[1]) == 0);
}

void test_inet() {
    ASSERT(htonl(0x00000001) == 0x01000000);
    ASSERT(htons(0x0001) == 0x0100);
    ASSERT(ntohl(0x00000001) == 0x01000000);
    ASSERT(ntohs(0x0001) == 0x0100);
}

void test_ctype() {
    ASSERT(isalnum('a'));
    ASSERT(isalnum('Y'));
    ASSERT(isalnum('0'));
    ASSERT(isalnum('5'));
    ASSERT(isalnum('9'));
    ASSERT(!isalnum('.'));
    ASSERT(!isalnum(':'));
    ASSERT(!isalnum(' '));
    ASSERT(isalpha('a'));
    ASSERT(isalpha('Y'));
    ASSERT(!isalpha('2'));
    ASSERT(isblank(' '));
    ASSERT(isblank('\t'));
    ASSERT(!isblank('\n'));
    ASSERT(!isblank('a'));
    ASSERT(iscntrl('\0'));
    ASSERT(iscntrl('\a'));
    ASSERT(iscntrl('\a'));
    ASSERT(iscntrl('\r'));
    ASSERT(iscntrl('\n'));
    ASSERT(iscntrl('\e'));
    ASSERT(isdigit('0'));
    ASSERT(isdigit('9'));
    ASSERT(isdigit('5'));
    ASSERT(!isdigit('a'));
    ASSERT(!isdigit('b'));
    ASSERT(!isdigit('H'));
    ASSERT(!isdigit('Y'));
    ASSERT(isgraph('.'));
    ASSERT(isgraph('2'));
    ASSERT(isgraph('u'));
    ASSERT(isgraph('p'));
    ASSERT(!isgraph(' '));
    ASSERT(islower('a'));
    ASSERT(islower('o'));
    ASSERT(!islower('G'));
    ASSERT(isprint('G'));
    ASSERT(isprint('h'));
    ASSERT(isprint(' '));
    ASSERT(!isprint('\t'));
    ASSERT(isspace(' '));
    ASSERT(isspace('\t'));
    ASSERT(isspace('\v'));
    ASSERT(!isspace('a'));
    ASSERT(isupper('A'));
    ASSERT(isupper('I'));
    ASSERT(isupper('P'));
    ASSERT(!isupper(' '));
    ASSERT(!isupper('.'));
    ASSERT(!isupper('a'));
    ASSERT(isxdigit('1'));
    ASSERT(isxdigit('7'));
    ASSERT(isxdigit('a'));
    ASSERT(isxdigit('E'));
    ASSERT(isxdigit('f'));
    ASSERT(isxdigit('F'));
    ASSERT(!isxdigit('u'));
    ASSERT(!isxdigit('U'));
    ASSERT(_toupper('a') == 'A');
    ASSERT(_toupper('z') == 'Z');
    ASSERT(_tolower('A') == 'a');
    ASSERT(_tolower('Z') == 'z');
}

void test_string() {
    char buffer[128];
    char src[128];

    // memccpy
    memcpy(buffer, "abcdefgh", 8);
    ASSERT(memccpy(buffer, "aaaa", 'b', 4) == NULL);
    ASSERT(memcmp(buffer, "aaaaefgh", 8) == 0);

    memcpy(buffer, "abcdefgh", 8);
    memcpy(src, "aaak", 4);
    ASSERT(memccpy(buffer, src, 'k', 4) == &buffer[4]);
    ASSERT(memcmp(buffer, "aaakefgh", 8) == 0);

    // memchr
    memcpy(buffer, "abcdefge", 8);
    ASSERT(memchr(buffer, 'e', 4) == NULL);
    ASSERT(memchr(buffer, 'e', 5) == &buffer[4]);

    // memcmp
    memcpy(buffer, "aaaaaaaa", 8);
    ASSERT(memcmp(buffer, "aaaabaaa", 8) < 0);
    memcpy(buffer, "bbbbbbbb", 8);
    ASSERT(memcmp(buffer, "bbbbabbb", 8) > 0);
    memcpy(buffer, "aaaaaaaa", 8);
    ASSERT(memcmp(buffer, "aaaaaaaa", 8) == 0);

    // memcpy
    memset(buffer, 0x55, 4);
    memcpy(buffer, "aaaa", 4);
    ASSERT(memcmp(buffer, "aaaa", 4) == 0);

    // memmove
    memcpy(buffer, "abcdef", 7);
    ASSERT(memcpy(&buffer[2], &buffer[0], 3) == &buffer[2]);
    ASSERT(memcmp(buffer, "ababaf", 6) == 0);

    memcpy(buffer, "abcdef", 7);
    ASSERT(memmove(&buffer[2], &buffer[0], 3) == &buffer[2]);
    ASSERT(memcmp(buffer, "ababcf", 6) == 0);

    // memset
    ASSERT(memset(buffer, 'a', 4) == buffer);
    ASSERT(memcmp(buffer, "aaaa", 4) == 0);

    // stpcpy
    stpcpy(stpcpy(buffer, "big"), "bong");
    ASSERT(strcmp(buffer, "bigbong") == 0);

    // strcat
    memcpy(buffer, "big", 4);
    ASSERT(strcat(buffer, "bong") == buffer);
    ASSERT(strcmp(buffer, "bigbong") == 0);

    // strchr
    memcpy(buffer, "abcdefdd", 9);
    ASSERT(strchr(buffer, 'd') == &buffer[3]);

    // strcmp
    memcpy(buffer, "aaaaaaaa", 8);
    ASSERT(strcmp(buffer, "aaaabaaa") < 0);
    memcpy(buffer, "bbbbbbbb", 8);
    ASSERT(strcmp(buffer, "bbbbabbb") > 0);
    memcpy(buffer, "aaaaaaaa", 8);
    ASSERT(strcmp(buffer, "aaaaaaaa") == 0);
    memcpy(buffer, "aa", 3);
    ASSERT(strcmp(buffer, "aaaaaaaa") < 0);
    memcpy(buffer, "aa", 3);
    ASSERT(strcmp(buffer, "a") > 0);

    // strncmp
    memcpy(buffer, "aaaaaaaa", 8);
    ASSERT(strncmp(buffer, "aaaabaaa", 8) < 0);
    memcpy(buffer, "bbbbbbbb", 8);
    ASSERT(strncmp(buffer, "bbbbabbb", 8) > 0);
    memcpy(buffer, "aaaaaaaa", 8);
    ASSERT(strncmp(buffer, "aaaaaaaa", 8) == 0);
    memcpy(buffer, "aa", 3);
    ASSERT(strncmp(buffer, "aaaaaaaa", 8) < 0);
    memcpy(buffer, "aa", 3);
    ASSERT(strncmp(buffer, "a", 8) > 0);
    memcpy(buffer, "asdsadff", 6);
    ASSERT(strncmp(buffer, "asdsadaa", 6) == 0);

    // strcoll
    // soon

    // strcpy
    memset(buffer, 0x72, 8);
    ASSERT(strcpy(buffer, "asdd") == buffer);
    ASSERT(strlen(buffer) == 4);

    // strncpy
    memcpy(buffer, "aaaaaaaaaaaa", 12);
    ASSERT(strncpy(buffer, "gd", 3) == buffer);
    ASSERT(strlen(buffer) == 2);
    ASSERT(strncpy(buffer, "g", 3) == buffer);
    ASSERT(strlen(buffer) == 1);
    ASSERT(strncpy(buffer, "asdd", 8) == buffer);
    ASSERT(strlen(buffer) == 4);
    memcpy(buffer, "aaaaaaaa", 8);
    ASSERT(strncpy(buffer, "zzzzzzzz", 8) == buffer);
    ASSERT(buffer[7] == 'z');
    ASSERT(buffer[8] != '\0');
    memset(buffer, '\0', 128);

    // strcspn
    memcpy(buffer, "abcdefgh", 8);
    ASSERT(strcspn(buffer, "de") == 3);
    ASSERT(strcspn(buffer, "ed") == 3);
    ASSERT(strcspn(buffer, "fg") == 5);

    // strerror
    ASSERT(strcmp(strerror(0), "Success") == 0);
    ASSERT(strcmp(strerror(EINVAL), "Invalid argument") == 0);

    int a = strerror_r(EINVAL, buffer, 10);
    ASSERT(strcmp(buffer, "Invalid a") == 0);
    ASSERT(a == ERANGE);

    a = strerror_r(EINVAL, buffer, 17);
    ASSERT(strcmp(buffer, "Invalid argument") == 0);
    ASSERT(a == 0);

    a = strerror_r(EINVAL, buffer, 16);
    ASSERT(strcmp(buffer, "Invalid argumen") == 0);
    ASSERT(a == ERANGE);

    // strcspn
    memcpy(buffer, "abcdefgh", 8);
    ASSERT(strpbrk(buffer, "de") == &buffer[3]);
    ASSERT(strpbrk(buffer, "ed") == &buffer[3]);
    ASSERT(strpbrk(buffer, "fg") == &buffer[5]);

    // strrchr
    memcpy(buffer, "abcdefgd", 9);
    ASSERT(strrchr(buffer, 'd') == &buffer[7]);
    ASSERT(strrchr(buffer, 'a') == &buffer[0]);

    // strspn
    memcpy(buffer, "12321asda!@#asd", 9);
    ASSERT(strspn(buffer, "1234567890") == 5);
    ASSERT(strspn(buffer, "!@#") == 0);
    ASSERT(strspn(buffer, "12") == 2);

    // strstr
    memcpy(buffer, "abcdefghabcd****", 17);
    ASSERT(strstr(buffer, "abcd") == &buffer[0]);
    ASSERT(strstr(buffer, "efgh") == &buffer[4]);
    ASSERT(strstr(buffer, "hgfe") == NULL);
    ASSERT(strstr(buffer, "****") == &buffer[12]);
    ASSERT(strstr(buffer, "**") == &buffer[12]);

    // strtok
    memcpy(buffer, "big bong,test, of,  this,, thing,,,,,  ", 40);
    ASSERT(strtok(buffer, " ,") == &buffer[0]);
    ASSERT(buffer[3] == '\0');
    ASSERT(strtok(NULL, " ,") == &buffer[4]);
    ASSERT(buffer[8] == '\0');
    ASSERT(strtok(NULL, " ,") == &buffer[9]);
    ASSERT(strtok(NULL, " ,") == &buffer[15]);
    ASSERT(strtok(NULL, " ,") == &buffer[20]);
    ASSERT(strtok(NULL, " ,") == &buffer[27]);
    ASSERT(strtok(NULL, " ,") == NULL);
    ASSERT(strtok(NULL, " ,") == NULL);
    ASSERT(buffer[38] != '\0');
    ASSERT(buffer[39] == '\0');

    memcpy(buffer, ",,big bong", 11);
    ASSERT(strtok(buffer, " ,") == &buffer[2]);
    ASSERT(strtok(NULL, " ,") == &buffer[6]);

    memcpy(buffer, ",,", 3);
    ASSERT(strtok(buffer, " ,") == NULL);

    // strtok_r
    char* ptr = NULL;

    memcpy(buffer, "big bong,test, of,  this,, thing,,,,,  ", 40);
    ASSERT(strtok_r(buffer, " ,", &ptr) == &buffer[0]);
    ASSERT(buffer[3] == '\0');
    ASSERT(strtok_r(NULL, " ,", &ptr) == &buffer[4]);
    ASSERT(buffer[8] == '\0');
    ASSERT(strtok_r(NULL, " ,", &ptr) == &buffer[9]);
    ASSERT(strtok_r(NULL, " ,", &ptr) == &buffer[15]);
    ASSERT(strtok_r(NULL, " ,", &ptr) == &buffer[20]);
    ASSERT(strtok_r(NULL, " ,", &ptr) == &buffer[27]);
    ASSERT(strtok_r(NULL, " ,", &ptr) == NULL);
    ASSERT(strtok_r(NULL, " ,", &ptr) == NULL);
    ASSERT(buffer[38] != '\0');
    ASSERT(buffer[39] == '\0');

    memcpy(buffer, ",,big bong", 11);
    ASSERT(strtok_r(buffer, " ,", &ptr) == &buffer[2]);
    ASSERT(strtok_r(NULL, " ,", &ptr) == &buffer[6]);

    memcpy(buffer, ",,", 3);
    ASSERT(strtok_r(buffer, " ,", &ptr) == NULL);
}

void test_pdevs() {
    char buffer[4];

    int null = open("/dev/null", O_RDONLY);
    ASSERT(read(null, buffer, 4) == 0);
    close(null);

    FILE* fnull = fopen("/dev/null", "r");
    ASSERT(fread(buffer, 4, 1, fnull) == 0);
    ASSERT(getc(fnull) == EOF);
    fclose(fnull);
}

void test_bong() {
    int*  iptr;
    char* cptr;

#if defined(__i386__)
    __asm__("pushf\norl $0x40000,(%esp)\npopf");
#elif defined(__x86_64__)
    __asm__("pushf\norl $0x40000,(%rsp)\npopf");
#endif

    /* malloc() always provides memory which is aligned for all fundamental types */
    cptr = malloc(sizeof(int) + 1);

    /* Increment the pointer by one, making it misaligned */
    iptr = (int*) ++cptr;

    printf("bong: %p\n", cptr);

    /* Dereference it as an int pointer, causing an unaligned access */
    *iptr = 42;

    __asm__("ud2");

    /*
       Following accesses will also result in sigbus error.
       short *sptr;
       int    i;

       sptr = (short *)&i;
       // For all odd value increments, it will result in sigbus.
       sptr = (short *)(((char *)sptr) + 1);
       *sptr = 100;

    */
}

double rando(double min, double max) {
    return min + ((double) rand() / (double) RAND_MAX) * (max - min);
}

void rect(uint32_t* fb, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t rgb) {
    for (int j = 0; j < height; j++)
        for (int i = 0; i < width; i++)
            fb[((y + j) % 512) * 1280 + ((x + i) % 512)] = rgb;
}

void test_fb() {
    ioctl(1, 0x01, 0x01);
    printf("testA\n");

    printf("tty res: %ix%ix%i (%i)\n", ioctl(1, 0x04), ioctl(1, 0x05), ioctl(1, 0x06),
           ioctl(1, 0x07));

    uint32_t* fb =
        (uint32_t*) mmap(NULL, ioctl(1, 0x07), PROT_READ | PROT_WRITE, 0, STDOUT_FILENO, 0);
    if (!fb) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    int color[4] = {0x59c4dd, 0x2e7081, 0xc8dadf};

    rect(fb, 0, 0, 512, 512, color[0]);

    for (int i = 0; i < 4000; i++)
        rect(fb, rando(0, 1280), rando(0, 720), rando(0, 512 / 32), rando(0, 512 / 32),
             color[i % 3]);
}

#if __aex__
void test_aex() {
    /*while (true) {
        void* mem = malloc(1048576 * 512);
        memset(mem, 72, 1048576 * 512);
    }*/

    // mount("/dev/sda1", "/mnt/", NULL, 0x00, NULL);

    // mkdir("/mnt/test3", 0);
    /*mkdir("/mnt/test3/test4", 0);
    // FILE* aaaa = fopen("/mnt/test3/menel.txt", "r");
    // fclose(aaaa);

    FILE* file = fopen("/mnt/gfd_abcdefghij.txt", "r");
    perror("fopen");
    char abuffer[129] = {};

    int ln = 0;
    while (fread(abuffer, 128, 1, file) != 0) {
        printf("%i: %s\n", ln++, abuffer);
    }

    fseek(file, 0, 0);

    for (int i = 0; i < 2048 / 8 + 512; i++)
        fwrite("abcdefgh", 8, 1, file);

    // fclose(file);

    // unlink("/mnt/test3/gfd_abcdefghij.txt");
    rename("/mnt/gfd_abcdefghij.txt", "/mnt/test5");*/

    /*while (true) {
        uint8_t buffer[9] = {};

        int random = open("/dev/random", O_RDONLY);
        read(random, buffer, 8);

        printf("random says: %i %i %i %i %i %i %i %i\n", buffer[0], buffer[1], buffer[2],
    buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);

        nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
    }*/
}
#endif

void signalbong(int) {
    printf("it workey\n");
}

void signalbong_b(int sig, siginfo_t* info, void* ucontext) {
    ASSERT(sig == SIGUSR1);
    ASSERT(info->si_signo == SIGUSR1);
    ASSERT(info->si_value.sival_int == 0x2137);
    ASSERT(info->si_code == SI_QUEUE);
}

void test_signals() {
    struct sigaction act;

    act.sa_handler = signalbong;
    act.sa_flags   = 0;

    sigaction(SIGUSR1, &act, NULL);
    kill(getpid(), SIGUSR1);

    if (fork() == 0) {
        char* const argv[3] = {
            "utest",
            "pagefault",
            NULL,
        };

        ASSERT(execve("utest", argv, NULL) != -1);
    }
    else {
        int stat;
        wait(&stat);

        ASSERT(stat == 0x80 | SIGSEGV);
    }

    sigset_t set;
    int      sig;

    sigfillset(&set);
    sigprocmask(SIG_BLOCK, &set, NULL);

    sigaction(SIGUSR1, &act, NULL);
    kill(getpid(), SIGUSR1);

    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigwait(&set, &sig);

    ASSERT(sig == SIGUSR1);

    kill(getpid(), SIGUSR1);

    siginfo_t info;
    ASSERT(sigwaitinfo(&set, &info) == SIGUSR1);
    ASSERT(info.si_signo == SIGUSR1);

    int ret        = sigtimedwait(&set, &info, (const struct timespec[]){{0, 100000000L}});
    int errno_pres = errno;

    ASSERT(ret == -1);
    ASSERT(errno_pres == EAGAIN);

    struct sigaction act_b;
    union sigval     bong;

    act.sa_sigaction = signalbong_b;
    act.sa_flags     = SA_SIGINFO;
    bong.sival_int   = 0x2137;

    ASSERT(sigaction(SIGUSR1, &act, NULL) == 0);

    sigrelse(SIGUSR1);
    sigqueue(getpid(), SIGUSR1, bong);

    sigfillset(&set);
    sigprocmask(SIG_BLOCK, &set, NULL);
    kill(getpid(), SIGUSR1);

    sigset_t pending;
    sigpending(&pending);

    int signo;

    ASSERT(sigismember(&pending, SIGUSR1));
    ASSERT(sigwait(&pending, &signo) == 0);
    ASSERT(signo == SIGUSR1);

    act.sa_handler = signalbong;
    act.sa_flags   = 0;

    ASSERT(sigaction(SIGUSR1, &act, NULL) == 0);

    sigemptyset(&set);
    kill(getpid(), SIGUSR1);

    ASSERT(sigsuspend(&set) == -1);

    sigpending(&pending);

    ASSERT(!sigismember(&pending, SIGUSR1));
    ASSERT(signal(SIGUSR1, SIG_DFL) == signalbong);
    ASSERT(signal(SIGUSR1, signalbong) == SIG_DFL);
}

void test_pthread_defcancel();
void test_pthread_asynccancel();
void test_pthread_masking();
void test_pthread_canceltoggle();

void* bong(void* arg) {
    ASSERT(arg == (void*) 0x7777);
    return (void*) 0x6666;
}

void test_pthread() {
    int oldstate = 0;
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
    ASSERT(oldstate == PTHREAD_CANCEL_ENABLE);

    int oldtype = 0;
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);
    ASSERT(oldtype == PTHREAD_CANCEL_DEFERRED);

    pthread_t      thread;
    pthread_attr_t attr;
    void*          retval;

    pthread_attr_init(&attr);
    ASSERT(pthread_create(&thread, &attr, bong, (void*) 0x7777) == 0);
    ASSERT(pthread_kill(thread, 0) == 0);
    ASSERT(pthread_join(thread, &retval) == 0);

    ASSERT(retval == (void*) 0x6666);

    test_pthread_defcancel();
    test_pthread_asynccancel();
    test_pthread_masking();
    test_pthread_canceltoggle();
}

void* test_pthread_defcancel_secondary(void*) {
    sleep(1000);

    fprintf(stderr, "it brokey\n");
    exit(EXIT_FAILURE);

    return NULL;
}

void test_pthread_defcancel() {
    pthread_t thread;

    ASSERT(pthread_create(&thread, NULL, test_pthread_defcancel_secondary, NULL) == 0);
    nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);

    ASSERT(pthread_cancel(thread) == 0);
    ASSERT(pthread_join(thread, NULL) == 0);
}

void* test_pthread_asynccancel_secondary(void*) {
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    while (true)
        ;

    return NULL;
}

void test_pthread_asynccancel() {
    pthread_t thread;

    ASSERT(pthread_create(&thread, NULL, test_pthread_asynccancel_secondary, NULL) == 0);
    nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);

    ASSERT(pthread_cancel(thread) == 0);
    ASSERT(pthread_join(thread, NULL) == 0);
}

void test_pthread_masking_handler(int) {
    fprintf(stderr, "Signal masking is brokey\n");
    exit(EXIT_FAILURE);
}

void test_pthread_masking() {
    struct sigaction act = {};

    act.sa_handler = test_pthread_masking_handler;
    act.sa_flags   = 0;

    sigaction(SIGUSR1, &act, NULL);

    sigset_t mask;

    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);

    pthread_sigmask(SIG_BLOCK, &mask, NULL);
    pthread_kill(pthread_self(), SIGUSR1);
}

volatile int canceltoggle_test = 0;

void* test_pthread_canceltoggle_secondary(void*) {
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    pthread_cancel(pthread_self());

    canceltoggle_test = 1;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);

    canceltoggle_test = 2;
    return NULL;
}

void test_pthread_canceltoggle() {
    pthread_t thread;

    ASSERT(pthread_create(&thread, NULL, test_pthread_canceltoggle_secondary, NULL) == 0);
    ASSERT(pthread_join(thread, NULL) == 0);

    ASSERT(canceltoggle_test == 1);
}
