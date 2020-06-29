/**
 * Joslin Bokungu Ndjoli 20115028
 * Mahamat Youssouf Issa 20116410
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>

typedef struct command_struct command;
typedef struct command_chain_head_struct command_head;
typedef int bool;
#define  true 1
#define false 0

#define NULL_TERMINATOR '\0'

typedef enum {
    BIDON, NONE, OR, AND, ALSO
} operator;

struct command_struct {
    int *ressources;
    char **call;
    int call_size;
    int count;
    operator op;
    bool also; // ajouter
    command *next;

};

struct command_chain_head_struct {
    int *max_resources;
    int max_resources_count;
    command *command;
    pthread_mutex_t *mutex;
    bool background;
    int length;
};

// Forward declaration
typedef struct banker_customer_struct banker_customer;

struct banker_customer_struct {
    command_head *head;
    banker_customer *next;
    banker_customer *prev;
    int *current_resources;
    int * need_ressources;
    int depth;
};

typedef int error_code;
#define HAS_ERROR(err) ((err) < 0)
#define HAS_NO_ERROR(err) ((err) >= 0)
#define NO_ERROR 0
#define ERROR ((-1))
#define CAST(type, src)((type)(src))


typedef struct {
    char **commands;
    int *command_caps;
    unsigned int command_count;
    unsigned int ressources_count;
    int file_system_cap;
    int network_cap;
    int system_cap;
    int any_cap;
    int *all_resources_qty;  // ajouter
    int res_number; // ajouter
    int no_banquers;
} configuration;


// some functions declarations
int resource_count(int resource_no);
error_code resource_no(char *res_name);
void freeIntTable(int* table);
void freeBankerCustomerStruct(banker_customer* head);
void freeWholeBankerStruct();
bool check_conformity(command_head* client);

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
#define FS_CMDS_COUNT 10
#define FS_CMD_TYPE 0
#define NETWORK_CMDS_COUNT 6
#define NET_CMD_TYPE 1
#define SYS_CMD_COUNTS 3
#define SYS_CMD_TYPE 2

// Configuration globale
configuration *conf = NULL;

static int nbCustomer = 0, OTHER_TYPE;
void freeStringArray(char **arr) {
    if (arr != NULL) {
        for (int i = 0; arr[i] != NULL; i++) {
            free(arr[i]);
        }
    }
    free(arr);
}

command *freeAndNext( command *current) {
    if (current == NULL) return current;
    command *next = current->next;
    freeStringArray(current->call);
    free(current);
    return next;
}


/**
 *
 * @param current
 * @return call the command return  -1 if an error occur
 */
int callCommand( command *current) {
    if (current->count <= 0) return 1;

    int exitCode = -1;     // the exit code of the child
    pid_t pid = 0;
    for (int i = 0; i < current->count; i++) {
        pid = fork();

        if (pid < 0) {        // forking failed
            return pid;
        } else if (pid == 0) {
            // -----------------------------------------------------
            //                    CHILD PROCESS
            // -----------------------------------------------------
            char *cmd_name = current->call[0];
            execvp(cmd_name, current->call);    // execvp searches for command[0] in PATH, and then calls command

            printf("bash: %s: command not found\n", cmd_name);    // if we reach this, exec couldn't find the command

            exit(49);    // and then we exit with 2, signaling an error. This also frees ressources
        }
    }

    // PID is correct
    waitpid(pid, &exitCode, 0); // Wait for the child process to exit.
    signed int x =(int) WIFEXITED(exitCode);
    if (!x) return 0;

    x = WEXITSTATUS(exitCode);
    if (x != 0) return 0;
    return 1;
}




void displayResourceLines(command_head *head){
    int header = 1;
    char *tab[conf->res_number+1];
    tab[conf->res_number] = NULL;
    int resTab[conf->res_number],*ta=resTab;
    if(header){

        for (int k = 0; k < conf->res_number;++k) {
            ta[k] = head->max_resources[k];
        }
        tab[0] = "A";tab[1] = "B"; tab[2] = "C"; tab[conf->res_number-1] = "Other";

        for (int j = 0; j < conf->res_number >> 1; ++j) {
                tab[j+3] = conf->commands[j];

        }
        int max  = conf->res_number;
        for (int i = 0; i < conf->res_number; ++i) {
            if(i < 3 ) printf("%s\t", tab[i]);
            else if (i != (conf->res_number-1) )printf("%s ", tab[i]);
            else  printf("%s \n", tab[i]);
        }
    }

    for ( int i = 0; i < conf->res_number; ++i) {
        // if(i != 0)
        if(i <= 2 ) printf("%d\t", resTab[i]);
        else if (i != (conf->res_number-1) ){
            int blank = strlen(tab[i]);
            char espcae[blank] ,*sp = espcae;
            for (int j = 0; j < blank; ++j) {
                sp[j] = ' ';
            }
            sp[blank-1] = '\0';
            printf("%d %s", resTab[i],sp);
        }
        else printf("%d\n", resTab[i]);
    }
    printf("\n");
}

