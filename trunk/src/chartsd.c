/***************************************************************************
 *   Copyright (C) 2005 by Alexander Mieland                               *
 *   dma147@linux-stats.org                                                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE
#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <getopt.h>
#include <unistd.h>
#include <alloca.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <gd.h>
#include <gdfontl.h>
#include <gdfontt.h>
#include <gdfonts.h>
#include <gdfontmb.h>
#include <gdfontg.h>
#include <confuse.h>



/** variables declaration */
FILE *png;
FILE *pnd;
FILE *abar;
cfg_t *cfg;
gdImagePtr image;
struct tm *zeit;
struct timeval t_now, t_last;
typedef void (*sighandler_t)(int);

const char *program;
const char *Version="0.3";

char *config_file = "/etc/charts.d/temp.hda.conf";
char *sensor = NULL;
char *unit = NULL;
char *datafile = NULL;
char *diagramfile = NULL;
char *INOUT;
char starttime[20];
char stoptime[20];
char s[64]; 
char maxin[20];
char maxout[20];
char avgout[20];
char highout[20];
char lowout[20];
char dia_title[256];

float XStep;
float YStep;
float total = 0.0;
float average = 0.0;
float highest = 0.0;
float lowest = 0.0;
double timedelta;

int interval = 60;
int shown_intervals = 300;
int WIDTH = 600;
int HEIGHT = 250;
int maxvalue = 65;
int minvalue = 30;
int run=1;
int LINES = 0;
int rcount = 0;
int R = 0;
int G = 255;
int B = 0;
int ac = 0;
int ca = 0;
int opt;
int XPOS;
int YPOS;
int NULLX;
int NULLY;
int URSPRUNGX;
int URSPRUNGY;
int maxX;
int maxY;
int Xlength;
int Ylength;
int X1;
int X2;
int Y1;
int Y2;
int black;
int white;
int red;
int green;
int grey;
int blue;
int linecolor;
int thiscolor;
int i;
int j;
int LINESTART;
int start;
int stop;
int diff;
int LINESTART;


/** function declaration */
int intvgl(const void *v1, const void *v2);
static sighandler_t handle_signal (int sig_nr, sighandler_t signalhandler);
static void start_daemon (const char *log_name, int facility);
void skipline(FILE *f);
void usage(int err);
void version(int err);
int fileExists (char * fileName);
int main(int argc, char *argv[]);



/** functions */
int intvgl(const void *v1, const void *v2)
{
	return (*(int *)v1 - *(int *)v2);
}

static sighandler_t handle_signal (int sig_nr, sighandler_t signalhandler)
{
  struct sigaction neu_sig, alt_sig;
  neu_sig.sa_handler = signalhandler;
  sigemptyset (&neu_sig.sa_mask);
  neu_sig.sa_flags = SA_RESTART;
  if (sigaction (sig_nr, &neu_sig, &alt_sig) < 0)
    return SIG_ERR;
  return alt_sig.sa_handler;
}

static void start_daemon (const char *log_name, int facility)
{
  int i;
  pid_t pid;

  if ((pid = fork ()) != 0)
    exit (EXIT_FAILURE);

  if (setsid() < 0) {
    printf("%s: Can not start session for daemonizing!\n", log_name);
    exit (EXIT_FAILURE);
  }
  handle_signal (SIGHUP, SIG_IGN);
  if ((pid = fork ()) != 0)
    exit (EXIT_FAILURE);
  chdir ("/");
  umask (0);
  for (i = sysconf (_SC_OPEN_MAX); i > 0; i--)
    close (i);
  openlog ("chartsd", LOG_PID | LOG_CONS| LOG_NDELAY, facility);
}


int fileExists (char * fileName)
{
  struct stat buf;
  int i = stat ( fileName, &buf );
  if ( i == 0 )
  {
    return 1;
  }
  return 0;
}

void skipline(FILE *f)
{
  int ch;
  do 
  {
    ch = getc(f);
  } while ( ch != '\n' && ch != EOF );
}

const struct option longopts[] = {
  { "config-file",  1, 0, 'c' },
  { "help",         0, 0, 'h' },
  { "version",      1, 0, 'v' },
  { 0, 0, 0, 0 }
};

void usage(int err)
{
  fprintf(stderr,
          "\nUsage: %s [options]\n\n"
              "Options:\n"
              "   --config-file <file>    -c   The config file to use (Default: /etc/charts.d/temp.hda.conf)\n"
              "   --help                  -h   This help screen\n"
              "   --version               -v   Version information\n\n",
          program);
  exit(err);
}

void version(int err)
{
  printf("\nThis is %s %s\n", program, Version);
  fputs("Written by Alexander Mieland <dma147@linux-stats.org>\n", stdout);
  fputs("IRC: #archlinux, #archlinux.de, #linux-stats on irc.freenode.net\n\n", stdout);
  fputs("If you've found any bugs or have any questions,\nplease feel free to contact me\n\n", stdout);
  exit(err);
}

