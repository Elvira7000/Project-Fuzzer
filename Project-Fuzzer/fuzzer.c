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


int main(int argc, char* argv[])
{
    tar_t header;

    memcpy(header.magic, TMAGIC, TMAGLEN);
    memcpy(header.version, TVERSION, TVERSLEN);

    unsigned long size_bytes = 512;

    if (argc < 2){
        fprintf(stderr, "Usage: %s <extracteur>\n", argv[0]);
        return 1;
    }
    char *extracteur = argv[1];


    init_header(&header, "Elvira.txt", '0', 1000);    // Créer un header
    calculate_checksum(&header);


    FILE *f = fopen("archive.tar", "wb");
    if (f == NULL){
        fprintf(stderr, "Erreur: impossible de créer archive.tar\n");
        return 1;
    }
    fwrite(&header, 1, 512, f);

    // Ecriture du contenu et padding 
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

    // Bloc de fin
    char zeros[1024];
    memset(zeros, 0, sizeof(zeros));
    fwrite(zeros, 1, 1024, f);

    fclose(f);   //fermeture du fichier

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "%s archive.tar",extracteur);

     FILE *pipe = popen(cmd, "r");  // 1. Exécution
    if (pipe == NULL) {
        fprintf(stderr, "Erreur: impossible de lancer l'extracteur\n");
        return 1;
    }

    char ligne[256];
    while (fgets(ligne, sizeof(ligne), pipe) != NULL) {  // 2. Lecture ligne par ligne
        if (strstr(ligne, "*** The program has crashed ***")) {  // 3. Analyse
            printf("CRASH DÉTECTÉ !\n");
        }
    }

    pclose(pipe);  // 4. Fermeture

    // Ecriture du contenu de l'archive

    

    return 0;

    /* unsigned long file_size = 1000;    Déclaration d'une variable pour la taille du fichier en entrée*/


    /*snprintf(header.size, 12, "%011lo", file_size);
    printf("%s\n", header.size);
    return 0;             test de conversion de la taille d'un fichier en octal*/ 
    
    /*printf("size of the header: %zu bytes\n", sizeof(tar_t));  vérification de la taille de la structure*/

  
    //Calculer son checksum
    
    // Ecrire dans test.tar
    
    
    // printf("archive.tar créé !\n");



}
