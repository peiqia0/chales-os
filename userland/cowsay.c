/*
 *
 * The original cowsay program in C
 * By: Aaron
 * Started: June 22, 2012
 *
 */
/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

/* TO DO:
    - Move printcow to its own file
    - Tongue string
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

////////////////
//Functions
//////
void printcow(void);
void displayhelp(void);

////////////////
//Variables
//////
char eyes='o';
char upperbubble='\\';
char lowerbubble='\\';

unsigned int counter;
unsigned int argscharcount=0;
unsigned int nextarg;
unsigned short skiparg;
unsigned short thought=0;

int main(int argc, char *argv[]) {

    /*Handle Arguments*/
    if( argc == 1 ) {
        displayhelp();
        return -1;
    }
    for (counter=1; counter < argc; counter++) {
        if ( !strcmp(argv[counter], "-e") || !strcmp(argv[counter], "--eyes") ) {
            nextarg=(counter + 1);
            if ( strlen(argv[nextarg]) == 1 ) {
                strcpy(&eyes, argv[nextarg]);
            }
            else {
                displayhelp();
                return -1;
            }
        }
        else if ( !strcmp(argv[counter], "-t") || !strcmp(argv[counter], "--thought") ) {
            thought=1;
            strcpy(&upperbubble, "O");
            strcpy(&lowerbubble, "o");
        }
        else if ( !strcmp(argv[counter], "-h") || !strcmp(argv[counter], "--help") ) {
            displayhelp();
            return 0;
        }
    }

    /* Count characters in non-flag arguments */
    for(counter=1; counter < argc; counter++) {
        skiparg=0;
        if ( !strcmp(argv[counter], "-t") || !strcmp(argv[counter], "--thought") ) {
            skiparg=1;
        }
        else if (counter < argc && skiparg == 0) {
            argscharcount=(argscharcount + 1 + (strlen(argv[counter])));
        }
    }
    if (argscharcount == 0) {
        displayhelp();
        return -1;
    }
    argscharcount=argscharcount + 1;

    /* Display speech bubble */
    printf(" ");
    for(counter=1; counter <= argscharcount; counter++) {
        printf("_");
    }

    if ( thought == 0 ) {
        printf("\n< ");
    }
    else if ( thought == 1 ) {
        printf("\n( ");
    }

    for(counter=1; counter < argc; counter++) {
        skiparg=0;
        if ( !strcmp(argv[counter], "-t") || !strcmp(argv[counter], "--thought") ) {
            skiparg=1;
        }
        else if ( skiparg == 0 ) {
            printf("%s ", argv[counter]);
        }
    }

    if ( thought == 0 ) {
        printf(">\n ");
    }
    else if ( thought == 1 ) {
        printf(")\n ");
    }
    for(counter=1; counter <= argscharcount; counter++) {
        printf("-");
    }
    printf("\n");
    printcow();
    return 0;
}

void displayhelp(void) {
    printf("This program displays all of its arguments in a speech bubble.\n\
With your choice of animal/person/thing underneath it. Or in\n\
other words this program is a re-make of the famous Perl\n\
program ``cowsay\"\n\
\n\
Usage:\n\
    The flags should be the first set of arguments.\n\
    ccowsay <flag(s)> Sentence to say\n\
\n\
Flags:\n\
    -h or --help    - Displays this help text\n\
    -l or --list    - List all cowfiles\n\
    -e or --eyes    - Specify character used for the eyes. Must be ONE character!\n\
    -t or --thought - Thought bubble\n");
}

void printcow(void) {
printf("        %c   ^__^\n\
         %c  (%c%c)\\_______\n\
            (__)\\       )\\/\\\n\
                ||----w |\n\
                ||     ||\n", upperbubble, lowerbubble, eyes, eyes);
}