void display(int* ch){

    command_head* c = malloc(sizeof(command_head));
    c->max_resources = ch;
    displayResourceLines(c);
}
/**
 *
 * @param head
 */
void freeCommands( command *head) {
    command *courrant = head;
    while (courrant != NULL) {
        command* tmp = courrant;
        courrant = courrant->next;
        freeStringArray(tmp->call);
        free(tmp);
    }
}

/**
 * Libere les ressources utiliser par une structure
 * command_head.
 * @param commandHead
 */
void freeHeadCommand(command_head* commandHead){
    if(commandHead == NULL) return;
    freeIntTable(commandHead->max_resources);
    freeCommands(commandHead->command);
    pthread_mutex_destroy(commandHead->mutex);
    free(commandHead);
}

/***********************************************************
    la fonction pour parser une ligne de commande.
 */
error_code nextCommands(char const *line,  command **out) {
    char **call = NULL;
    char *wordPtr = NULL;
    command *c = NULL;

    int currentCallSize = 0;
    command *head = NULL;
    command *current = NULL;

    int escaped = 0;
    int size =0;
    for (int index = 0; line[index] != NULL_TERMINATOR; index++) {
        int nextSpace = index;
        while (1) {
            if (!escaped) {
                if (line[nextSpace] == ' ' || NULL_TERMINATOR == line[nextSpace]) {
                    break;
                }
            }
            if (line[nextSpace] == '(') escaped = 1;
            if (line[nextSpace] == ')') escaped = 0;
            nextSpace++;
        }

        wordPtr = malloc(sizeof(char) * (nextSpace - index + 1));
        if (wordPtr == NULL) goto error;

        int i = 0;
        for (; i < nextSpace - index; i++) wordPtr[i] = line[i + index];
        wordPtr[i] = NULL_TERMINATOR;

        operator  op = BIDON;
        if (strcmp(wordPtr, "||") == 0) op = OR;
        if (strcmp(wordPtr, "&&") == 0) op = AND;
        if (strcmp(wordPtr, "&") == 0) op = ALSO;
        if (op != ALSO && line[nextSpace] == NULL_TERMINATOR) op = NONE;

        if(op == OR || op == AND || op == ALSO) free(wordPtr);

        if (op == BIDON || op == NONE) {
            if (call == NULL) {
                currentCallSize = 1;
                call = malloc(sizeof(char *) * 2);
                if (call == NULL) goto error;
                call[1] = NULL;
            } else {
                currentCallSize++;
                char **tempPtr = realloc(call, (currentCallSize + 1) * sizeof(char *));
                if (tempPtr == NULL) goto error;

                call = tempPtr;
                call[currentCallSize] = NULL;
            }

            call[currentCallSize - 1] = wordPtr;
        }

        if (op != BIDON) {
            c = malloc(sizeof(command));
            if (c == NULL) goto error;

            if (head == NULL){ head = c; size++;}
            else {
                current->next = c;
                size++;
            }

            c->count = 1;
            if (call[0][strlen(call[0]) - 1] == ')') {
                char *command = call[0];

                unsigned long command_len = strlen(command); //rn et fn ici

                int paren_pos = 0;
                for (; command[paren_pos] != '('; paren_pos++);

                wordPtr = malloc(paren_pos * sizeof(char));
                if(wordPtr == NULL) goto error;

                for (int j = 0; j < paren_pos; j++) wordPtr[j] = command[j + 1];
                wordPtr[paren_pos - 1] = NULL_TERMINATOR;

                int nb = atoi(wordPtr); // NOLINT(cert-err34-c)
                c->count = ('r' == command[0]) ? nb : 0;

                free(wordPtr);
                if (NULL == (wordPtr = malloc(sizeof(char) * (command_len - paren_pos - 1)))) {
                    goto error;
                }

                memcpy(wordPtr, &command[paren_pos + 1], command_len - paren_pos - 2);
                wordPtr[command_len - paren_pos - 2] = NULL_TERMINATOR;
                free(command);

                unsigned long resized_len = strlen(wordPtr);

                int space_nb = 0;
                for (int j = 0; j < resized_len; j++) {
                    space_nb += wordPtr[j] == ' ';
                }

                char **temp;
                if (NULL == (temp = realloc(call, sizeof(char *) * (space_nb + 2)))) {
                    goto error;
                } else {
                    call = temp;
                    call[space_nb + 1] = NULL;
                }

                int copy_index = 0;
                char *token = strtok(wordPtr, " ");
                while (token != NULL) {
                    char *arg = malloc(sizeof(char) * (strlen(token)+1));
                    /* if (NULL == (arg )) {
                         goto error;
                     }*/

                    strcpy(arg, token);
                    call[copy_index] = arg;

                    token = strtok(NULL, " ");

                    copy_index++;
                }

                free(wordPtr);
            }
            c->call_size = currentCallSize;
            c->call = call;

            int res = resource_no(call[0]); // find the resource type
            c->ressources = malloc(sizeof(int));
            if(c->ressources == NULL) goto error;
            c->ressources[0] = res;
            c->also = false;
            c->next = NULL;
            c->op = op;
            if(op == ALSO){
                head->also = true;
                c->op = NONE;
            }
            current = c;
            call = NULL;

        }

        if (line[nextSpace] == NULL_TERMINATOR) break;
        index = nextSpace;
    }
    out[0] = head;
    return size;

    error:
    free(wordPtr);
    freeStringArray(call);
    freeAndNext(c);
    freeCommands(head);
    return ERROR;
}



