C+
C                       D S A _ V A R I A N C E _ T O _ E R R
C
C  Routine name:
C     DSA_VARIANCE_TO_ERR
C
C  Function:
C     Converts a variance array to a standard deviation error array
C
C  Description:
C     This routine is passed the address of an array of specified
C     type and size that contains variance values. It converts these
C     into standard deviation error values in a second array of
C     the same type and size whose address it is also passed.  The
C     two arrays may actually be the same arrays in the calling
C     program.
C
C  Language:
C     FORTRAN
C
C  Call:
C     CALL DSA_VARIANCE_TO_ERR (NELM,TYPE,VAR_ADDR,ERR_ADDR,ERRORS)
C
C  Parameters:   (">" input, "!" modified, "W" workspace, "<" output)
C
C     (>) NELM        (Integer,ref) The number of elements to be converted.
C     (>) TYPE        (Fixed string,descr) The type of the array elements
C                     to be converted.  This can be any of the recognised
C                     DTA types ('INT', 'FLOAT', etc), and is not case-
C                     sensitive.
C     (>) VAR_ADDR    (Integer,ref) The address of the start of the array
C                     containing the variance values to be converted.
C     (<) ERR_ADDR    (Integer,ref) The address of the start of the array
C                     containing the resulting SD errors.
C     (<) ERRORS      (Integer,ref) The number of elements that gave rise
C                     to conversion errors.
C
C  External variables used:  None.
C
C  External subroutines / functions used:
C     DSA_SQRT_{x}
C
C  Prior requirements:  None.
C
C  Support: Keith Shortridge, AAO
C
C  Version date: 29th August 1992
C-
C  Subroutine / function details:
C     DSA_SQRT_{x}    Take the square root of the elements of an array.
C
C  History:
C     28th June 1989.   Original version.  KS / AAO.
C     21st Aug 1992     Automatic portability modifications
C                       ("INCLUDE" syntax etc) made. KS/AAO
C     29th Aug 1992     "INCLUDE" filenames now upper case. KS/AAO
C+
      SUBROUTINE DSA_VARIANCE_TO_ERR (NELM,TYPE,VAR_ADDR,ERR_ADDR,
     :                                                         ERRORS)
C
      IMPLICIT NONE
C
C     Parameters
C
      INTEGER NELM, ERR_ADDR, VAR_ADDR, ERRORS
      CHARACTER*(*) TYPE
C
C     Functions used
C
      INTEGER ICH_FOLD, DYN_ELEMENT
C
C     Dynamic memory system definition - defines DYNAMIC_MEM
C
      INCLUDE 'DYNAMIC_MEMORY'
C
C     Local variables
C
      CHARACTER CHR*1        ! First character of TYPE
      INTEGER   EPTR         ! Dynamic memory array element for error data
      INTEGER   INVOKE       ! Dummy function value
      INTEGER   VPTR         ! Dynamic memory array element for variance data
C
      CHR=TYPE(1:1)
      INVOKE=ICH_FOLD(CHR)
      EPTR=DYN_ELEMENT(ERR_ADDR)
      VPTR=DYN_ELEMENT(VAR_ADDR)
      ERRORS=0
      IF (CHR.EQ.'B') THEN
         CALL DSA_SQRT_B (NELM,DYNAMIC_MEM(VPTR),
     :                                         DYNAMIC_MEM(EPTR),ERRORS)
      ELSE IF (CHR.EQ.'F') THEN
         CALL DSA_SQRT_F (NELM,DYNAMIC_MEM(VPTR),
     :                                         DYNAMIC_MEM(EPTR),ERRORS)
      ELSE IF (CHR.EQ.'D') THEN
         CALL DSA_SQRT_D (NELM,DYNAMIC_MEM(VPTR),
     :                                         DYNAMIC_MEM(EPTR),ERRORS)
      ELSE IF (CHR.EQ.'U') THEN
         CALL DSA_SQRT_U (NELM,DYNAMIC_MEM(VPTR),
     :                                         DYNAMIC_MEM(EPTR),ERRORS)
      ELSE IF (CHR.EQ.'I') THEN
         CALL DSA_SQRT_I (NELM,DYNAMIC_MEM(VPTR),
     :                                         DYNAMIC_MEM(EPTR),ERRORS)
      ELSE IF (CHR.EQ.'S') THEN
         CALL DSA_SQRT_S (NELM,DYNAMIC_MEM(VPTR),
     :                                         DYNAMIC_MEM(EPTR),ERRORS)
      END IF
C
      END
