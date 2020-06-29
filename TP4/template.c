/* Mahamat Issa 20116410
 * Joslin Ndjoli
 */
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
#pragma ide diagnostic ignored "readability-non-const-parameter"
#define MAXR 1000000
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FAT_NAME_LENGTH 11
#define FAT_EOC_TAG 0x0FFFFFF8
#define FAT_DIR_ENTRY_SIZE 32
#define HAS_NO_ERROR(err) ((err) >= 0)
#define NO_ERR 0
#define GENERAL_ERR -1
#define OUT_OF_MEM -3
#define RES_NOT_FOUND -4
#define CAST(t, e) ((t) (e))
#define as_uint16(x) \
((CAST(uint16,(x)[1])<<8U)+(x)[0])
#define as_uint32(x) \
((((((CAST(uint32,(x)[3])<<8U)+(x)[2])<<8U)+(x)[1])<<8U)+(x)[0])
#define true 1
#define false 0
typedef unsigned char uint8;
typedef uint8 bool;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef int error_code;
//typedef signed int uint ;

/**
 * Pourquoi est-ce que les champs sont construit de cette façon et non pas directement avec les bons types?
 * C'est une question de portabilité. FAT32 sauvegarde les données en BigEndian, mais votre système de ne l'est
 * peut-être pas. Afin d'éviter ces problèmes, on lit les champs avec des macros qui convertissent la valeur.
 * Par exemple, si vous voulez lire le paramètre BPB_HiddSec et obtenir une valeur en entier 32 bits, vous faites:
 *
 * BPB* bpb;
 * uint32 hidden_sectors = as_uint32(BPB->BPP_HiddSec);
 *
 */
typedef struct BIOS_Parameter_Block_struct {
    uint8 BS_jmpBoot[3];
    uint8 BS_OEMName[8];
    uint8 BPB_BytsPerSec[2];  // 512, 1024, 2048 or 4096
    uint8 BPB_SecPerClus;     // 1, 2, 4, 8, 16, 32, 64 or 128
    uint8 BPB_RsvdSecCnt[2];  // 1 for FAT12 and FAT16, typically 32 for FAT32
    uint8 BPB_NumFATs;        // should be 2
    uint8 BPB_RootEntCnt[2];
    uint8 BPB_TotSec16[2];
    uint8 BPB_Media;
    uint8 BPB_FATSz16[2];
    uint8 BPB_SecPerTrk[2];
    uint8 BPB_NumHeads[2];
    uint8 BPB_HiddSec[4];
    uint8 BPB_TotSec32[4];
    uint8 BPB_FATSz32[4];
    uint8 BPB_ExtFlags[2];
    uint8 BPB_FSVer[2];
    uint8 BPB_RootClus[4];
    uint8 BPB_FSInfo[2];
    uint8 BPB_BkBootSec[2];
    uint8 BPB_Reserved[12];
    uint8 BS_DrvNum;
    uint8 BS_Reserved1;
    uint8 BS_BootSig;
    uint8 BS_VolID[4];
    uint8 BS_VolLab[11];
    uint8 BS_FilSysType[8];
} BPB;

typedef struct FAT_directory_entry_struct {
    uint8 DIR_Name[FAT_NAME_LENGTH];
    uint8 DIR_Attr;
    uint8 DIR_NTRes;
    uint8 DIR_CrtTimeTenth;
    uint8 DIR_CrtTime[2];
    uint8 DIR_CrtDate[2];
    uint8 DIR_LstAccDate[2];
    uint8 DIR_FstClusHI[2];
    uint8 DIR_WrtTime[2];
    uint8 DIR_WrtDate[2];
    uint8 DIR_FstClusLO[2];
    uint8 DIR_FileSize[4];
} FAT_entry;

uint8 ilog2(uint32 n) {
    uint8 i = 0;
    while ((n >>= 1U) != 0)
        i++;
    return i;
}