/*
 *  LA fonction readline
 *
 */

error_code readLine(char **out) {
    size_t size = 10;                       // size of the char array
    char *line = malloc(sizeof(char) * size);       // initialize a ten-char line
    if (line == NULL) return ERROR;   // if we can't, terminate because of a memory issue

    for (int at = 0; 1; at++) {
        if (at >= size) {
            size = 2 * size;
            char *tempPtr = realloc(line, size * sizeof(char));
            if (tempPtr == NULL) {
                free(line);
                return ERROR;
            }

            line = tempPtr;
        }

        int tempCh = getchar();

        if (tempCh == EOF) { // if the next char is EOF, terminate
            free(line);
            return ERROR;
        }

        char ch = (char) tempCh;

        if (ch == '\n') {        // if we get a newline
            line[at] = NULL_TERMINATOR;    // finish the line with return 0
            break;
        }



        line[at] = ch; // sets ch at the current index and increments the index
    }

    out[0] = line;
    return 0;
}

/**
 * Cette fonction analyse la première ligne et remplie la configuration
 * @param line la première ligne du shell
 * @param conf pointeur vers le pointeur de la configuration
 * @return un code d'erreur (ou rien si correct)
 */
error_code parse_first_line(char *line) {


    int res_init_no = 3;
    int count = 0; // nbr de commandes speciales (ex:sed, echo)
    char *line_cp= malloc(sizeof(line));
    if(line_cp== NULL) return HAS_ERROR(-4);
    int res_count = 0; // nbr total de ressources
    line_cp = strdup(line);
    char occ[3] = {line_cp[0],line_cp[1],'\0'};

    // debut
    conf->commands = NULL;
    conf->command_caps = NULL;
    conf->command_count = 0;

    char** liste_cmd = (char**) malloc(sizeof(char*) *4);
    if(liste_cmd == NULL) return HAS_ERROR(-3);

    if(line != NULL) {
        int global=0,len =4;
        char *token = strtok(line_cp, "&");
        char *tmp_01;
        while(token != NULL) {
            if(count >= len){
                len *=2;
                liste_cmd = realloc(liste_cmd, sizeof(char*)*len);
                if(liste_cmd == NULL) return HAS_ERROR(-3);
            }
            tmp_01 = strdup(token);
            liste_cmd[count] = tmp_01;
            count++;
            token = strtok(NULL, "&");
        }

        liste_cmd[count] = NULL;
        count = 0;
        if(strcmp(occ, "&&") != 0) {  //pas de ressource speciale?

            // traitement pour parser les commandes redefini.
            char *t1 = strdup(liste_cmd[global++]);
            char *t2 = strdup(liste_cmd[global++]);
            char** commands_name = (char**) malloc(sizeof(char*)*2);
            if(commands_name == NULL) return HAS_ERROR(-43);
            len = 2;
            // identification de toutes les commandes redefinies.
            char* token1 = strtok(t1, ",");
            while(token1 != NULL) {
                if(count >= len){
                    len +=1;
                    commands_name = realloc(commands_name, sizeof(char*)*len);
                    if(commands_name == NULL) return HAS_ERROR(-4);
                }
                int taille = (int) strlen(token1);
                commands_name[count] = malloc(sizeof(char) * taille);
                if(commands_name[count] == NULL){
                    freeStringArray(liste_cmd);
                    return HAS_ERROR(-3);
                }
                strcpy(commands_name[count], token1);
                token1 = strtok(NULL, ",");
                count++;    // nombre de commandes speciales
            }
            commands_name[count] = NULL;
            conf->commands = commands_name;
            // allocation de la memoire.
            int* commands_cap = (int*) malloc(sizeof(int)* count);
            if(commands_cap == NULL) return HAS_ERROR(-4);
            int* ressource_qty = (int*) malloc(sizeof(int) * count);
            if(ressource_qty == NULL) return HAS_ERROR(-4);

            char* token2 = strtok(t2, ",");
            int c = 0;
            char* str;
            /* extraire la quantite de ressource de chaque
             commande redefinie.*/
            while(token2 != NULL) {
                str = token2;
                // convert to integer
                int counte = CAST(int,strtol(str,NULL,10));
                // assign the value
                ressource_qty[c] = counte;
                commands_cap[c] = res_init_no++;
                res_count += counte;
                c++;
                token2 = strtok(NULL, ",");
            }
            conf->command_caps = commands_cap;
            conf->all_resources_qty = ressource_qty;

        }
            /*
             * Extraire le nombre de ressource de 3 type de ressource
             * et la categorie other.
             */
            int val =0,type=0;
            while(liste_cmd[global] != NULL) {

                tmp_01 = liste_cmd[global++];
                val = CAST(int,strtol(tmp_01,NULL,10)); // caster en int
                res_count += val;
                switch (type++){ // assigner les bonnes valeurs dans la configuration gloabale.
                    case FS_CMD_TYPE:  conf->file_system_cap = val;
                    case NET_CMD_TYPE:     conf->network_cap = val;
                    case SYS_CMD_TYPE:      conf->system_cap = val;
                    case 3:                    conf->any_cap = val;
                }
            }
            // assignation d'autre champs de la configuration
            conf->command_count = count;
            conf->ressources_count = res_count;
            conf->res_number =4+count;
            OTHER_TYPE = res_init_no;
            freeStringArray(liste_cmd); // liberation de la memoire utiliser.
            return HAS_ERROR(1);
    }
    else {
        return HAS_ERROR(-3);
    }
}



