
#include "ardsrv.h"

// Position des capteurs
char entree[13][9] = 
{
	"NO", "Piece", "Boitier", "Radiat.1", "Radiat.2", "Reserv.1",
	"Reserv.2", "Pompe", "B.Marche", "Alim.", "C.Graph.", "C.P.U", ""
};
// Type de capteur
char capteur[26][14] = 
{
	"NO", "Luminosite", "Temperature", "Pression", "Interrupteur", "Tachymetre", "Humidite", "Mouvement", "Magnetisme",
	"Gyroscope", "Accelerometre", "InfraRouge", "MicroPhone", "UltraSon", "Tension",
	"Courant", "Puissance", "Dephasage", "Frequence", "Periode", "Rap.cyclique", "Resistance",
	"Impedance", "Capacite", "Distance", ""
};
// Type des capteurs
char dgtanl[25] =
{
	'A', 'N', 'A', 'A', 'A', 'N', 'A', 'A', 'A', 'A', 'N', 'A',
	'N', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 0
};
// Unite de mesure des capteurs
char unite[25][3] =
{
	"°C", "%", "B", " ", "G", " ", "g", "Lm", " ", " ", " ", "V",
	"A", "W", " ", "Hz", "S", "%", "O", "H", "F", " ", "M", " ", ""
};
// Organe de sortie
char sortie[17][17] = 
{
	"NO", "LED.Boitier", "Ventilo.Boitier", "Ventilo.Radiat.1", "Ventilo.Radiat.2", "Peltier.Reserv.1",
	"Peltier.Reserv.2", "LED.Bleue", "LED.Verte", "LED.Rouge", "LED.Erreur", 
	"Pompe.1", "Pompe.2", "Ventilo.4", "Ventilo.5", "Ventilo.6", ""
/*
	"Climatisation", "Chauffage", "Ventilateur", "LED.Rouge", "LED.Verte", "LED.Bleue", 
	"Peltier", "Pompe", "Relais", "Transistor", "Moteur", "Servo", ""
*/
};

// table valeur/parametre (voir bblar.h)
//unsigned char srt[PRM+1][SRT+1];
char eeprom_srt[] = 
{
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	3, 5, 6, 9, 10, 11, 14, 15, 16, 4, 8, 
	3, 5, 5, 5, 5, 5, 4, 4, 4, 2, 0,
	'H', '2', '0', '3', '1', '4', '2', '2', '2', 'G', 'h',
	0x32, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x19, 0x19, 0x23, 0x50, 0,
	0xC8, 0x32, 0x32, 0x32, 0x32, 0x32, 0x19, 0x23, 0x23, 0x50, 0xFF,
	0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0xFF, 0xFF, 0, 0xFF, 0,
	0xC8, 0xC8, 0xC8, 0xC8, 0xC8, 0xC8, 0, 0xFF, 0xFF, 0, 0xFF,
};

char *bff;
int  sfd, gi_fin, inx;
char gr_file[2][32];				// open port serie (1) ou fifo (2)
int  go_srl[2];						// open port serie (1) ou fifo (2)
char gr_rcp[LNGRCV+6];
int  gi_rcp;
char gr_rcv[LNGRCV+1];
char gr_get[256];					// memorisation du get
int  port;							// port du site
char racine[120];					// dossier racine du site
FILE *go;

int  web(socklen_t, int);
int  save(char *);
int  gener(char *);
void genere_mesure(int j);
void genere_sortie_dgt(int j);
void genere_sortie_pwm(int j);
void genere_sortie_autre(int j);
int  tstnmb(char *);
int  tstrcn(char *);
int  prment(int, char **);
void cbk_ctrlc(int sig);
void fv_majprm(char *pr_nom, char *pr_vlr);

//
// gros filtre sur le nom de url demandee
// autorise: s*->slntr.html w*->wlstr.html l*->lcntr.html t*->tnstr.html c*->cnfgr.html
// tout le reste --> index.html
//

