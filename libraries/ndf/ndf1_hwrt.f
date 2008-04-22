      SUBROUTINE NDF1_HWRT( IDCB, APPN, NLINES, TEXT, STATUS )
*+
*  Name:
*     NDF1_HWRT

*  Purpose:
*     Write text lines to the history component of an NDF.

*  Language:
*     Starlink Fortran 77

*  Invocation:
*     CALL NDF1_HWRT( IDCB, APPN, NLINES, TEXT, STATUS )

*  Description:
*     The routine writes lines of text to the history component of an
*     NDF. If the history has not yet been modified by the current
*     application, it creates a new history record, initialises it, and
*     inserts the text suppled. If the history has already been
*     modified, then the new text is simply appended to any already
*     present. The routine returns without action if the NDF does not
*     have a history component.

*  Arguments:
*     IDCB = INTEGER (Given)
*        DCB index identifying the NDF whose history is to be modified.
*     APPN = CHARACTER * ( * ) (Given)
*        Name of the application. This is only used (to initialise the
*        new history record) if the history has not yet been modified
*        by the current application, otherwise it is ignored. If a
*        blank value is given, then a suitable default will be used
*        instead.
*     NLINES = INTEGER (Given)
*        Number of new lines of text to be added to the history record
*        (must be at least 1).
*     TEXT( NLINES ) = CHARACTER * ( * ) (Given)
*        Array of text lines.
*     STATUS = INTEGER (Given and Returned)
*        The global status.

*  Notes:
*     -  If a new history record is being initialised, then the text
*     length (characters per line) of the new record is determined by
*     the length of the elements of the TEXT array. If the history
*     record has already been written to, then the existing text length
*     is not altered and assignment of new values takes place using
*     Fortran character assignment rules. No error occurs if text is
*     truncated.
*     -  This routine does not perform any formatting or translation
*     operations on the text supplied.

*  Copyright:
*     Copyright (C) 1993 Science & Engineering Research Council

*  Licence:
*     This program is free software; you can redistribute it and/or
*     modify it under the terms of the GNU General Public License as
*     published by the Free Software Foundation; either version 2 of
*     the License, or (at your option) any later version.
*     
*     This program is distributed in the hope that it will be
*     useful,but WITHOUT ANY WARRANTY; without even the implied
*     warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
*     PURPOSE. See the GNU General Public License for more details.
*     
*     You should have received a copy of the GNU General Public License
*     along with this program; if not, write to the Free Software
*     Foundation, Inc., 59 Temple Place,Suite 330, Boston, MA
*     02111-1307, USA

*  Authors:
*     RFWS: R.F. Warren-Smith (STARLINK, RAL)
*     {enter_new_authors_here}

*  History:
*     7-MAY-1993 (RFWS):
*        Original version.
*     11-MAY-1993 (RFWS):
*        Moved formatting of date/time to another routine.
*     2-JUN-1993 (RFWS):
*        Obtain and format the date/time as two separate operations.
*     16-JUN-1993 (RFWS):
*        Added defaulting of the application name.
*     4-AUG-1993 (RFWS):
*        Fixed bug which updated the DCB history record text length
*        even if the record already existed.
*     13-AUG-1993 (RFWS):
*        Ensure that new history record structures are empty before
*        attempting to use them.
*     6-SEP-1993 (RFWS):
*        Check STATUS before passing mapped character values.
*     27-SEP-1993 (RFWS):
*        Left justify the application name when writing it.
*     1-OCT-1993 (RFWS):
*        Added creation of the HOST and USER history components.
*     26-APR-1994 (RFWS):
*        Added creation of the DATASET component.
*     {enter_further_changes_here}

*  Bugs:
*     {note_any_bugs_here}

*-
      
*  Type Definitions:
      IMPLICIT NONE              ! No implicit typing

*  Global Constants:
      INCLUDE 'SAE_PAR'          ! Standard SAE constants
      INCLUDE 'CNF_PAR'          ! For CNF_PVAL function
      INCLUDE 'DAT_PAR'          ! DAT_ public constants
      INCLUDE 'NDF_CONST'        ! NDF_ private constants      
      INCLUDE 'NDF_PAR'          ! NDF_ public constants      

*  Global Variables:
      INCLUDE 'NDF_DCB'          ! NDF_ Data Control Block
