#include <stdlib.h>
#include <stdio.h>
//
/*
 *
 * TP0 IFT2245:
 * Joslin Bukungu Ndjoli : 20115028
 * Mahamat Youssouf Issa : 20116410
 * date: 23 janv 2020.
 */


typedef unsigned char byte;
typedef int error_code;

#define ERROR (-1)
#define HAS_ERROR(code) ((code) < 0)
#define HAS_NO_ERROR(code) ((code) >= 0)



/*
    Declaration de fonction pour le programme.
*/

void printChar(char * out,int len); // pour afficher le contenu d u  pointeur de char.
error_code memcpy(void *dest, void *source, size_t len);
void memory_error();

/**
 * Cette fonction compare deux cha�nes de caract�res.
 * @param p1 la premi�re cha�ne
 * @param p2 la deuxi�me cha�ne
 * @return le r�sultat de la comparaison entre p1 et p2. Un nombre plus petit que
 * 0 d�note que la premi�re cha�ne est lexicographiquement inf�rieure � la deuxi�me.
 * Une valeur nulle indique que les deux cha�nes sont �gales tandis qu'une valeur positive
 * indique que la premi�re cha�ne est lexicographiquement sup�rieure � la deuxi�me.
 */
int strcmp(char *p1, char *p2) {
    char *s1 = (char *) p1;
    char *s2 = (char *) p2;
    char c1, c2;
    do {
        c1 = (char) *s1++;
        c2 = (char) *s2++;
        if (c1 == '\0')
            return c1 - c2;
    } while (c1 == c2);
    return c1 - c2;
}

/**
 * Ex. 1: Calcul la longueur de la cha�ne pass�e en param�tre selon
 * la sp�cification de la fonction strlen standard
 * @param s un pointeur vers le premier caract�re de la cha�ne
 * @return le nombre de caract�res dans le code d'erreur, ou une erreur
 * si l'entr�e est incorrecte
 */
error_code strlen(char *s) {

    char fin = '\0';
    if(s != NULL) {
        int count = 0;
        while(*s++ != fin){
            count++;
        }
        return count;
    }
    else {
        return ERROR;
    }
}

/**
// * Ex.2 :Retourne le nombre de lignes d'un fichier sans changer la position
// * courante dans le fichier.
// * @param fp un pointeur vers le descripteur de fichier
// * @return le nombre de lignes, ou -1 si une erreur s'est produite
// */
error_code no_of_lines(FILE *fp) {


    FILE * tmp = fp;
    error_code numberLine =0;
    char c = ' ';
    // Extract characters from file and store in character c
    // On extrait les characteres du fichier pour les stocker dans la variable c.
    for (c = getc(tmp); c != EOF; c = getc(tmp))
        if (c == '\n') //on incremente notre compteur si on a un \n.
            numberLine  += 1;
    fseek(fp,0,SEEK_SET);
    return numberLine; // on retourne le nombre de ligne .
}


int equals(char*p, char* q){

    int res = strcmp(p,q);
    if(res == 0) res=  1;
    else res = 0;

    return res;
}
///**
// * Ex.3: Lit une ligne au complet d'un fichier
// * @param fp le pointeur vers la ligne de fichier
// * @param out le pointeur vers la sortie
// * @param max_len la longueur maximale de la ligne � lire
// * @return le nombre de caract�re ou ERROR si une erreur est survenue
// */

error_code readline(FILE *fp, char **out, size_t max_len) {


    if (fp == NULL)
        return ERROR;
    char *line = malloc(sizeof(char) * max_len +1);
    if(line == NULL) return ERROR;
    if ( line != NULL){

        char c = ' ';
        int nb = 0;
        while ((c = getc(fp))  != EOF) {

            if (c == '\n') {
                break; // on est a la fin de la ligne.
            }
            if (nb < max_len) { // si on a pas depasser le max du caractere
                line[nb++] = c; // on lit le charactere sur notre buffer
            } else break;

        }
        if(c == EOF && nb == 0)
            return EOF;

        line[nb] = '\0';// caractere de fin d'un char; // A t-on besoin ???
        memcpy(*out,line,nb);// on prend une copie du string lu
        free(line);// on free la memoire du string allouer.
        return nb;
    } else {
        return ERROR;
    }
}


