#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include <time.h>

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

    if(cpm_options.link && access(cpm_options.infile, F_OK) == -1){
        FatalError('k',"VSTUPNY SUBOR NEEXISTUJE",30);
    }
    if(cpm_options.link && access(cpm_options.outfile, F_OK) != -1){
        FatalError('k',"VYSTUPNY SUBOR UZ EXISTUJE",30);
    }

    //-u --unmask
    /*if (cpm_options.umask) {
        if(umask(cpm_options.umask_options) != 0){
            FatalError(cpm_options.umask,"INA CHYBA",32);
        }
    }*/
    
    
    //-------------------------------------------------------------------
    // Kopirovanie suborov
    //-------------------------------------------------------------------
    struct stat s;


    if(!cpm_options.directory){
        //open input file
        if (access(cpm_options.infile, F_OK) != 0){ 
            FatalError('B',"SUBOR NEEXISTUJE",21);
        }
        /*if(access(cpm_options.infile, R_OK) != 0 && access(cpm_options.infile, W_OK) != 0) {
            FatalError('B',"INA CHYBA",21);
        }*/
        int file_in = -1;


        if(cpm_options.truncate){
            file_in = open(cpm_options.infile, O_RDWR);
        }else{
            file_in = open(cpm_options.infile, O_RDONLY);
        }

        
        //get size and rights
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
            file_out = open(cpm_options.outfile, O_APPEND | O_WRONLY);
            if (file_out == -1) {
                FatalError(cpm_options.append,"SUBOR NEEXISTUJE",22);
            }
            

        }

        // -l --lseek
        if(cpm_options.lseek){
            file_out = open(cpm_options.outfile, O_WRONLY); 
            if (file_out == -1) {
                FatalError(cpm_options.lseek,"SUBOR NEEXISTUJE",33);
            }
            if(lseek(file_in, cpm_options.lseek_options.pos1, SEEK_CUR) == -1){
                FatalError(cpm_options.lseek,"CHYBA POZICIE outfile",33);
            }

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
            file_out = open(cpm_options.outfile, O_TRUNC | O_CREAT | O_WRONLY, input_mode);
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


        

        
        if (cpm_options.chmod) {
            if(chmod(cpm_options.outfile, cpm_options.chmod_mode) != 0){
                FatalError(cpm_options.truncate,"INA CHYBA",34);
            }
        }

        
        
        
        //-t --truncate
        if (cpm_options.truncate) {
            if(cpm_options.truncate_size < 0){
                FatalError(cpm_options.truncate,"ZAPORNA VELKOST",31);
            }
            if(ftruncate(file_in, cpm_options.truncate_size) != 0){
                FatalError(cpm_options.truncate,"INA CHYBA",31);
            }
        }
        
        //problematic on windows
        // -k --link /---/ make hard link to file
        if(cpm_options.link){
            
            if (link(cpm_options.infile, cpm_options.outfile) != 0) {
                FatalError(cpm_options.link,"INA CHYBA",30);
            }
        }



         close(file_in);
         close(file_out);
    }
    
    
    //-------------------------------------------------------------------
    // Vypis adresara
    //-------------------------------------------------------------------
    //-D --directory
    if (cpm_options.directory) {
        DIR *d;
        struct dirent *t;

        if((d = opendir(cpm_options.infile)) == NULL){
            FatalError(cpm_options.directory,"VSTUPNY SUBOR NIE JE ADRESAR",28);
        }

        struct stat s;
        char MY_TIME[50];

        FILE *fptr;

        fptr = fopen(cpm_options.outfile,"w");

        while((t = readdir(d)) != NULL){
            if(stat(t->d_name,&s) != 0){
                FatalError(cpm_options.directory,"VYSTUPNY SUBOR - CHYBA",28);
            }

            char permision[11] = {0};
            permision[0] = (S_ISREG(s.st_mode)) ? ('-') : ('d');
            //Owner permissions:
            permision[1] = (s.st_mode & S_IRUSR) ? ('r') : ('-');
            permision[2] = (s.st_mode & S_IWUSR) ? ('w') : ('-');
            permision[3] = (s.st_mode & S_IXUSR) ? ('x') : ('-');


            //problematic on windows, no user-group-others model;
            //Group permissions:
            permision[4] = (s.st_mode & S_IRGRP) ? ('r') : ('-');
            permision[5] = (s.st_mode & S_IWGRP) ? ('w') : ('-');
            permision[6] = (s.st_mode & S_IXGRP) ? ('x') : ('-');
            //Others permissions:
            permision[7] = (s.st_mode & S_IROTH) ? ('r') : ('-');
            permision[8] = (s.st_mode & S_IWOTH) ? ('w') : ('-');
            permision[9] = (s.st_mode & S_IXOTH) ? ('x') : ('-');




            permision[10] = '\0';


            strftime(MY_TIME, 100, "%d-%m-%Y", localtime( &s.st_mtime));
            fprintf(fptr,"%s %d %d %d %lu %s %s\n",permision,s.st_nlink, s.st_uid, s.st_gid, s.st_size, MY_TIME, t->d_name);
        }
        fclose(fptr);
    }
        
    //-------------------------------------------------------------------
    // Osetrenie prepinacov po kopirovani
    //-------------------------------------------------------------------
    
    //- m (0777) --chmod
    if (cpm_options.chmod) {
        if(chmod( cpm_options.outfile, cpm_options.chmod_mode) != 0){
            FatalError(cpm_options.chmod,"ZLE PRAVA",34);
        }
    }

    
    //- d --delete
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

