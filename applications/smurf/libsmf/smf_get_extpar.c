/*
*+
*  Name:
*     smf_get_extpar

*  Purpose:
*     Obtain values of extinction correction parameters from a keymap

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     C function

*  Invocation:
*     smf_get_extpar( AstKeyMap *keymap, smf_tausrc *tausrc,
*                     smf_extmeth *extmeth, int *status )

*  Arguments:
*     keymap = AstKeyMap* (Given)
*        keymap containing parameters
*     tausrc = smf_tausrc* (Returned)
*        Source of optical depth data 
*     extmeth = smf_extmeth* (Returned)
*        Method to use for airmass calculation 
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:

*     This function parses string representations of the tau source
*      (TAUSOURCE) and method (TAUMETHOD) for extinction correction,
*      stored in an astKeyMap, and returns enumerated types. For
*      further descriptions of tausrc and extmeth see documentation
*      for the EXTINCTION task. If a key cannot be located, the relevant
*      NULL values are returned for the two enumerated types.
*     
*  Authors:
*     EC: Edward Chapin (UBC)
*     {enter_new_authors_here}

*  History:
*     2009-09-29 (EC):
*        Initial version factored out of smurf_extinction
*     {enter_further_changes_here}

*  Notes:

*  Copyright:
*     Copyright (C) 2009 University of British Columbia
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

#include <stdio.h>
#include <ctype.h>

/* Starlink includes */
#include "ast.h"
#include "mers.h"
#include "ndf.h"
#include "sae_par.h"
#include "star/ndg.h"
#include "prm_par.h"

#include "par_par.h"

/* SMURF includes */
#include "libsmf/smf.h"
#include "libsmf/smf_err.h"

#define FUNC_NAME "smf_get_extpar"

void smf_get_extpar( AstKeyMap *keymap, smf_tausrc *tausrc,
                     smf_extmeth *extmeth, int *status ) {

  const char *tempstr = NULL;


  /* Main routine */
  if (*status != SAI__OK) return;

  /* Obtain parameters from keymap when non-NULL pointers given */

  if( tausrc ) {
    if( astMapGet0C( keymap, "TAUSRC", &tempstr ) ) {
      
      switch( toupper(tempstr[0]) ) {
      case 'C':
        *tausrc = SMF__TAUSRC_CSOTAU;
        break;
      case 'F':
        *tausrc = SMF__TAUSRC_TAU;
        break;
      case 'W':
        *tausrc = SMF__TAUSRC_WVMRAW;
        break;
      default:
        *tausrc = SMF__TAUSRC_NULL;
      }
    } else {
      *tausrc = SMF__TAUSRC_NULL;
    }
  }

  if( extmeth ) {
    if( astMapGet0C( keymap, "TAUMETHOD", &tempstr ) ) {
      switch( toupper(tempstr[0]) ) {
      case 'A':
        *extmeth = SMF__EXTMETH_ADAPT;
        break;
      case 'Q':
        *extmeth = SMF__EXTMETH_SINGLE;
        break;
      case 'F':
        *extmeth = SMF__EXTMETH_FULL;
        break;
      default:
        *extmeth = SMF__EXTMETH_NONE;
      } 
    } else {
      *extmeth = SMF__EXTMETH_NONE;
    }
  }  
}