// les fonctions auxiliaire pour accomplir le TP4.

void displayFatEntry(FAT_entry *entry); // afficher les details d'une repertoire/fichier
void displayBPBInfos(BPB* bootEntry); // afficher les details du BPB


int normalizeName(char *output, char *filename); // normalise un nom de dossier/fichier en un nom valide de FAT32
// lire 512 bytes d'un secteur donnee
int read_sectors(FILE* file,uint32 sector_number, unsigned char *buffer, int num_sectors);
uint8 getTotalLevel(char * path);
uint32 getDirClusterNum( FAT_entry * dirEntry);


// les variables globales utilisees.
uint32 first_data_sector;
uint32 root_cluster;
uint32 RootDirSectors;

//--------------------------------------------------------------------------------------------------------
//                                           DEBUT DU CODE
//--------------------------------------------------------------------------------------------------------

/**
 * Exercice 1
 *
 * Prend cluster et retourne son addresse en secteur dans l'archive
 * @param block le block de paramètre du BIOS
 * @param cluster le cluster à convertir en LBA
 * @param first_data_sector le premier secteur de données, donnée par la formule dans le document
 * @return le LBA
 */
uint32 cluster_to_lba(BPB *block, uint32 cluster, uint32 first_data_secto) {
    if (cluster < 2) return first_data_sector;

    uint32 lba = ((cluster - 2) * block->BPB_SecPerClus) + first_data_secto;
    return lba;
}

/**
 * Exercice 2
 *
 * Va chercher une valeur dans la cluster chain
 * @param block le block de paramètre du système de fichier
 * @param cluster le cluster qu'on veut aller lire
 * @param value un pointeur ou on retourne la valeur
 * @param archive le fichier de l'archive
 * @return un code d'erreur
 */
error_code get_cluster_chain_value(BPB *block,
                                   uint32 cluster,
                                   uint32 *value,
                                   FILE *archive) {

    uint16 byte_per_sec = as_uint16( block->BPB_BytsPerSec);
    uint32 fat_start = as_uint16(block->BPB_RsvdSecCnt);
    uint8  bps = ilog2(byte_per_sec);
    uint32 FatSector = (uint32) ( fat_start+ ((cluster << 2) >> bps));
    uint32 FatOffset = (cluster << 2) % byte_per_sec;
    uint8 buff[513];
    // read 512 bytes into buf.
    if(read_sectors(archive,FatSector, buff, block->BPB_SecPerClus) < 0) return HAS_NO_ERROR(-1);

    // cast to an array so we get an easier time.
    uint8* clusterchain = (uint8*) buff;

    // using FatOffset, we just index into the array to get the value we want.
    uint32  cchain = *((uint32*)&clusterchain[FatOffset]) & 0x0FFFFFFF;
    *value = cchain;
    return HAS_NO_ERROR(0);

}


/**
 * Exercice 3
 *
 * Vérifie si un descripteur de fichier FAT identifie bien fichier avec le nom name
 * @param entry le descripteur de fichier
 * @param name le nom de fichier
 * @return 0 ou 1 (faux ou vrai)
 */
bool file_has_name(FAT_entry *entry, char *name) {

    // allocate
    char * file = malloc(sizeof(char )*11);
    normalizeName(file,name);

    char *dirName;
    dirName = (char*) entry->DIR_Name;

    dirName[11] = '\0';

    bool res = 0 == strcmp(dirName, file);
    // free memory
    free(file);
    return res ;
}

/**
 * Exercice 4
 *
 * Prend un path de la forme "/dossier/dossier2/fichier.ext et retourne la partie
 * correspondante à l'index passé. Le premier '/' est facultatif.
 * @param path l'index passé
 * @param level la partie à retourner (ici, 0 retournerait dossier)
 * @param output la sortie (la string)
 * @return -1 si les arguments sont incorrects, -2 si le path ne contient pas autant de niveaux
 * -3 si out of memory
 */