const char *FILE_SYSTEM_CMDS[FS_CMDS_COUNT] = {
        "ls",
        "cat",
        "find",
        "grep",
        "tail",
        "head",
        "mkdir",
        "touch",
        "rm",
        "whereis"
};

const char *NETWORK_CMDS[NETWORK_CMDS_COUNT] = {
        "ping",
        "netstat",
        "wget",
        "curl",
        "dnsmasq",
        "route"
};

const char *SYSTEM_CMDS[SYS_CMD_COUNTS] = {
        "uname",
        "whoami",
        "exec"
};

/**
 * Cette fonction prend en paramètre un nom de ressource et retourne
 * le numéro de la catégorie de ressource
 * @param res_name le nom
 * @param config la configuration du shell
 * @return le numéro de la catégorie (ou une erreur)
 */
error_code resource_no(char *res_name) {

    // la commande est-il redefinie?
    for (int k = 0; k < conf->command_count; ++k) {
        if (strcmp(res_name, conf->commands[k]) == 0) {
            return conf->command_caps[k]; // alors on retourne son propre type;
        }
    }
    // commande de type fichier commande
    for (int i = 0; i < FS_CMDS_COUNT; ++i) {
        if((strcmp(res_name, FILE_SYSTEM_CMDS[i]) == 0)) {
            return FS_CMD_TYPE;
        }
    }
    // commande de type network commands
    for (int i = 0; i < NETWORK_CMDS_COUNT; ++i) {
        if(strcmp(res_name, NETWORK_CMDS[i]) == 0) {
           return NET_CMD_TYPE;
        }
    }
    // commande de type de systeme commands
    for (int i = 0; i < SYS_CMD_COUNTS; ++i) {
        if(strcmp(res_name, SYSTEM_CMDS[i]) == 0) {
           return SYS_CMD_TYPE;
        }
    }
   // alors ce de type other
    return OTHER_TYPE;
}

/**
 * Cette fonction prend en paramètre un numéro de ressource et retourne
 * la quantitée disponible de cette ressource
 * @param resource_no le numéro de ressource
 * @param conf la configuration du shell
 * @return la quantité de la ressource disponible
 */
int resource_count(int resource_no) {

    // chercher en premier parmi les commandes redefinie
    if(conf->commands != NULL && (resource_no > 2)){
        int res_no = resource_no;
        unsigned int limit = conf->command_count;

        for (int i = 0; i < limit; ++i) {
            if (conf->command_caps[i] == res_no){
                return conf->all_resources_qty[i];
            }
        }
    }
    // si non chercher dans les categories principales.
    switch (resource_no) {
        case FS_CMD_TYPE:  return conf->file_system_cap;
        case NET_CMD_TYPE:     return conf->network_cap;
        case SYS_CMD_TYPE:      return conf->system_cap;
        default:                  return conf->any_cap;
        }
}



// Forward declaration
error_code evaluate_whole_chain(command_head *head);


/**
 * Créer une chaîne de commande qui correspond à une ligne de commandes
 * @param config la configuration
 * @param line la ligne de texte à parser
 * @param result le résultat de la chaîne de commande
 * @return un code d'erreur
 */

