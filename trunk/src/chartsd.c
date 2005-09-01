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
gdImagePtr 	image;
struct tm 	*zeit;
struct timeval t_now, t_last;
typedef void (*sighandler_t)(int);

FILE 				*png;
FILE 				*pnd;
FILE 				*abar;
cfg_t 			*cfg;

const char 	*program;
const char 	*Version						=	"0.5";

char 				*config_file 				=	"/etc/charts.d/temp.hda.conf";
char 				*sensor 						= NULL;
char 				*unit 							= NULL;
char 				*datafile 					= NULL;
char 				*diagramfile 				= NULL;
char 				*INOUT;
char 				starttime[20];
char 				stoptime[20];
char 				s[64];
char 				maxin[20];
char 				maxout[20];
char 				avgout[20];
char 				highout[20];
char 				lowout[20];
char				gridvalue[20];
char 				dia_title[256];
char				*LINE[8];


float 			XStep;
float 			YStep;
float				gridStep;
float 			total 							= 0.0;
float 			average 						= 0.0;
float 			highest 						= 0.0;
float 			lowest 							= 0.0;
double 			timedelta;

int 				interval						= 60;
int 				shown_intervals 		= 300;
int 				WIDTH 							= 600;
int 				HEIGHT 							= 250;
int 				maxvalue 						= 65;
int 				minvalue 						= 30;
int					show_maxvalue_line	=	1;
int					show_minvalue_line	= 1;
int					show_average_line		= 1;
int					show_highest_value	= 1;
int					show_lowest_value		= 1;
int					show_grid_lines			= 1;
int					show_legend					= 0;
int					legend_pos					= 2;
int					legend_width				= 120;
int 				run									=	1;
int 				LINES 							= 0;
int					COLS								= 0;
int 				rcount 							= 0;
int 				R 									= 0;
int 				G 									= 255;
int 				B 									= 0;
int 				ac 									= 0;
int 				ca 									= 0;
int					COLOR[7];
int 				opt;
int 				XPOS;
int 				YPOS;
int 				NULLX;
int 				NULLY;
int 				URSPRUNGX;
int 				URSPRUNGY;
int 				maxX;
int					minX;
int 				maxY;
int					minY;
int 				Xlength;
int 				Ylength;
int 				X1;
int 				X2;
int 				Y1;
int 				Y2;
int 				black;
int 				white;
int 				red;
int 				green;
int 				grey;
int					lightgrey;
int 				blue;
int 				cyan;
int 				magenta;
int 				gold;
int 				darkgreen;
int 				darkcyan;
int 				darkgold;
int 				linecolor;
int 				thiscolor;
int					darkgrey;
int 				i;
int 				j;
int					c;
int 				LINESTART;
int 				linestart;
int 				start;
int 				stop;
int 				diff;
int 				LINESTART;
int 				leglines;
int 				legheight;
int  				legwidth;
int  				leg_ursprungX;    
int  				leg_ursprungY;
int  				left;
int  				top;



/** function declaration */
int intvgl(const void *v1, const void *v2);
static sighandler_t handle_signal (int sig_nr, sighandler_t signalhandler);
static void start_daemon (const char *log_name, int facility);
void usage(int err);
void version(int err);
int fileExists (char * fileName);
int main(int argc, char *argv[]);



/** a helper function for qsort() */
int intvgl(const void *v1, const void *v2)
{
	return (*(int *)v1 - *(int *)v2);
}


/** signal handler */
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


/** this function starts the daemon process */
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


/** checks of a given file exists */
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


/** set the allowed command line arguments */
const struct option longopts[] = {
  { "config-file",  1, 0, 'c' },
  { "help",         0, 0, 'h' },
  { "version",      1, 0, 'v' },
  { 0, 0, 0, 0 }
};


/** prints the help screen */
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


/** prints the version screen */
void version(int err)
{
  printf("\nThis is %s %s\n", program, Version);
  fputs("Written by Alexander Mieland <dma147@linux-stats.org>\n", stdout);
  fputs("IRC: #archlinux, #archlinux.de, #linux-stats on irc.freenode.net\n\n", stdout);
  fputs("If you've found any bugs or have any questions,\nplease feel free to contact me\n\n", stdout);
  exit(err);
}