error_code break_up_path(char *path, uint8 level, char **output) {

    char *chemin= malloc(sizeof(char) * strlen(path));
    if(chemin== NULL) return OUT_OF_MEM;
    char *token = NULL;
    char* tmp = NULL;
    chemin =  strdup(path);

    uint8 current =0;
    uint32 size = 2;
    token = strtok(chemin, "/");
    // parsing the path
    while(token != NULL) {
        size++;
        tmp = strdup(token);
        if (level == current++){

            output[0] = strdup(tmp);
        }
        token = strtok(NULL, "/");
    }
    free(chemin);
    return size;
}


/**
 * Exercice 5
 *
 * Lit le BIOS parameter block
 * @param archive fichier qui correspond à l'archive
 * @param block le block alloué
 * @return un code d'erreur
 */
error_code read_boot_block(FILE *archive, BPB **block) {
    uint8  buff[513],*buffer=buff; // le buffer qui contiendra les 512 bytes du boot sector
    size_t count;

    BPB * bootEntry = malloc(sizeof(BPB)); // allocation de la memoire
    if(bootEntry == NULL) return OUT_OF_MEM;
    // lire les 512 premiers bytes pour le boot sector;
    count = fread(buffer,sizeof(uint8),512,archive);
    if(count != 512){
        printf("Erreur de disque \n");
        return GENERAL_ERR;
    }
    // copier sur la structure BPB.
    memcpy(bootEntry, buffer, sizeof(BPB));
    *block = bootEntry;
    uint32  BPB_RootEntCnt = as_uint16(bootEntry->BPB_RootEntCnt);
    uint32 BPB_BytsPerSec = as_uint16(bootEntry-> BPB_BytsPerSec);
    uint32  BPB_RsvdSecCnt =as_uint16(bootEntry->BPB_RsvdSecCnt);
    uint32 BPB_FATSz32 = as_uint32(bootEntry->BPB_FATSz32);
    RootDirSectors = ( (BPB_RootEntCnt * 32) + (BPB_BytsPerSec -1)) / BPB_BytsPerSec;
    first_data_sector = (uint32 ) ((BPB_RsvdSecCnt)+ (bootEntry->BPB_NumFATs *BPB_FATSz32) + RootDirSectors);
    return NO_ERR;
}

/**
 * Exercice 6
 *
 * Trouve un descripteur de fichier dans l'archive
 * @param archive le descripteur de fichier qui correspond à l'archive
 * @param path le chemin du fichier
 * @param entry l'entrée trouvée
 * @return un code d'erreur
 */
error_code find_file_descriptor(FILE *archive, BPB *block, char *path, FAT_entry **entry) {
    FAT_entry *  new_entry;
    new_entry = malloc(sizeof(FAT_entry));
    uint32 start_cluster = root_cluster;
    if(new_entry == NULL) return OUT_OF_MEM;

    uint8 * buff = malloc(sizeof(uint8)* 513);
    uint8 level = getTotalLevel(path);
    // parcourir chaque dossier

    for(uint8 i =0; i < level; i++){
        bool is_fond = false;
        // calcul de l'addresse logique
        uint32 lba = cluster_to_lba(block,start_cluster,first_data_sector);
        // lecture du secteur
        read_sectors(archive,lba,buff,block->BPB_SecPerClus);
        char *out;
        // trouver le fichier/dossier courant
        break_up_path(path,i,&out);

        // trouver le Fat_entry correspondant
        for(uint32 j = 0; ; j++){
            memcpy(new_entry, buff + j*sizeof( FAT_entry), sizeof(FAT_entry));

            if (new_entry->DIR_Name[0] == 0x00) break; // fin des entrees du repertoire
            if (new_entry->DIR_Name[0] == 0xE5) continue; // fichier/dossier supprimer ou inutiliser


            //displayFatEntry(new_entry);

            int res = file_has_name(new_entry,out);// comparer l'input avec le Fat_entry reel

            if (res){ // a-ton-eu le fichier/dossier
                start_cluster = getDirClusterNum(new_entry);
                is_fond = true;
                break;
            }
        }
        if (!is_fond){
            free(buff);
            return RES_NOT_FOUND;
        }
    }

    entry[0] = new_entry;

    free(buff);
    return HAS_NO_ERROR(0);
}

