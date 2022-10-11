#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "options.h"

void FatalError(char c, const char* msg, int exit_status);
void PrintCopymasterOptions(struct CopymasterOptions* cpm_options);


int main(int argc, char* argv[])
{
    struct CopymasterOptions cpm_options = ParseCopymasterOptions(argc, argv);
    //-------------------------------------------------------------------
    // Kontrola hodnot prepinacov
    //-------------------------------------------------------------------
    
    // Vypis hodnot prepinacov odstrante z finalnej verzie
    
    //PrintCopymasterOptions(&cpm_options);
    
    //-------------------------------------------------------------------
    // Osetrenie prepinacov pred kopirovanim
    //-------------------------------------------------------------------
    
    if (cpm_options.fast && cpm_options.slow) {
        fprintf(stderr, "CHYBA PREPINACOV\n"); 
        exit(EXIT_FAILURE);
    }
    if (cpm_options.create && cpm_options.overwrite) {
        fprintf(stderr, "CHYBA PREPINACOV\n"); 
        exit(EXIT_FAILURE);
    }
    
    // TODO Nezabudnut dalsie kontroly kombinacii prepinacov ...
    
    //-------------------------------------------------------------------
    // Kopirovanie suborov
    //-------------------------------------------------------------------
    
    // TODO Implementovat kopirovanie suborov

    //open input file
    int file_in = open(cpm_options.infile, O_RDONLY);
    int file_out;
    if (file_in < 0) {
        FatalError('B',"SUBOR NEEXISTUJE",21);
    }

    //get size and rights
    struct stat s;
    fstat(file_in,&s);
    int size = s.st_size; 
    mode_t input_mode = s.st_mode;

    if(cpm_options.inode && !S_ISREG(s.st_mode)){
        FatalError('i',"ZLY TYP VSTUPNEHO SUBORU",27);
    }
    if(cpm_options.inode && s.st_ino != cpm_options.inode_number){
        FatalError('i',"ZLY INODE",27);
    }



    // no arg or -s --slow
    if(argc == 3 || cpm_options.slow){
        file_out = open(cpm_options.outfile, O_CREAT | O_WRONLY , input_mode);
        fstat(file_out,&s);
        mode_t input_mode = s.st_mode;
        if(S_IWUSR != input_mode){
            FatalError('B',"INA CHYBA",21);
        }

        char ch;
        while(read(file_in, &ch, 1) > 0){
            write(file_out, &ch, 1);
        }
    }

    // -f --fast
    if(cpm_options.fast){
        file_out = open(cpm_options.outfile, O_CREAT | O_WRONLY, input_mode);
        char ch[size];
        read(file_in, &ch, size);
        write(file_out, &ch, size);
    }

    // -c (0644) --create
    if(cpm_options.create){
        file_out = open(cpm_options.outfile, O_EXCL | O_CREAT| O_WRONLY, cpm_options.create_mode);
        if (file_out < 0) {
            FatalError('c',"SUBOR EXISTUJE",23);
        }
        char ch[size];
        read(file_in, &ch, size);
        write(file_out, &ch, size);
    }

    // -o --overwrite
    if(cpm_options.overwrite){
        file_out = open(cpm_options.outfile, O_TRUNC | O_WRONLY);
        if (file_out < 0) {
            FatalError('o',"SUBOR NEEXISTUJE",24);
        }
        char ch[size];
        read(file_in, &ch, size);
        write(file_out, &ch, size);
    } 

    // -a --append
    if(cpm_options.append){
        int file_out = open(cpm_options.outfile, O_WRONLY | O_APPEND);
        if (file_out < 0) {
            FatalError('a',"SUBOR NEEXISTUJE",22);
        }
        char ch[size];
        read(file_in, &ch, size);
        write(file_out, &ch, size);
    }

    // -l --lseek
    if(cpm_options.lseek){
        int file_out = open(cpm_options.outfile, O_WRONLY);
        if (file_out < 0) {
            FatalError('l',"SUBOR NEEXISTUJE",33);
        }
        int a = lseek(file_in, cpm_options.lseek_options.pos1, SEEK_CUR);
        printf("%d",a);

        /*if( < 0){
            FatalError('B',"CHYBA POZICIE infile",33);
        }*/

        if(lseek(file_out, cpm_options.lseek_options.pos2, cpm_options.lseek_options.x) < 0){
            FatalError('l',"CHYBA POZICIE outfile",33);
        }



        size = cpm_options.lseek_options.num;
        char ch[size];
        read(file_in, &ch, size);
        write(file_out, &ch, size);
        
    }



    //-------------------------------------------------------------------
    // Vypis adresara
    //-------------------------------------------------------------------
    
    if (cpm_options.directory) {
        FatalError(cpm_options.directory,"INA CHYBA",23);
        // TODO Implementovat vypis adresara
    }
        
    //-------------------------------------------------------------------
    // Osetrenie prepinacov po kopirovani
    //-------------------------------------------------------------------
    
    if(cpm_options.delete_opt && S_ISREG(s.st_mode)){
        if (remove(file_in) != 0) {
            FatalError('d',"SUBOR NEBOL ZMAZANY",26);
        }

    }






    
    close(file_in);
    close(file_out);
    return 0;
}


void FatalError(char c, const char* msg, int exit_status)
{
    fprintf(stderr, "%c:%d\n", c, errno); 
    fprintf(stderr, "%c:%s\n", c, strerror(errno));
    fprintf(stderr, "%c:%s\n", c, msg);
    exit(exit_status);
}
/*
void PrintCopymasterOptions(struct CopymasterOptions* cpm_options)
{
    if (cpm_options == 0)
        return;
    
    printf("infile:        %s\n", cpm_options->infile);
    printf("outfile:       %s\n", cpm_options->outfile);
    
    printf("fast:          %d\n", cpm_options->fast);
    printf("slow:          %d\n", cpm_options->slow);
    printf("create:        %d\n", cpm_options->create);
    printf("create_mode:   %o\n", (unsigned int)cpm_options->create_mode);
    printf("overwrite:     %d\n", cpm_options->overwrite);
    printf("append:        %d\n", cpm_options->append);
    printf("lseek:         %d\n", cpm_options->lseek);
    
    printf("lseek_options.x:    %d\n", cpm_options->lseek_options.x);
    printf("lseek_options.pos1: %ld\n", cpm_options->lseek_options.pos1);
    printf("lseek_options.pos2: %ld\n", cpm_options->lseek_options.pos2);
    printf("lseek_options.num:  %lu\n", cpm_options->lseek_options.num);
    
    printf("directory:     %d\n", cpm_options->directory);
    printf("delete_opt:    %d\n", cpm_options->delete_opt);
    printf("chmod:         %d\n", cpm_options->chmod);
    printf("chmod_mode:    %o\n", (unsigned int)cpm_options->chmod_mode);
    printf("inode:         %d\n", cpm_options->inode);
    printf("inode_number:  %lu\n", cpm_options->inode_number);
    
    printf("umask:\t%d\n", cpm_options->umask);
    for(unsigned int i=0; i<kUMASK_OPTIONS_MAX_SZ; ++i) {
        if (cpm_options->umask_options[i][0] == 0) {
            // dosli sme na koniec zoznamu nastaveni umask
            break;
        }
        printf("umask_options[%u]: %s\n", i, cpm_options->umask_options[i]);
    }
    
    printf("link:          %d\n", cpm_options->link);
    printf("truncate:      %d\n", cpm_options->truncate);
    printf("truncate_size: %ld\n", cpm_options->truncate_size);
    printf("sparse:        %d\n", cpm_options->sparse);
}
*/