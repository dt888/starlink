#include "sae_par.h"
#include "cupid.h"
#include "mers.h"
#include "star/hds.h"
#include "ast.h"
#include "star/kaplibs.h"

void cupidClumpCat( const char *param, char *cloc, double *tab, int size, 
                    int i, int ndim, const char *ttl ){
/*
*  Name:
*     cupidClumpCat

*  Purpose:
*     Add a clump into a catalogue, and optionally write the catlogue to
*     disk.

*  Synopsis:
*     void cupidClumpCat( const char *param, char *loc, double *tab, 
*                         int size, int i, int ndim, const char *ttl )

*  Description:
*     The clump parameters contained within the HDS object located by "loc" 
*     are copied into the "tab" array. If "param" is not NULL, the contents 
*     of the "tab" array are then written out to a catalogue.

*  Parameters:
*     param
*        Name of the ADAM parameter to associated with the output
*        catalogue. If NULL, no catalogue is created but the clump
*        paramaters supplied in "cloc" are still added to "tab".
*     cloc
*        Pointer to an HDS locator for the object holding the clump
*        parameters. Ignore if NULL.
*     tab
*        Pointer to an array holding clump parameters. If "cloc" is not NULL 
*        the clump parameters supplied in "cloc" are added to the end of this 
*        table. The first (fastest moving) axis corresponds to clump
*        number, and the second axis corresponds to column number within
*        the catalogue.
*     size
*        The maximum number of clumps which can be stored in "tab".
*     i
*        The number of clumps contained in "tab" once any clump described by 
*        "cloc" has been addded.
*     ndim
*        The number of pixel axes in the data.
*     ttl
*        A title for the output catalogue (if any).

*  Authors:
*     DSB: David S. Berry
*     {enter_new_authors_here}

*  History:
*     11-NOV-2005 (DSB):
*        Original version.
*     {enter_further_changes_here}

*  Bugs:
*     {note_any_bugs_here}
*/      

/* Local Variables: */
   AstFrame *frm1;               /* Frame describing clump parameters */
   AstFrame *frm2;               /* Frame describing clump centres */
   AstFrameSet *iwcs;            /* FrameSet to be stored in output catalogue */
   AstMapping *map;              /* Mapping from "frm1" to "frm2" */
   char dloc[ DAT__SZLOC + 1 ];  /* Component locator */
   double *t;                    /* Pointer to next table value */
   int inperm[ 3 ];              /* Input axis permutation array */
   int j;                        /* Loop index */
   int ncol;                     /* Number of columns in table */
   int outperm[ 3 ];             /* Output axis permutation array */

/* Abort if an error has already occurred. */
   if( *status != SAI__OK ) return;

/* Check the parameters. */
   if( i > size ) {
      *status = SAI__ERROR;
      msgSeti( "I", i );
      msgSeti( "SIZE", size );
      errRep( "CUPIDCLUMP_CAT_ERR1", "cupidClumpCat: Table size (^SIZE) "
              "is to small to accomodate ^I clumps (internal CUPID "
              "programming error).", status );
   }

/* Determine the number of columns in the catalogue (this equals the
   number of rows in the table). */
   ncol = ( ( ndim == 1 ) ? CUPID__GCNP1 : ( 
              (ndim ==2 ) ? CUPID__GCNP2 : CUPID__GCNP3 ) ) + 1;

/* If a clump has been supplied, add it to the table */
   if( cloc && *status == SAI__OK ) {

/* Get a pointer to the first element of the next free column in "tab". */
      t = tab + i - 1;

/* Extract each value from the Clump object and store it in the table. */
      datFind( cloc, "SUM", dloc, status );
      datGetD( dloc, 0, NULL, t, status );
      t += size;
      datAnnul( dloc, status );
      
      datFind( cloc, "PEAK", dloc, status );
      datPutD( dloc, 0, NULL, t, status );
      t += size;
      datAnnul( dloc, status );
   
      datFind( cloc, "OFFSET", dloc, status );
      datPutD( dloc, 0, NULL, t, status );
      t += size;
      datAnnul( dloc, status );
         
      datFind( cloc, "XCENTRE", dloc, status );
      datPutD( dloc, 0, NULL, t, status );
      t += size;
      datAnnul( dloc, status );
         
      datFind( cloc, "XFWHM", dloc, status );
      datPutD( dloc, 0, NULL, t, status );
      t += size;
      datAnnul( dloc, status );
   
      if( ndim > 1 ) {
         datFind( cloc, "YCENTRE", dloc, status );
         datPutD( dloc, 0, NULL, t, status );
         t += size;
         datAnnul( dloc, status );
            
         datFind( cloc, "YFWHM", dloc, status );
         datPutD( dloc, 0, NULL, t, status );
         t += size;
         datAnnul( dloc, status );
            
         datFind( cloc, "ANGLE", dloc, status );
         datPutD( dloc, 0, NULL,  t, status );
         t += size;
         datAnnul( dloc, status );
            
         if( ndim > 2 ) {
   
            datFind( cloc, "VCENTRE", dloc, status );
            datPutD( dloc, 0, NULL, t, status );
            t += size;
            datAnnul( dloc, status );
               
            datFind( cloc, "VFWHM", dloc, status );
            datPutD( dloc, 0, NULL, t, status );
            t += size;
            datAnnul( dloc, status );
            
            datFind( cloc, "VGRADX", dloc, status );
            datPutD( dloc, 0, NULL, t, status );
            t += size;
            datAnnul( dloc, status );
   
            datFind( cloc, "VGRADY", dloc, status );
            datPutD( dloc, 0, NULL, t, status );
            t += size;
            datAnnul( dloc, status );
   
         }
      }      
   }

/* If required,write the tableto disk. */
   if( param && *status == SAI__OK ) {

/* Start an AST context. */
      astBegin;

/* Create a Frame with "ncol" axes describing the table columns. */
      frm1 = astFrame( ncol, "Domain=PARAMETERS,Title=Clump parameters" );

/* Create a Frame with "ndim" axes describing the pixel coords at the
   clump centre. */
      frm2 = astFrame( ndim, "Domain=PIXEL,Title=Pixel coordinates" );

/* Set the attributes of these two Frames */
      astSetC( frm1, "Symbol(1)", "Sum" );
      astSetC( frm1, "Symbol(2)", "Peak" );
      astSetC( frm1, "Symbol(3)", "Offset" );
      astSetC( frm1, "Symbol(4)", "XCentre" );
      astSetC( frm1, "Symbol(5)", "XFWHM" );
      astSetC( frm2, "Symbol(1)", "P1" );
   
      if( ndim > 1 ) {
         astSetC( frm1, "Symbol(6)", "YCentre" );
         astSetC( frm1, "Symbol(7)", "YFWHM" );
         astSetC( frm1, "Symbol(8)", "Angle" );
         astSetC( frm2, "Symbol(2)", "P2" );
   
         if( ndim > 2 ) {
            astSetC( frm1, "Symbol(9)",  "VCentre" );
            astSetC( frm1, "Symbol(10)", "VFWHM" );
            astSetC( frm1, "Symbol(11)", "VGradX" );
            astSetC( frm1, "Symbol(12)", "VGradY" );
            astSetC( frm2, "Symbol(3)", "P3" );
         }
      }

/* Create a Mapping (a PermMap) from the Frame representing the "ncol" clump
   parameters, to the "ndim" Frame representing clump centre positions. The
   inverse transformation supplies bad values for the other parameters. */
      for( j = 0; j < ncol; j++ ) inperm[ j ] = 0;

      inperm[ 4 ] = 0;
      inperm[ 6 ] = 1;
      inperm[ 9 ] = 2;

      outperm[ 0 ] = 4;
      outperm[ 1 ] = 6;
      outperm[ 2 ] = 9;

      map = (AstMapping *) astPermMap( ncol, inperm, ndim, outperm, NULL, "" );

/* Create a FrameSet to store in the output catalogue. It has two Frames,
   the base Frame has "ncol" axes - each axis describes one of the table
   columns. The current Frame has 2 axes and describes the clump (x,y)
   position. */
      iwcs = astFrameSet( frm1, "ID=FIXED_BASE" );
      astAddFrame( iwcs, AST__BASE, map, frm2 );
      astSetI( iwcs, "CURRENT", 1 );

/* Create the output catalogue */
      kpg1Wrlst( param, size, i, ncol, tab, AST__BASE, iwcs,
                 ttl, 1, NULL, 1, status );

/* End the AST context. */
      astEnd;

   }
}