/**
 * Exercice 7
 *
 * Lit un fichier dans une archive FAT
 * @param entry l'entrée du fichier
 * @param buff le buffer ou écrire les données
 * @param max_len la longueur du buffer
 * @return un code d'erreur qui va contenir la longueur des donnés lues
 */
error_code read_file(FILE *archive, BPB *block, FAT_entry *entry, void *buff, size_t max_len) {
    if (buff == NULL) return HAS_NO_ERROR(-2);
    uint32 SIZE = as_uint32(entry->DIR_FileSize);
    char* buffer = CAST(char*,buff);
    // calcul du cluster du fichier.
    uint32  cluster = getDirClusterNum(entry);
    uint8 data[513];
    size_t curr = 0;
    uint32 next;
    // lires tous les secteurs du cluster de entry
    for (uint32 i = 0; ; ++i,SIZE -= 512) {
        // calcul de l'addresse logique du fichier.
        uint32 data_s = cluster_to_lba(block,cluster,first_data_sector);

        read_sectors(archive,data_s,data,block->BPB_SecPerClus);
        data[512] = '\0';
        // printf("%s" , data);
        if (curr+512 < max_len){
            for (int j = 0; j <512 ; ++j) {
                buffer[curr++] = data[j];
            }
        } else{
            size_t remain = max_len - curr;
            if(remain > 0){
                for (int j = 0; j < remain; ++j) {
                    buffer[curr++] = data[j];
                }
            }
        }
        //trouver le prochain cluster a lire
        get_cluster_chain_value(block,cluster,&next,archive);
        cluster = next;
        // fin de fichier? si oui on boucle la boucle.
        if (cluster >= FAT_EOC_TAG) break;
    }

    return HAS_NO_ERROR(0);
}
uint32 getDirClusterNum( FAT_entry * dirEntry){
    return (as_uint16(dirEntry->DIR_FstClusHI) << 16) | as_uint16(dirEntry->DIR_FstClusLO);
}

/**
 * affiche les details d'un Fat_entry
 * @param entry
 */
void displayFatEntry(FAT_entry *entry){
    printf("filename %s\n", entry->DIR_Name);
    printf("File size = %d\n", as_uint32(entry->DIR_FileSize));
    printf("file attr %d\n", entry->DIR_Attr & 0x10);
    printf("start cluster %d\n", getDirClusterNum(entry));
}

void displayBPBInfos(BPB* bootEntry){

    printf("Name = %s\n", bootEntry->BS_OEMName);
    printf("Bytes per Sector = %d \n",as_uint16(bootEntry->BPB_BytsPerSec));
    printf("Sector per Cluster = %d\n", bootEntry->BPB_SecPerClus);
    printf("Reserved Sector Count = %d\n", as_uint16(bootEntry->BPB_RsvdSecCnt));
    printf("Number of FATs = %d\n", bootEntry->BPB_NumFATs);
    printf("Le nombre d'entrées root %d \n", as_uint16(bootEntry->BPB_RootEntCnt));
    printf("Le nombre total de secteurs %d \n", as_uint16(bootEntry->BPB_TotSec16));
    printf("Media type : %d \n", bootEntry->BPB_Media);
    printf("Le nombre de secteur %d \n", as_uint16(bootEntry->BPB_FATSz16));
    printf("Le nombre de secteur par track %d \n", as_uint16(bootEntry->BPB_SecPerTrk));
    printf("Le nombre de têtes %d\n", as_uint16(bootEntry->BPB_NumHeads));
    printf("Le nombre de secteurs cachés %d \n", as_uint32(bootEntry->BPB_HiddSec));
    printf("Le nombre total de secteurs %d \n", as_uint32(bootEntry->BPB_TotSec32));
    printf("La taille d'une table FAT %d \n", as_uint16(bootEntry->BPB_FATSz32));
    printf("La version du système de fichier %d \n", as_uint16(bootEntry->BPB_FSVer));
    printf("Le cluster de la table du dossier root(toujours 2) %d \n",as_uint32(bootEntry->BPB_RootClus) );
    printf("Type de system de fichier %s \n",bootEntry->BS_FilSysType);
}

