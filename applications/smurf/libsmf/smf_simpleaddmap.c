/*
*+
*  Name:
*     smf_simpleaddbinmap

*  Purpose:
*     Weighted addition of two maps with the same dimensions.

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     C function

*  Invocation:
*     smf_simpleaddmap( double *map1, double *mapweight1, 
*                       unsigned int *hitsmap1, double *mapvar1, 
*                       double *map2, double *mapweight2, 
*                       unsigned int *hitsmap2, double *mapvar2, int msize, 
*                       int *status ) {

*  Arguments:
*     map1 = double* (Given and Returned)
*        The first map 
*     mapweight1 = double* (Given and Returned)
*        Relative weighting for each pixel in map1
*     hitsmap1 = unsigned int* (Given and Returned)
*        Number of samples that land in map1 pixels
*     mapvar1 = double* (Given and Returned)
*        Variance of each pixel in map1
*     map2 = double* (Given)
*        The second map 
*     mapweight2 = double* (Given)
*        Relative weighting for each pixel in map2
*     hitsmap2 = unsigned int* (Given)
*        Number of samples that land in map2 pixels
*     mapvar2 = double* (Given and Returned)
*        Variance of each pixel in map2
*     msize = int (Given)
*        Number of pixels in the maps
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     This routine adds all of the pixels from map2 to map1 using inverse
*     variance weighting. 
*     
*  Authors:
*     Edward Chapin (UBC)
*     {enter_new_authors_here}

*  History:
*     2008-04-16 (EC):
*        Initial version.
*     {enter_further_changes_here}

*  Notes:
*     There is an assumption that variance is consistent with weight. In this
*     routine the variance map is set to 1/(mapweight1 + mapweight2).

*  Copyright:
*     Copyright (C) 2005-2006 Particle Physics and Astronomy Research Council.
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
#include <string.h>

/* Starlink includes */
#include "ast.h"
#include "mers.h"
#include "sae_par.h"
#include "star/ndg.h"
#include "prm_par.h"

/* SMURF includes */
#include "libsmf/smf.h"

#define FUNC_NAME "smf_simpleaddmap"

void smf_simpleaddmap( double *map1, double *mapweight1, 
                       unsigned int *hitsmap1, double *mapvar1, 
                       double *map2, double *mapweight2, 
                       unsigned int *hitsmap2, double *mapvar2, int msize, 
                       int *status ) {

  /* Local Variables */
  dim_t i;                   /* Loop counter */
  
  /* Main routine */
  if (*status != SAI__OK) return;

  /* Check for NULL inputs */
  if( (map1==NULL) || (mapweight1==NULL) || (hitsmap1==NULL) ||
      (mapvar1==NULL) || (map2==NULL) || (mapweight2==NULL) || 
      (hitsmap2==NULL) || (mapvar2==NULL) ) {
      *status = SAI__ERROR;
      errRep(FUNC_NAME, "Addmap failed due to NULL inputs.", status);      
      return;
  }

  /* Loop over every pixel and store the weighted values in arrays associated
     with map1 */

  for( i=0; i<msize; i++ ) {
    if( map1[i] == VAL__BADD ) {
      /* If bad pixel in map1 just copy map2 */
      map1[i] = map2[i];
      mapweight1[i] = mapweight2[i]; 
      hitsmap1[i] = hitsmap2[i]; 
      mapvar1[i] = mapvar2[i];
    } else if( map2[i] != VAL__BADD ) {
      /* Add together if both maps have good pixels */
      map1[i] = mapweight1[i]*map1[i] + mapweight2[i]*map2[i];
      mapweight1[i] += mapweight2[i];
      hitsmap1[i] += hitsmap2[i];

      if( !mapweight1[i] ) {
	*status = SAI__ERROR;
	errRep(FUNC_NAME, "Addmap failed due to divide-by-zero", status);      
	return;
      } else {
	map1[i] /= mapweight1[i];
      }

      /* Variance should be consistent with weight */
      mapvar1[i] = 1/mapweight1[i];
      
    }
  }
  
}