///**
// * Ex.4: Structure qui d�note une transition de la machine_file_file de Turing
// */
typedef struct {

    char *etat_courant;
    char *nouvel_etat;
    char mouvement;
    char symbole_a_lire;
    char symbole_a_ecrire;

} transition;
//
///**
// * Ex.5: Copie un bloc m�moire vers un autre
// * @param dest la destination de la copie
// * @param src  la source de la copie
// * @param len la longueur (en byte) de la source
// * @return nombre de bytes copi�s ou une erreur s'il y a lieu
// */
error_code memcpy(void *dest, void *src, size_t len) {

    if(src != NULL) {
        // On cast dest et source en char comme on ne peut pas faire d'assignement avec le
        // le type void
        char *csrc = (char *)src;
        char *cdest = (char *)dest;
        byte nb =0;
        // Copie de la memoire de src vers la destination. Exactement len bytes seront
        // copies. Comme un caractere est represente sur 1 byte, alors on copiera donc
        // len caracteres (char)
        for (int i=0; i<len; i++)
        {
            *(cdest+i) = *(csrc +i);
            nb++;
        }
        return nb;
    }
    else {
        return ERROR;
    }
}
//
/**
 *
 * @param line la chaine de caractere a parser
 * @param index  l'index du debut du parsing;
 * @param size  la longueur de la chaine de charactere
 * @return un pointeur de char
 */
char* get_state(char ** line,size_t size){

    int index = 0;
    char* copiOne = *line;

    char c;
    char * real = NULL;
    do {
        c= *copiOne++;
        int curentLenState = 5;
        char * etat = malloc(sizeof(char)* curentLenState);
        if(etat == NULL) return NULL;
        if(c == '('){
            index++;
            char *fill = etat;
            int lenState = 0;
            while ( (c = *copiOne++) != ','){ // tant que ne rencontre pas ',';
                if(lenState < curentLenState ){ // la longueur de l'etat est long?
                } else{
                    etat = realloc(etat,lenState*2);
                    if(etat  == NULL)
                        return NULL;
                    curentLenState = lenState*2;
                }
                lenState++;
                *fill++  =c;
                index++;

            }
            if(c == ','){
                real = malloc(sizeof(char) * lenState+1);
                real[lenState] = '\0';
                memcpy(real,etat,lenState);
                free(etat);
                *line = copiOne;
                return real;
            }

        } else{ break; }
    } while (0);
    return NULL;
}
/**
 * Ex.6: Analyse une ligne de transition
 * @param line la ligne � lire
 * @param len la longueur de la ligne
 * @return la transition ou NULL en cas d'erreur
 */
transition *parse_line(char *line, size_t len){

    if(line != NULL){
        int index = 0;
        char * string = line;
        int currLent =0;
        transition* trans = malloc(sizeof(transition));
        // initialison des etas;
        trans->etat_courant = '\0';
        trans->nouvel_etat = '\0';
        if(trans == NULL) return ERROR; // allocation echouee?
        // on parse la chaine
        trans->etat_courant = get_state(&string,len);
        trans->symbole_a_lire = *string++;
        while (  *++string != '(') ;
        trans->nouvel_etat = get_state(&string,strlen(string));
        trans->symbole_a_ecrire = *string++;
//
        trans->mouvement  = *++string;
        return trans;
    } else {
        return NULL;
    }
}
/**
 * Ex.7: Execute la machine_file_file de turing dont la description est fournie
 * @param machine_file_file_file le fichier de la description
 * @param input la cha�ne d'entr�e de la machine_file_file de turing
 * @return le code d'erreur
 */

/**
 * Cette fonction libere la memoire allouer pour le fichier de
 * description de la machine de Turing.
 * @param tab
 * @param len la longuer de la machine
 */
void FreeMemory(transition ** tab,int len){

    transition** p = tab;
    for(int j =0; j < len-2; j++){
        free((*p)->etat_courant);
        free((*p)->nouvel_etat);
        free(*p++);
    }
}

void memory_error(){
    printf("Allocation memoire echouee\n");
    exit(ERROR);
}
transition* findState(transition* rub,transition** desc,int len){
    transition ** des = desc;

    for(int i=0; i < len; i++){
        int pre = equals((*des)->etat_courant,rub->etat_courant);
        int sec =  rub->symbole_a_lire == (*des)->symbole_a_lire;
        if((*des)->symbole_a_lire == ' ') if(rub->symbole_a_lire == '\0') sec=1;
        if( pre && sec){
            rub->mouvement = (*des)->mouvement;
            rub->symbole_a_ecrire = (*des)->symbole_a_ecrire;
            int len = strlen((*des)->nouvel_etat);
            rub->nouvel_etat = (char *) malloc(sizeof(char)*len+1); // allocation de memoire pour l'etat trouver.
            if(rub->nouvel_etat == NULL)
                return NULL;
            memcpy(rub->nouvel_etat,(*des)->nouvel_etat,len); // on copie le nouveau etat.
            rub->nouvel_etat[len] ='\0'; // le fin de charactere.
            return rub; // on retourne.
        }
        des++;
    }
    return NULL;
}