int main(int argc, char *argv[])
{
  program = argv[0];
  
  sensor = strdup("Temperature of harddisc (hda)");
  unit = strdup("C");
  datafile = strdup("/var/log/charts.hda.log");
  diagramfile = strdup("/tmp/charts.hda.png");
  
  while ( (opt = getopt_long(argc, argv, "c:hv", longopts, NULL)) != -1 ) 
  {
    switch ( opt ) 
    {
      case 'c':
        config_file = optarg;
        break;
      case 'h':
        usage(0);
        break;
      case 'v':
        version(0);
        break;
      default:
        usage(1);
        break; 
    }
  }
  
  
  /** starting the daemon-process */
  start_daemon ("chartsd", LOG_LOCAL0);
  
  if ( !fileExists(config_file) )
  {
    fprintf(stderr, "%s: %s: configuration file not found!\n", argv[0], config_file);
    syslog( LOG_ERR, "%s: %s: configuration file not found!\n", argv[0], config_file);
    syslog( LOG_ERR, "chartsd shutting down!");
    run=0;
    return EXIT_FAILURE;
  }
  
  cfg_opt_t opts[] = {
    CFG_SIMPLE_STR("SENSOR", &sensor),
    CFG_SIMPLE_STR("UNIT", &unit),
    CFG_SIMPLE_STR("DATAFILE", &datafile),
    CFG_SIMPLE_STR("DIAGRAMFILE", &diagramfile),
    CFG_SIMPLE_INT("INTERVAL", &interval),
    CFG_SIMPLE_INT("WIDTH", &WIDTH),
    CFG_SIMPLE_INT("HEIGHT", &HEIGHT),
    CFG_SIMPLE_INT("MAXVALUE", &maxvalue),
    CFG_SIMPLE_INT("MINVALUE", &minvalue),
    CFG_SIMPLE_INT("R", &R),
    CFG_SIMPLE_INT("G", &G),
    CFG_SIMPLE_INT("B", &B),
    CFG_END()
  };
  cfg = cfg_init(opts, 0);
  cfg_parse(cfg, config_file);
  cfg_free(cfg);

  if ( !fileExists(datafile) )
  {
    fprintf(stderr, "%s: %s: data file not found!\n", argv[0], datafile);
    syslog( LOG_ERR, "%s: %s: data file not found!\n", argv[0], datafile);
    syslog( LOG_ERR, "chartsd shutting down!");
    run=0;
    return EXIT_FAILURE;
  }
  

  if (interval < 1) 
  {
    interval = 1;
  }

  while (run)
  {
    
    if (rcount == 0)
      syslog( LOG_NOTICE, "chartsd started.\n");
    
    gettimeofday(&t_now, NULL);
    
    timedelta = (double)(t_now.tv_sec - t_last.tv_sec) + 
        (t_now.tv_usec - t_last.tv_usec)/1000000;
    
    
    
    if ( !(abar = fopen(datafile, "r")) )
    {
      fprintf(stderr, "%s: %s: %s\n", argv[0], datafile, strerror(errno));
      syslog( LOG_ERR, "%s: %s: %s\n", argv[0], datafile, strerror(errno));
      syslog( LOG_ERR, "chartsd shutting down!");
      run=0;
      return EXIT_FAILURE;
    }
    
    LINES = 0;
    while ( (fgets(s,64,abar)) != NULL )      
      LINES++;
    LINES++;
    float DATA[LINES][2];
    
    
    
    if( !(image = gdImageCreate(WIDTH, HEIGHT)) )
    {
      fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
      syslog( LOG_ERR, "%s: %s\n", argv[0], strerror(errno));
      syslog( LOG_ERR, "chartsd shutting down!");
      run=0;
      return EXIT_FAILURE;
    }
    
    white = gdImageColorAllocate(image, 255, 255, 255);
    black = gdImageColorAllocate(image, 0, 0, 0);
    red   = gdImageColorAllocate(image, 255, 0, 0);
    green = gdImageColorAllocate(image, 0, 255, 0);
    grey  = gdImageColorAllocate(image, 127, 127, 127);
    blue  = gdImageColorAllocate(image, 0, 0, 255);
    
    linecolor = gdImageColorAllocate(image, R, G, B);
    
    image->transparent = white;
    
    
    fseek(abar, 0L, SEEK_SET);
    for (i=0; i<LINES; i++)
    {
      for (j=0; j<2; j++)
      {
        fscanf(abar,"%f",&DATA[i][j]);
      }
    }
    fclose(abar);
    
    LINES = LINES - 1;
    if ((LINES-shown_intervals) <= 0)
      LINESTART = 0;
    else
      LINESTART = (LINES-shown_intervals);
    
    total = 0.0;
    float VALUES[2][(LINES-LINESTART)];
    for (j=0; j<2; j++)
    {
      ac=0;
      for (i=LINESTART; i<LINES; i++)
      {
      	if (j == 1)
      	{
      		total = (total + (float)DATA[i][j]);
      	}
        VALUES[j][ac] = (float)DATA[i][j];
        ac++;
      }
    }
    average = (total / ac);
    
    
    float values[ac];
    j=1;
    ca = 0;
		for (i=0; i<(LINES-LINESTART); i++)
		{
			values[ca] = (float)VALUES[j][i];
			ca++;
		}
    qsort(values, ca, sizeof(values[0]), intvgl);
    
    highest = values[(ca-1)];
    lowest = values[0];
    
    
    NULLX = 0;
    NULLY = HEIGHT;
    XPOS = NULLX;
    YPOS = NULLY;
    URSPRUNGX = NULLX+60;
    URSPRUNGY = NULLY-20;
    
   
    start = (int)VALUES[0][0];
    stop = (int)VALUES[0][(ac-1)];
    
    
    diff = (stop - start);
    
    zeit = localtime( (time_t*)&start );
    strftime( starttime, 20, "%I:%M:%S%p", zeit);
    
    zeit = localtime( (time_t*)&stop );
    strftime( stoptime, 20, "%I:%M:%S%p", zeit);    
    
    
    
    /** **************   START OF ACTUAL USE OF BANDWIDTH ************* */
    Xlength = (WIDTH - 20) - 45;
    Ylength = (HEIGHT - 20) - 20;
    maxY = maxvalue + (maxvalue / 3);
    maxX = shown_intervals;
    
    XStep = ( (float)Xlength / (float)maxX );
    YStep = ( (float)Ylength / (float)maxY );
    
    sprintf(maxin, "%i %s", maxvalue, unit);
    sprintf(maxout, "%i %s", minvalue, unit);
    sprintf(avgout, "%.2f %s", average, unit);
    sprintf(highout, "Highest: %.2f %s", highest, unit);
    sprintf(lowout, "Lowest: %.2f %s", lowest, unit);
    
    /** Title of diagram */
    sprintf(dia_title, "%s    (%s - %s)", sensor, starttime, stoptime);
    gdImageString(image, gdFontMediumBold, 2, 2, dia_title, black);
    
    /** axes of coordinates */
    gdImageLine (image, (NULLX+60), (NULLY-15), (NULLX+60), 15, black);
    gdImageLine (image, (NULLX+55), (NULLY-20), (WIDTH-5), (NULLY-20), black);
    
    /** max/min/average lines */
    gdImageLine (image, (NULLX+55), (URSPRUNGY - (YStep * maxvalue)), (WIDTH-5), (URSPRUNGY - (YStep * maxvalue)), red);
    gdImageLine (image, (NULLX+55), (URSPRUNGY - (YStep * minvalue)), (WIDTH-5), (URSPRUNGY - (YStep * minvalue)), blue);
    gdImageDashedLine (image, (NULLX+55), (URSPRUNGY - (YStep * average)), (WIDTH-5), (URSPRUNGY - (YStep * average)), green);
    
    /** max/min/average values */
    gdImageString(image, gdFontSmall, (NULLX+2), (URSPRUNGY - (YStep * maxvalue) - 5), maxin, black);
    gdImageString(image, gdFontSmall, (NULLX+2), (URSPRUNGY - (YStep * minvalue) - 5), maxout, black);
    gdImageString(image, gdFontSmall, (NULLX+2), (URSPRUNGY - (YStep * average) - 5), avgout, black);
    
    /** highest/lowest display */
    gdImageString(image, gdFontSmall, (NULLX+15), (NULLY-13), highout, red);
    gdImageString(image, gdFontSmall, (NULLX+150), (NULLY-13), lowout, blue);
    
    for (j=1; j<2; j++)
    {
      for (i=0; i<(LINES-LINESTART); i++)
      {
        thiscolor = linecolor;
        
        X1 = URSPRUNGX + (XStep * i);
        Y1 = URSPRUNGY - (YStep * VALUES[j][i]);
        X2 = URSPRUNGX + (XStep * (i+1));
        Y2 = URSPRUNGY - (YStep * VALUES[j][(i+1)]);
        gdImageLine (image, X1, Y1, X2, Y2, thiscolor);
        
      }
    }
    /** **************   END OF ACTUAL USE OF BANDWIDTH ************* */
    
    /** MY COPYRIGHT! PLEASE LEAVE THIS FOLLOWING LINE UNTOUCHED! */
    gdImageString(image, gdFontSmall, (WIDTH-155), (NULLY-13), "http://chartsd.berlios.de", black);
    /** MY COPYRIGHT! PLEASE LEAVE THE LINE ABOVE UNTOUCHED! */
    
    if ( !(png = fopen(diagramfile, "wb")) )
    {
      fprintf(stderr, "%s: %s: %s\n", argv[0], diagramfile, strerror(errno));
      syslog( LOG_ERR, "%s: %s: %s\n", argv[0], diagramfile, strerror(errno));
      syslog( LOG_ERR, "chartsd shutting down!");
      run=0;
      return EXIT_FAILURE;
    }
    gdImagePng(image, png);
    fclose(png);
    gdImageDestroy(image);
    
    t_last = t_now;
    
    rcount++;
    sleep(interval);
  }
  syslog( LOG_NOTICE, "chartsd shutdown.\n");
  closelog();
  return EXIT_SUCCESS;
}