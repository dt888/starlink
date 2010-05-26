/*
*+
*  Name:
*     smf_create_tswcs

*  Purpose:
*     Create a time series WCS given a smfHead

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     smf_create_tswcs( const smfHead *hdr, AstFrameSet **frameset, int *status )

*  Arguments:
*     hdr = smfHead* (Given)
*        The supplied smfHead must have allState, telpos, nframes, instap, and
*        a complete fitshdr populated.
*     fset = AstFrameSet **fset (Returned)
*        Newly constructed frameset for time-series data cube.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Create a time-series WCS for a data cube using information
*     stored in the FITS header and JCMTState. This is a wrapper for
*     sc2store_timeWcs.

*  Notes:

*  Authors:
*     Edward Chapin (UBC)
*     {enter_new_authors_here}

*  History:
*     2010-05-19 (EC)
*        Initial version -- based on timeWcs() in sc2store.c
*     {enter_further_changes_here}

*  Copyright:
*     Copyright (C) 2010 University of British Columbia.
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

/* Starlink includes */
#include "mers.h"
#include "ndf.h"
#include "sae_par.h"
#include "star/atl.h"
#include "prm_par.h"
#include "par_par.h"

/* SMURF includes */
#include "libsmf/smf.h"
#include "sc2da/sc2store_par.h"
#include "sc2da/sc2store_sys.h"
#include "sc2da/sc2store.h"

#define FUNC_NAME "smf_create_tswcs"

void smf_create_tswcs( smfHead *hdr, AstFrameSet **frameset, int *status ){

  /* Local Variables */
  JCMTState *allState=NULL;     /* Full array of JCMTState for time series */
  double dut1;                  /* UT1-UTC (seconds) */
  int i;                        /* loop counter */
  double *instap=NULL;          /* pointer to 2-element instrument aperture */
  int ntime;                    /* number of time slices */
  int subnum;                   /* Subarray number */
  SC2STORETelpar telpar;        /* Additional telescope information */
  double *telpos=NULL;          /* telescope position pointer */
  double *times=NULL;           /* pointer to a time array */

  if( *status != SAI__OK ) return;

  if( !frameset ) {
    *status = SAI__ERROR;
    errRep( "", FUNC_NAME ": NULL reference to frameset pointer supplied",
            status );
    return;
  }

  *frameset = NULL;

  if( !hdr ) {
    *status = SAI__ERROR;
    errRep( "", FUNC_NAME ": NULL smfHead supplied", status );
    return;
  }

  /* Obtain everything we need from the header */
  allState = hdr->allState;
  instap = hdr->instap;
  ntime = (int) hdr->nframes;
  telpos = hdr->telpos;

  if( !allState || !telpos || !ntime || !instap || !hdr->fitshdr ) {
    *status = SAI__ERROR;
    errRep( "", FUNC_NAME ": Incomplete smfHead supplied", status );
    return;
  }

  if( astGetFitsF( hdr->fitshdr, "DUT1", &dut1 ) ) {
    dut1 *= SPD;
  } else {
    dut1 = 0.0;
  }

  if( !astOK ) {
    *status = SAI__ERROR;
    errRep( "", FUNC_NAME ": AST error reading DUT1 from smfHead", status );
    return;
  }

  smf_find_subarray( hdr, NULL, 0, &subnum, status );

  /* copy RTS_END values into times, populate telpar, and call sc2ast_timeWcs */
  times = astCalloc( ntime, sizeof(*times), 1 );

  if( *status == SAI__OK ) {
    telpar.dut1 = dut1;
    telpar.longdeg = -telpos[0];
    telpar.latdeg = telpos[1];
    telpar.instap_x = instap[0];
    telpar.instap_y = instap[1];

    for( i=0; i<ntime; i++ ) {
      times[i] = allState[i].rts_end;
    }

    *frameset = sc2store_timeWcs( subnum, ntime, &telpar, times, status );

    /* Clear the time zone information which is irrelevant and we don't know it*/
    if( *status == SAI__OK ) {
      astClear( *frameset, "LToffset" );
    }
  }
  /* Clean up */
  if( times ) times = astFree( times );

}