*        DCB_HAPPN = CHARACTER * ( NDF__SZAPP ) (Read)
*           Name of the currently executing application.
*        DCB_HLOC( NDF__MXDCB ) = CHARACTER * ( DAT__SZLOC ) (Read)
*           Locator for NDF history component.
*        DCB_HNREC( NDF__MXDCB ) = INTEGER (Read)
*           Number of valid history records present.
*        DCB_HRLOC( NDF__MXDCB ) = CHARACTER * ( DAT__SZLOC ) (Read)
*           Locator for array of history records.
*        DCB_HTLEN( NDF__MXDCB ) = LOGICAL (Read and Write)
*           Current history record text length.
*        DCB_FORFL( NDF__MXDCB ) = CHARACTER * ( NDF__SZFIL ) (Read)
*           Name of associated foreign format file (if it exists).
*        DCB_IFMT( NDF__MXDCB ) = INTEGER (Read)
*           FCB code identifying the format of an associated foreign
*           file (zero if no such file exists).
*        DCB_LOC( NDF__MXDCB ) = CHARACTER * ( DAT__SZLOC ) (Read)
*           Data object locator.

*  Arguments Given:
      INTEGER IDCB
      CHARACTER * ( * ) APPN
      INTEGER NLINES
      CHARACTER * ( * ) TEXT( NLINES )

*  Status:
      INTEGER STATUS             ! Global status

*  External References:
      INTEGER CHR_LEN            ! Significant length of a string

*  Local Variables:
      CHARACTER * ( 1 ) MACH     ! Machine name (junk)
      CHARACTER * ( 1 ) RELE     ! System release number (junk)
      CHARACTER * ( 1 ) SYST     ! Operating system name (junk)
      CHARACTER * ( 1 ) VERS     ! System version number (junk)
      CHARACTER * ( DAT__SZLOC ) CELL ! Locator for array sell
      CHARACTER * ( DAT__SZLOC ) LOC ! Temporary locator
      CHARACTER * ( DAT__SZLOC ) TLOC ! Locator for new text lines
      CHARACTER * ( NDF__SZAPP ) LAPPN ! Local application name
      CHARACTER * ( NDF__SZHDT ) TIME ! Formatted creation time string
      CHARACTER * ( NDF__SZHST ) NODE ! Host machine node name
      CHARACTER * ( NDF__SZREF ) REF ! Dataset reference name
      CHARACTER * ( NDF__SZUSR ) USER ! User name
      INTEGER CLEN               ! Length of character object
      INTEGER DIM( 1 )           ! Object dimension size
      INTEGER EL                 ! Number of array elements mapped
      INTEGER F                  ! First significant character position
      INTEGER L                  ! Last significant character position
      INTEGER PNTR               ! Pointer to mapped text lines
      INTEGER SUB( 1 )           ! Array subscript
      INTEGER YMDHM( 5 )         ! Integer date/time field values
      LOGICAL INIT               ! Initialise new history record?
      REAL SEC                   ! Seconds field value

*.

*  Check inherited global status.
      IF ( STATUS .NE. SAI__OK ) RETURN

*  Ensure that history information is available in the DCB.
      CALL NDF1_DH( IDCB, STATUS )
      IF ( STATUS .EQ. SAI__OK ) THEN

*  Check if a history component is present, otherwise there is nothing
*  more to do.
         IF ( DCB_HLOC( IDCB ) .NE. DAT__NOLOC ) THEN

*  Note if a new history record must be initialised. This will be so if
*  history has not yet been modified by the current application so that
*  the current record's text length is still zero.
            INIT = ( DCB_HTLEN( IDCB ) .EQ. 0 )

*  Obtain a new (empty) history record structure if necessary.
            IF ( INIT ) THEN
               CALL NDF1_HINCR( IDCB, STATUS )

*  If OK, record the text length associated with the new history
*  record.
               IF ( STATUS .EQ. SAI__OK ) THEN
                  DCB_HTLEN( IDCB ) = LEN( TEXT( 1 ) )
               END IF
            END IF

*  IF OK, obtain a locator to the current element of the history record
*  structure array.
            IF ( STATUS .EQ. SAI__OK ) THEN
               SUB( 1 ) = DCB_HNREC( IDCB )
               CALL DAT_CELL( DCB_HRLOC( IDCB ), 1, SUB, CELL, STATUS )

*  If this is a new history record structure, then it must be
*  initialised.
               IF ( INIT ) THEN

*  Ensure that it is empty (we may be re-using a structure which
*  previously held information which was not properly deleted).
                  CALL NDF1_HRST( CELL, STATUS )

*  Obtain the current date and time and convert it to standard history
*  format.
                  CALL NDF1_GTIME( YMDHM, SEC, STATUS )
                  CALL NDF1_FMHDT( YMDHM, SEC, TIME, STATUS )

*  Create a DATE component in the history record and write the date/time
*  string to it.
                  CALL DAT_NEW0C( CELL, 'DATE', NDF__SZHDT, STATUS )
                  CALL CMP_PUT0C( CELL, 'DATE', TIME, STATUS )

