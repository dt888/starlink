/*
*+
*  Name:
*     smf_jsatilexy2i

*  Purpose:
*     Convert the (x,y) indices of a sky tile into a scalar tile index.

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     C function

*  Invocation:
*     int smf_jsatilexy2i( int xt, int yt, smfJSATiling *skytiling, int *status )

*  Arguments:
*     xt = int (Given)
*        The zero-based index of the tile in the X (RA) direction.
*     yt = int (Given)
*        The zero-based index of the tile in the Y (Dec) direction.
*     skytiling = smfJSATiling * (Given)
*        Pointer to a structure holding parameters describing the tiling
*        scheme used for the required JCMT instrument, as returned by
*        function smf_jsatiling.
*     status = int * (Given)
*        Pointer to the inherited status variable.

*  Returned Value:
*     The zero-based scalar tile index of the requested tile. VAL__BADI is
*     returned if no tile has the requested X adn Y offsets.

*  Description:
*     This function returns a scalar integer index for a tile which is
*     offfset by given numbers of tiles along the X (RA) and Y (Dec) axes
*     away from the bottom left tile in the all-sky map.

*  Authors:
*     DSB: David S Berry (JAC, UCLan)
*     {enter_new_authors_here}

*  History:
*     11-MAR-2011 (DSB):
*        Initial version.
*     {enter_further_changes_here}

*  Copyright:
*     Copyright (C) 2011 Science & Technology Facilities Council.
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
#include "sae_par.h"

/* SMURF includes */
#include "libsmf/smf.h"




#include "libsmf/jsatiles.h"   /* Move this to smf_typ.h and smf.h when done */




int smf_jsatilexy2i( int xt, int yt, smfJSATiling *skytiling, int *status ){

/* Local Variables: */
   int itile;
   int fi;
   int ty;
   int tx;
   int fy;
   int fx;

/* Initialise the returned tile index. */
   itile = VAL__BADI;

/* Check inherited status */
   if( *status != SAI__OK ) return itile;

/* The facet with the lowest tile indices is split between the bottom
   left and top right corners of the grid. Fill the bottom left half of
   the bottom left facet with bad values. */
   if( yt < skytiling->ntpf - 1 && xt < skytiling->ntpf - 1 - yt ) {
      itile = VAL__BADI;

/* Fill the top right half of thetop right facet with bad values. */
   } else if( yt >= 9*skytiling->ntpf - 1 ||
              xt >= 9*skytiling->ntpf - 1 - yt ) {
      itile = VAL__BADI;

/* Now handle the other parts of the grid. */
   } else {

/* Get the (x,y) indices of the facet containing the tile. */
      fx = xt/skytiling->ntpf;
      fy = yt/skytiling->ntpf;

/* The top right facet is a copy of the bottom left facet. */
      if( fx == 4 && fy == 4 ) {
         fx = fy = 0;
         xt -= 4*skytiling->ntpf;
         yt -= 4*skytiling->ntpf;
      }

/* Check they are within bounds. */
      if( fx >= 0 && fx < 5 &&
          fy >= 0 && fy < 5 &&
          fx >= fy - 1 && fx <= fy + 1 ) {

/* Get the scalar zero-based index of the facet within the collection of
   12 facets. */
         fi = 2*fy + fx;

/* Get the scalar zero-based index of the first tile within the facet. */
         itile = fi*skytiling->ntpf*skytiling->ntpf;

/* Get the (x,y) offsets of the tile into the facet. */
         tx = xt - fx*skytiling->ntpf;
         ty = yt - fy*skytiling->ntpf;

/* Get the scalar index of the tile within the facet. Add this index onto
   the index of the first tile in the facet, to get the index of the
   required tile. */
         itile += ty*skytiling->ntpf + tx;
      }
   }

/* Return the tile index. */
   return itile;
}