error_code create_command_chain(const char *line, command_head **result) {

    command_head* cHead = malloc(sizeof(command_head));
    if(cHead == NULL) return ERROR;
    command *head = malloc(sizeof(command));
    if (head == NULL) return ERROR;
    if (line == NULL) return ERROR;

    // parsing the entry
    int size = nextCommands(line,&head);
    // if error occur then exit.
    if(size < 0 ) return ERROR;

    // assignation of good value and memory allocations
    cHead->command = head;
    cHead->mutex = malloc(sizeof(pthread_mutex_t)); // a voir
    pthread_mutex_init(cHead->mutex ,NULL);
    cHead->background = head->also;
    cHead->length = size;
    // evaluate the whole chain
    evaluate_whole_chain(cHead);

    result[0]= cHead;
    return HAS_NO_ERROR(size);

}

/**
 * Cette fonction compte les ressources utilisées par un block
 * La valeur interne du block est mise à jour
 * @param command_block le block de commande
 * @return un code d'erreur
 */
error_code count_ressources(command_head *head, command *command_block) {


    if(command_block == NULL && head == NULL) return -1;
    int max = conf->res_number;
    command* header  = command_block;
    head->max_resources_count = 0;

    head->max_resources = malloc(sizeof(int)* max);// allocationde la memoire
    if(head->max_resources == NULL) return ERROR; // erreur d'allocation de memoire.
    int tab[max], *count_res = tab; // the table of result
    // init
    for (int i = 0; i < max; ++i) {
        count_res[i] = 0;
    }
    while (header != NULL){

        count_res[*(header->ressources)] += header->count;
        header = header->next;
    }
    // assignation de bonne valeur de ressource
    for (int j = 0; j < max; ++j) {
        head->max_resources[j] = count_res[j];
        head->max_resources_count += count_res[j];
    }

    return NO_ERROR;
}

/**
 * Cette fonction parcours la chaîne et met à jour la liste des commandes
 * @param head la tête de la chaîne
 * @return un code d'erreur
 */

error_code evaluate_whole_chain(command_head *head) {

    // count ressource;
    count_ressources(head,head->command);

    return NO_ERROR;
}

// ---------------------------------------------------------------------------------------------------------------------
//                                              BANKER'S FUNCTIONS
// ---------------------------------------------------------------------------------------------------------------------

static banker_customer *first;
static pthread_mutex_t *register_mutex = NULL;
static pthread_mutex_t *available_mutex = NULL;
// Do not access directly!
//  use mutexes when changing or reading _available!
int *_available = NULL;
int* _max_ressource = NULL;

/**
 * Cette fonction initialise le tableau de ressource dont un client aura besoin
 * pour s'executer.
 * @param customer
 */
error_code init_need_ressource(banker_customer* customer){

    int max = conf->res_number;
    customer->current_resources = malloc(sizeof(int)* max); // allocation de current ressource
    if(customer->current_resources == NULL) return HAS_ERROR(-1);
    customer->need_ressources = malloc(sizeof(int)* max); // allocation de need ressource
    if(customer->need_ressources == NULL) return HAS_ERROR(-1);
    for (int i = 0; i < max; ++i) {
        customer->need_ressources[i] = customer->head->max_resources[i];
        customer->current_resources[i] =0;
    }
    return HAS_NO_ERROR(2);
}


/**
 * Cette fonction enregistre une chaîne de commande pour être exécutée
 * Elle retourne NULL si la chaîne de commande est déjà enregistrée ou
 * si une erreur se produit pendant l'exécution.
 * @param head la tête de la chaîne de commande
 * @return le pointeur vers le compte client retourné
 */

banker_customer *register_command(command_head *head) {

    // acquire the lock
    pthread_mutex_lock(register_mutex);
    // les sections critques
    // le client est il en regle?
    if(!check_conformity(head)){
        // client demande plus de ressource
        // que le banquier n'a pas exit alors.
        pthread_mutex_unlock(register_mutex);
        freeHeadCommand(head); // liberation de la memoire.
        return NULL;
    }
    if(first == NULL){

        first = malloc(sizeof(banker_customer));
        if(first == NULL) goto fin; // erreur de memoire
        first->head = head;
        first->next = NULL;
        first->prev =NULL;
        first->depth = -1;
       if(HAS_ERROR(init_need_ressource(first))) goto fin; // init need table
        nbCustomer++;

        pthread_mutex_unlock(register_mutex);
        return first;
    } else{
        banker_customer* theNew = malloc(sizeof(banker_customer));
        if(theNew == NULL ) goto fin; // memoire error
        theNew->head = head;
        theNew->depth = -1;
        theNew->next = NULL;
        if(HAS_ERROR(init_need_ressource(theNew))) goto fin;
        banker_customer* current = first;
        while (current->next != NULL) current = current->next;
        current->next = theNew;
        theNew->prev = current;
        nbCustomer++;
        pthread_mutex_unlock(register_mutex);
        return theNew;
    }
    fin:
    printf("erreur de memoire occured\n");
    pthread_mutex_unlock(register_mutex);
    //pthread_mutex_unlock(register_mutex);
    return NULL;
}

