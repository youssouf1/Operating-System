
#include <stdint.h>
#include <stdio.h>

#include "pt.h"

#include "conf.h"

struct page
{
    bool readonly : 1;
    bool valid : 1;
    unsigned int frame_number : 16;
};

static FILE *pt_log = NULL;
static struct page page_table[NUM_PAGES];
static int frame_table[NUM_FRAMES];

static unsigned int pt_lookup_count = 0;
static unsigned int pt_page_fault_count = 0;
static unsigned int pt_set_count = 0;
static unsigned int pointer = 0;

/* Initialise la table des pages, et indique où envoyer le log des accès.  */
void pt_init (FILE *log)
{
    for ( int i=0; i < NUM_PAGES; i++){
        page_table[i].valid = false;
        if( i < NUM_FRAMES){
            frame_table[i] = false;
        }
    }
    pt_log = log;
}

/******************** ¡ NE RIEN CHANGER CI-DESSUS !  ******************/

/* Recherche dans la table des pages.
 * Renvoie le `frame_number`, si valide, ou un nombre négatif sinon.  */

static int pt__lookup (unsigned int page_number)
{

    if(page_table[page_number].valid){

        int frame = page_table[page_number].frame_number;
        frame_table[frame] = true;
        return frame;
    }

    return -1;
}
int pt_full(){
    int count = 0;
    for(int i =0; i < NUM_PAGES; i++){
        if(page_table[i].valid) count++;
    }
    return count == NUM_FRAMES;
}
int getPages(unsigned int frame){

    for(int i =0; i < NUM_PAGES; i++) {
        if (page_table[i].frame_number == frame && page_table[i].valid)
            return i;
    }
        return -1;

}
/**
 * La fonction qui choisie une frame victime pour une nouvelle page
 * non existante dans le page table.
 *
 * Cet fonction utilise l'algorithme second change(horloge)
 * @return
 */
int pt__lookdown(){

    int victime; // numero de frame victime
    while(true){ // tant que nous avons pas eu de victime

        if(!frame_table[pointer]){ // si le pointeur pointe vers le '0' alors
            victime = pointer;  // on choisi comme victime
            frame_table[pointer] = true; // pour la nouvelle entree on donne une second change
            pointer = (pointer +1) % NUM_FRAMES;// on avance le pointeur d'un pas.
            return victime;
        }
        // sinon on donne une seconde change a la frame courante
        frame_table[pointer] = false; // pour la prochaine fois elle sera l'eventuel victime.
        pointer = (pointer +1) % NUM_FRAMES; // on avance d'un pas.
    }
}




/* Modifie l'entrée de `page_number` dans la page table pour qu'elle
 * pointe vers `frame_number`.  */
static void pt__set_entry (unsigned int page_number, unsigned int frame_number)
{
    page_table[page_number].frame_number = frame_number;
    page_table[page_number].valid = true;
}

/* Marque l'entrée de `page_number` dans la page table comme invalide.  */
void pt_unset_entry (unsigned int page_number)
{
    // TODO: COMPLÉTER CETTE FONCTION.
    page_table[page_number].valid = false;
}

/* Renvoie si `page_number` est `readonly`.  */
bool pt_readonly_p (unsigned int page_number)
{
    return (page_table[page_number].readonly)? true: false;
}

/* Change l'accès en écriture de `page_number` selon `readonly`.  */
void pt_set_readonly (unsigned int page_number, bool readonly)
{

    if(page_table[page_number].readonly == readonly){}
    else page_table[page_number].readonly = readonly;
}




/******************** ¡ NE RIEN CHANGER CI-DESSOUS !  ******************/

void pt_set_entry (unsigned int page_number, unsigned int frame_number)
{
    pt_set_count++;
    pt__set_entry (page_number, frame_number);
}

int pt_lookup (unsigned int page_number)
{
    pt_lookup_count++;
    int fn = pt__lookup (page_number);
    if (fn < 0) pt_page_fault_count++;
    return fn;
}

/* Imprime un sommaires des accès.  */
void pt_clean (void)
{
    fprintf (stdout, "PT lookups   : %3u\n", pt_lookup_count);
    fprintf (stdout, "PT changes   : %3u\n", pt_set_count);
    fprintf (stdout, "Page Faults  : %3u\n", pt_page_fault_count);

    if (pt_log)
    {
        for (unsigned int i = 0; i < NUM_PAGES; i++)
        {
            fprintf (pt_log,
                     "%d -> {%d,%s%s}\n",
                     i,
                     page_table[i].frame_number,
                     page_table[i].valid ? "" : " invalid",
                     page_table[i].readonly ? " readonly" : "");
        }
    }
}