/**
 * Ex.7: Execute la machine de turing dont la description est fournie
 * @param machine_file le fichier de la description
 * @param input la cha?ne d'entr?e de la machine de turing
 * @return le code d'erreur
 */


int execute(char * machine_file,char* input){

    FILE* the_file  = fopen(machine_file, "r"); // on ouvre le fichier pour collecter les donnees.
    if(the_file == NULL) {
        printf("fichier nulle\n");
        return ERROR;
    }
    int nbLine = no_of_lines(the_file);
    char* tableau = (char*) malloc(sizeof(char) * 255); // le tableau

    transition** Description = (transition **) malloc(sizeof(transition **)* nbLine); // on alloue de la memoire pour toutes les
    // descriptions de la machine_file_file;
    if(Description == NULL){
        free(tableau); // en cas d'erreur on libere la memoire deja allouer.
        return  ERROR; // l'allocation de la memoire a t elle echouee? si ou on retourne ERROR
    }

    transition** parser = Description;
    char * qa, *qr,*q0; // l'etat acceptant, rejettant et l'etat initial.

    for(int i =0; i <= nbLine; i++){
        int b = readline(the_file,&tableau,255);
        if(i >=3){
            *parser++ = parse_line(tableau,b);
        }else{ // l'allocation de la memoire pour l'etat rejettant,l'etat acceptant et l'etat initial

            switch (i){
                case 0: q0 = malloc(sizeof(char)*b+1); memcpy(q0,tableau,b); q0[b]= '\0' ;break;
                case 1: qa = malloc(sizeof(char)*b+1); memcpy(qa,tableau,b); qa[b]= '\0' ;break;
                case 2: qr = malloc(sizeof(char)*b+1); memcpy(qr,tableau,b); qr[b]= '\0' ;break;
            }
        }
    }
    transition* ruban =(transition*) malloc(sizeof(transition));
    if(ruban == NULL) // allocation echouer
        return ERROR;
    // l'index et la taille courant du ruban.
    int realLen = strlen(input)*2;
    int index =0;
    char * inp = (char*) malloc(sizeof(char) * realLen); // allocation du ruban;
    if(inp == NULL){
        free(ruban);
        FreeMemory(Description,nbLine); // liberation de la memoire pour toutes les descriprrions du fichier;
        free(tableau); // liberation de la memoire allouer pour le pointeur de lecture de ligne

    }
    memcpy(inp,input,realLen); // copie de l'input sur le ruban.
    ruban->etat_courant = malloc(sizeof(char)* strlen(q0));
    if(ruban->etat_courant == NULL){
        FreeMemory(Description,nbLine); // liberation de la memoire pour toutes les descriprrions du fichier;
        free(tableau); // liberation de la memoire allouer pour le pointeur de lecture de ligne

    }
    memcpy(ruban->etat_courant,q0,strlen(q0));
    ruban->symbole_a_lire = *inp;
    // l'index et la taille pour l'etat courant;
    char * travail =inp;
    int code = 0;
    while (1){
        ruban = findState(ruban,Description,nbLine-3);
        if(ruban != NULL){
            ruban->etat_courant = ruban->nouvel_etat;
            if( equals(ruban->etat_courant,qa)){
                code =  1;
                break;
            } else if(equals(ruban->etat_courant,qa) ){
                break;
            }
            char newChar =  ruban->symbole_a_ecrire;
            *inp = newChar;
            switch (ruban->mouvement){
                case 'D': inp++; index++; break; // on avance d'un caractere
                case 'R': break; // on reste sur place
                case 'G': inp--;index--; break; // on recule d'un caractere.
            }
            if(index > realLen){ // a ton atteint la limite ??
                realLen *=2;
                inp = realloc(inp, sizeof(char)*realLen); // realocation du ruban.
                if(inp == NULL) break; // on arrete la boucle pour liberer ci-dessus les differents resources.
            }
            ruban->symbole_a_lire = *inp;
        } else { // input incorrect
            break;
        }
    }

    FreeMemory(Description,nbLine); // liberation de la memoire pour toutes les descriprrions du fichier;
    free(tableau); // liberation de la memoire allouer pour le pointeur de lecture de ligne
    free(q0); free(qr); free(qa); // liberation de la memoire pour les etats initial,acceptant,rejettant.

    return code;
}


int main() {
    // test #7

    char * input = "1111";
    int ok = execute("power_len.txt",input);

    if(ok == 1){
        printf("la machine de Turing accepte le mot %s\n",input);
    } else{
        printf("la machine de Turing rejette le mot %s\n",input);
    }


    return 0;
}