/**
 * Cette fonction enlève un client de la liste
 * de client de l'algorithme du banquier.
 * Elle libère les ressources associées au client.
 * @param customer le client à enlever
 * @return un code d'erreur
 */
error_code unregister_command(banker_customer *customer) {
    pthread_mutex_lock(register_mutex);
    int errCode = 0;
    /* base case */
    if (first == NULL || customer == NULL)
        goto exit;

    /* If cutomer to be deleted is the first coustoner */
    if (first == customer){
        first = customer->next;
    }

    /* Change next only if customer to be deleted is NOT the last customer */
    if (customer->next != NULL){
        customer->next->prev = customer->prev;
    }
    /* Change prev only if customer to be deleted is NOT the first customer */
    if (customer->prev != NULL){
        customer->prev->next = customer->next;
    }
    // take the ressources
    pthread_mutex_lock(available_mutex); // acquire the lock
    for (int i = 0; i < conf->res_number; ++i) {
        _available[i] += customer->current_resources[i];
        customer->current_resources[i] = 0;
    }
    pthread_mutex_unlock(available_mutex);// release
    // liberation de la memoire
    freeBankerCustomerStruct(customer);
    nbCustomer--; // decrease the number of customers

    pthread_mutex_unlock(register_mutex);
    return errCode;
    exit:
    pthread_mutex_unlock(register_mutex);
    return ERROR;

}


/**
 * Exécute l'algo du banquier sur work et finish.
 *
 * @param work
 * @param finish
 * @return
 */
int bankers(int *work, int *finish) {

    bool is_safe = true;

    banker_customer* list_one = NULL;

    // initialisation de finish
    for (int j = 0; j < nbCustomer; ++j) {
        finish[j] = false;
    }
    for (int i = 0; i < conf->res_number; ++i) {
        work[i] = _available[i];
    }
    banker_customer* client = first;
    list_one = first;
    // le nombre de ressources manquants pour les executions de chaque client
    int nombre_res = conf->res_number;
    for (int count = 0; count < nbCustomer; ++count) {
        int i =0;
        while (i < nbCustomer){
            // trouver un client qui peut faire son execution
            if(finish[i] == false){
                int j;
                for( j = 0;j < nombre_res;j++){
                    if(list_one->need_ressources[j] > work[j]){
                        break;
                    }
                }
                // client qui peut acceder au ressource et terminer
                if(j == nombre_res){
                    finish[i] = true;
                    for(j =0; j < nombre_res; j++){ // client a terminer
                        // on recupere les ressources liberer par le client
                        work[j] += list_one->current_resources[j];
                    }
                }
            }
            list_one = client->next;
            i++;
        }
    }
    // verifier si tous les clients ont executer pour determiner si c'est un etat sur ou pas sure.
    for (int k = 0; k < nbCustomer; ++k) { // a t-on trouver un client qui n'a pas eu access au ressource?
        if(finish[k] == false) is_safe = false; // si ou alors l'etat n'est pas sur.
    }

    return is_safe;
}


/**
 * Prépare l'algo. du banquier.
 *
 * Doit utiliser des mutex pour se synchroniser. Doit allour des structures en mémoire. Doit finalement faire "comme si"
 * la requête avait passé, pour défaire l'allocation au besoin...
 *
 * @param customer
 *
 */

void call_bankers(banker_customer *customer) {

    pthread_mutex_lock(available_mutex); // acquerir le mutex de available

    command * cmd = customer->head->command;
    int ind =0;
    while (ind != customer->depth) {
        cmd = cmd->next;
        ind++;
    }
    // reconnaitre le type de ressource demander.

    int res_type = resource_no(cmd->call[0]);
    int request = cmd->count;
    int cliend_need = customer->need_ressources[res_type];
    if (cliend_need < request) {
        pthread_mutex_unlock(available_mutex);
        return; // alors on a  une erreur.
    }
    if (request > _available[res_type]) { // si la requete demander ne pas etre
        // alors le client doit attendre.
    } else {
        //   printf("simulation de banker\n");
        _available[res_type] -= request; // avail - request[j]
        customer->current_resources[res_type] += request; // allocated[j] + request
        customer->need_ressources[res_type] -= request; // needed[j] - request
        int res_number = conf->res_number;
        int *work = malloc(sizeof(int) * res_number); // NULL???
        if(work == NULL) return; // alors quitter

        int *finish = malloc(sizeof(int) * nbCustomer); // NULL???
        if(finish == NULL) return;
        // appel de l'algorithme du banquier.
        int response = bankers(work, finish);

        if (response == true) { // alors on peut donner l'acces au ressource au client.
            customer->depth = -1;
            pthread_mutex_unlock(customer->head->mutex); // le client est deverouiller.
        } else { // remettre les modifcations car l'etat n'est pas safe et le client doit attendre.
            _available[res_type] += request; // avail - request[j]
            customer->current_resources[res_type] -= request; // allocated[j] + request
            customer->need_ressources[res_type] += request; // needed[j] - request
        }
        freeIntTable(work);
        freeIntTable(finish);
    }

    pthread_mutex_unlock(available_mutex); // relase the lock.
}
/**
 * Cette fonction effectue une requête des ressources au banquier. Utilisez les mutex de la bonne façon, pour ne pas
 * avoir de busy waiting...
 *
 * @param customer le ticket client
 * @param cmd_depth la profondeur de la commande a exécuter
 * @return un code d'erreur
 */
