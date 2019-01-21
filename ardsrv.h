
#ifndef ARDSRV_H
#define ARDSRV_H 1

#include <gtk/gtk.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <aio.h>

#include "bblar.h"
#include "bbllg.h"

#define BFF 1023
#define gboolean int
#define gpointer void *
#define gchar char
//#define FALSE 0
//#define TRUE !FALSE

//#define NMR 8																											// numero sonde temperature
//#define TMP 9																											// temperature mesuree (entiere)
//#define FIN 11																										// dernier pour creation
//#define NMBSRT 16																										// nombre (taille du tableau = nombre de sortie)
//#define SND 1																											// sonde de temperature

#define CLOCKID CLOCK_MONOTONIC
#define SIG SIGUSR1

#ifndef ACK
#define ACK			6
#endif
#ifndef NACK
#define NACK		0x15
#endif

#ifndef INDEX
#define INDEX		0
#endif
#ifndef SORTIE
#define SORTIE		1
#endif
#ifndef ETAT
#define ETAT		2
#endif
#ifndef BROCHE
#define BROCHE		3
#endif
#ifndef SMINI
#define SMINI		4
#endif
#ifndef SMAXI
#define SMAXI		5
#endif
#ifndef ENTREE
#define ENTREE		6
#endif
#ifndef CAPTEUR
#define CAPTEUR		7
#endif
#ifndef TYPE
#define TYPE		8
#endif
#ifndef NUMERO
#define NUMERO		9
#endif
#ifndef MMINI
#define MMINI		10
#endif
#ifndef MMAXI
#define MMAXI		11
#endif
#ifndef FONCTION
#define FONCTION	12
#endif
#ifndef MAJ
#define MAJ			13
#endif
#ifndef END
#define END			14
#endif

// nom du fichier config
#ifndef NOMCNF
#define NOMCNF "/var/local/cnf/ardsrv"
#endif

#endif

