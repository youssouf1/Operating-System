#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "conf.h"
#include "common.h"
#include "vmm.h"
#include "tlb.h"
#include "pt.h"
#include "pm.h"

static unsigned int read_count = 0;
static unsigned int write_count = 0;
static FILE* vmm_log;
static int index =0;


void vmm_init (FILE *log)
{
    // Initialise le fichier de journal.
    vmm_log = log;
}


// NE PAS MODIFIER CETTE FONCTION
static void vmm_log_command (FILE *out, const char *command,
                             unsigned int laddress, /* Logical address. */
                             unsigned int page,
                             unsigned int frame,
                             unsigned int offset,
                             unsigned int paddress, /* Physical address.  */
                             char c) /* Caractère lu ou écrit.  */
{
    if (out)
        fprintf (out, "%s[%c]@%05d: p=%d, o=%d, f=%d pa=%d\n", command, c, laddress,
                 page, offset, frame, paddress);
}

// Fonction ajoutee

/**
 *
 * @param page
 * @return
 * cette fonction choisi un frame libre pour assigner a une page
 * ou cas ou les frames sont tous assignes, cette procedure
 * choisie une frame victime pour l'assigner a la
 * nouvelle page.
 */
unsigned int replacePage(unsigned int page){

    unsigned int victime = pt__lookdown();
    if(pt_full()){

        pm_backup_page(victime ,getPages(victime));
        pt_unset_entry(victime);

    }
    pm_download_page(page,(unsigned int)victime);
    pt_set_entry(page,(unsigned int)victime);
    tlb_add_entry(page,(unsigned int)victime,false);
    return victime;

}

/* Effectue une lecture à l'adresse logique `laddress`.  */
char vmm_read (unsigned int laddress){
    char c = '!';
    read_count++;
    unsigned int page = laddress >> 8;
    unsigned int offset = laddress & 255;

    int frame = tlb_lookup(page, false);

    if(frame < 0){ // frame non trouver dans le tlb
        frame = pt_lookup(page);
        if (frame < 0)
        frame = replacePage(page); // trouver une frame victime et remplacer!
    }

    unsigned int paddress = (((unsigned int) frame) << 8) + offset;
    c = pm_read(paddress);
    vmm_log_command (stdout, "READING",  laddress, page, frame, offset, paddress, c);
    return c;



}

/* Effectue une écriture à l'adresse logique `laddress`.  */
void vmm_write (unsigned int laddress, char c)
{
    write_count++;

    unsigned int page = laddress>>8;
    unsigned int offset = laddress&255;

    int frame = -1;


    frame = tlb_lookup(page,true);
    if(frame == -1){
        frame = pt_lookup(page);
        if (frame < 0)
        frame = replacePage(page);
    }

    unsigned int paddress=(((unsigned int)frame)<<8) + offset;
    pm_write(paddress,c);

    vmm_log_command (stdout, "WRITING", laddress, page, frame, offset, paddress, c);
}


// NE PAS MODIFIER CETTE FONCTION
void vmm_clean (void)
{
    fprintf (stdout, "VM reads : %4u\n", read_count);
    fprintf (stdout, "VM writes: %4u\n", write_count);
}
