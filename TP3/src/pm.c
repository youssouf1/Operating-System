#include <stdio.h>
#include <string.h>


#include "conf.h"
#include "pm.h"

static FILE *pm_backing_store;
static FILE *pm_log;
static char pm_memory[PHYSICAL_MEMORY_SIZE];
static unsigned int download_count = 0;
static unsigned int backup_count = 0;
static unsigned int read_count = 0;
static unsigned int write_count = 0;

// Initialise la mémoire physique
void pm_init (FILE *backing_store, FILE *log)
{
    pm_backing_store = backing_store;
    pm_log = log;
    memset (pm_memory, '\0', sizeof (pm_memory));
}

// Charge la page demandée du backing store
void pm_download_page (unsigned int page_number, unsigned int frame_number)
{
    download_count++;
    char frame_buffer[256];
    fseek(pm_backing_store, (page_number*256), SEEK_SET);
    fread(frame_buffer, 256, 1, pm_backing_store);
    rewind(pm_backing_store);

    for(int offset=0;offset<256;++offset)
    {
        pm_memory[offset+frame_number*256]=frame_buffer[offset];
    }
}

// Sauvegarde la frame spécifiée dans la page du backing store
void pm_backup_page (unsigned int frame_number, unsigned int page_number)
{
    backup_count++;
    char frame_buffer[256];

    for(int offset=0;offset<256;++offset)
    {
        frame_buffer[offset]=pm_memory[offset+frame_number*256];
    }

    fseek(pm_backing_store, (page_number*256), SEEK_SET);

    fwrite(frame_buffer, 1, 256, pm_backing_store);

    rewind(pm_backing_store);
}

char pm_read (unsigned int physical_address)
{
    read_count++;
    return pm_memory[physical_address];

}

void pm_write (unsigned int physical_address, char c)
{
    write_count++;
    pm_memory[physical_address]=c;
}


void pm_clean (void)
{
    // Enregistre l'état de la mémoire physique.
    if (pm_log)
    {
        for (unsigned int i = 0; i < PHYSICAL_MEMORY_SIZE; i++)
        {
            if (i % 80 == 0)
                fprintf (pm_log, "%c\n", pm_memory[i]);
            else
                fprintf (pm_log, "%c", pm_memory[i]);
        }
    }
    fprintf (stdout, "Page downloads: %2u\n", download_count);
    fprintf (stdout, "Page backups  : %2u\n", backup_count);
    fprintf (stdout, "PM reads : %4u\n", read_count);
    fprintf (stdout, "PM writes: %4u\n", write_count);
}
