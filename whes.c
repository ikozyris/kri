/*
    Copyright (C) 2022-2023  ikozyris
    Copyright (C) 2022-2023  gkozyris
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>

FILE* fp;
size_t lines = 1;
int ch, indx, m;
char txt[2001], fname[51];

int e()
{
    fp = fopen(fname, "r");
    if (fp  == NULL) {
        perror("fopen(): ");
        return -1;
    }
    while ((ch = getc(fp)) != EOF) {
        if (ch == '\n')
            printf("$");
        putc(ch, stdout);

    }
    printf("$\n");
    fclose(fp);
    return 0;
}

int rl()
{
    fp = fopen(fname,"r");
    if(fp  == NULL) {
        perror("fopen(): ");
        return -1;
    }
    printf ("%03zu |", lines++);
    while ((ch = getc(fp)) != EOF) {
        putchar(ch);
        if (ch == '\n') {
            printf ("%03zu |", lines++);
        }
    }
    printf("\n");
    fclose(fp);
    return 0;
}

int r()
{
    fp = fopen(fname,"r");
    if (fp  == NULL) {
        perror("fopen(): ");
        return -1;
    }
    while ((ch = getc(fp)) != EOF)
        putchar(ch);
    printf("\n");
    fclose(fp);
    return 0;
}

int wrt()
{
    printf("Yocto 0.7.2_\t Press ` and ENTER to save and exit\n");
    while ((ch = getchar()) != '`') {//reads until '`'
        txt[indx++] = ch;
        if (indx >= 500) break;
    }   
    txt[indx] = '\0';
    fp = fopen(fname,"w");
    fprintf(fp, "%s", txt); //writes file
    fclose(fp); //closes file
    return 0;
}

int hlp()
{
    printf("Usage: yocto [OPTION]... [FILE]...                                            \n"
           "   -i,  --interacive        Interactive mode aka legacy                       \n"
           "   -w   --write             Write to file                                     \n"
           "   -r   --read              Read file                                         \n"
           "   -rl  --read-lines        Read with the line number before each line        \n"
           "   -e,  --show-ends         Display $ at end of each line                     \n"
           "   -sn  --single-number     Print total line number at the start              \n"
           "   -h   --help              Display this help and exit                        \n"
           "   -v   --version           Output version information and exit             \n\n"
           "Examples:                                                                     \n"
           "    yocto -sn ~/text.txt    Read text.txt without printing the lines          \n"
           "    yocto -i                Start program in interactive mode               \n\n"
	   "Documentation can be found on github.com/ikozyris/yocto/wiki                  \n");
    return 0;
}

int sn()
{
    indx = 0; 
    fp = fopen(fname, "r");
    if (fp  == NULL) {
        perror("fopen(): ");
        return -1;
    }
    while ((ch = fgetc(fp)) != EOF) {
        txt[indx++] = ch;
	if (ch == '\n') lines++;
    }
    printf("lines:%ld\n-------\n", lines);
    fclose(fp);
    printf("%s\n", txt);
    return 0;
}