int normalizeName(char *output, char *filename){
    int i;
    int dotPos = -1;
    char ext[3];
    int pos;

    int len = (int)strlen(filename);
    // initialisatioin de variables.
    memset(output, ' ', 11);
    memset(ext, ' ', 3);
    // cas special
    if ( (strcmp(filename,"..") ==0) || (strcmp(filename,".")==0) ){
        for (int j = 0; j < len; ++j) {
            output[j] = filename[j];
        }
        return HAS_NO_ERROR(2);
    }

    // trouver le '.' seperateur
    for (i = 0; i< len; i++)
        if (filename[i]=='.')
            dotPos = i;

    // Extract extensions
    if (dotPos!=-1){
        // Copy first three chars of extension
        for (i = (dotPos+1); i < (dotPos+4); i++)
            if (i<len)
                ext[i-(dotPos+1)] = filename[i];
        // Shorten the length to the dot position
        len = dotPos;
    }
    // ajout du  filename
    pos = 0;
    for (i=0;i<len;i++){
        if ( (filename[i]!=' ') && (filename[i]!='.') )
            output[pos++] = (char)toupper(filename[i]);
        //fin du nom du fichier
        if (pos==8) break;
    }
    // ajout de l'extension
    for (i=8;i<11;i++){
        output[i] = (char)toupper(ext[i-8]);
    }
    output[11] = '\0';
    return HAS_NO_ERROR(2);
}

/**
 * Cette fonction retourne le nombre total de niveau du path
 * @param path
 * @return
 */
uint8 getTotalLevel(char * path){
    uint8 levels = 0;
    // Count levels in path string
    while (*path){
        // Fast forward through actual subdir text to next slash
        for (; *path;){
            // If slash detected escape from for loop
            if (*path == '/') { path++; break; }
            path++;
        }
        // Increase number of subdirs founds
        levels++;
    }
    return levels;

}

///*fonction de lecture d'un secteur  */
int read_sectors(FILE* file,uint32 sector_number, unsigned char *buffer, int num_sectors){
    uint32 dest, len;     /* used for error checking only */
    uint32 bps = 512;
    if(file == NULL){
        printf("fichie nulle\n");
        return -1;
    }
    fseek(file,0,SEEK_SET);
    dest = fseek(file, sector_number*bps, SEEK_SET);
    if(dest != 0)
    {
        printf("Error in reading sector %d\n",dest);
    }
    if(num_sectors > MAXR)
    {
        printf("Buffer size too small");
    }
    len =  fread(buffer,sizeof(uint8),bps*num_sectors,file);

    if(len != bps*num_sectors)
    {
        printf("Error in reading sector len: R%d\n",len);
        return -1;
    }
    return len;
}

int main(int argc, char *argv[]) {

    BPB * bs;

    uint8 tab[MAXR];
    uint8*  buff = tab;
    FILE * file = fopen("floppy.img","rb");

    // lecture du BPB et initialisation
    read_boot_block(file,&bs);
    //displayBPBInfos( bs);

    // trouver une description
    FAT_entry* entry;
    find_file_descriptor(file,bs,"spanish/los.txt",&entry);

    // lire le fichier
    if(entry != NULL)
        read_file(file,bs,entry,buff,MAXR);
    printf("%s",buff);
    return 0;
}