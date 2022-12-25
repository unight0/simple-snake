#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define CSI "\e["

int is_snake(int *sn_x, int *sn_y, int sn_l,
             int x, int y, int only_body);

void render(int *sn_x, int *sn_y,
            int sn_l, int food_x, int food_y,
            int width, int height,
            char bg) {
    if (!bg) bg = ' ';
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            //Don't print if there is something
            if (!(y == food_y && x == food_x)
                && !is_snake(sn_x, sn_y, sn_l, x, y, 1)) {
                printf(CSI"%d;%dH%c", y, x, bg);
            }
        }
    }
    for (int i = 0; i < sn_l-1; i++) {
        //print snake(colored)
        printf(CSI"%d;%dH"CSI"32mS", sn_y[i], sn_x[i]);
        puts(CSI"39m");
    }
    printf(CSI"%d;%dH"CSI"33mH", sn_y[sn_l-1], sn_x[sn_l-1], CSI);
    puts(CSI"39m");
    
    //print food(colored)
    printf(CSI"%d;%dH"CSI"31mF", food_y, food_x, CSI);
    puts(CSI"39m");
    fflush(stdin);
}

int is_snake(int *sn_x, int *sn_y, int sn_l, int x, int y, int only_body) {
    for (int i = 0; i < sn_l-(1+only_body); i++) {
        if (x == sn_x[i]
          &&y == sn_y[i]) {
            return 1;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    int bg = '.';
    long height = 16;
    long width  = 32;
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (!strcmp(argv[i], "-h") || !strcmp(argv[1], "--help")) {
                puts("SNAKE GAME\n"
                     "OPTIONS:\n"
                     "  (-h    | --help) -- this message.\n"
                     "  (--nbg | --no-background) -- disable background.\n"
                     "  (--bg  | --background) -- set background character. If character is not provided, does the same the '--nbg' does\n"
                     "  (--sh  | --height) -- set height of playfield.\n"
                     "  (--sw  | --width) -- set width of playfield.\n"
                     "CONTROLS:\n"
                     "  Use [awsd] or [hjkl] keys to move.\n"
                     "  Use [q] key to exit.\n"
                     "ABOUT:\n"
                     "  Snake head is 'H'\n"
                     "  Snake body is 'S'.\n"
                     "  Food is 'F'.\n"
                     "  Field is cycled.");
                exit(0);
            }
            else if(!strcmp(argv[i], "--nbg") || !strcmp(argv[i], "--help")) {
                bg = 0;
            }
            else if(!strcmp(argv[i], "--bg") || !strcmp(argv[i], "--background")) {
                if (argc > i+1)
                    bg = argv[i+1][0];
                else
                    bg = 0;
            }
            else if(!strcmp(argv[i], "--sh") || !strcmp(argv[i], "--height")) {
                if (argc > i+1) {
                    //Check for errors while parsing num
                    char *endptr = NULL;
                    errno = 0;
                    height = strtol(argv[i+1], &endptr, 10);
                    if (errno) {
                        printf("Error reading '%s' argument occured: %s\n", argv[i], strerror(errno));
                        exit(-1);
                    }
                    if (endptr == argv[i+1]) {
                        printf("No digits were found in argument for '%s'\n", argv[i]);
                        exit(-1);
                    }
                }
                else {
                    printf("No argument to '%s' was provided.\n", argv[i]);
                    exit(-1);
                }
            }
            else if(!strcmp(argv[i], "--sw") || !strcmp(argv[i], "--width")) {
                if (argc > i+1) {
                    //Check for errors while parsing num
                    char *endptr = NULL;
                    errno = 0;
                    width = strtol(argv[i+1], &endptr, 10);
                    if (errno) {
                        printf("Error reading '%s' argument occured: %s\n", argv[i], strerror(errno));
                        exit(-1);
                    }
                    if (endptr == argv[i+1]) {
                        printf("No digits were found in argument for '%s'\n", argv[i]);
                        exit(-1);
                    }
                }
                else {
                    printf("No argument to '%s' was provided.\n", argv[i]);
                    exit(-1);
                }
            }
        }
    }
    int max_len = height*width;
    //Disable newline when reading characters && disable echo
    static struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON|ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    //Variables
    srand(time(NULL));
    int *sn_x = malloc(max_len*sizeof(int)),
        *sn_y = malloc(max_len*sizeof(int)),
        sn_l = 1,
        food_x = rand()%(width-2) + 1,
        food_y = rand()%(height-2) + 1,
        dir_x = 0,
        dir_y = 0,
        gameover = 0,
        win = 0;
    sn_x[0] = width/2;
    sn_y[0] = height/2;
    //memcpy(sn_x, 0, max_len*sizeof(int));
    //memcpy(sn_y, 0, max_len*sizeof(int));
    //Hide cursor
    puts(CSI"?25l");
    puts(CSI"H"CSI"J");
    while(!gameover && !win) {
        render(sn_x, sn_y, sn_l, food_x, food_y, width, height, bg);
        //Set timeout for input so things can move without user input
        fd_set fds;
        struct timeval tv = {
            .tv_sec = 0,
            .tv_usec = 1000*(200*(1-((float)sn_l)/((float)max_len)))
        };
        //printf(CSI"16;16H\nmax_len=%d;SN_L=%d;(1-SN_L/max_len)=%f;USEC=%d", max_len, sn_l, 1-((float)sn_l)/((float)max_len), tv.tv_usec);
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
        if (FD_ISSET(STDIN_FILENO, &fds)) {
            char c = getchar();
            if (c == 'q') break;
            else if (c == 'a'||c == 'h') {
                //Forbit reverse movement
                if (dir_x != 1 || sn_l == 1) {
                    dir_x = -1; dir_y = 0;
                }
            }
            else if (c == 's'||c == 'j') {
                //Forbit reverse movement
                if (dir_y != -1 || sn_l == 1) {
                    dir_x = 0; dir_y = 1;
                }
            }
            else if (c == 'w'||c == 'k') {
                //Forbit reverse movement
                if (dir_y != 1 || sn_l == 1) {
                    dir_x = 0; dir_y = -1;
                }
            }
            else if (c == 'd'||c == 'l') {
                //Forbit reverse movement
                if (dir_x != -1 || sn_l == 1) {
                    dir_x = 1; dir_y = 0;
                }
            }
        }
        if (sn_l > 1) {
            memmove(&sn_x[0], &sn_x[1], (sn_l-1)*sizeof(int));
            memmove(&sn_y[0], &sn_y[1], (sn_l-1)*sizeof(int));
        }
        sn_x[sn_l-1] += dir_x;
        sn_y[sn_l-1] += dir_y;
        if(sn_x[sn_l-1] < 1) {
            sn_x[sn_l-1] = width-1;
            continue;
        }
        else if(sn_x[sn_l-1] >= width) {
            sn_x[sn_l-1] = 1;
            continue;
        }
        if(sn_y[sn_l-1] < 1) {
            sn_y[sn_l-1] = height-1;
            continue;
        }
        else if(sn_y[sn_l-1] >= height) {
            sn_y[sn_l-1] = 1;
            continue;
        }
        if (sn_x[sn_l-1] == food_x && sn_y[sn_l-1] == food_y) {
            int fine_place = 0;
            // Try to place until not placing in the snake
            do {
                food_x = rand()%(width-2) + 1;
                food_y = rand()%(height-2) + 1;
                fine_place = !is_snake(sn_x, sn_y, sn_l, food_x, food_y, 0);
            } while(!fine_place);
            sn_l++;
            sn_x[sn_l-1] = sn_x[sn_l-2];
            sn_y[sn_l-1] = sn_y[sn_l-2];
            win = sn_l==max_len;
        }
        else if(is_snake(sn_x, sn_y, sn_l, sn_x[sn_l-1], sn_y[sn_l-1], 1))
            gameover = 1;
        //usleep(1000*125);
    }
    free(sn_x);
    free(sn_y);
    //Soft reset
    puts("\ec");
    if (gameover)
        printf("You loose!\n");
    else if (win)
        printf("You win!\n");
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}
