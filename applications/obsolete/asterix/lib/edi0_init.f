      SUBROUTINE EDI0_INIT( STATUS )
*+
*  Name:
*     EDI0_INIT

*  Purpose:
*     Load EDI ADI definitions

*  Language:
*     Starlink Fortran

*  Invocation:
*     CALL EDI0_INIT( STATUS )

*  Description:
*     {routine_description}

*  Arguments:
*     STATUS = INTEGER (givend and returned)
*        The global status.

*  Examples:
*     {routine_example_text}
*        {routine_example_description}

*  Pitfalls:
*     {pitfall_description}...

*  Notes:
*     {routine_notes}...

*  Prior Requirements:
*     {routine_prior_requirements}...

*  Side Effects:
*     {routine_side_effects}...

*  Algorithm:
*     {algorithm_description}...

*  Accuracy:
*     {routine_accuracy}

*  Timing:
*     {routine_timing}

*  External Routines Used:
*     {name_of_facility_or_package}:
*        {routine_used}...

*  Implementation Deficiencies:
*     {routine_deficiencies}...

*  References:
*     EDI Subroutine Guide : http://www.sr.bham.ac.uk/asterix-docs/Programmer/Guides/edi.html

*  Keywords:
*     package:edi, usage:private

*  Copyright:
*     Copyright (C) University of Birmingham, 1995

*  Authors:
*     DJA: David J. Allan (Jet-X, University of Birmingham)
*     {enter_new_authors_here}

*  History:
*     9 Aug 1995 (DJA):
*        Original version.
*     {enter_changes_here}

*  Bugs:
*     {note_any_bugs_here}

*-

*  Type Definitions:
      IMPLICIT NONE              ! No implicit typing

*  Global Constants:
      INCLUDE 'SAE_PAR'          ! Standard SAE constants

*  Global Variables:
      INCLUDE 'EDI_CMN'
*       EDI_INIT = LOGICAL (returned)
*         EDI system is initialised?

*  Status:
      INTEGER 			STATUS             	! Global status

*  External References:
      EXTERNAL			EDI1_CREAT
      EXTERNAL			EDI1_MAP
      EXTERNAL			EDI1_UNMAP

*  Local Variables:
      INTEGER			DID			! Dummy identifier
*.

*  Check inherited global status.
      IF ( STATUS .NE. SAI__OK ) RETURN

*  If not already initialised
      IF ( .NOT. EDI_INIT ) THEN

*    Requires the data models package
        CALL ADI_REQPKG( 'dsmodels', STATUS )

*    Define EDI interface
        CALL ADI_DEFFUN( 'ListMap(_EventDS,_HDSfile,_CHAR,'/
     :      /'_CHAR,_CHAR,_INTEGER,_INTEGER)', EDI1_MAP, DID, STATUS )
        CALL ADI_DEFFUN( 'ListUnmap(_EventDS,_HDSfile,_CHAR)',
     :                               EDI1_UNMAP, DID, STATUS )
        CALL ADI_DEFFUN( 'ListCreate(_EventDS,_HDSfile,_EventList)',
     :                               EDI1_CREAT, DID, STATUS )

*    Mark as initialised
        EDI_INIT = .TRUE.

      END IF

*  Report any errors
      IF ( STATUS .NE. SAI__OK ) CALL AST_REXIT( 'EDI0_INIT', STATUS )

      END