*  Determine the significant length of the application name. Use the
*  name supplied if it is not blank, otherwise use the default stored
*  in the DCB. If this is also blank, then determine the default
*  application name locally and use that.
                  IF ( APPN .NE. ' ' ) THEN
                     CALL CHR_FANDL( APPN, F, L )
                  ELSE IF ( DCB_HAPPN( 1 : 1 ) .NE. ' ' ) THEN
                     F = 1
                     L = CHR_LEN( DCB_HAPPN )
                  ELSE
                     CALL NDF1_GETAP( LAPPN, STATUS )
                     F = 1
                     L = MAX( 1, CHR_LEN( LAPPN ) )
                  END IF

*  Create a COMMAND component in the history record with the required
*  length and write the application name to it.
                  CALL DAT_NEW0C( CELL, 'COMMAND', L - F + 1, STATUS )
                  IF ( APPN .NE. ' ' ) THEN
                     CALL CMP_PUT0C( CELL, 'COMMAND', APPN( F : L ),
     :                               STATUS )
                  ELSE IF ( DCB_HAPPN( 1 : 1 ) .NE. ' ' ) THEN
                     CALL CMP_PUT0C( CELL, 'COMMAND',
     :                               DCB_HAPPN( F : L ), STATUS )
                  ELSE
                     CALL CMP_PUT0C( CELL, 'COMMAND', LAPPN( F : L ),
     :                               STATUS )
                  END IF

*  Obtain the user name and determine its length. Create a USER
*  component in the history record and write the name to it.
                  CALL PSX_CUSERID( USER, STATUS )
                  L = MAX( 1, CHR_LEN( USER ) )
                  CALL DAT_NEW0C( CELL, 'USER', L, STATUS )
                  CALL CMP_PUT0C( CELL, 'USER', USER, STATUS )

*  Obtain the host machine node name and determine its length. Create a
*  HOST component in the history record and write the name to it.
                  CALL PSX_UNAME( SYST, NODE, RELE, VERS, MACH, STATUS )
                  L = MAX( 1, CHR_LEN( NODE ) )
                  CALL DAT_NEW0C( CELL, 'HOST', L, STATUS )
                  CALL CMP_PUT0C( CELL, 'HOST', NODE, STATUS )

*  Obtain a reference name for the dataset being processed and determine
*  its length. Use the name of the foreign format file if there is one
*  associated with the NDF.
                  IF ( DCB_IFMT( IDCB ) .EQ. 0 ) THEN
                     CALL DAT_REF( DCB_LOC( IDCB ), REF, L, STATUS )
                     L = MAX( 1, L )
                  ELSE
                     REF = DCB_FORFL( IDCB )
                     L = MAX( 1, CHR_LEN( REF ) )
                  END IF

*  Create a DATASET component in the history record and write the
*  dataset name to it.
                  CALL DAT_NEW0C( CELL, 'DATASET', L, STATUS )
                  CALL CMP_PUT0C( CELL, 'DATASET', REF, STATUS )

*  Create a new TEXT array in the history record with the required
*  number of lines and characters per line.
                  CALL DAT_NEW1C( CELL, 'TEXT', LEN( TEXT( 1 ) ),
     :                            NLINES, STATUS )
                  CALL DAT_FIND( CELL, 'TEXT', TLOC, STATUS )

*  If we are appending to an existing history record, then obtain a
*  locator for the TEXT array and determine its size.
               ELSE
                  CALL DAT_FIND( CELL, 'TEXT', LOC, STATUS )
                  CALL DAT_SIZE( LOC, DIM( 1 ), STATUS )

*  Increase its size by the required number of lines.
                  DIM( 1 ) = DIM( 1 ) + NLINES
                  CALL DAT_ALTER( LOC, 1, DIM, STATUS )

*  Obtain a locator to a slice of the TEXT array composed of the new
*  lines just added and annul the locator to the whole array.
                  SUB( 1 ) = DIM( 1 ) - NLINES + 1
                  CALL DAT_SLICE( LOC, 1, SUB, DIM, TLOC, STATUS )
                  CALL DAT_ANNUL( LOC, STATUS )
               END IF

*  Map the new lines in the TEXT array for writing and copy the history
*  text into them.
               CALL DAT_MAPV( TLOC, '_CHAR', 'WRITE', PNTR, EL, STATUS )
               CALL DAT_CLEN( TLOC, CLEN, STATUS )
               IF ( STATUS .EQ. SAI__OK ) THEN
                  CALL NDF1_HCPY( NLINES, %VAL( CNF_PVAL( PNTR ) ),
     :                            TEXT, STATUS, 
     :                            %VAL( CNF_CVAL( CLEN ) ) )
               END IF

*  Annul the TEXT array locator, thus unmapping the new lines.
               CALL DAT_ANNUL( TLOC, STATUS )

*  Annul the locator for the current history record.
               CALL DAT_ANNUL( CELL, STATUS )
            END IF
         END IF
      END IF
 
*  Call error tracing routine and exit.
      IF ( STATUS .NE. SAI__OK ) CALL NDF1_TRACE( 'NDF1_HWRT', STATUS )

      END
