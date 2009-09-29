/*
 *
 *   Copyright (c) International Business Machines  Corp., 2002
 *
 *   This program is free software;  you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY;  without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 *   the GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program;  if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/* ported from SPIE, section2/filesuite/stream3.c, by Airong Zhang */

/*======================================================================
	=================== TESTPLAN SEGMENT ===================
>KEYS:  < fseek() ftell()
>WHAT:  < 1) Ensure ftell reports the correct current byte offset.
>HOW:   < 1) Open a file, write to it, reposition the file pointer and
	     check it.
>BUGS:  <
======================================================================*/
#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "test.h"
#include "usctest.h"

char *TCID = "stream03";
int TST_TOTAL = 1;
extern int Tst_count;
int     local_flag;

#define PASSED 1
#define FAILED 0

char progname[] = "stream03()" ;
char tempfile1[40]="";
long ftell();

/*--------------------------------------------------------------------*/
int main(int ac, char *av[])
{
	FILE *stream;
	char buf[30];
	char *junk="abcdefghijklmnopqrstuvwxyz";
	long pos;
	off_t opos;
	int lc;                 /* loop counter */
        char *msg;              /* message returned from parse_opts */

         /*
          * parse standard options
          */
        if ((msg = parse_opts(ac, av, (option_t *)NULL, NULL)) != (char *)NULL){
                         tst_resm(TBROK, "OPTION PARSING ERROR - %s", msg);
                 tst_exit();
                 /*NOTREACHED*/
        }

        local_flag = PASSED;
	tst_tmpdir();

	for (lc = 0; TEST_LOOPING(lc); lc++) {

		sprintf(tempfile1, "stream03.%d", getpid());
	/*--------------------------------------------------------------------*/
	//block0:
		if(creat(tempfile1,0666) < 0) {
			tst_resm(TBROK,"\tcreat failed\n");
			tst_exit();
		}
		if((stream=fopen(tempfile1,"a+")) == NULL) {
			tst_resm(TBROK,"\tfopen a+ failed\n");
			tst_exit();
		}
		/* make sure offset of zero at start */
		pos=ftell(stream);
		if ( pos != 0 ) {
			tst_resm(TFAIL,"file pointer descrepancy 1");
			local_flag = FAILED;
		}
		/* write something and check */
		if(fwrite(junk,sizeof(*junk),strlen(junk),stream) == 0) {
			tst_resm(TFAIL,"\tfwrite failed\n");
			tst_exit();
		}
		pos=ftell(stream);
		if ((size_t)pos != strlen(junk) ) {
			tst_resm(TFAIL, "strlen(junk):file pointer descrepancy 2");
			local_flag = FAILED;
		}
		/* rewind and check */
		rewind(stream);
		pos=ftell(stream);
		if ( pos != 0 ) {
			tst_resm(TFAIL,0,"file pointer descrepancy 3");
			local_flag = FAILED;
		}
		/* seek from current position and then check */
		if (fseek(stream,strlen(junk),1) != 0) {
			tst_resm(TFAIL,"\tfseek failed\n");
			tst_exit();
		}
		pos=ftell(stream);
		if ((size_t)pos != strlen(junk) ) {
			tst_resm(TFAIL,"strlen(junk),file pointer descrepancy 4");
			local_flag = FAILED;
		}
		/* seek from end of file and then check */
		if (fseek(stream,0,2) != 0) {
			tst_resm(TFAIL,"\tfseek failed\n");
			tst_exit();
		}
		pos=ftell(stream);
		if ((size_t)pos != strlen(junk) ) {
			tst_resm(TFAIL,"strlen(junk),file pointer descrepancy 5");
			local_flag = FAILED;
		}
		/* rewind with seek and then check */
		if (fseek(stream,0,0) != 0) {
			tst_resm(TFAIL,"\tfseek failed\n");
			tst_exit();
		}
		pos=ftell(stream);
		if (pos != 0 ) {
			tst_resm(TFAIL,"file pointer descrepancy 6");
			local_flag = FAILED;
		}

		/* read till EOF, do getc and then check ftell */
		while (fgets (buf, sizeof(buf), stream));
		pos=ftell(stream);
		(void) getc(stream);
		pos=ftell(stream);
		if ((size_t)pos != strlen(junk) ) {
			tst_resm(TFAIL,"strlen(junk),file pointer descrepancy 7");
			local_flag = FAILED;
		}
		fclose(stream);
		if (local_flag == PASSED) {
                        tst_resm(TPASS, "Test passed in block0.\n");
                } else {
                        tst_resm(TFAIL, "Test failed in block0.\n");
                }

                local_flag = PASSED;

		unlink(tempfile1);
	/*--------------------------------------------------------------------*/
	//block1:
		if(creat(tempfile1,0666) < 0) {
			tst_resm(TFAIL,"\tcreat failed\n");
			tst_exit();
		}
		if((stream=fopen(tempfile1,"a+")) == NULL) {
			tst_resm(TFAIL,"\tfopen a+ failed\n");
			tst_exit();
		}
		/* make sure offset of zero at start */
		opos=ftello(stream);
		if ( opos != 0 ) {
			tst_resm(TFAIL,"file pointer descrepancy 1");
			local_flag = FAILED;
		}
		/* write something and check */
		if(fwrite(junk,sizeof(*junk),strlen(junk),stream) == 0) {
			tst_resm(TFAIL,"\tfwrite failed\n");
			tst_exit();
		}
		opos=ftello(stream);
		if ((size_t)opos != strlen(junk) ) {
			tst_resm(TFAIL,"strlen(junk),file pointer descrepancy 2");
			local_flag = FAILED;
		}
		/* rewind and check */
		rewind(stream);
		opos=ftello(stream);
		if ( opos != 0 ) {
			tst_resm(TFAIL,"file pointer descrepancy 3");
			local_flag = FAILED;
		}
		/* seek from current position and then check */
		if (fseeko(stream, (off_t)strlen(junk), 1) != 0) {
			tst_resm(TFAIL,"\tfseeko failed\n");
			tst_exit();
		}
		opos=ftello(stream);
		if ((size_t)opos != strlen(junk) ) {
			tst_resm(TFAIL,"strlen(junk),file pointer descrepancy 4");
			local_flag = FAILED;
		}
		/* seek from end of file and then check */
		if (fseeko(stream, (off_t)0, 2) != 0) {
			tst_resm(TFAIL,"\tfseeko failed\n");
			tst_exit();
		}
		opos=ftello(stream);
		if ((size_t)opos != strlen(junk) ) {
			tst_resm(TFAIL,"strlen(junk),file pointer descrepancy 5");
			local_flag = FAILED;
		}
		/* rewind with seek and then check */
		if (fseeko(stream, (off_t)0, 0) != 0) {
			tst_resm(TFAIL,"\tfseeko failed\n");
			tst_exit();
		}
		opos=ftello(stream);
		if (opos != 0 ) {
			tst_resm(TFAIL,"file pointer descrepancy 6");
			local_flag = FAILED;
		}

		/* read till EOF, do getc and then check ftello */
		while (fgets (buf, sizeof(buf), stream));
		opos=ftello(stream);
		(void) getc(stream);
		opos=ftello(stream);
		if ((size_t)opos != strlen(junk) ) {
			tst_resm(TFAIL,"strlen(junk),file pointer descrepancy 7");
			local_flag = FAILED;
		}
		fclose(stream);
		if (local_flag == PASSED) {
                        tst_resm(TPASS, "Test passed in block1.\n");
                } else {
                        tst_resm(TFAIL, "Test failed in block1.\n");
                }
	/*--------------------------------------------------------------------*/
		unlink(tempfile1);
	} /* end for */
	tst_rmdir();
	tst_exit();
	return(0);
}