error_code request_resource(banker_customer *customer, int cmd_depth) {
    pthread_mutex_lock(customer->head->mutex); // lock the mutex for the first time,
    command* cmd = customer->head->command;
    for (int i = 0; i < cmd_depth; ++i) {
        cmd = cmd->next;
    }
    // assign the good depth
    customer->depth = cmd_depth;
    // lock for the second time till the banker unlock the first one .
    pthread_mutex_lock(customer->head->mutex);
    // we have granted by the banker
    return NO_ERROR;


}
/**
 *Cette fonction execute un seul block de commande
 * @param commande
 * @param index
 * @return le prochain block de commande a executer.
 */
command *appel_commande(command* commande, int *index){

    int k = *index;
    int succes = callCommand(commande);
    operator op = commande->op;
    commande  = commande->next;
    k++;

    switch (op) {
        default:
        case NONE:
        case BIDON:
            break;
        case AND:
            if (!succes) k++;
            break;
        case OR:
            if (succes) {
                commande= commande->next;
                k++;
                while (commande!= NULL && (commande->op == OR || commande->op == NONE)){
                    commande = commande->next;
                    k++;
                }
                if (commande != NULL && commande->op == AND){
                    commande= commande->next;
                    k++;
                }
            }
    }
    *index = k;
    return commande;
}
/**
 *
 * @param client
 * @return false si le client  demande plus de ressource que
 * les ressources dont le banquier dispose sinon true.
 */
bool check_conformity(command_head* client){

    bool is_conform = true;

    // savoir si le client demande plus de resource que le banquier a.
    for (int i = 0; i < conf->res_number; ++i) {
        if(client->max_resources[i] > _max_ressource[i]){
            is_conform = false;
            break;
        }
    }
    return is_conform;
}
void* client_thread(void* par) {

    command_head *cmds = (command_head *) par;

    banker_customer *customer = register_command(cmds); // register the commands;
    // client s'est il enregistrer avec success?
    if(customer == NULL){  // client non enregistrer
        pthread_exit(NULL); // alors quitter.
    }
    command *commande = customer->head->command;
    int k =0;
    for (; k < cmds->length;) {
        // request and execute ;
        request_resource(customer, k);
        int res_type = resource_no(commande->call[0]);
        int request = commande->count;
        // execution de la commande
        commande =  appel_commande(commande,&k);
        // liberer la ressource utiliser.
        pthread_mutex_lock(available_mutex);
        _available[res_type] += request; // avail - request[j]
        customer->current_resources[res_type] -= request; // allocated[j] + request
        pthread_mutex_unlock(available_mutex);
        pthread_mutex_unlock(customer->head->mutex); // release the resource mutex
    }
    // desincrire le client
    unregister_command(customer);
    // exit fin du thread.
    pthread_exit(NULL);
}
/**
 * Parcours la liste de clients en boucle. Lorsqu'on en trouve un ayant fait une requête, on l'évalue et recommence
 * l'exécution du client, au besoin
 *
 * @return
 */