// void PrintCopymasterOptions(struct CopymasterOptions* cpm_options)
// {
//     if (cpm_options == 0)
//         return;
    
//     printf("infile:        %s\n", cpm_options->infile);
//     printf("outfile:       %s\n", cpm_options->outfile);
    
//     printf("fast:          %d\n", cpm_options->fast);
//     printf("slow:          %d\n", cpm_options->slow);
//     printf("create:        %d\n", cpm_options->create);
//     printf("create_mode:   %o\n", (unsigned int)cpm_options->create_mode);
//     printf("overwrite:     %d\n", cpm_options->overwrite);
//     printf("append:        %d\n", cpm_options->append);
//     printf("lseek:         %d\n", cpm_options->lseek);
    
//     printf("lseek_options.x:    %d\n", cpm_options->lseek_options.x);
//     printf("lseek_options.pos1: %ld\n", cpm_options->lseek_options.pos1);
//     printf("lseek_options.pos2: %ld\n", cpm_options->lseek_options.pos2);
//     printf("lseek_options.num:  %lu\n", cpm_options->lseek_options.num);
    
//     printf("directory:     %d\n", cpm_options->directory);
//     printf("delete_opt:    %d\n", cpm_options->delete_opt);
//     printf("chmod:         %d\n", cpm_options->chmod);
//     printf("chmod_mode:    %o\n", (unsigned int)cpm_options->chmod_mode);
//     printf("inode:         %d\n", cpm_options->inode);
//     printf("inode_number:  %lu\n", cpm_options->inode_number);
    
//     printf("umask:\t%d\n", cpm_options->umask);
//     for(unsigned int i=0; i<kUMASK_OPTIONS_MAX_SZ; ++i) {
//         if (cpm_options->umask_options[i][0] == 0) {
//             // dosli sme na koniec zoznamu nastaveni umask
//             break;
//         }
//         printf("umask_options[%u]: %s\n", i, cpm_options->umask_options[i]);
//     }
    
//     printf("link:          %d\n", cpm_options->link);
//     printf("truncate:      %d\n", cpm_options->truncate);
//     printf("truncate_size: %ld\n", cpm_options->truncate_size);
//     printf("sparse:        %d\n", cpm_options->sparse);
// }
