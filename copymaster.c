#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>


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

    if(cpm_options.link && !access(cpm_options.infile, F_OK)){
        FatalError('k',"VSTUPNY SUBOR NEEXISTUJE",30);
    }
    if(cpm_options.link && access(cpm_options.outfile, F_OK)){
        FatalError('k',"VYSTUPNY SUBOR UZ EXISTUJE",30);
    }
    
    
    //-------------------------------------------------------------------
    // Kopirovanie suborov
    //-------------------------------------------------------------------
    
    //open input file
    if (access(cpm_options.infile, F_OK) != 0){ 
        FatalError('B',"SUBOR NEEXISTUJE",21);
    }
    if(access(cpm_options.infile, R_OK) != 0) {
        FatalError('B',"INA CHYBA",21);
    }
    int file_in = -1;
    file_in = open(cpm_options.infile, O_RDONLY);
    
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

    

    int file_out = -1;

    // -c (0644) --create TODO doriesit prava
    if(cpm_options.create){
        file_out = open(cpm_options.outfile, O_EXCL | O_CREAT| O_WRONLY, cpm_options.create_mode);
        if (file_out == -1) {
            FatalError('c',"SUBOR EXISTUJE",23);
        }
    }

    // -o --overwrite 
    if(cpm_options.overwrite){
        file_out = open(cpm_options.outfile, O_TRUNC | O_WRONLY);
        if (file_out == -1) {
            FatalError('o',"SUBOR NEEXISTUJE",24);
        }
        
    } 

    // -a --append
    if(cpm_options.append){
        int file_out = open(cpm_options.outfile, O_APPEND | O_WRONLY);
        if (file_out == -1) {
            FatalError(cpm_options.append,"SUBOR NEEXISTUJE",22);
        }
        

    }

    // -l --lseek
    if(cpm_options.lseek){
        int file_out = open(cpm_options.outfile, O_WRONLY); 
        if (file_out == -1) {
            FatalError(cpm_options.lseek,"SUBOR NEEXISTUJE",33);
        }
        int a = lseek(file_in, cpm_options.lseek_options.pos1, SEEK_CUR);
        printf("%d",a);

        /*if( == -1){
            FatalError('B',"CHYBA POZICIE infile",33);
        }*/

        if(lseek(file_out, cpm_options.lseek_options.pos2, cpm_options.lseek_options.x) == -1){
            FatalError(cpm_options.lseek,"CHYBA POZICIE outfile",33);
        }



        size = cpm_options.lseek_options.num;
        
    }

    // no arg || -s --slow || -f --fast || -i --inode
    if(file_out == -1){
        if (access(cpm_options.outfile, F_OK) == 0 && access(cpm_options.outfile, W_OK) != 0) {
            FatalError('B',"INA CHYBA",21);
        }
        file_out = open(cpm_options.outfile, O_TRUNC | O_CREAT | O_WRONLY , input_mode);
    }

    //-s --slow
    if(cpm_options.slow){
        char ch;
        while(read(file_in, &ch, 1) > 0){
            write(file_out, &ch, 1);
        }
    }else{
        char ch[size];
        read(file_in, &ch, size);
        write(file_out, &ch, size);
    }

    
    close(file_in);
    close(file_out);

    

    
    
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
    



    //-t --truncate
    if (cpm_options.truncate) {
        if(cpm_options.truncate_size < 0){
            FatalError(cpm_options.truncate,"ZAPORNA VELKOST",31);
        }

        if(truncate(cpm_options.infile, cpm_options.truncate_size) != 0){
            FatalError(cpm_options.truncate,"INA CHYBA",31);
        }
    }


    // -k --link /---/ make hard link to file
    if(cpm_options.link){
        
        if (link(cpm_options.infile, cpm_options.outfile) != 0) {
            FatalError(cpm_options.link,"INA CHYBA",30);
        }
    }

    if (cpm_options.chmod) {
        if(chmod( cpm_options.outfile, cpm_options.chmod_mode) != 0){
            FatalError(cpm_options.chmod,"ZLE PRAVA",34);
        }
    }

    

    if(cpm_options.delete_opt && S_ISREG(s.st_mode)){
        if (remove(cpm_options.infile) != 0) {
            FatalError('d',"SUBOR NEBOL ZMAZANY",26);
        }
    }

    return 0;
}


void FatalError(char c, const char* msg, int exit_status)
{
    fprintf(stderr, "%c:%d\n", c, errno); 
    fprintf(stderr, "%c:%s\n", c, strerror(errno));
    fprintf(stderr, "%c:%s\n", c, msg);
    fprintf(stderr, "%c:%d\n", c, exit_status);
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