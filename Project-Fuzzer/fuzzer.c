#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

/*Structure tar PSIX 1003.1-1990*/

struct tar_t
{                              /* byte offset */
    char name[100];               /*   0 */
    char mode[8];                 /* 100 */
    char uid[8];                  /* 108 */
    char gid[8];                  /* 116 */
    char size[12];                /* 124 */
    char mtime[12];               /* 136 */
    char chksum[8];               /* 148 */
    char typeflag;                /* 156 */
    char linkname[100];           /* 157 */
    char magic[6];                /* 257 */
    char version[2];              /* 263 */
    char uname[32];               /* 265 */
    char gname[32];               /* 297 */
    char devmajor[8];             /* 329 */
    char devminor[8];             /* 337 */
    char prefix[155];             /* 345 */
    char padding[12];             /* 500 */
};
typedef struct tar_t tar_t;

#define TMAGIC "ustar"
#define TMAGLEN 6
#define TVERSION "00"
#define TVERSLEN 2

/* Initialisation d'un header valide */

void init_header(tar_t *h, const char *name, char typeflag,
                 unsigned long size_bytes)
{
    memset(h, 0, sizeof(*h));
    strncpy(h->name,    name,      99);
    strncpy(h->mode,    "0000644",  8);
    strncpy(h->uid,     "0000000",  8);
    strncpy(h->gid,     "0000000",  8);
    snprintf(h->size,   12, "%011lo", size_bytes);
    snprintf(h->mtime,  12, "%011lo", (unsigned long) time(NULL));
    h->typeflag = typeflag;
    strncpy(h->magic,   TMAGIC,    TMAGLEN);
    strncpy(h->version, TVERSION,       TVERSLEN);
    strncpy(h->uname,   "root",    32);
    strncpy(h->gname,   "root",    32);
}

//implementation checksum

unsigned int calculate_checksum(tar_t *header){
    // Mettre ckeksum à des espaces
    memset(header->chksum, ' ', 8);

    unsigned int check = 0;
    unsigned char *ptr = (unsigned char *)header;

    //Additionner tous les octets du header
    for (int i = 0; i < 512; i++){
        check += ptr[i];
    }

    //Ecrire le résultat en octal dans chksum
    snprintf(header->chksum, sizeof(header->chksum), "%06o", check);

    header->chksum[6] = '\0';
    header->chksum[7] = ' ';
    return check;
}

    // Ecriture du contenu + padding

void write_content(FILE *f, unsigned long size_bytes) {
    if (size_bytes > 0){
        char *content = malloc(size_bytes);
        memset(content, 'A', size_bytes);
        fwrite(content, 1, size_bytes, f);
        free(content);

        unsigned long padding = (512 - (size_bytes % 512)) % 512;
        char zeros[512];
        memset(zeros, 0, sizeof(zeros));
        fwrite(zeros, 1, padding, f);
         
    }
    
}



// Test de crash

int test_archive(const char *extracteur, const char *nom_archive) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "%s %s",extracteur, nom_archive);

    FILE *pipe = popen(cmd, "r");  
    if (pipe == NULL) return 0;

    char ligne[256];
    int crash = 0;
    while (fgets(ligne, sizeof(ligne), pipe) != NULL) {  
        if (strstr(ligne, "*** The program has crashed ***")) {  
            crash = 1;
        }
    }

    pclose(pipe);
    return crash;

}

// save crashing

void save_crashing(const char *message){
    FILE *crashing = fopen("crashing", "a");
    if (crashing != NULL){
        fprintf(crashing, "%s\n", message);
        fclose(crashing);
    }
}

// Generation d'archive multifichiers

void generate_archive(const char *filename, int N){
    FILE *f = fopen("archive.tar", "wb");
    if (f == NULL) return ;

    tar_t header;
    for (int i = 0; i < 5; i++) {
        unsigned long size_bytes = (i % 2 == 0) ? 1000 : 0;
        char nom[100];
        snprintf(nom, sizeof(nom), "fichier%d.txt", i);
        init_header(&header, nom, '0', size_bytes);
        calculate_checksum(&header);
        fwrite(&header, 1, 512, f);
        write_content(f, size_bytes);
    }

    char fin[1024];
    memset(fin, 0, sizeof(fin));
    fwrite(fin, 1, 1024, f);
    fclose(f);

}

// Fuzzing structure