/** main function */
int main(int argc, char *argv[])
{
  
  /** get the name of this application */
  program = argv[0];
  
  
  /** set some default values for some variables */
  sensor = strdup("Temperature of harddisc (hda)");
  unit = strdup("C");
  datafile = strdup("/var/log/charts.hda.log");
  diagramfile = strdup("/tmp/charts.hda.png");
  LINE[0] = strdup("Line 1");
  LINE[1] = strdup("Line 2");
  LINE[2] = strdup("Line 3");
  LINE[3] = strdup("Line 4");
  LINE[4] = strdup("Line 5");
  LINE[5] = strdup("Line 6");
  LINE[6] = strdup("Line 7");
  LINE[7] = strdup("Line 8");
  
  /** check which cli arguments were given */
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
  
  
  /** check if the configuration file exists */
  if ( !fileExists(config_file) )
  {
    fprintf(stderr, "%s: %s: configuration file not found!\n", argv[0], config_file);
    syslog( LOG_ERR, "%s: %s: configuration file not found!\n", argv[0], config_file);
    syslog( LOG_ERR, "chartsd shutting down!");
    run=0;
    return EXIT_FAILURE;
  }
  
  
  /** this is some stuff from libconfuse to parse the configuration file */
  cfg_opt_t opts[] = {
    CFG_SIMPLE_STR("SENSOR", &sensor),
    CFG_SIMPLE_STR("UNIT", &unit),
    CFG_SIMPLE_STR("DATAFILE", &datafile),
    CFG_SIMPLE_STR("DIAGRAMFILE", &diagramfile),
    CFG_SIMPLE_STR("LINE_1", &LINE[0]),
    CFG_SIMPLE_STR("LINE_2", &LINE[1]),
    CFG_SIMPLE_STR("LINE_3", &LINE[2]),
    CFG_SIMPLE_STR("LINE_4", &LINE[3]),
    CFG_SIMPLE_STR("LINE_5", &LINE[4]),
    CFG_SIMPLE_STR("LINE_6", &LINE[5]),
    CFG_SIMPLE_STR("LINE_7", &LINE[6]),
    CFG_SIMPLE_STR("LINE_8", &LINE[7]),
    CFG_SIMPLE_INT("INTERVAL", &interval),
    CFG_SIMPLE_INT("SHOWN_INTERVALS", &shown_intervals),
    CFG_SIMPLE_INT("WIDTH", &WIDTH),
    CFG_SIMPLE_INT("HEIGHT", &HEIGHT),
    CFG_SIMPLE_INT("MAXVALUE", &maxvalue),
    CFG_SIMPLE_INT("MINVALUE", &minvalue),
    CFG_SIMPLE_INT("SHOW_MAXVALUE_LINE", &show_maxvalue_line),
    CFG_SIMPLE_INT("SHOW_MINVALUE_LINE", &show_minvalue_line),
    CFG_SIMPLE_INT("SHOW_AVERAGE_LINE", &show_average_line),
    CFG_SIMPLE_INT("SHOW_HIGHEST_VALUE", &show_highest_value),
    CFG_SIMPLE_INT("SHOW_LOWEST_VALUE", &show_lowest_value),
    CFG_SIMPLE_INT("SHOW_GRID_LINES", &show_grid_lines),
    CFG_SIMPLE_INT("SHOW_LEGEND", &show_legend),
    CFG_SIMPLE_INT("LEGEND_POSITION", &legend_pos),
    CFG_SIMPLE_INT("LEGEND_WIDTH", &legend_width),
    CFG_SIMPLE_INT("R", &R),
    CFG_SIMPLE_INT("G", &G),
    CFG_SIMPLE_INT("B", &B),
    CFG_END()
  };
  cfg = cfg_init(opts, 0);
  cfg_parse(cfg, config_file);
  cfg_free(cfg);
	
	
	/** check if the log-file exists */
  if ( !fileExists(datafile) )
  {
    fprintf(stderr, "%s: %s: data file not found!\n", argv[0], datafile);
    syslog( LOG_ERR, "%s: %s: data file not found!\n", argv[0], datafile);
    syslog( LOG_ERR, "chartsd shutting down!");
    run=0;
    return EXIT_FAILURE;
  }
  
	
	/** set the interval to 1 if it is smaller than 1 */
  if (interval < 1) 
    interval = 1;

  
  /** starting the daemon-process */
  start_daemon ("chartsd", LOG_LOCAL0);
  
  
  /** the main daemon loop */
  while (run)
  {
    
    /** tell syslog that we have started the daemon */
    if (rcount == 0)
      syslog( LOG_NOTICE, "chartsd started.\n");
    
    
    /** get the actual time of day */
    gettimeofday(&t_now, NULL);
    timedelta = (double)(t_now.tv_sec - t_last.tv_sec) + 
        (t_now.tv_usec - t_last.tv_usec)/1000000;
    
    
    /** open the log-file for reading */
    if ( !(abar = fopen(datafile, "r")) )
    {
      fprintf(stderr, "%s: %s: %s\n", argv[0], datafile, strerror(errno));
      syslog( LOG_ERR, "%s: %s: %s\n", argv[0], datafile, strerror(errno));
      syslog( LOG_ERR, "chartsd shutting down!");
      run=0;
      return EXIT_FAILURE;
    }
    
    
    /** count the lines of the log-file */
    LINES = 0;
    while ( (fgets(s,64,abar)) != NULL )      
      LINES++;
    LINES++;
    
    
    /** set the filepointer to the beginning of the log-file */
    fseek(abar, 0L, SEEK_SET);
    
    
    /** count the columns in the logfile */
    COLS = 0;
    while ( (c = fgetc(abar)) != EOF )
    {
    	if (c == '\n')
    		break;
    	if (c == '\t')
    		COLS++;
    }
		COLS++;
    
    
    /** define an array for the data */
    float DATA[LINES][COLS];
    
    
    /** set the filepointer to the beginning of the log-file */
    fseek(abar, 0L, SEEK_SET);
    
    
    /** read the log-file and give the data to the array */
    for (i=0; i<LINES; i++)
    {
      for (j=0; j<COLS; j++)
      {
        fscanf(abar,"%f",&DATA[i][j]);
      }
    }
    fclose(abar);
    
    
    /** the last line of the log-file usual contains only zeros */
    LINES = LINES - 1;
    
    
    /** set the beginning of the data, the start (we don't want the whole logfile in the diagram!) */
    if ((LINES-shown_intervals) <= 0)
      LINESTART = 0;
    else
      LINESTART = (LINES-shown_intervals);
    
    
    /** write only the needed data back to the *empty* logfile */
    /** delete logfile */
	 	if((remove(datafile)) < 0) 
	 	{
      fprintf(stderr, "%s: %s: %s\n", argv[0], datafile, strerror(errno));
      syslog( LOG_ERR, "%s: %s: %s\n", argv[0], datafile, strerror(errno));
      syslog( LOG_ERR, "chartsd shutting down!");
      run=0;
      return EXIT_FAILURE;
	 	}
	 	
    /** open logfile for rewriting the data */
    if ( !(abar = fopen(datafile, "a")) )
    {
      fprintf(stderr, "%s: %s: %s\n", argv[0], datafile, strerror(errno));
      syslog( LOG_ERR, "%s: %s: %s\n", argv[0], datafile, strerror(errno));
      syslog( LOG_ERR, "chartsd shutting down!");
      run=0;
      return EXIT_FAILURE;
    }
    linestart = (LINESTART - 50);
    if (linestart <= 0)
    	linestart = 0;
		for (i=linestart; i<LINES; i++)
		{
			char WLINE[35];
			sprintf(WLINE, "%d", (int)DATA[i][0]);
			for (j=1; j<COLS; j++)
			{
    		sprintf(WLINE, "%s\t%.4f", WLINE, DATA[i][j]);
			}
			sprintf(WLINE, "%s\n", WLINE);
			fwrite(WLINE, strlen(WLINE), 1, abar);
		}
		fclose(abar);
    
    
    /** set total counter to 0 */
    total = 0.0;
    
    
    /** we have to create a new data-array because the values are in a wrong direction */
    float VALUES[COLS][(LINES-LINESTART)];
    for (j=0; j<COLS; j++)
    {
      ac=0;
      for (i=LINESTART; i<LINES; i++)
      {
      	if (j == 1)
      	{
      		/** counts the total value */
      		total = (total + (float)DATA[i][j]);
      	}
        VALUES[j][ac] = (float)DATA[i][j];
        ac++;
      }
    }
    
    
    /** calculate the average of the given data */
    average = (total / ac);
    
    
    /** some stuff to get the highest and the lowest value */
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
    
    
    /** get the starting and the stopping time */
    start = (int)VALUES[0][0];
    stop = (int)VALUES[0][(ac-1)];
    diff = (stop - start);
    zeit = localtime( (time_t*)&start );
    strftime( starttime, 20, "%I:%M:%S%p", zeit);
    zeit = localtime( (time_t*)&stop );
    strftime( stoptime, 20, "%I:%M:%S%p", zeit);    
    
    
    
    /** **************   START OF DIAGRAM GENERATION ************* */
    Xlength = (WIDTH - 20) - 45;
    Ylength = (HEIGHT - 20) - 20;
    maxY = maxvalue + (maxvalue / 3);
    minY = 0;
    maxX = shown_intervals;
    minX = 0;
    
    XStep = ( (float)Xlength / (float)maxX );
    YStep = ( (float)Ylength / (float)maxY );
    
    sprintf(maxin, "%.2f %s", (float)maxvalue, unit);
    sprintf(maxout, "%.2f %s", (float)minvalue, unit);
    sprintf(avgout, "%.2f %s", average, unit);
    sprintf(highout, "Highest: %.2f %s", highest, unit);
    sprintf(lowout, "Lowest: %.2f %s", lowest, unit);
    
    
    /** create a new picture */
    if( !(image = gdImageCreate(WIDTH, HEIGHT)) )
    {
      fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
      syslog( LOG_ERR, "%s: %s\n", argv[0], strerror(errno));
      syslog( LOG_ERR, "chartsd shutting down!");
      run=0;
      return EXIT_FAILURE;
    }
    
    
    /** set some colors */
    white 			= gdImageColorAllocate(image, 255, 255, 255);
    black 			= gdImageColorAllocate(image, 0, 0, 0);
    red   			= gdImageColorAllocate(image, 255, 0, 0);
    green 			= gdImageColorAllocate(image, 0, 255, 0);
    darkgrey		= gdImageColorAllocate(image, 90, 90, 90);
    grey  			= gdImageColorAllocate(image, 127, 127, 127);
    lightgrey  	= gdImageColorAllocate(image, 230, 230, 230);
    blue  			= gdImageColorAllocate(image, 0, 0, 255);
    cyan  			= gdImageColorAllocate(image, 0, 255, 255);
    darkcyan		= gdImageColorAllocate(image, 0, 205, 205);
    magenta			= gdImageColorAllocate(image, 255, 0, 255);
    darkgreen 	= gdImageColorAllocate(image, 0, 205, 0);
    gold				= gdImageColorAllocate(image, 255, 215, 0);
    darkgold 		= gdImageColorAllocate(image, 205, 173, 0);
    
    
    /** set an array with some default colors */
    COLOR[0] = darkgreen;
    COLOR[1] = darkcyan;
    COLOR[2] = darkgold;
    COLOR[3] = green;
    COLOR[4] = magenta;
    COLOR[5] = cyan;
    COLOR[6] = gold;
    
    
    /** set the background to white */
    image->transparent = white;
    
    
    /** Title of diagram */
    sprintf(dia_title, "%s    (%s - %s)", sensor, starttime, stoptime);
    gdImageString(image, gdFontMediumBold, 2, 2, dia_title, black);
    
    
    /** lightgrey grid lines */
    if (show_grid_lines == 1)
    {
    	gridStep = (maxY / 8);
    	for (i=1; i<=8; i++)
    	{
				char gridvalue[20];
    		sprintf(gridvalue, "%.2f %s", (gridStep * i), unit);
    		gdImageLine (image, (NULLX+55), ((NULLY - 20)-(YStep * (gridStep * i))), (WIDTH-5), ((NULLY - 20)-(YStep * (gridStep * i))), lightgrey);
	    	gdImageString(image, gdFontSmall, (NULLX+2), (((NULLY - 20)-(YStep * (gridStep * i))) - 5), gridvalue, lightgrey);
    	}
    }
    
    
    /** axes of coordinates */
    gdImageLine (image, (NULLX+60), (NULLY-15), (NULLX+60), 15, black);
    gdImageLine (image, (NULLX+55), (NULLY-20), (WIDTH-5), (NULLY-20), black);
    
    
    /** max/min/average lines */
    if (show_maxvalue_line == 1)
    	gdImageLine (image, (NULLX+55), (URSPRUNGY - (YStep * maxvalue)), (WIDTH-5), (URSPRUNGY - (YStep * maxvalue)), red);
    if (show_minvalue_line == 1)
    	gdImageLine (image, (NULLX+55), (URSPRUNGY - (YStep * minvalue)), (WIDTH-5), (URSPRUNGY - (YStep * minvalue)), blue);
    if (show_average_line == 1)
    	gdImageDashedLine (image, (NULLX+55), (URSPRUNGY - (YStep * average)), (WIDTH-5), (URSPRUNGY - (YStep * average)), green);
    
    
    /** max/min/average values */
    if (show_maxvalue_line == 1)
    	gdImageString(image, gdFontSmall, (NULLX+2), (URSPRUNGY - (YStep * maxvalue) - 5), maxin, red);
    if (show_minvalue_line == 1)
  	  gdImageString(image, gdFontSmall, (NULLX+2), (URSPRUNGY - (YStep * minvalue) - 5), maxout, blue);
    if (show_average_line == 1)
	    gdImageString(image, gdFontSmall, (NULLX+2), (URSPRUNGY - (YStep * average) - 5), avgout, green);
    
    
    /** highest/lowest display */
    if (show_highest_value == 1)
    	gdImageString(image, gdFontSmall, (NULLX+15), (NULLY-13), highout, red);
    if (show_lowest_value == 1)
    	gdImageString(image, gdFontSmall, (NULLX+150), (NULLY-13), lowout, blue);
    

		if (show_legend == 1)
    {
    	leglines = (COLS-1);
    	legheight = ((leglines * 14));
    	legwidth = legend_width;
    	
    	if (legend_pos == 1)
    	{
    		leg_ursprungX = (NULLX+70);
    		leg_ursprungY = 30;
    	}
    	else if (legend_pos == 2)
    	{
    		leg_ursprungX = ((WIDTH-legwidth)-20);
    		leg_ursprungY = 20;
    	}
    	else if (legend_pos == 3)
    	{
    		leg_ursprungX = (NULLX+70);
    		leg_ursprungY = ((NULLY-30)-legheight);
    	}
    	else if (legend_pos == 4)
    	{
    		leg_ursprungX = ((WIDTH-legwidth)-20);
    		leg_ursprungY = ((NULLY-30)-legheight);
    	}
			X1 = leg_ursprungX;
			Y1 = leg_ursprungY;
			X2 = (leg_ursprungX + legwidth);
			Y2 = (leg_ursprungY + legheight);
    	
			gdImageFilledRectangle (image, X1+1, Y1-1, X2+1, Y2-1, darkgrey);
			gdImageFilledRectangle (image, X1+2, Y1-2, X2+2, Y2-2, darkgrey);
    	
    	
			gdImageFilledRectangle (image, X1, Y1, X2, Y2, lightgrey);
			gdImageRectangle (image, X1, Y1, X2, Y2, black);
			left = X1+10;
			top = Y1;
			for (j=1; j<COLS; j++)
			{
    		thiscolor = COLOR[(j-1)];
				left = X1+10;
				top = top + 12;
				gdImageFilledRectangle (image, left, (top-5), left+5, top, thiscolor);
				gdImageRectangle (image, left, (top-5), left+5, top, black);
				left = left+15;
				gdImageString(image, gdFontSmall, left, (top-10), LINE[(j-1)], thiscolor);
			}
    }
    
    for (j=1; j<COLS; j++)
    {
    	thiscolor = COLOR[(j-1)];
      for (i=0; i<(LINES-LINESTART); i++)
      {
        X1 = URSPRUNGX + (XStep * i);
        Y1 = URSPRUNGY - (YStep * VALUES[j][i]);
        X2 = URSPRUNGX + (XStep * (i+1));
        Y2 = URSPRUNGY - (YStep * VALUES[j][(i+1)]);
        gdImageLine (image, X1, Y1, X2, Y2, thiscolor);
      }
    }
    /** **************   END OF DIAGRAM GENERATION ************* */
    
    
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
