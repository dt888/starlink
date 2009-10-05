/*
*+
*  Name:
*     CALCNOISE

*  Purpose:
*     Calculate noise image

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     ADAM A-task

*  Invocation:
*     smurf_calcnoise( int *status );

*  Arguments:
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     This routine calculates the white noise on the array by performing
*     an FFT to generate a power spectrum and then extracting the
*     data between two frequency ranges.

*  Notes:
*     Transforming data loses the VARIANCE and QUALITY components.

*  ADAM Parameters:
*     FREQ = _REAL (Given)
*          Frequency range (Hz) to use to calculate the white noise [2,10]
*     IN = NDF (Read)
*          Input files to be transformed. Files from the same sequence
*          will be combined. Note that
*     MSG_FILTER = _CHAR (Read)
*          Control the verbosity of the application. Values can be
*          NONE (no messages), QUIET (minimal messages), NORMAL,
*          VERBOSE, DEBUG or ALL. [NORMAL]
*     OUT = NDF (Write)
*          Output files. Number of output files may differ from the
*          number of input files.
*     OUTFILES = LITERAL (Write)
*          The name of text file to create, in which to put the names of
*          all the output NDFs created by this application (one per
*          line). If a null (!) value is supplied no file is created. [!]

*  Related Applications:
*     SMURF: SC2CONCAT, SC2CLEAN, SC2FFT

*  Authors:
*     Tim Jenness (JAC, Hawaii)
*     {enter_new_authors_here}

*  History:
*     2009-09-24 (EC):
*        Initial version - based on sc2fft task
*     {enter_further_changes_here}

*  Copyright:
*     Copyright (C) 2009 Science and Technology Facilities Council.
*     All Rights Reserved.

*  Licence:
*     This program is free software; you can redistribute it and/or
*     modify it under the terms of the GNU General Public License as
*     published by the Free Software Foundation; either version 3 of
*     the License, or (at your option) any later version.
*
*     This program is distributed in the hope that it will be
*     useful, but WITHOUT ANY WARRANTY; without even the implied
*     warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
*     PURPOSE. See the GNU General Public License for more details.
*
*     You should have received a copy of the GNU General Public
*     License along with this program; if not, write to the Free
*     Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
*     MA 02111-1307, USA

*  Bugs:
*     {note_any_bugs_here}
*-
*/

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdio.h>

#include "star/ndg.h"
#include "star/grp.h"
#include "ndf.h"
#include "mers.h"
#include "par.h"
#include "prm_par.h"
#include "sae_par.h"
#include "msg_par.h"
#include "fftw3.h"

#include "smurf_par.h"
#include "libsmf/smf.h"
#include "smurflib.h"
#include "libsmf/smf_err.h"
#include "sc2da/sc2store.h"

#define FUNC_NAME "smurf_calcnoise"
#define TASK_NAME "CALCNOISE"

void smurf_sc2fft( int *status ) {

  smfArray *darks = NULL;   /* dark frames */
  Grp *fgrp = NULL;         /* Filtered group, no darks */
  size_t i=0;               /* Counter, index */
  smfData *idata=NULL;      /* Pointer to input smfData */
  Grp *igrp = NULL;         /* Input group of files */
  int inverse=0;            /* If set perform inverse transform */
  char fname[GRP__SZNAM+1]; /* Name of container file without suffix */
  smfData *odata=NULL;      /* Pointer to output smfData to be exported */
  Grp *ogrp = NULL;         /* Output group of files */
  size_t outsize;           /* Total number of NDF names in the output group */
  char *pname=NULL;         /* Poiner to fname */
  int polar=0;              /* Flag for FFT in polar coordinates */
  int power=0;              /* Flag for squaring amplitude coeffs */
  size_t size;              /* Number of files in input group */
  smfWorkForce *wf = NULL;  /* Pointer to a pool of worker threads */

  /* Main routine */
  ndfBegin();

  /* Find the number of cores/processors available and create a pool of
     threads of the same size. */
  wf = smf_create_workforce( smf_get_nthread( status ), status );

  /* Get input file(s) */
  kpg1Rgndf( "IN", 0, 1, "", &igrp, &size, status );

  /* Filter out darks */
  smf_find_darks( igrp, &fgrp, NULL, 1, SMF__NULL, &darks, status );

  /* input group is now the filtered group so we can use that and
     free the old input group */
  size = grpGrpsz( fgrp, status );
  grpDelet( &igrp, status);
  igrp = fgrp;
  fgrp = NULL;

  /* Get output file(s) */
  if( size > 0 ) {
    kpg1Wgndf( "OUT", igrp, size, size, "More output files required...",
               &ogrp, &outsize, status );
  } else {
    msgOutif(MSG__NORM, " ", TASK_NAME ": All supplied input frames were DARK,"
             " nothing to do", status );
  }

  /* Are we doing an inverse transform? */
  parGet0l( "INVERSE", &inverse, status );

  /* Are we using polar coordinates instead of cartesian for the FFT? */
  parGet0l( "POLAR", &polar, status );

  /* Are we going to assume amplitudes are squared? */
  parGet0l( "POWER", &power, status );

  /* If power is true, we must be in polar form */
  if( power && !polar) {
    msgOutif( MSG__NORM, " ", TASK_NAME
              ": power spectrum requested so setting POLAR=TRUE", status );
    polar = 1;
  }

  /* Loop over input files */
  for( i=1; (*status==SAI__OK)&&(i<=size); i++ ) {

    /* Open the file */
    smf_open_and_flatfield( igrp, NULL, i, darks, &idata, status );

    /* Check whether we need to transform the data at all */
    if( smf_isfft(idata,NULL,NULL,NULL,status) == inverse ) {

      /* If inverse transform, convert to cartesian representation first */
      if( inverse && polar ) {
        smf_fft_cart2pol( idata, 1, power, status );
      }

      /* Tranform the data */
      odata = smf_fft_data( wf, idata, inverse, status );
      smf_convert_bad( odata, status );

      if( inverse ) {
        /* If output is time-domain, ensure that it is ICD bolo-ordered */
        smf_dataOrder( odata, 1, status );
      } else if( polar ) {
        /* Store FFT of data in polar form */
        smf_fft_cart2pol( odata, 0, power, status );
      }

      /* Export the data to a new file */
      pname = fname;
      grpGet( ogrp, i, 1, &pname, GRP__SZNAM, status );
      smf_write_smfData( odata, NULL, NULL, fname, NDF__NOID, status );

      /* Free resources */
      smf_close_file( &odata, status );
    } else {
      msgOutif( MSG__NORM, " ",
                "Data are already transformed. No output will be produced",
                status );
    }

    /* Free resources */
    smf_close_file( &idata, status );
  }

  /* Write out the list of output NDF names, annulling the error if a null
     parameter value is supplied. */
  if( *status == SAI__OK ) {
    grpList( "OUTFILES", 0, 0, NULL, ogrp, status );
    if( *status == PAR__NULL ) errAnnul( status );
  }

  /* Tidy up after ourselves: release the resources used by the grp routines */
  grpDelet( &igrp, status);
  grpDelet( &ogrp, status);

  if( wf ) wf = smf_destroy_workforce( wf );

  ndfEnd( status );

  /* Ensure that FFTW doesn't have any used memory kicking around */
  fftw_cleanup();
}