void *banker_thread_run() {

    for(;;){
        pthread_mutex_lock(register_mutex); //acquire the register lock
        banker_customer* current_head = first;
        while (current_head != NULL){

            // un client qui a demande acces au(x) ressource(s).
            if(current_head->depth != -1){
                // printf("Nous avons eu un client[%s] avec depth %d\n ",current_head->head->command->call[0], current_head->depth);
                // appel de banker sur le client
                call_bankers(current_head);
            }
            current_head = current_head->next;
        }
        pthread_mutex_unlock(register_mutex); // release the register lock
    }
    return NULL;
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------

// constructor and destructor declaration
error_code constructor();
void destructor();
/**
 * Utilisez cette fonction pour initialiser votre shell
 * Cette fonction est appelée uniquement au début de l'exécution
 * des tests (et de votre programme).
 */
error_code init_shell() {

    error_code err =  constructor(); // appel du constructeur

    if(err) return err;
    char* line =  "blocker,echo&2,20&2&1&1&20";
    //readLine(&line);
    // parse the line;
    err =  parse_first_line(line);
    // allocation de la memoire .
    _available = malloc(sizeof(int)*conf->res_number);
    if(_available == NULL) return ERROR;
    _max_ressource = malloc(sizeof(int)*conf->res_number);
    if(_max_ressource == NULL) return ERROR;
    // initialisation du tableau global _available.
    for (int i = 0; i < conf->res_number; ++i) {
        _available[i] = resource_count(i);
        _max_ressource[i] = _available[i];
    }

    return err;
}
/**
 * Utilisez cette fonction pour nettoyer les ressources de votre
 * shell. Cette fonction est appelée uniquement à la fin des tests
 * et de votre programme.
 */
void close_shell() {
    freeWholeBankerStruct();
   destructor();
}

/**
 * Utilisez cette fonction pour y placer la boucle d'exécution (REPL)
 * de votre shell. Vous devez aussi y créer le thread banquier
 */
void run_shell() {

    // banker thread
    pthread_t banker;
    // run the banker
    pthread_create(&banker,NULL,banker_thread_run,NULL);
    char *line;
    int concurent_len = 1,current=0;
    pthread_t* sp_thread = malloc(sizeof(pthread_t)*1);
    int i =0;
    if(sp_thread == NULL) return;

    while (1) {
        command_head *head;
        //printf("User-> ");
        pthread_t customer_thread;

        if (HAS_ERROR(readLine(&line))) goto top;
        if (strlen(line) == 0) continue;

        // exit option
        if( strcmp("exit", line) == 0){
            free(line);
            free(sp_thread);
            return;
        }

        if (HAS_ERROR(create_command_chain(line, &head))) {
            printf("Erreur au niveau de create_command_chain\n");
            free(sp_thread);
            return;
        }
        free(line);
        // execution en paralelle.
        if (head->background) {
            // besoin d'allocation de memoire?
            if(current >= concurent_len){
                concurent_len +=1;
                sp_thread = realloc(sp_thread, sizeof(pthread_t)*concurent_len);
                if(sp_thread == NULL) return; // erreur de memoire?
            }
            // lancer l'exection
            pthread_create(&sp_thread[current++],NULL,client_thread,head);
        } else { // execution sequentielle
            // create du thread
            pthread_create(&customer_thread, NULL, client_thread, head);
            // attendre jusqu'a ce le thread fini.
            pthread_join(customer_thread, NULL);

        }
        i++;
    }
    top:
    printf("An error has occured");

}


/**
 * la definition de la fonction constructor
 * elle alloue la memoire pour les variables
 * globales du programme.
 * @return
 */
error_code constructor(){

    // allocation de memoire.
    conf = malloc(sizeof(configuration));
    if(conf == NULL) ERROR;
    register_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    if(register_mutex == NULL) return ERROR;
    available_mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    if(available_mutex == NULL) return ERROR;

    // initialisation des mutex
    pthread_mutex_init(register_mutex,NULL);
    pthread_mutex_init(available_mutex,NULL);

    return 0;
}
void freeIntTable(int* table){

    free(table);
}
/**
 * Libere la memoire utiliser par une instance d'un objet
 * de la structure banker_customer;
 * @param head
 */
void freeBankerCustomerStruct(banker_customer* header){
    if(header == NULL) return;
    freeIntTable(header->need_ressources);
    freeIntTable(header->current_resources);
    freeHeadCommand(header->head);
    free(header);
}
/**
 * Libere la memoire utiliser par la variable
 * globale first de type banker_coustomer.
 */
void freeWholeBankerStruct(){

    // utilisation du mutex car si on quitte brusquement alors
    // peut etre que des threads sont deja en cours d'execution

    pthread_mutex_lock(register_mutex);
    while (first != NULL){
        banker_customer* n = first;
        first = first->next;
        freeBankerCustomerStruct(n);
    }
    pthread_mutex_unlock(register_mutex);
}


/**
 * La fonction destructor dessaloue l'espace de memoire.
 */
void destructor(){

    freeStringArray(conf->commands); // free the memory used by all the array commands
    freeIntTable(conf->command_caps);
    freeIntTable(conf->all_resources_qty);
    free(conf);
    freeIntTable(_available);
    freeIntTable(_max_ressource);
    // destruction of the mutex
    pthread_mutex_destroy(available_mutex);
    pthread_mutex_destroy(register_mutex);
}


/**
 * Vous ne devez pas modifier le main!
 * Il contient la structure que vous devez utiliser. Lors des tests,
 * le main sera complètement enlevé!
 */

int main(void) {
    if (HAS_NO_ERROR(init_shell())) {
        run_shell();
        close_shell();
    } else {
        printf("Error while executing the shell.");
    }

    return 0;
}
