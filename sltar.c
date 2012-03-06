/* sltar - suckless tar
 * Copyright (C) <2007> Enno boland <g s01 de>
 * Copyright (C) <2012> Alex Kozadaev <bsdard yahoo com>
 *
 * See LICENSE for further informations
 */
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

/*
 * TODO:
 * 1. add archive creation capabilities
 * 2. fix the extraction bug when the archive does not directory block.
 * 3. suckless has refused the gzip/bzip2, although it's listed on their website. 
 *    Since it's done already - I leave it for now.
 */

#define C		 (1   )	/* create */
#define X		 (1<<1)	/* extract */
#define T		 (1<<2)	/* test */
#define V		 (1<<3)	/* verbose */
#define F		 (1<<4)	/* file */
#define Z		 (1<<5)	/* gzip/ungzip */
#define J		 (1<<6)	/* bzip/bunzip2 */

#define GUZ	 "gunzip -c "
#define BUZ	 "bunzip2 -c "

enum Header {
	MODE = 100, UID = 108, GID = 116, SIZE = 124, MTIME = 136,
	TYPE = 156, LINK = 157, MAJ = 329, MIN = 337, END = 512
};

static int /* create archive */
c(char a, int fin_idx,
  int argc, char **flist) /* to be revised */
{
	fputs("Creating tars does not work yet\n", stderr);
	return EXIT_FAILURE;
}

static int /* extract */
xt(char a, char *fin_name)
{
	int l;
	char b[END],fname[101],lname[101];
	FILE *fin = NULL;		 /* file handle for reading */
	FILE *fout = NULL;		 /* file handle for writing */
	char cmd[120] = "\0";

	/* TODO: review/rewrite the code */
	if (a & F) /* reading flags */
		if (a & Z || a & J) { /* gzip/bzip2 - file */
			strncat(cmd, (a & Z) ? GUZ : BUZ, sizeof(cmd));
			strncat(cmd, fin_name, sizeof(fin_name));
			if (!(fin = popen(cmd, "r"))) {
				perror(fin_name);
				return EXIT_FAILURE;
			}
		}
		else {	/* tar file */
			if (!(fin = fopen(fin_name, "r"))) {
				perror(fin_name);
				return EXIT_FAILURE;
			}
		}
	else 
		if (a & Z || a & J) { /* gzip/bzip2 - stdin */
			strncat(cmd, (a & Z) ? GUZ : BUZ, sizeof(cmd));
			if (!(fin = popen(cmd, "r"))) {
				perror(fin_name);
				return EXIT_FAILURE;
			}
		}
		else /* tar - stdin */
			fin = stdin;

	for(lname[100] = fname[100] = l = 0; fread(b,END,1,fin); l -= END)
		if(l <= 0) {
			if(*b == '\0')
				break;
			memcpy(fname,b,100);
			memcpy(lname,b + LINK,100);
			l = strtoull(b + SIZE,0,8) + END;
			if(a & T) { /* test */
				puts(fname);
				continue;
			}
			if(fout) {
				fclose(fout);
				fout = 0;
			}
			unlink(fname);

			if (a & V) /* verbose */
				puts(fname);

			switch(b[TYPE]) {
			case '0': /* file */
				if(!(fout = fopen(fname,"w")) || chmod(fname,strtoul(b + MODE,0,8)))
					perror(fname);
				break;
			case '1': /* hardlink */
				if(!link(lname,fname))
					perror(fname);
				break;
			case '2': /* symlink */
				if(!symlink(lname,fname))
					perror(fname);
				break;
			case '5': /* directory */
				if(mkdir(fname,(mode_t) strtoull(b + MODE,0,8)))
					perror(fname);
				break;
			case '3': /* char device */
			case '4': /* block device */
				if(mknod(fname, (b[TYPE] == '3' ? S_IFCHR : S_IFBLK) | strtoul(b + MODE,0,8),
						 makedev(strtoul(b + MAJ,0,8),
							 strtoul(b + MIN,0,8))))
					perror(fname);
				break;
			case '6': /* fifo */
				if(mknod(fname, S_IFIFO | strtoul(b + MODE,0,8), 0))
					perror(fname);
				break;
			default:
				fprintf(stderr,"not supported filetype %c\n",b[TYPE]);
			}
			if(getuid() == 0 && chown(fname, strtoul(b + UID,0,8),strtoul(b + GID,0,8)))
				perror(fname);
		}
		else if(a & X && fout && !fwrite(b,l > 512 ? END : l,1,fout)) {
			perror(fname);
			break;
		}

	if (fout)
		fclose(fout);
	if (a & F && fin_name && fin) {
		if (a & Z || a & J)
			pclose(fin);
		else
			fclose(fin);
	}

	return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
	int i;
	char a = 0;

	if (argc > 1) 
		for(i = 0; i < strlen(argv[1]); i++)
			switch(argv[1][i]) {
			case 'c': a += C; break;	/* create */		/* TODO: Needs to be implemented */
			case 'x': a += X; break;	/* extract */
			case 't': a += T; break;	/* test */
			case 'v': a += V; break;	/* verbose */		/* TODO: to be implemented in c(...) */
			case 'f': a += F; 
					  if (!argv[2]) {
						  fputs("error: invalid arguments", stderr);
						  return EXIT_FAILURE;
					  }
					  break;      	/* file */
			case 'z': a += Z; break;	/* gzip/gunzip */	/* TODO: GZIP c(...): to be implemented */
			case 'j': a += J; break;	/* bzip2/bunzip2 */	/* TODO: BZIP2 c(...): to be implemented */
			default:
					  fputs("error: unknown option. exiting...\n", stderr);
					  return EXIT_FAILURE;
			}

	if (a & X || a & T)
		return xt(a, argv[2]);
	else if (a & C)
		return c(a, 2, argc, argv);
	else 
		fputs("sltar " VERSION " - suckless tar\nusage: sltar {ctx}[fjvz] <filename>\n", stderr);

	return EXIT_SUCCESS;
}
/* EOF */