int web(socklen_t lng, int fd)
{
	int  li, lu, j, k, l, err, maj, v;
	char *p, *d, *f, nom[8], vlr[8], nmr[11]="0123456789";
	char url[8];
	
	if ((lu = read(fd, bff, BFF)) == -1)
	{
		lg_strcpy(lg.fnc, "web");
		lg.grv= LG_INF;
		lg_strcpy(lg.mss, "read");
		lg_strcpy(lg.err, "Erreur");
		lg_ecr();
		return(1);
	}
	bff[lu]=0;
//	printf("recu=%d<%s>\n", lu, bff);
	for (j=0,p=bff; strlen(p)>0; j++)
	{
		sprintf(url, "recu%d", j);
		lg_strcpy(lg.fnc, url);
		lg.grv= LG_INF;
		sprintf(url, "p=%ld", strlen(p));
		lg_strcpy(lg.mss, url);
		lg_strcpy(lg.err, p);
		lg_ecr();
		k = strlen(p);
		if (k > 127)
			p += 127;
		else
			p += k;
	}

	if ((strncasecmp(bff,"get ", 4)) && (strncasecmp(bff,"post ", 5)))
	{
		//printf("err get\n");
		lg_strcpy(lg.fnc, "web");
		lg.grv= LG_INF;
		lg_strcpy(lg.mss, "demande non traitee");
		lg_strcpy(lg.err, "!= get");
		lg_ecr();
		return(1);
	}
	if ((*bff == 'g') || (*bff == 'G'))
		p = bff + 4;
	else
		p = bff + 5;

	if (*p != '/')
	{
		//printf("err /\n");
		lg_strcpy(lg.fnc, "web");
		lg.grv= LG_INF;
		lg_strcpy(lg.mss, "chemin demande invalide");
		lg_strcpy(lg.err, "!= /");
		lg_ecr();
		return(1);
	}
	p++;
	
	d = p;
	while ((*d != ' ') && (*d != 0))																					// suppression apres blanc
		d++;
	*d = 0;
	strncpy(gr_get, p, 255);
	gr_get[255] = 0;

	if (strncmp(p, "index.html", 10) == 0)
	{
		lg_strcpy(lg.fnc, "web");
		lg.grv= LG_INF;
		lg_strcpy(lg.mss, "recu");
		lg_strcpy(lg.err, "index.html");
		lg_ecr();

		d = p + 11;																										// apres le 0
//POST /index.html HTTP/1.1\r\n...Content-Length: ...\r\naaa=50&aab=200&...&aka=OFF&akb=ON
		if ((*bff == 'p') || (*bff == 'P') && ((p = strstr(d, "Content-Length: ")) != NULL))							// post
		{
			d = p + 16;
			if ((p = strstr(d, "\r\n\r\n")) != NULL)																	// \r\n
			{
				d = p + 4;																								// debut variable
				while (p = strchr(d, '='))
				{
					strncpy(nom, d, p-d);
					nom[p-d] = 0;
					
					d = p + 1;
					if (p = strchr(d, '&'))
					{
						strncpy(vlr, d, p-d);
						vlr[p-d] = 0;
					}
					else
					{
						strcpy(vlr, d);
						sprintf(lg.err, "<%s>=<%s>", nom, vlr);
						lg_ecr();
						
						fv_majprm(nom, vlr);
						break;
					}
					
					d = p + 1;
					sprintf(lg.err, "<%s>=<%s>", nom, vlr);
					lg_ecr();
					
					fv_majprm(nom, vlr);
				}
			}
		}
	}
	
	strncpy(url, gr_get, 5);
	url[5] = 0;
	
	li = -1;
	switch (url[0])
	{
		// ?s= parametre entree seuil temp. mini maxi
		case 's':																										// seuil_entree.html
			if (!save("slntr"))																							// sauve seuil_entree.html
				gener("slntr");																							// genere l'html
			li = open("slntr.html", O_RDONLY);																			// envoi seuil_entree.html
			break;
		// ?w= parametre sortie seuil p.w.m mini maxi
		case 'w':																										// seuil_sortie.html
			if (!save("wlsrt"))																							// sauve seuil_sortie.html
				gener("wlsrt");																							// genere l'html
			li = open("wlsrt.html", O_RDONLY);																			// envoi seuil_sortie.html
			break;
		// ?l= selection parametre entree
		case 'l':																										// seuil_entree.html
			if (!save("lcntr"))																							// sauve seuil_entree.html
				gener("lcntr");																							// genere l'html
			li = open("lcntr.html", O_RDONLY);																			// envoi seuil_entree.html
			break;
		// ?l= selection parametre sortie
		case 't':																										// seuil_entree.html
			if (!save("tnsrt"))																							// sauve seuil_entree.html
				gener("tnsrt");																							// genere l'html
			li = open("tnsrt.html", O_RDONLY);																			// envoi seuil_entree.html
			break;
		// ?c= configuration
		//case 'r':	// configuration capteur selection (temp. Inter. Relais ...)
		//case 'n':	// configuration entree  selection (piece boitier ratiateur ...)
		//case 'e':	// configuration sortie  etat (no off digital pwm)
		case 'c':																										// configuration.html
			if (!save("cnfgr"))																							// sauve configuration.html
				gener("cnfgr");																							// genere l'html
			li = open("cnfgr.html", O_RDONLY);																			// envoi configuration.html
			break;
		default:																										// index.html
			//printf("url=%s\n", url);
			if (!save("index"))																							// sauve index.html
				gener("index");																							// genere l'html
			li = open("index.html", O_RDONLY);																			// envoi index.html
			break;
	}

	strcpy(bff, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n");													// repond ok
	write(fd, bff, strlen(bff));

	while ((lu = read(li, bff, BFF)) > 0)																				// envoi le html demande
		if (write(fd, bff, lu) != lu)
			perror("write");

	usleep(5000);																										// to allow socket to drain

	close(li);
	return(0);
}

int save(char *htm)
{
	int li, lo, lu;
	char fch[16];
	
	strcpy(fch, htm);
	strcat(fch, ".html");
	if ((li = open(fch, O_RDONLY)) == -1)
	{
		perror(fch);
		return(1);
	}
	strcpy(fch, htm);
	strcat(fch, ".tmp");
	if ((lo = open(fch, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) == -1)
	{
		perror(fch);
		close(li);
		return(1);
	}
	
	while ((lu = read(li, bff, BFF)) > 0)
		if (write(lo, bff, lu) != lu)
			perror("write");
			
	close(lo);
	close(li);
	
	return(0);
}

int gener(char *htm)
{
	FILE *li;
	char *d, *f, *p, txt[120], fch[16];
	int  td, j, k, lu, pwm;
	
	strcpy(fch, htm);
	strcat(fch, ".tmp");
	if ((li = fopen(fch, "r")) == NULL)
	{
		//perror("fopen");
		lg_strcpy(lg.fnc, "gener");
		lg.grv= LG_ERR;
		lg_strcpy(lg.mss, "fopen");
		lg_strcpy(lg.err, "lecture");
		lg_ecr();
		return(1);
	}
	strcpy(fch, htm);
	strcat(fch, ".html");
	if ((go = fopen(fch, "w")) == NULL)
	{
		//perror("fopen");
		lg_strcpy(lg.fnc, "gener");
		lg.grv= LG_ERR;
		lg_strcpy(lg.mss, "fopen");
		lg_strcpy(lg.err, "ecriture");
		lg_ecr();
		fclose(li);
		return(1);
	}

	j = -1;
	while (fgets(bff, BFF, li))																							// garde le debut
	{
		if (f = strchr(bff, '\n'))
			*f = 0;
		if (f = strchr(bff, '\r'))
			*f = 0;
		strcat(bff, "\n");
		
		if (j == -1)
		{
			if ((strstr(bff, "<TH>")) && (strstr(bff, "</TH>")))														// cherche entete tableau
				j++;

			fputs(bff, go);
			continue;
		}
		else if (j == 0)
		{
			fputs(bff, go);
			
			if (strstr(bff, "</TR>"))																					// fin ligne entete tableau
				break;
		}
	}
	
//printf("web ar->srt[0][$%x]=%s\n", ar->srt[0][NEN], capteur[ar->srt[0][NEN]]);
	if (!strcmp(htm, "index"))																							// index.html
	{
		for (j=0; j<SRT; j++)																							// le reste est genere dynamiquement
		{
			if (ar->srt[j][OTP] == OTP_NO)																				// sortie desactivee
				continue;
			
			fputs("\t\t\t<TR>\n\t\t\t\t<TD>", go);
			fputs(capteur[ar->srt[j][NCP]], go);
			fputs("</TD>\n\t\t\t\t<TD>", go);
			fputs(entree[ar->srt[j][NEN]], go);
			fputs("</TD>\n\t\t\t\t<TD>", go);
			fputs(sortie[ar->srt[j][NSR]], go);
			fputs("</TD>\n\t\t\t\t<TD id=\"", go);
			genere_mesure(j);
			fputs("</TD>\n\t\t\t\t<TD id=\"", go);
			if (ar->srt[j][OTP] == OTP_DGT)
				genere_sortie_dgt(j);
			else if (ar->srt[j][OTP] == OTP_PWM)
				genere_sortie_pwm(j);
			else
				genere_sortie_autre(j);
			fputs("</TD>\n\t\t\t</TR>\n", go);
		}
		
		fputs("\t\t</TABLE>\n\t\t</p>", go);
	}
	else if (!strcmp(htm, "slntr"))																						// seuil entree = slntr.html
	{																													// le reste est genere dynamiquement
		for (j=0; j<SRT; j++)
		{
			if (ar->srt[j][OTP] == OTP_NO)																			// sortie desactivee
				continue;
			
			fputs("\t\t\t\t<TR>\n\t\t\t\t\t<TD>", go);
			fputs(entree[ar->srt[j][NEN]], go);																			// entree
			fputs("</TD>\n\t\t\t\t\t<TD id=\"", go);																	// Lu
			genere_mesure(j);
			
			fputs("</TD>\n\t\t\t\t\t<TD>", go);
			if (ar->srt[j][ITP] == ITP_DGT)															// en digital
			{
				//strcpy(txt, "\n\t\t\t\t\t\t<select name=\"aaa\" id=\"aaa\">");
				sprintf(txt, "\n\t\t\t\t\t\t<select name=\"a%ca\" id=\"a%ca\">", 'a'+j, 'a'+j);
				fputs(txt, go);
				if (ar->srt[j][IMN])
				{
					strcpy(txt, "\n\t\t\t\t\t\t\t<option value=\"ON\" selected>ON</option>");
					fputs(txt, go);
					strcpy(txt, "\n\t\t\t\t\t\t\t<option value=\"OFF\">OFF</option>");
					fputs(txt, go);
				}
				else
				{
					strcpy(txt, "\n\t\t\t\t\t\t\t<option value=\"OFF\" selected>OFF</option>");
					fputs(txt, go);
					strcpy(txt, "\n\t\t\t\t\t\t\t<option value=\"ON\">ON</option>");
					fputs(txt, go);
				}
				strcpy(txt, "\n\t\t\t\t\t\t</select>");
				fputs(txt, go);
			}
			else if (ar->srt[j][ITP] == ITP_OWR)														// temperature ajoute °C
			{
				sprintf(txt, "\n\t\t\t\t\t\t<input type=\"number\" value=\"%d\" min=-20 max=100 step=5 name=\"a%ca\" id=\"a%ca\"/>", ar->srt[j][IMN], 'a'+j, 'a'+j);
				fputs(txt, go);
				sprintf(txt, "<label for=\"a%ca\">&deg;C</label>", 'a'+j);
				fputs(txt, go);
			}
			else																										// analogique
			{
				sprintf(txt, "\n\t\t\t\t\t\t<input type=\"number\" value=\"%d\" min=0 max=255 step=1 name=\"a%ca\" id=\"a%ca\"/>", ar->srt[j][IMN], 'a'+j, 'a'+j);
				fputs(txt, go);
				sprintf(txt, "<label for=\"a%ca\">0-255</label>", 'a'+j);
				fputs(txt, go);
			}
			fputs("\n\t\t\t\t\t</TD>\n\t\t\t\t\t<TD>", go);
			if (ar->srt[j][ITP] == ITP_DGT)															// en digital
			{
				//strcpy(txt, "\n\t\t\t\t\t\t<select name=\"aab\" id=\"aab\">");
				sprintf(txt, "\n\t\t\t\t\t\t<select name=\"a%cb\" id=\"a%cb\">", 'a'+j, 'a'+j);
				fputs(txt, go);
				if (ar->srt[j][IMX])
				{
					strcpy(txt, "\n\t\t\t\t\t\t\t<option value=\"ON\" selected>ON</option>");
					fputs(txt, go);
					strcpy(txt, "\n\t\t\t\t\t\t\t<option value=\"OFF\">OFF</option>");
					fputs(txt, go);
				}
				else
				{
					strcpy(txt, "\n\t\t\t\t\t\t\t<option value=\"OFF\" selected>OFF</option>");
					fputs(txt, go);
					strcpy(txt, "\n\t\t\t\t\t\t\t<option value=\"ON\">ON</option>");
					fputs(txt, go);
				}
				strcpy(txt, "\n\t\t\t\t\t\t</select>");
				fputs(txt, go);
			}
			else if (ar->srt[j][ITP] == ITP_OWR)														// temperature ajoute °C
			{
				sprintf(txt, "\n\t\t\t\t\t\t<input type=\"number\" value=\"%d\" min=-20 max=100 step=5 name=\"a%cb\" id=\"a%cb\"/>", ar->srt[j][IMX], 'a'+j, 'a'+j);
				fputs(txt, go);
				sprintf(txt, "<label for=\"a%cb\">&deg;C</label>", 'a'+j);
				fputs(txt, go);
			}
			else																										// analogique
			{
				sprintf(txt, "\n\t\t\t\t\t\t<input type=\"number\" value=\"%d\" min=0 max=255 step=1 name=\"a%cb\" id=\"a%cb\"/>", ar->srt[j][IMX], 'a'+j, 'a'+j);
				fputs(txt, go);
				sprintf(txt, "<label for=\"a%cb\">0-255</label>", 'a'+j);
				fputs(txt, go);
			}
			fputs("\n\t\t\t\t\t</TD>\n\t\t\t\t</TR>\n", go);
		}
		
		fputs("\t\t\t</TABLE>\n\t\t\t</p>\n\t\t</form>", go);
	}
	else if (!strcmp(htm, "wlsrt"))																						// seuil sortie = wlsrt.html
	{																													// le reste est genere dynamiquement
		for (j=0; j<SRT; j++)
		{
			if (ar->srt[j][OTP] == OTP_NO)																			// sortie desactivee
				continue;
			
			fputs("\t\t\t<TR>\n\t\t\t\t<TD>", go);
			fputs(sortie[ar->srt[j][NSR]], go);																				// sortie
			fputs("</TD>\n\t\t\t\t<TD id=\"", go);
			if (ar->srt[j][OTP] == OTP_DGT)
				genere_sortie_dgt(j);
			else if (ar->srt[j][OTP] == OTP_PWM)
				genere_sortie_pwm(j);
			else
				genere_sortie_autre(j);
			fputs("</TD>\n\t\t\t\t<TD>", go);

			if (ar->srt[j][OTP] == OTP_PWM)																				// PWM
			{
				sprintf(txt, "\n\t\t\t\t\t<input type=\"number\" value=\"%d\" min=0 max=255 step=1 name=\"b%ca\" id=\"b%ca\"/>", ar->srt[j][OMN], 'a'+j, 'a'+j);
				fputs(txt, go);
				sprintf(txt, "<label for=\"b%ca\">0-255</label>", 'a'+j);
				fputs(txt, go);
				//fputs("%", go);
			}
			else if (ar->srt[j][OTP] == OTP_DGT)																			// DGT
			{
				sprintf(txt, "\n\t\t\t\t\t<select name=\"b%ca\" id=\"b%ca\">", 'a'+j, 'a'+j);
				fputs(txt, go);
				if (ar->srt[j][OMN])
				{
					strcpy(txt, "\n\t\t\t\t\t\t<option value=\"ON\" selected>ON</option>");
					fputs(txt, go);
					strcpy(txt, "\n\t\t\t\t\t\t<option value=\"OFF\">OFF</option>");
					fputs(txt, go);
				}
				else
				{
					strcpy(txt, "\n\t\t\t\t\t\t<option value=\"OFF\" selected>OFF</option>");
					fputs(txt, go);
					strcpy(txt, "\n\t\t\t\t\t\t<option value=\"ON\">ON</option>");
					fputs(txt, go);
				}
				strcpy(txt, "\n\t\t\t\t\t</select>");
				fputs(txt, go);
			}
			else if (ar->srt[j][OTP] == OTP_OFF)																			// OFF
				fputs("OFF", go);
			else if (ar->srt[j][OTP] == OTP_ON)																		// ON
				fputs("OFF", go);
			fputs("\n\t\t\t\t</TD>\n\t\t\t\t<TD>", go);
			if (ar->srt[j][OTP] == OTP_PWM)																				// PWM
			{
				sprintf(txt, "\n\t\t\t\t\t<input type=\"number\" value=\"%d\" min=0 max=255 step=1 name=\"b%cb\" id=\"b%cb\"/>", ar->srt[j][OMX], 'a'+j, 'a'+j);
				fputs(txt, go);
				sprintf(txt, "<label for=\"b%cb\">0-255</label>", 'a'+j);
				fputs(txt, go);
				//fputs("%", go);
			}
			else if (ar->srt[j][OTP] == OTP_DGT)																			// DGT
			{
				sprintf(txt, "\n\t\t\t\t\t<select name=\"b%cb\" id=\"b%cb\">", 'a'+j, 'a'+j);
				fputs(txt, go);
				if (ar->srt[j][OMX])
				{
					strcpy(txt, "\n\t\t\t\t\t\t<option value=\"ON\" selected>ON</option>");
					fputs(txt, go);
					strcpy(txt, "\n\t\t\t\t\t\t<option value=\"OFF\">OFF</option>");
					fputs(txt, go);
				}
				else
				{
					strcpy(txt, "\n\t\t\t\t\t\t<option value=\"OFF\" selected>OFF</option>");
					fputs(txt, go);
					strcpy(txt, "\n\t\t\t\t\t\t<option value=\"ON\">ON</option>");
					fputs(txt, go);
				}
				strcpy(txt, "\n\t\t\t\t\t</select>");
				fputs(txt, go);
			}
			else if (ar->srt[j][OTP] == OTP_OFF)																			// OFF
				fputs("OFF", go);
			else if (ar->srt[j][OTP] == OTP_ON)																		// ON
				fputs("OFF", go);
			fputs("\n\t\t\t\t</TD>\n\t\t\t</TR>\n", go);
		}
		
		fputs("\t\t\t</TABLE>\n\t\t\t</p>\n\t\t</form>", go);
	}
	else if (!strcmp(htm, "lcntr"))																						// selection entree = lcntr.html
	{																													// le reste est genere dynamiquement
		for (j=0; j<SRT; j++)
		{
			if (ar->srt[j][OTP] == OTP_NO)																			// sortie desactivee
				continue;
			
			fputs("\t\t\t<TR>\n\t\t\t\t<TD>", go);
			fputs(entree[ar->srt[j][NEN]], go);																			// entree
			fputs("</TD>\n\t\t\t\t<TD id=\"", go);
			genere_mesure(j);
			
			fputs("</TD>\n\t\t\t\t<TD>", go);
			if (ar->srt[j][ITP] == ITP_ANL)
			{
				sprintf(txt, "\n\t\t\t\t\t<select name=\"c%ca\" id=\"c%ca\">", 'a'+j, 'a'+j);
				fputs(txt, go);
				strcpy(txt, "\n\t\t\t\t\t\t<option value=\"T\">Temperature</option>");
				fputs(txt, go);
				strcpy(txt, "\n\t\t\t\t\t\t<option value=\"A\" selected>Analogique</option>");
				fputs(txt, go);
				strcpy(txt, "\n\t\t\t\t\t\t<option value=\"N\">Numerique</option>");
				fputs(txt, go);
			}
			else if (ar->srt[j][ITP] == ITP_OWR)
			{
				sprintf(txt, "\n\t\t\t\t\t<select name=\"c%ca\" id=\"c%ca\">", 'a'+j, 'a'+j);
				fputs(txt, go);
				strcpy(txt, "\n\t\t\t\t\t\t<option value=\"T\" selected>Temperature</option>");
				fputs(txt, go);
				strcpy(txt, "\n\t\t\t\t\t\t<option value=\"A\">Analogique</option>");
				fputs(txt, go);
				strcpy(txt, "\n\t\t\t\t\t\t<option value=\"N\">Numerique</option>");
				fputs(txt, go);
			}
			else
			{
				sprintf(txt, "\n\t\t\t\t\t<select name=\"c%ca\" id=\"c%ca\">", 'a'+j, 'a'+j);
				fputs(txt, go);
				strcpy(txt, "\n\t\t\t\t\t\t<option value=\"T\">Temperature</option>");
				fputs(txt, go);
				strcpy(txt, "\n\t\t\t\t\t\t<option value=\"A\">Analogique</option>");
				fputs(txt, go);
				strcpy(txt, "\n\t\t\t\t\t\t<option value=\"N\" selected>Numerique</option>");
				fputs(txt, go);
			}
			strcpy(txt, "\n\t\t\t\t\t</select>");
			fputs(txt, go);
			fputs("\n\t\t\t\t</TD>\n\t\t\t\t<TD>", go);
			k = ar->srt[j][IBR];
			sprintf(txt, "<input type=\"number\" value=\"%d\" min=0 max=23 step=1 name=\"c%cb\" id=\"c%cb\">", k, 'a'+j, 'a'+j);
			fputs(txt, go);
			fputs("</TD>\n\t\t\t</TR>\n", go);
		}
		
		fputs("\t\t\t</TABLE>\n\t\t\t</p>\n\t\t</form>", go);
	}
	else if (!strcmp(htm, "tnsrt"))																						// selection sortie = tnsrt.html
	{																													// le reste est genere dynamiquement
		for (j=0; j<SRT; j++)
		{
			if (ar->srt[j][OTP] == OTP_NO)																			// sortie desactivee
				continue;
			
			fputs("\t\t\t<TR>\n\t\t\t\t<TD>", go);
			fputs(sortie[ar->srt[j][NSR]], go);																				// sortie
			fputs("</TD>\n\t\t\t\t<TD id=\"", go);
			if (ar->srt[j][OTP] == OTP_DGT)
				genere_sortie_dgt(j);
			else if (ar->srt[j][OTP] == OTP_PWM)
				genere_sortie_pwm(j);
			else
				genere_sortie_autre(j);
			fputs("</TD>\n\t\t\t\t<TD>", go);

			sprintf(txt, "\n\t\t\t\t\t<select name=\"d%ca\" id=\"d%ca\">", 'a'+j, 'a'+j);
			fputs(txt, go);
			sprintf(txt, "\n\t\t\t\t\t\t<option value=\"O\"%s>NO</option>", (ar->srt[j][OTP]==OTP_NO)?" selected":"");
			fputs(txt, go);
			sprintf(txt, "\n\t\t\t\t\t\t<option value=\"F\"%s>OFF</option>", (ar->srt[j][OTP]==OTP_OFF)?" selected":"");
			fputs(txt, go);
			sprintf(txt, "\n\t\t\t\t\t\t<option value=\"N\"%s>ON</option>", (ar->srt[j][OTP]==OTP_ON)?" selected":"");
			fputs(txt, go);
			sprintf(txt, "\n\t\t\t\t\t\t<option value=\"D\"%s>DGT</option>", (ar->srt[j][OTP]==OTP_DGT)?"":" selected");
			fputs(txt, go);
			sprintf(txt, "\n\t\t\t\t\t\t<option value=\"W\"%s>PWM</option>", (ar->srt[j][OTP]==OTP_PWM)?" selected":"");
			fputs(txt, go);
			strcpy(txt, "\n\t\t\t\t\t</select>");
			fputs(txt, go);
			fputs("\n\t\t\t\t</TD>\n\t\t\t\t<TD>", go);
			sprintf(txt, "<input type=\"number\" value=\"%d\" min=0 max=23 step=1 name=\"d%cb\" id=\"d%cb\">", ar->srt[j][OBR], 'a'+j, 'a'+j);
			fputs(txt, go);
			fputs("</TD>\n\t\t\t</TR>\n", go);
		}
		
		fputs("\t\t\t</TABLE>\n\t\t\t</p>\n\t\t</form>", go);
	}
	else if (!strcmp(htm, "cnfgr"))																						// configuration = cnfgr.html
	{																													// le reste est genere dynamiquement
		for (j=0; j<SRT; j++)
		{
			if (ar->srt[j][OTP] == OTP_NO)																			// sortie desactivee
				continue;
			
			fputs("\t\t\t<TR>\n\t\t\t\t<TD>", go);
			sprintf(txt, "\n\t\t\t\t\t<select name=\"e%ca\" id=\"e%ca\">", 'a'+j, 'a'+j);
			fputs(txt, go);
			for (k=0; capteur[k][0]!=0; k++)
			{
				sprintf(txt, "\n\t\t\t\t\t\t<option value=\"%c\"%s>%s</option>", 'a'+k, (ar->srt[j][NCP]==k)?" selected":"", capteur[k]);
				fputs(txt, go);
			}
			strcpy(txt, "\n\t\t\t\t\t</select>\n\t\t\t\t</TD>");
			fputs(txt, go);
			fputs("\n\t\t\t\t<TD>", go);
			sprintf(txt, "\n\t\t\t\t\t<select name=\"e%cb\" id=\"e%cb\">", 'a'+j, 'a'+j);
			fputs(txt, go);
			for (k=0; entree[k][0]!=0; k++)
			{
				sprintf(txt, "\n\t\t\t\t\t\t<option value=\"%c\"%s>%s</option>", 'a'+k, (ar->srt[j][NEN]==k)?" selected":"", entree[k]);
				fputs(txt, go);
			}
			strcpy(txt, "\n\t\t\t\t\t</select>\n\t\t\t\t</TD>");
			fputs(txt, go);
			fputs("\n\t\t\t\t<TD>", go);
			sprintf(txt, "\n\t\t\t\t\t<select name=\"e%cc\" id=\"e%cc\">", 'a'+j, 'a'+j);
			fputs(txt, go);
			for (k=0; sortie[k][0]!=0; k++)
			{
				sprintf(txt, "\n\t\t\t\t\t\t<option value=\"%c\"%s>%s</option>", 'a'+k, (ar->srt[j][NSR]==k)?" selected":"", sortie[k]);
				fputs(txt, go);
			}
			strcpy(txt, "\n\t\t\t\t\t</select>\n\t\t\t\t</TD>");
			fputs(txt, go);
			fputs("\n\t\t\t\t</TD>\n\t\t\t</TR>\n", go);
		}
		
		fputs("\t\t\t</TABLE>\n\t\t\t</p>\n\t\t</form>", go);
	}
	
	fputs("\n\t</BODY>\n</HTML>\n", go);
				
	fclose(go);
	fclose(li);
	
	return(0);
}

void genere_mesure(int j)
{
	char txt[120];
	
	if (ar->srt[j][ITP] == ITP_OWR)																						// capteur temperature
	{
		if (ar->srt[j][IMN] > ar->srt[j][MSR])
			fputs("low", go);
		else if (ar->srt[j][IMX] < ar->srt[j][MSR])
			fputs("high", go);
		else if ((ar->srt[j][IMN] == ar->srt[j][MSR]) || (ar->srt[j][IMX] == ar->srt[j][MSR]))
			fputs("egal", go);
		else
			fputs("ok", go);
		fputs("\">", go);
		sprintf(txt, "%d.%d&deg;C", ar->srt[j][MSR]>>2, 0);
	}
	else if (ar->srt[j][ITP] == ITP_DGT)																				// capteur digital
	{
		if (ar->srt[j][MSR] == 0)
		{
			fputs("off", go);
			strcpy(txt, "OFF");
		}
		else
		{
			fputs("on", go);
			strcpy(txt, "ON");
		}
		fputs("\">", go);
	}
	else if (ar->srt[j][ITP] == ITP_ANL)																				// capteur analogique
	{
		if (ar->srt[j][IMN] > ar->srt[j][MSR])
			fputs("low", go);
		else if (ar->srt[j][IMX] < ar->srt[j][MSR])
			fputs("high", go);
		else if ((ar->srt[j][IMN] == ar->srt[j][MSR]) || (ar->srt[j][IMX] == ar->srt[j][MSR]))
			fputs("egal", go);
		else
			fputs("ok", go);
		fputs("\">", go);
		sprintf(txt, "%d", ar->srt[j][MSR]);
	}
	fputs(txt, go);																										// lu = mesure
}

void genere_sortie_dgt(int j)
{
	if (ar->srt[j][ITP] != ITP_DGT)																						// entree PAS dgt
	{
		if (ar->srt[j][IMN] == ar->srt[j][IMX])																			// seuil mesure mmn=mmx
		{
			if (ar->srt[j][OMN] == ar->srt[j][OMX])																		// Smn=Smx
			{
				if (ar->srt[j][IMN] == ar->srt[j][MSR])
				{
					if (!ar->srt[j][OMN])
						fputs("off\">OFF", go);
					else
						fputs("on\">ON", go);
				}
				else
				{
					if (!ar->srt[j][OMN])
						fputs("on\">ON", go);
					else
						fputs("off\">OFF", go);
				}
			}
			else																										// Smn!=Smx
			{
				if (ar->srt[j][MSR] < ar->srt[j][IMN])
				{
					if (!ar->srt[j][OMN])
						fputs("off\">OFF", go);
					else
						fputs("on\">ON", go);
				}
				else if (ar->srt[j][MSR] > ar->srt[j][IMX])
				{
					if (!ar->srt[j][OMX])
						fputs("off\">OFF", go);
					else
						fputs("on\">ON", go);
				}
				else 																									//if == rien (garde la cmm existante)
				{
					if (!ar->srt[j][CMM])
						fputs("off\">OFF", go);
					else
						fputs("on\">ON", go);
				}
			}
		}
		else																											// seuil mesure mmn!=mmx
		{
			if (ar->srt[j][OMN] == ar->srt[j][OMX])																		// seuil sortie smn=smx
			{
				if ((ar->srt[j][MSR] > ar->srt[j][IMN]) && (ar->srt[j][MSR] < ar->srt[j][IMX]))
				{
					if (!ar->srt[j][OMN])
						fputs("off\">OFF", go);
					else
						fputs("on\">ON", go);
				}
				else if ((ar->srt[j][MSR] < ar->srt[j][IMN]) || (ar->srt[j][MSR] > ar->srt[j][IMX]))
				{
					if (!ar->srt[j][OMN])
						fputs("on\">ON", go);
					else
						fputs("off\">OFF", go);
				}
				else 																									//if == rien (garde la cmm existante)
				{
					if (!ar->srt[j][CMM])
						fputs("off\">OFF", go);
					else
						fputs("on\">ON", go);
				}
			}
			else																										// seuil sortie smn!=smx
			{
				if ((ar->srt[j][IMN] < ar->srt[j][IMX]) && (ar->srt[j][OMN] < ar->srt[j][OMX]))							// meme ordre
				{
					if (ar->srt[j][MSR] < ar->srt[j][IMN])
					{
						if (!ar->srt[j][OMN])
							fputs("off\">OFF", go);
						else
							fputs("on\">ON", go);
					}
					else if (ar->srt[j][MSR] > ar->srt[j][IMX])
					{
						if (!ar->srt[j][OMX])
							fputs("off\">OFF", go);
						else
							fputs("on\">ON", go);
					}
					else																								// if >= && <= rien hysteresis
					{
						if (!ar->srt[j][CMM])
							fputs("off\">OFF", go);
						else
							fputs("on\">ON", go);
					}
				}
				else if ((ar->srt[j][IMN] > ar->srt[j][IMX]) && (ar->srt[j][OMN] > ar->srt[j][OMX]))						// meme ordre
				{
					if (ar->srt[j][MSR] < ar->srt[j][IMN])
					{
						if (!ar->srt[j][OMN])
							fputs("off\">OFF", go);
						else
							fputs("on\">ON", go);
					}
					else if (ar->srt[j][MSR] > ar->srt[j][IMX])
					{
						if (!ar->srt[j][OMX])
							fputs("off\">OFF", go);
						else
							fputs("on\">ON", go);
					}
					else																								// if >= && <= rien hysteresis
					{
						if (!ar->srt[j][CMM])
							fputs("off\">OFF", go);
						else
							fputs("on\">ON", go);
					}
				}
				else																									// ordre different
				{
				}
			}
		}
	}
	else																												// entree digitale
	{
		if (ar->srt[j][IMN] == ar->srt[j][IMX])																		// seuil mesure on=on OU off=off
		{
			if (ar->srt[j][OMN] == ar->srt[j][OMX])																	// Wmin == Wmax
			{
				if (ar->srt[j][IMN] == ar->srt[j][MSR])
				{
					if (!ar->srt[j][OMN])
						fputs("off\">OFF", go);
					else
						fputs("on\">ON", go);
				}
				else
				{
					if (!ar->srt[j][OMN])
						fputs("on\">ON", go);
					else
						fputs("off\">OFF", go);
				}
			}
			else																							// Wmin != Wmax
			{
				if (ar->srt[j][MSR] < ar->srt[j][IMN])
				{
					if (!ar->srt[j][OMN])
						fputs("off\">OFF", go);
					else
						fputs("on\">ON", go);
				}
				else if (ar->srt[j][MSR] > ar->srt[j][IMX])
				{
					if (!ar->srt[j][OMX])
						fputs("off\">OFF", go);
					else
						fputs("on\">ON", go);
				}
				else 																									//if == rien (garde la cmm existante)
				{
					if (!ar->srt[j][CMM])
						fputs("off\">OFF", go);
					else
						fputs("on\">ON", go);
				}
			}
		}
		else																								// seuil mesure on!=off OU off!=on
		{
			if (ar->srt[j][OMN] == ar->srt[j][OMX])																	// seuil sortie on=on OU off=off
			{
				if ((ar->srt[j][MSR] > ar->srt[j][IMN]) && (ar->srt[j][MSR] < ar->srt[j][IMX]))
				{
					if (!ar->srt[j][OMN])
						fputs("off\">OFF", go);
					else
						fputs("on\">ON", go);
				}
				else if ((ar->srt[j][MSR] < ar->srt[j][IMN]) || (ar->srt[j][MSR] > ar->srt[j][IMX]))
				{
					if (!ar->srt[j][OMN])
						fputs("on\">ON", go);
					else
						fputs("off\">OFF", go);
				}
				else 																									//if == rien (garde la cmm existante)
				{
					if (!ar->srt[j][CMM])
						fputs("off\">OFF", go);
					else
						fputs("on\">ON", go);
				}
			}
			else																							// seuil sortie on!=off OU off!=on
			{
				if (ar->srt[j][IMN] == ar->srt[j][OMN])																// ordre identique on.off et on.off OU off.on et off.on
				{
					if (ar->srt[j][MSR] == ar->srt[j][OMN])
					{
						if (!ar->srt[j][OMN])
							fputs("off\">OFF", go);
						else
							fputs("on\">ON", go);
					}
					else
					{
						if (!ar->srt[j][OMN])
							fputs("on\">ON", go);
						else
							fputs("off\">OFF", go);
					}
				}
				else																						// ordre inverse on.off et off.on OU off.on et on.off
				{
					if (ar->srt[j][MSR] == ar->srt[j][OMN])
					{
						if (ar->srt[j][OMN])
							fputs("off\">OFF", go);
						else
							fputs("on\">ON", go);
					}
					else
					{
						if (ar->srt[j][OMN])
							fputs("on\">ON", go);
						else
							fputs("off\">OFF", go);
					}
				}
			}
		}
	}
}

void genere_sortie_pwm(int j)
{
	char txt[120];
	
	if (ar->srt[j][MSR] < ar->srt[j][OMN])
		fputs("low", go);
	else if (ar->srt[j][MSR] > ar->srt[j][OMX])
		fputs("high", go);
	else if ((ar->srt[j][MSR] == ar->srt[j][OMN]) || (ar->srt[j][MSR] == ar->srt[j][OMX]))
		fputs("egal", go);
	else
		fputs("ok", go);
	fputs("\">", go);
	sprintf(txt, "%d", ar->srt[j][CMM]);
	fputs(txt, go);
}

void genere_sortie_autre(int j)
{
	if (ar->srt[j][OTP] == OTP_NO)																						// no
		fputs("on\">NO", go);
	if (ar->srt[j][OTP] == OTP_OFF)																						// off
		fputs("off\">OFF", go);
	if (ar->srt[j][OTP] == OTP_ON)																						// on
		fputs("on\">ON", go);
}

//
// ardsrv [-?] [-auto] [port] [racine]
// ardsrv
// ardsrv -?
// ardsrv -auto
// ardsrv 9999
// ardsrv /var/local/www/ardsrv
// ardsrv 9999 /var/local/www/ardsrv
// ardsrv /var/local/www/ardsrv 9999
//
// racine doit etre un dossier sous /var/local/www ou sous /opt/lampp/htdocs
// ou si nom dossier simple (sans /) doit exister sous /var/local/www ou /opt/lampp/htdocs
//

int main(int argc, char **argv, char **argenv)
{
	int li_mmr;
	int j;
	int cfd;
	char c;
	socklen_t pas;																										// peer addr size
	struct sockaddr_in cln_adr;
	struct sockaddr_in srv_adr;

	j = prment(argc, argv);
	switch (j)
	{
		//case 1:
		//	printf("Parametre invalide !!!\n");
		//	break;
		case 2:
			printf("Port invalide !!!\n");
			break;
		case 3:
			printf("Dossier racine invalide !!!\n");
			break;
	}
	if (j != 0)
		return(0);
		
	//printf("PERE ardsrv %d %s --> lancer en tache de fond ...\n", port, racine);
	if (fork() != 0)																									// pere
		return(0);
		
	(void) signal(SIGCLD, SIG_IGN);																						// ignore child death
	(void) signal(SIGHUP, SIG_IGN);																						// ignore terminal hangups
	//signal(SIGINT, cbk_ctrlc);
	for (j=2;j<32;j++)
		(void) close(j);																								// close open files
	(void) setpgrp();																									// break away from process group

	lg.ntr = 5;
	lg.grv= LG_INF;
	lg_strcpy(lg.prj, "ardsrv");
	lg_strcpy(lg.unt, "ardsrv");
	lg.tll = 100000;
	lg.nmb = 3;
	lg_cns();

	if ((li_mmr = shmget(CLEF, sizeof(Ar), 0660)) == -1)
	{
		printf("FILS Erreur shmget() ardcmm n'a pas ete lance ...\n");
		lg_strcpy(lg.fnc, "main");
		lg.grv= LG_SYS;
		lg_strcpy(lg.mss, "shmget");
		lg_strcpy(lg.err, "Erreur");
		lg_ecr();
		exit(1);
	}
	if ((gp_mmr = shmat(li_mmr, 0, 0)) == (void *) -1)																	// shmat(mmr, 0, SHM_RDONLY)
	{
		printf("FILS Erreur shmat()\n");
		lg_strcpy(lg.fnc, "main");
		lg.grv= LG_SYS;
		lg_strcpy(lg.mss, "shmat");
		lg_strcpy(lg.err, "Erreur");
		lg_ecr();
		exit(1);
	}
	ar = (Ar *) gp_mmr;
	
	sfd = -1;
	gi_fin = 0;	

	lg_strcpy(lg.fnc, "main");
	lg.grv= LG_INF;
	lg_strcpy(lg.mss, "chdir");
	lg_strcpy(lg.err, racine);
	lg_ecr();
	if (chdir(racine) == -1)
	{
		printf("FILS Erreur chdir()\n");
		return(3);
	}
	
	if ((bff = malloc(BFF+1)) == NULL)
	{
		printf("FILS Erreur malloc()\n");
		lg_strcpy(lg.fnc, "main");
		lg.grv= LG_SYS;
		lg_strcpy(lg.mss, "malloc");
		lg_strcpy(lg.err, "Erreur");
		lg_ecr();
		exit(1);
	}
	*(bff+BFF) = 0;

	/* setup the network socket */
	if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)																	// AF_UNIX
	{
		printf("FILS Erreur socket()\n");
		lg_strcpy(lg.fnc, "main");
		lg.grv= LG_SYS;
		lg_strcpy(lg.mss, "socket");
		lg_strcpy(lg.err, "Erreur");
		lg_ecr();
		free(bff);
		exit(1);
	}
	memset(&srv_adr, 0, sizeof(struct sockaddr_in));
	
	srv_adr.sin_family = AF_INET;
	srv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	srv_adr.sin_port = htons(port);
	
	if (bind(sfd, (struct sockaddr *) &srv_adr, sizeof(srv_adr)) == -1)
	{
		printf("FILS Erreur bind()\n");
		lg_strcpy(lg.fnc, "main");
		lg.grv= LG_SYS;
		lg_strcpy(lg.mss, "bind");
		lg_strcpy(lg.err, "Erreur");
		lg_ecr();
		close(sfd);
		free(bff);
		exit(1);
	}
	if (listen(sfd, 64) == -1)
	{
		printf("FILS Erreur listen()\n");
		lg_strcpy(lg.fnc, "main");
		lg.grv= LG_SYS;
		lg_strcpy(lg.mss, "listen");
		lg_strcpy(lg.err, "Erreur");
		lg_ecr();
		close(sfd);
		free(bff);
		exit(1);
	}

	printf("FILS ardsrv %d %s --> lancer en tache de fond ...\n", port, racine);
	while (!gi_fin) 
	{
		pas = sizeof(cln_adr);
		if ((cfd = accept(sfd, (struct sockaddr *) &cln_adr, &pas)) == -1)
		{
			//printf("FILS Erreur accept()\n");
			lg_strcpy(lg.fnc, "main");
			lg.grv= LG_SYS;
			lg_strcpy(lg.mss, "accept");
			lg_strcpy(lg.err, "Erreur");
			lg_ecr();
			close(sfd);
			free(bff);
			//exit(1);
			break;
		}
		if (pas > sizeof(cln_adr))
		{
			//printf("FILS Erreur accept trop petit\n");
			lg_strcpy(lg.fnc, "main");
			lg.grv= LG_SYS;
			lg_strcpy(lg.mss, "accept");
			lg_strcpy(lg.err, "(trop petit)");
			lg_ecr();
			close(sfd);
			free(bff);
			exit(1);
		}
		web(pas, cfd);
		close(cfd);
	}
	
	if (gp_mmr != (void *) -1)
		shmdt(gp_mmr);

	lg_strcpy(lg.fnc, "main");
	lg.grv= LG_SYS;
	lg_strcpy(lg.mss, "fin");
	lg_strcpy(lg.err, "");
	lg_ecr();
	return(0);
}

//
// controle le port
//
// retour 0 ok p -> port
//

int tstnmb(char *p)
{
	int k;
	char chiffre[] = "0123456789";
	
	if (strlen(p) > 5)
		return(2);

	for (k=0; *(p+k)!=0; k++)																		// que des chiffres ?
	{
		if (strchr(chiffre, *(p+k)) == NULL)
			break;
	}
	if (*(p+k) != 0)
		return(2);

	sscanf(p, "%d", &port);
	return(0);
}

//
// controle dossier racine
//
// retour 0 ok p -> racine
//

int tstrcn(char *p)
{
	int k;
	char ok[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
	char lettre[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	
	racine[0] = 0;
	if (*p == '/')																										// racine commence par /
	{
		if (strncmp(p, "/var/local/www/", 15) != 0)
		{
			if (strncmp(p, "/opt/lampp/htdocs/", 18) != 0)
				return(3);
			else
			{
				p += 18;
				strcpy(racine, "/opt/lampp/htdocs/");
			}
		}
		else
		{
			p += 15;
			strcpy(racine, "/var/local/www/");
		}
	}
	
	if (strlen(p) > 80)																									// nom de sous dossier sous www ou htdocs ou simple trop long (> 80)
		return(3);
	if (*p == 0)
		return(3);
	if (strchr(lettre, *p) == NULL)
		return(3);
	
	for (k=1; *(p+k)!=0; k++)																							// autre caractere n'est pas ok (a-z A-Z 0-9 _) ?
	{
		if (strchr(ok, *(p+k)) == NULL)
			break;
	}
	if (*(p+k) != 0)
		return(3);

	if (racine[0] != 0)
	{
		strcat(racine, p);
		if (access(racine, F_OK) != 0)
			return(3);
	}
	else
	{
		sprintf(racine, "/var/local/www/%s", p);
		if (access(racine, F_OK) != 0)
		{
			sprintf(racine, "/opt/lampp/htdocs/%s", p);
			if (access(racine, F_OK) != 0)
				return(3);
		}
	}
	
	return(0);
}

//
// Controle les parametres de lancement du prog
//

int prment(int argc, char **argv)
{
	int  j, k;
	char lettre[] = "/abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char chiffre[] = "0123456789";
	char ok[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
	char mss[] = "Serveur WEB pour watercooling (MAJ par ardcmm)\n\tardsrv [-?] [-auto] [port] [racine]\n\tardsrv -auto <==> ardsrv 9999 /var/local/www/ardsrv\n";

	port = 9999;
	strcpy(racine, "/var/local/www/ardsrv");

	if ((argc < 2) || (argc > 3))																						// nombre de parametre incorrect
	{
		printf("%s", mss);
		return(1);
	}
	
	if (argc == 2)
	{
		if (*(*(argv+1)) == '-')
		{
			if (*(*(argv+1)+1) == '?')																					// demande d'aide
			{
				printf("%s", mss);
				return(1);
			}
			else
			{
				if (strcmp(*(argv+1), "-auto") != 0)																	// parametrage inconnu
				{
					printf("%s", mss);
					return(1);
				}
				else																									// mode auto
				{
					return(0);
				}
			}
		}

		if (strchr(chiffre, *(*(argv+1))) != NULL)																		// 1er caractere un chiffre
			return(tstnmb(*(argv+1)));
		else
			return(tstrcn(*(argv+1)));
	}

// deux parametres
	
	if (strchr(chiffre, *(*(argv+1))) != NULL)																			// 1er parametre un chiffre
	{
		if (strchr(lettre, *(*(argv+2))) == NULL)																		// 2eme parametre ni une lettre ni un /
			return(1);

		j = tstnmb(*(argv+1));																							// 1 chiffre 1 lettre
		if (j != 0)
			return(j);
		return(tstrcn(*(argv+2)));
	}
	else																												// 1er pas un chiffre
	{
		if (strchr(lettre, *(*(argv+1))) == NULL)																		// 1er parametre ni une lettre ni un /
			return(1);
			
		if (strchr(chiffre, *(*(argv+2))) == NULL)																		// 2eme parametre pas un chiffre
			return(1);

		j = tstnmb(*(argv+2));																							// 1 lettre 1 chiffre
		if (j != 0)
			return(j);
		return(tstrcn(*(argv+1)));
	}
		
	return(0);
}

void cbk_ctrlc(int sig)
{
	if (sfd != -1)	
		close(sfd);

	if (gp_mmr != (void *) -1)
		shmdt(gp_mmr);

	gi_fin = 1;

	lg_strcpy(lg.fnc, "cbk_ctrlc");
	lg.grv= LG_INF;
	lg_strcpy(lg.mss, "CTRL-C");
	lg_strcpy(lg.err, "");
	lg_ecr();
}

void fv_majprm(char *pr_nom, char *pr_vlr)
{
	// POST ... ABC=D&... --> nom=ABC VLR=D
	
	// A --> a=slntr
	// B --> IND
	// C --> a=MMN b=MMX
	// D --> valeur

	// A --> b=wlsrt
	// B --> IND
	// C --> a=SMN b=SMX
	// D --> valeur

	// A --> c=lcntr
	// B --> IND
	// C --> a=TIO b=CPT
	// D --> valeur

	// A --> d=tnsrt
	// B --> IND
	// C --> a=TIO b=SPN
	// D --> valeur

	// A --> e=cnfgr
	// B --> IND
	// C --> a=capteur b=entree c=sortie 
	// D --> valeur
/*					
	switch (*pr_nom)
	{
		case 'a':												// slntr
			if (*(pr_nom+2) == 'a')								// mmn
			{
				if (ar->srt[TIO][*(pr_nom+1)-'a'] & 6)				// entree anl ou owr
					ar->srt[MMN][*(pr_nom+1)-'a'] = atoi(pr_vlr);
				else											// entree dgt
				{
					if (!strcmp(pr_vlr, "OFF"))
						ar->srt[MMN][*(pr_nom+1)-'a'] = 0;
					else
						ar->srt[MMN][*(pr_nom+1)-'a'] = 255;
				}
			}
			else if (*(pr_nom+2) == 'b')						// mmx
			{
				if (ar->srt[TIO][*(pr_nom+1)-'a'] & 6)				// entree anl ou owr
					ar->srt[MMX][*(pr_nom+1)-'a'] = atoi(pr_vlr);
				else											// entree dgt
				{
					if (!strcmp(pr_vlr, "OFF"))
						ar->srt[MMX][*(pr_nom+1)-'a'] = 0;
					else
						ar->srt[MMX][*(pr_nom+1)-'a'] = 255;
				}
			}
			break;
			
		case 'b':												// wlsrt
			if (*(pr_nom+2) == 'a')								// smn
				ar->srt[SMN][*(pr_nom+1)-'a'] = atoi(pr_vlr);
			else if (*(pr_nom+2) == 'b')						// smx
				ar->srt[SMX][*(pr_nom+1)-'a'] = atoi(pr_vlr);
			break;
			
		case 'c':												// lcntr
			if (*(pr_nom+2) == 'a')								// type d'entree (tio)
			{
				switch (*(pr_vlr))
				{
					case 'T':
						ar->srt[TIO][*(pr_nom+1)-'a'] &= ~6;
						ar->srt[TIO][*(pr_nom+1)-'a'] |= 4;
						break;
					case 'A':
						ar->srt[TIO][*(pr_nom+1)-'a'] &= ~6;
						ar->srt[TIO][*(pr_nom+1)-'a'] |= 2;
						break;
					case 'N':
						ar->srt[TIO][*(pr_nom+1)-'a'] &= ~6;
						break;
				}
			}
			else if (*(pr_nom+2) == 'b')						// numero entree (cpt)
			{
				switch (ar->srt[TIO][*(pr_nom+1)-'a'] & 6)
				{
					case 4:
						ar->srt[CPT][*(pr_nom+1)-'a'] = (*(pr_vlr) - '0') + '0';
						break;
					case 2:
						ar->srt[CPT][*(pr_nom+1)-'a'] = (*(pr_vlr) - '0') + 'A';
						break;
					case 0:
						ar->srt[CPT][*(pr_nom+1)-'a'] = (*(pr_vlr) - '0') + 'a';
						break;
				}
			}
			break;
			
		case 'd':												// tnsrt
			if (*(pr_nom+2) == 'a')								// etat sortie (tio)
			{
				switch (*(pr_vlr))
				{
					case 'O':									// no
						ar->srt[TIO][*(pr_nom+1)-'a'] &= ~0x39;
						ar->srt[TIO][*(pr_nom+1)-'a'] |= 0x20;
						break;
					case 'F':									// off
						ar->srt[TIO][*(pr_nom+1)-'a'] &= ~0x39;
						ar->srt[TIO][*(pr_nom+1)-'a'] |= 8;
						break;
					case 'N':									// on
						ar->srt[TIO][*(pr_nom+1)-'a'] &= ~0x39;
						ar->srt[TIO][*(pr_nom+1)-'a'] |= 0x10;
						break;
					case 'D':									// dgt
						ar->srt[TIO][*(pr_nom+1)-'a'] &= ~0x39;
						break;
					case 'W':									// pwm
						ar->srt[TIO][*(pr_nom+1)-'a'] &= ~0x39;
						ar->srt[TIO][*(pr_nom+1)-'a'] |= 1;
						break;
				}
			}
			else if (*(pr_nom+2) == 'b')						// broche de sortie (spn)
				ar->srt[SPN][*(pr_nom+1)-'a'] = *(pr_vlr) - '0';
			break;
			
		case 'e':												// cnfgr
			if (*(pr_nom+2) == 'a')								// nom capteur
				ar->srt[NCP][*(pr_nom+1)-'a'] = *(pr_vlr);
			else if (*(pr_nom+2) == 'b')						// nom entree
				ar->srt[NEN][*(pr_nom+1)-'a'] = *(pr_vlr);
			else if (*(pr_nom+2) == 'c')						// nom sortie
				ar->srt[NSR][*(pr_nom+1)-'a'] = *(pr_vlr);
			break;
	}				
*/
}