void fuzz_structure(const char *extracteur, int *crash_count){
    tar_t header;

    // Cas 1: fausse taille
    {
        FILE *f = fopen("archive.tar", "wb");
        if (f == NULL) return ;

        init_header(&header, "fichier.txt", '0', 1000);
        calculate_checksum(&header);
        fwrite(&header, 1, 512, f);
        // pas de contenu
        char fin[1024];
        memset(fin, 0, sizeof(fin));
        fwrite(fin, 1, 1024, f);
        fclose(f);

        if (test_archive(extracteur, "archive.tar")) {
            printf("CRASH - fausse taille!\n");
            rename("archive.tar", "success_structureB1.tar");
            save_crashing("Crash - fausse taille");
            crash_count++;
        }
    }

    // 2e cas : Ecrire moins que prévu 

    {
        FILE *f = fopen("archive.tar", "wb");
        if (f == NULL) return ;

        init_header(&header, "fichier.txt", '0', 1000);
        calculate_checksum(&header);
        fwrite(&header, 1, 512, f);
        char petit[10];
        memset(petit, 'A', sizeof(petit));
        fwrite(petit, 1, 10, f);
        char fin[1024];
        memset(fin, 0, sizeof(fin));
        fwrite(fin, 1, 1024, f);
        fclose(f);

        if (test_archive(extracteur, "archive.tar")) {
            printf("CRASH - moins que prévu!\n");
            rename("archive.tar", "success_structureB2.tar");
            save_crashing("Crash - moins que prévu");
            crash_count++;
        }
    }

    // 3e cas: Taille 0 mais données parasites

    {
        FILE *f = fopen("archive.tar", "wb");
        if (f == NULL) return ;

        init_header(&header, "fichier.txt", '0', 0);
        calculate_checksum(&header);
        fwrite(&header, 1, 512, f);
        char parasite[512];
        memset(parasite, 'A', sizeof(parasite));
        fwrite(parasite, 1, 10, f);
        char fin[1024];
        memset(fin, 0, sizeof(fin));
        fwrite(fin, 1, 1024, f);
        fclose(f);

        if (test_archive(extracteur, "archive.tar")) {
            printf("CRASH - données parasites!\n");
            rename("archive.tar", "success_structureB3.tar");
            save_crashing("Crash - données parasites");
            crash_count++;
        }
    }

// Test : Fichiers sans fin

    //Cas 1 : arret brutal

    {
        FILE *f = fopen("archive.tar", "wb");
        if (f == NULL) return ;

        init_header(&header, "fichier.txt", '0', 512);
        calculate_checksum(&header);
        fwrite(&header, 1, 512, f);
        write_content(f, 512);
        // pas de bloc de fin
        fclose(f);

        if (test_archive(extracteur, "archive.tar")) {
            printf("CRASH - arret brutal!\n");
            rename("archive.tar", "success_structureC1.tar");
            save_crashing("Crash - arret brutal");
            crash_count++;
        }
    }

    // Cas 2 : un seul bloc de fin
    {
        FILE *f = fopen("archive.tar", "wb");
        if (f == NULL) return ;

        init_header(&header, "fichier.txt", '0', 512);
        calculate_checksum(&header);
        fwrite(&header, 1, 512, f);
        write_content(f, 512);
        char fin[512];
        memset(fin, 0, sizeof(fin));
        fwrite(fin, 1, 512, f);
        fclose(f);

        if (test_archive(extracteur, "archive.tar")) {
            printf("CRASH - un seul bloc de fin!\n");
            rename("archive.tar", "success_structureC2.tar");
            save_crashing("Crash - un seul bloc de fin");
            crash_count++;
        }
    }

// Cas : 02 headers vides consécutifs
    {
         FILE *f = fopen("archive.tar", "wb");
        if (f == NULL) return;

    // Premier header vide
        init_header(&header, "fichier1.txt", '0', 0);
        calculate_checksum(&header);
        fwrite(&header, 1, 512, f);
    // pas de données

    // Deuxième header vide immédiatement après
        init_header(&header, "fichier2.txt", '0', 0);
        calculate_checksum(&header);
        fwrite(&header, 1, 512, f);
    // pas de données

        char fin[1024];
        memset(fin, 0, sizeof(fin));
        fwrite(fin, 1, 1024, f);
        fclose(f);

        if (test_archive(extracteur, "archive.tar")) {
            printf("CRASH - deux headers vides consécutifs!\n");
            rename("archive.tar", "success_strategieC3.tar");
            save_crashing("Crash - deux headers vides consécutifs");
            (*crash_count)++;
        }
    }
}


int main(int argc, char* argv[]) {
    if (argc < 2){
        fprintf(stderr, "Usage: %s <extracteur>\n", argv[0]);
        return 1;
    }
    char *extracteur = argv[1];
    tar_t header;
    int crash_count = 0;

// Test archives multifichiers : 100 fichiers

generate_archive("archive.tar", 100);
if (test_archive(extracteur, "archive.tar")) {
    printf("CRASH  - 100 fichiers!\n");
    rename("archive.tar", "success_crash_100.tar");
    save_crashing("Crash - 100 fichiers");
    crash_count++;
}

// Test archives multifichiers : 1000 fichiers

generate_archive("archive.tar", 1000);
if (test_archive(extracteur, "archive.tar")) {
    printf("CRASH  - 1000 fichiers!\n");
    rename("archive.tar", "success_crash_1000.tar");
    save_crashing("Crash - 1000 fichiers");
    crash_count++;
}
   
// fuzzing de la structure

fuzz_structure(extracteur, &crash_count);


printf("Total crashs détectés: %d\n", crash_count);
    return 0;


    /*unsigned long size_bytes = 512;

    init_header(&header, "Elvira.txt", '0', size_bytes);    // Créer un header
    calculate_checksum(&header);

    int N = 5;
    FILE *f = fopen("archive.tar", "wb");
    if (f == NULL){
        fprintf(stderr, "Erreur: impossible de créer archive.tar\n");
        return 1;
    }
    for (int i = 0; i < N; i++){
        unsigned long size_bytes = (i % 2 == 0) ? 1000 : 0;
        char nom[100];
        snprintf(nom, sizeof(nom), "fichier%d.txt", i);
        init_header(&header, nom, '0', size_bytes);
        calculate_checksum(&header);
        fwrite(&header, 1, 512, f);

// Ecriture du contenu et padding 
               

    }
    // Bloc de fin
    char zeros[1024];
    memset(zeros, 0, sizeof(zeros));
    fwrite(zeros, 1, 1024, f);

    fclose(f);   //fermeture du fichier




    // Ecriture du contenu de l'archive

    

    return 0;

    /* unsigned long file_size = 1000;    Déclaration d'une variable pour la taille du fichier en entrée*/


    /*snprintf(header.size, 12, "%011lo", file_size);
    printf("%s\n", header.size);
    return 0;             test de conversion de la taille d'un fichier en octal*/ 
    
    /*printf("size of the header: %zu bytes\n", sizeof(tar_t));  vérification de la taille de la structure*/

  
    //Calculer son checksum
    
    // Ecrire dans test.tar
    
    
    // printf("archive.tar créé !\n"); */



}
