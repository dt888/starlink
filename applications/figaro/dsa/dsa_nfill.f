C+
C                     D S A _ N F I L L _ A R R A Y
C
C  Routine name:
C     DSA_NFILL_ARRAY
C
C  Function:
C     Fills an array of the specified type with the numbers 1..N
C
C  Description:
C     Given the memory address for an array of specified type, this 
C     routine fills the array with the numbers 1..N   This is used 
C     to initialise axis arrays.
C
C  Language:
C     FORTRAN
C
C  Call:
C     CALL DSA_NFILL_ARRAY (NELM,ADDRESS,TYPE,STATUS)
C
C  Parameters:   (">" input, "!" modified, "W" workspace, "<" output)
C
C     (>) NELM        (Integer,ref) The number of elements in the array.
C     (>) ADDRESS     (Integer,ref) The address of the data array.
C     (>) TYPE        (Fixed string,descr) The type of the data array.
C                     This must be one of 'FLOAT','INT',SHORT','REAL'
C                     'BYTE', 'USHORT' or 'DOUBLE'.  Anything else is 
C                     ignored.
C     (>) STATUS      (Integer,ref) Status code.  If bad status is passed,
C                     this routine returns immediately.
C
C  External variables used:  None.
C
C  External subroutines / functions used:
C     DYN_ELEMENT, DSA_WRUSER, DSA_NFILLx
C
C  Prior requirements:  None.
C
C  Support: Keith Shortridge, AAO
C
C  Version date: 29th August 1992
C-
C  Subroutine / function details:
C     DSA_NFILLx    Fill array of type x with numbers 1..N
C     DSA_WRUSER    Output message to user.
C     DYN_ELEMENT   Dynamic memory element corresponding to address
C
C  History:
C     7th July 1987   Original version.  KS / AAO.
C     25th Apr 1989   Support for USHORT type added.  Also added test
C                     on number of elements for short arrays. KS / AAO.
C     21st Aug 1992   Automatic portability modifications
C                     ("INCLUDE" syntax etc) made. KS/AAO
C     29th Aug 1992   "INCLUDE" filenames now upper case. KS/AAO
C+
      SUBROUTINE DSA_NFILL_ARRAY (NELM,ADDRESS,TYPE,STATUS)
C
      IMPLICIT NONE
C
C     Parameters
C
      INTEGER NELM, ADDRESS, STATUS
      CHARACTER*(*) TYPE
C
C     Functions
C
      INTEGER DYN_ELEMENT
C
C     Local variables
C
      CHARACTER CHR*1                  ! First character of TYPE
      INTEGER   ELEMENT                ! Dynamic memory array element
C
C     DSA_ system dynamic memory.  Defines DYNAMIC_MEM.
C
      INCLUDE 'DYNAMIC_MEMORY'
C
C     DSA_ system error codes
C
      INCLUDE 'DSA_ERRORS'
C
C     Return immediately on bad status
C
      IF (STATUS.NE.0) RETURN
C
C     Call appropriate routine for data type
C
      ELEMENT=DYN_ELEMENT(ADDRESS)
      CHR=TYPE(1:1)
      IF (CHR.EQ.'F') THEN
         CALL DSA_NFILLF (NELM,DYNAMIC_MEM(ELEMENT))
      ELSE IF (CHR.EQ.'D') THEN
         CALL DSA_NFILLD (NELM,DYNAMIC_MEM(ELEMENT))
      ELSE IF (CHR.EQ.'I') THEN
         CALL DSA_NFILLI (NELM,DYNAMIC_MEM(ELEMENT))
      ELSE IF (CHR.EQ.'S') THEN
         IF (NELM.GT.32767) THEN
            CALL DSA_WRUSER('Cannot fill a short array of more than ')
            CALL DSA_WRUSER('32767 elements with the numbers 1..N')
            CALL DSA_WRFLUSH
            STATUS=DSA__INVTYP
         ELSE
            CALL DSA_NFILLS (NELM,DYNAMIC_MEM(ELEMENT))
         END IF
      ELSE IF (CHR.EQ.'U') THEN
         IF (NELM.GT.65535) THEN
            CALL DSA_WRUSER(
     :           'Cannot fill an unsigned short array of more than ')
            CALL DSA_WRUSER('65535 elements with the numbers 1..N')
            CALL DSA_WRFLUSH
            STATUS=DSA__INVTYP
         ELSE
            CALL DSA_NFILLU (NELM,DYNAMIC_MEM(ELEMENT))
         END IF
      ELSE IF (CHR.EQ.'B') THEN
         IF (NELM.GT.127) THEN
            CALL DSA_WRUSER('Cannot fill a byte array of more than ')
            CALL DSA_WRUSER('127 elements with the numbers 1..N')
            CALL DSA_WRFLUSH
            STATUS=DSA__INVTYP
         ELSE
            CALL DSA_NFILLB (NELM,DYNAMIC_MEM(ELEMENT))
         END IF
      END IF
C
      END
