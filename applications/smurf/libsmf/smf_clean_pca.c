/*
*+
*  Name:
*     smf_clean_pca

*  Purpose:
*     Clean smfData by removing the strongest correlations using PCA

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Subroutine

*  Invocation:
*     smf_clean_pca( smfWorkForce *wf, smfData *data, double thresh,
*                    smfData **components, smfData **amplitudes,
*                    int flagbad, AstKeyMap *keymap, int *status )

*  Arguments:
*     wf = smfWorkForce * (Given)
*        Pointer to a pool of worker threads (can be NULL)
*     data = smfData * (Given)
*        Pointer to the input smfData (assume that bolometer means have been
*        removed)
*     thresh = double (Given)
*        Outlier threshold for amplitudes to remove from data for cleaning
*     components = smfData ** (Returned)
*        New 3d cube of principal component time-series (ncomponents * 1 * time)
*        Can be NULL.
*     amplitudes = smfData ** (Returned)
*        New 3d cube giving amplitudes of each component for each bolometer
*        (bolo X * bolo Y * component amplitude). Can be NULL.
*     flagbad = int (Given)
*        If set, compare each bolometer to the first component as a template
*        to decide whether the data are good or not.
*     keymap = AstKeyMap * (Given)
*        Keymap containing parameters that control how flagbad works. See
*        smf_find_gains for details.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Return Value:

*  Description:
*     This function uses principal component analysis (PCA) to remove
*     correlated signals from the data. A new set of basis vectors
*     (eigenvectors, or components henceforth) are determined for the
*     N working bolometer time series such that their statistical
*     covariances are 0. These components are normalized by their
*     standard deviations, and then each bolometer is expressed as a
*     linear combination of the components. Both the components, and
*     their amplitudes (eigenvalues) in each bolometer time series may
*     be optionally returned. The largest amplitude components are
*     generally assumed to be noise sources, and are identified as
*     outliers from the general population and removed.
*
*     In addition, this routine can be used to flag bad bolometers in
*     the same way that the common-mode routines work. Once the
*     decomposition into principal components is complete (but prior
*     to cleaning), most of the signal in each bolometer should
*     resemble the first, largest component (sky+fridge).A fit of this
*     template to each bolometer is a useful way of finding entire
*     bolometers (or portions) that are outliers, and their quality
*     arrays are flagged accordingly.

*  Notes:
*     The input bolometer time series are assumed to have had their
*     means removed before entry.

*  Authors:
*     Ed Chapin (UBC)

*  History:
*     2011-03-16 (EC):
*        Initial version -- only does the projection, no filtering
*     2011-03-17 (EC):
*        -Add basic header to returned components smfData
*        -Parallelize as much as possible over exclusive time chunks

*  Copyright:
*     Copyright (C) 2011 University of British Columbia.
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
#include "ast.h"
#include "sae_par.h"
#include "mers.h"
#include "ndf.h"
#include "prm_par.h"

/* GSL includes */
#include "gsl/gsl_linalg.h"
#include "gsl/gsl_statistics_double.h"

/* SMURF routines */
#include "smf.h"
#include "smf_typ.h"
#include "smf_err.h"

/* ------------------------------------------------------------------------ */
/* Local variables and functions */

/* Structure containing information about blocks of time slices to be
   processed by each thread. All threads read/write to/from mutually
   exclusive parts of data. */

typedef struct smfPCAData {
  double *amp;            /* matrix of components amplitudes for each bolo */
  size_t abstride;        /* bolo stride in amp array */
  size_t acompstride;     /* component stride in amp array */
  size_t bstride;         /* bolo stride */
  double *comp;           /* data cube of components */
  gsl_matrix *cov;        /* bolo-bolo covariance matrix */
  double *covwork;        /* work array for covariance calculation */
  size_t ccompstride;     /* component stride in comp array */
  size_t ctstride;        /* time stride in comp array */
  smfData *data;          /* Pointer to input data */
  size_t *goodbolo;       /* Local copy of global goodbolo */
  int ijob;               /* Job identifier */
  dim_t nbolo;            /* Number of detectors  */
  size_t ngoodbolo;       /* Number of good bolos */
  dim_t ntslice;          /* Number of time slices */
  int operation;          /* 0=covar,1=eigenvect,2=projection */
  double *rms_amp;        /* VAL__BADD where modes need to be removed */
  size_t t1;              /* Index of first time slice for chunk */
  size_t t2;              /* Index of last time slice */
  size_t tstride;         /* time slice stride */
} smfPCAData;

/* Function to be executed in thread: FFT all of the bolos from b1 to b2 */

void smfPCAParallel( void *job_data_ptr, int *status );

void smfPCAParallel( void *job_data_ptr, int *status ) {
  double *amp=NULL;       /* matrix of components amplitudes for each bolo */
  size_t abstride;        /* bolo stride in amp array */
  size_t acompstride;     /* component stride in amp array */
  size_t bstride;         /* bolo stride */
  size_t ccompstride;     /* component stride in comp array */
  size_t ctstride;        /* time stride in comp array */
  double *comp=NULL;      /* data cube of components */
  gsl_matrix *cov=NULL;   /* bolo-bolo covariance matrix */
  double *covwork=NULL;   /* goodbolo * 3 work array for covariance */
  double *d=NULL;         /* Pointer to data array */
  size_t *goodbolo;       /* Local copy of global goodbolo */
  size_t i;               /* Loop counter */
  size_t j;               /* Loop counter */
  size_t k;               /* Loop counter */
  dim_t ngoodbolo;        /* number good bolos = number principal components */
  dim_t ntslice;          /* number of time slices */
  smfPCAData *pdata=NULL; /* Pointer to job data */
  double *rms_amp=NULL;   /* VAL__BADD for components to remove */
  size_t tstride;         /* time slice stride */

  if( *status != SAI__OK ) return;

  /* Pointer to the data that this thread will process */
  pdata = job_data_ptr;

  amp = pdata->amp;
  abstride = pdata->abstride;
  acompstride = pdata->acompstride;
  bstride = pdata->bstride;
  comp = pdata->comp;
  ccompstride = pdata->ccompstride;
  ctstride = pdata->ctstride;
  cov = pdata->cov;
  covwork = pdata->covwork;
  d = pdata->data->pntr[0];
  goodbolo = pdata->goodbolo;
  ngoodbolo = pdata->ngoodbolo;
  ntslice = pdata->ntslice;
  rms_amp = pdata->rms_amp;
  tstride = pdata->tstride;

  /* Check for valid inputs */
  if( !pdata ) {
    *status = SAI__ERROR;
    errRep( "", "smfPCAParallel: No job data supplied", status );
    return;
  }

  /* Debugging message indicating thread started work */
  msgOutiff( MSG__DEBUG, "",
             "smfPCAParallel: op=%i thread starting on time slices %zu -- %zu",
             status, pdata->operation, pdata->t1, pdata->t2 );


  /* if t1 past end of the work, nothing to do so we return */
  if( pdata->t1 >= pdata->ntslice ) {
    msgOutif( MSG__DEBUG, "",
              "smfPCAParallel: nothing for thread to do, returning",
              status);
    return;
  }

  if( (pdata->operation == 0) && (*status==SAI__OK) ) {
    /* Operation 0: accumulate sums for covariance calculation -------------- */

    for( i=0; i<ngoodbolo; i++ ) {
      double sum_xy;

      /*msgOutiff( MSG__DEBUG, "", "   bolo %zu", status, goodbolo[i] );*/

      for( j=i; j<ngoodbolo; j++ ) {
        sum_xy = 0;

        for( k=pdata->t1; k<=pdata->t2; k++ ) {
          sum_xy += d[goodbolo[i]*bstride + k*tstride] *
            d[goodbolo[j]*bstride + k*tstride];
        }

        /* Store sums in work array and normalize once all threads finish */
        covwork[ i + j*ngoodbolo ] = sum_xy;
      }
    }
  } else if( (pdata->operation == 1) && (*status == SAI__OK) ) {
    /* Operation 1: normalized eigenvectors --------------------------------- */

    for( i=0; i<ngoodbolo; i++ ) {   /* loop over comp */
      double u;

      /*msgOutiff( MSG__DEBUG, "", "   bolo %zu", status, goodbolo[i] );*/

      for( j=0; j<ngoodbolo; j++ ) { /* loop over bolo */

        u = gsl_matrix_get( cov, j, i );

        /* Calculate the vector */
        for( k=pdata->t1; k<=pdata->t2; k++ ) {
          comp[i*ccompstride+k*ctstride] += d[goodbolo[j] *
                                              bstride + k*tstride] * u;
        }
      }
    }
  } else if( (pdata->operation == 2) && (*status == SAI__OK) ) {
    /* Operation 2: project data along eigenvectors ------------------------- */

    for( i=0; i<ngoodbolo; i++ ) {    /* loop over bolometer */
      /*msgOutiff( MSG__DEBUG, "", "   bolo %zu", status, goodbolo[i] );*/
      for( j=0; j<ngoodbolo; j++ ) {  /* loop over component */
        for( k=pdata->t1; k<=pdata->t2; k++ ) {
          amp[goodbolo[i]*abstride + j*acompstride] +=
            d[goodbolo[i]*bstride + k*tstride] *
            comp[j*ccompstride + k*ctstride];
        }
      }
    }

  } else if( (pdata->operation == 3) && (*status == SAI__OK) ) {
    /* Operation 3: clean --------------------------------------------------- */
    double a;

    for( j=0; j<ngoodbolo; j++ ) {        /* loop over component */
      if( rms_amp[j] == VAL__BADD ) {

        /* Bad values in rms_amp indicate components that we are
           removing. Subtract the component scaled by the amplitude for
           each bolometer at all relevant time-slices */

        for( i=0; i<ngoodbolo; i++ ) {    /* loop over bolometer */
          a =  amp[goodbolo[i]*abstride + j*acompstride];
          for( k=pdata->t1; k<=pdata->t2; k++ ) {
            d[goodbolo[i]*bstride + k*tstride] -=
              a*comp[j*ccompstride + k*ctstride];
          }
        }
      }
    }

  } else if( *status==SAI__OK ) {
    *status = SAI__ERROR;
    errRep( "", "smfPCAParallel"
            ": possible programming error: invalid operation number", status );
  }

  /* Debugging message indicating thread finished work */
  msgOutiff( MSG__DEBUG, "",
             "smfPCAParallel: op=%i thread finishing time slices %zu -- %zu",
             status, pdata->operation, pdata->t1, pdata->t2 );
}

/* ------------------------------------------------------------------------ */


#define FUNC_NAME "smf_clean_pca"

void smf_clean_pca( smfWorkForce *wf, smfData *data, double thresh,
                    smfData **components, smfData **amplitudes, int flagbad,
                    AstKeyMap *keymap, int *status ) {


  double *amp=NULL;       /* matrix of components amplitudes for each bolo */
  size_t abstride;        /* bolo stride in amp array */
  size_t acompstride;     /* component stride in amp array */
  size_t bstride;         /* bolo stride */
  double *comp=NULL;      /* data cube of components */
  size_t ccompstride;     /* component stride in comp array */
  size_t ctstride;        /* time stride in comp array */
  gsl_matrix *cov=NULL;   /* bolo-bolo covariance matrix */
  double *d = NULL;       /* Pointer to data array */
  size_t i;               /* Loop counter */
  int ii;                 /* Loop counter */
  size_t j;               /* Loop counter */
  smfPCAData *job_data=NULL;/* job data */
  size_t k;               /* Loop counter */
  size_t *goodbolo=NULL;  /* Indices of the good bolometers for analysis */
  dim_t nbolo;            /* number of bolos */
  dim_t ndata;            /* number of samples in data */
  dim_t ngoodbolo;        /* number good bolos = number principal components */
  dim_t ntslice;          /* number of time slices */
  int nw;                 /* total available worker threads */
  smfPCAData *pdata=NULL; /* Pointer to job data */
  smf_qual_t *qua=NULL;   /* Pointer to quality array */
  gsl_vector *s=NULL;     /* singular values for SVD */
  size_t step;            /* step size for job division */
  size_t tstride;         /* time slice stride */
  gsl_matrix *v=NULL;      /* orthogonal square matrix for SVD */
  gsl_vector *work=NULL;  /* workspace for SVD */

  if (*status != SAI__OK) return;

  /* How many threads do we get to play with */
  nw = wf ? wf->nworker : 1;

  /* Check for NULL smfData pointer */
  if( !data || !data->pntr[0]) {
    *status = SAI__ERROR;
    errRep( " ", FUNC_NAME
            ": possible programming error, NULL data supplied", status );
    return;
  }

  /* Valid thresh */
  if( thresh < 0 ) {
    *status = SAI__ERROR;
    errRep( " ", FUNC_NAME
            ": possible programming error, thresh < 0", status );
    return;
  }

  smf_get_dims( data, NULL, NULL, &nbolo, &ntslice, &ndata, &bstride, &tstride,
                status );

  if( data->ndims != 3 ) {
    *status = SAI__ERROR;
    errRep( " ", FUNC_NAME
            ": possible programming error, smfData should be 3-dimensional",
            status );
    return;
  }

  if( data->dtype != SMF__DOUBLE ) {
    *status = SAI__ERROR;
    errRep( " ", FUNC_NAME
            ": possible programming error, smfData should be double precision",
            status );
    return;
  }

  if( ntslice <= 2 ) {
    *status = SAI__ERROR;
    errRep( " ", FUNC_NAME ": fewer than 2 time slices!", status );
    goto CLEANUP;
  }

  qua = smf_select_qualpntr( data, 0, status );

  if( qua ) {
    /* If quality supplied, identify good bolometers */
    ngoodbolo = 0;
    for( i=0; i<nbolo; i++ ) {
      if( !(qua[i*bstride]&SMF__Q_BADB) ) {
        ngoodbolo++;
      }
    }

    /* Now remember which were the good bolometers */
    goodbolo = astCalloc( ngoodbolo, sizeof(*goodbolo), 1 );
    ngoodbolo = 0;
    for( i=0; i<nbolo; i++ ) {
      if( !(qua[i*bstride]&SMF__Q_BADB) ) {
        goodbolo[ngoodbolo] = i;
        ngoodbolo++;
      }
    }

  } else {
    /* Otherwise assume all bolometers are good */
    ngoodbolo = nbolo;
    goodbolo = astCalloc( ngoodbolo, sizeof(*goodbolo), 1 );
    for( i=0; i<ngoodbolo; i++ ) {
      goodbolo[i] = i;
    }
  }

  if( ngoodbolo <= 2 ) {
    *status = SAI__ERROR;
    errRep( " ", FUNC_NAME ": fewer than 2 working bolometers!", status );
    goto CLEANUP;
  }

  /* Allocate arrays */
  amp = astCalloc( nbolo*nbolo, sizeof(*amp),1 );
  comp = astCalloc( ngoodbolo*ntslice, sizeof(*comp),1 );
  cov = gsl_matrix_alloc( ngoodbolo, ngoodbolo );
  s = gsl_vector_alloc( ngoodbolo );
  v = gsl_matrix_alloc( ngoodbolo, ngoodbolo );
  work = gsl_vector_alloc( ngoodbolo );

  /* These strides will make comp time-ordered */
  ccompstride = 1;
  ctstride = ngoodbolo;

  /* These strides will also make amp look time-ordered (sort-of: the time
     axis is now the component number */
  abstride = 1;
  acompstride = nbolo;

  /* input bolo data pointer */
  d = data->pntr[0];

  /* Allocate job data for threads */
  job_data = astCalloc( nw, sizeof(*job_data), 1 );

  /* Set up the division of labour for threads: independent blocks of time */

  if( nw > (int) ntslice ) {
    step = 1;
  } else {
    step = ntslice/nw;
  }

  for( ii=0; (*status==SAI__OK)&&(ii<nw); ii++ ) {
    pdata = job_data + ii;

    /* Blocks of time slices */
    pdata->t1 = ii*step;
    pdata->t2 = (ii+1)*step-1;

    /* Ensure that the last thread picks up any left-over tslices */
    if( (ii==(nw-1)) && (pdata->t1<(ntslice-1)) ) {
      pdata->t2=ntslice-1;
    }

    /* initialize work data */
    pdata->amp = amp;
    pdata->abstride = abstride;
    pdata->acompstride = acompstride;
    pdata->bstride = bstride;
    pdata->comp = comp;
    pdata->cov = NULL;
    pdata->covwork = NULL;
    pdata->ccompstride = ccompstride;
    pdata->ctstride = ctstride;
    pdata->data = data;
    pdata->goodbolo = NULL;
    pdata->ijob = -1;
    pdata->nbolo = nbolo;
    pdata->ngoodbolo = ngoodbolo;
    pdata->ntslice = ntslice;
    pdata->operation = 0;
    pdata->tstride = tstride;

    /* Each thread will accumulate sums of x, y, and x*y for each bolo when
       calculating the covariance matrix */
    pdata->covwork = astCalloc( ngoodbolo*ngoodbolo,
                                sizeof(*(pdata->covwork)), 1 );

    /* each thread gets its own copy of the goodbolo lookup table */
    pdata->goodbolo = astCalloc( ngoodbolo, sizeof(*(pdata->goodbolo)), 1 );
    if( *status == SAI__OK ) {
      memcpy( pdata->goodbolo, goodbolo,
              ngoodbolo*sizeof(*(pdata->goodbolo)) );
    }

  }

  if( *status == SAI__OK ) {

    /* Measure the covariance matrix using parallel code ---------------------*/

    msgOutif( MSG__VERB, "", FUNC_NAME
              ": measuring bolo-bolo covariance matrix...", status );

    /* Set up the jobs to calculate sums for each time block and submit */
    for( ii=0; ii<nw; ii++ ) {
      pdata = job_data + ii;
      pdata->operation = 0;
      pdata->ijob = smf_add_job( wf, SMF__REPORT_JOB, pdata, smfPCAParallel,
                                 NULL, status );
    }

    /* Wait until all of the submitted jobs have completed */
    smf_wait( wf, status );

    /* We now have to add together all of the sums from each thread and
       normalize */
    if( *status == SAI__OK ) {
      for( i=0; i<ngoodbolo; i++ ) {
        for( j=i; j<ngoodbolo; j++ ) {
          double c;
          double *covwork=NULL;
          double sum_xy;

          sum_xy = 0;

          for( ii=0; ii<nw; ii++ ) {
            pdata = job_data + ii;
            covwork = pdata->covwork;

            sum_xy += covwork[ i + j*ngoodbolo ];
          }

          c = sum_xy / ((double)ntslice-1);

          gsl_matrix_set( cov, i, j, c );
          gsl_matrix_set( cov, j, i, c );
        }
      }
    }
  }

  /* Factor cov = u s v^T, noting that the gsl routine calculates u in
     in-place of cov. --------------------------------------------------------*/

  msgOutif( MSG__VERB, "", FUNC_NAME
            ": perfoming singular value decomposition...", status );

  if( *status == SAI__OK ) {
    gsl_linalg_SV_decomp( cov, v, s, work );
  }

  /* Calculate normalized eigenvectors with parallel code --------------------*/

  msgOutif( MSG__VERB, "", FUNC_NAME
            ": calculating statistically-independent components...", status );

  /* The above calculation tells us what linear combinations of the original
     bolometer time series will give us the statistically independent new
     set of basis vectors (components), which we then normalize by their RMS. */

  /* Set up the jobs to calculate sums for each time block and submit */
  if( *status == SAI__OK ) {
    for( ii=0; ii<nw; ii++ ) {
      pdata = job_data + ii;
      pdata->cov = cov;
      pdata->operation = 1;
      pdata->ijob = smf_add_job( wf, SMF__REPORT_JOB, pdata, smfPCAParallel,
                                 NULL, status );
    }
  }

  /* Wait until all of the submitted jobs have completed */
  smf_wait( wf, status );

  /* Then normalize */
  for( i=0; (*status==SAI__OK)&&(i<ngoodbolo); i++ ) {
    double sigma;

    smf_stats1D( comp + i*ccompstride, ctstride, ntslice, NULL, 0,
                 0, NULL, &sigma, NULL, status );

    /* Apparently we need this to get the normalization right */
    sigma *= sqrt((double) ntslice);

    if( *status == SAI__OK ) {
      for( k=0; k<ntslice; k++ ) {
        comp[i*ccompstride + k*ctstride] /= sigma;
      }
    }
  }

  /* Now project the data along each of these normalized basis vectors
     to figure out the amplitudes of the components in each bolometer
     time series. ------------------------------------------------------------*/

  msgOutif( MSG__VERB, "", FUNC_NAME
              ": calculating component amplitudes in each bolo...", status );

  /* Set up the jobs  */
  if( *status == SAI__OK ) {
    for( ii=0; ii<nw; ii++ ) {
      pdata = job_data + ii;
      pdata->operation = 2;
      pdata->ijob = smf_add_job( wf, SMF__REPORT_JOB, pdata, smfPCAParallel,
                                 NULL, status );
    }
  }

  /* Wait until all of the submitted jobs have completed */
  smf_wait( wf, status );

  /* Check to see if the amplitudes are mostly negative or positive. If
     mostly negative, flip the sign of both the component and amplitudes */
  if( *status == SAI__OK ) {
    double total;
    for( j=0; j<ngoodbolo; j++ ) {    /* loop over component */
      total = 0;
      for( i=0; i<ngoodbolo; i++ ) {  /* loop over bolometer */
        total += amp[goodbolo[i]*abstride + j*acompstride];
      }

      /* Are most amplitudes negative for this component? */
      if( total < 0 ) {
        /* Flip sign of the amplitude */
        for( i=0; i<ngoodbolo; i++ ) { /* loop over bolometer */
          amp[goodbolo[i]*abstride + j*acompstride] =
            -amp[goodbolo[i]*abstride + j*acompstride];
        }

        /* Flip sign of the component */
        for( k=0; k<ntslice; k++ ) {
           comp[j*ccompstride + k*ctstride] =
             -comp[j*ccompstride + k*ctstride];
        }
      }
    }
  }

  /* Flag outlier bolometers if requested ------------------------------------*/

  if( (*status==SAI__OK) && flagbad ) {
    smfArray *data_array=NULL;
    smfArray *gain_array=NULL;
    smfGroup *gain_group=NULL;
    AstKeyMap *kmap=NULL;         /* Local keymap */
    int nrej;
    AstObject *obj=NULL;          /* Used to avoid compiler warnings */
    double *template=NULL;

    /* Obtain pointer to sub-keymap containing bolo rejection parameters.
       Since this is an essentially identical operation to what we do with
       COM, just lift its parameters for now */
    astMapGet0A( keymap, "COM", &obj );
    kmap = (AstKeyMap *) obj;
    obj = NULL;

    /* Create a 1d array containing a copy of the first component as a
       template */
    template = astCalloc( ntslice, sizeof(*template),1 );
    if( *status == SAI__OK ) {
      for( i=0; i<ntslice; i++ ) {
        template[i] = comp[i*ctstride];
      }
    }

    /* We need a smfData to store the gains of the template
       temporarily. Do this using smf_model_create (which uses
       smfArray's as inputs/outputs) to ensure that we get the
       dimensions that smf_find_gains will be expecting. */
    data_array = smf_create_smfArray( status );
    smf_addto_smfArray( data_array, data, status );

    smf_model_create( wf, NULL, &data_array, NULL, NULL, NULL, NULL, 1,
                      SMF__GAI, data->isTordered, NULL, 0, NULL, NULL,
                      &gain_group, 1, 1, &gain_array, keymap, status );

    /* Compare bolometers to the template in order to flag outliers */
    smf_find_gains( wf, data, template, kmap, SMF__Q_GOOD, SMF__Q_COM,
                    gain_array->sdata[0], &nrej, status );

    /* Clean up */
    if( template ) template = astFree( template );
    if( data_array ) {
      /* Data doesn't belong to us, so avoid freeing */
      data_array->owndata = 0;
      smf_close_related( &data_array, status );
    }
    if( gain_array ) smf_close_related( &gain_array, status );
    if( gain_group ) smf_close_smfGroup( &gain_group, status );
    if( kmap ) kmap = astAnnul( kmap );
  }

  /* Cleaning ----------------------------------------------------------------*/

  if( (*status == SAI__OK) && thresh ) {
    int converge=0;          /* Set if converged */
    size_t ngood;            /* Number of values that are still good */
    double *rms_amp=NULL;    /* RMS amplitude across bolos each component */
    double sum;
    double sum_sq;
    size_t iter=0;
    double x;

    /* First calculate the RMS of the amplitudes across the array for
       each component. This will be a positive number whose value
       gives a typical amplitude of the component that can be compared
       to other components */

    rms_amp = astCalloc( ngoodbolo, sizeof(*rms_amp), 1 );

    if( *status == SAI__OK ) {
      for( i=0; i<ngoodbolo; i++ ) {     /* Loop over component */

        sum = 0;
        sum_sq = 0;

        for( j=0; j<ngoodbolo; j++ ) {   /* Loop over bolo */
          x = amp[i*acompstride + goodbolo[j]*abstride];
          sum += x;
          sum_sq += x*x;
        }

        rms_amp[i] = sqrt( sum_sq / ((double)ngoodbolo) );
      }
    }

    /* Then, perform an iterative clip using the mean and standard
       deviation of the component amplitude RMS's. Note that the RMS
       is always massively dominated by the first mode, so we will
       always assume that it should be removed to help things be more
       well-behaved. */

    rms_amp[0] = VAL__BADD;

    while( !converge && (*status==SAI__OK) ) {
      double m;                /* mean */
      int new;                 /* Set if new values flagged */
      double sig;              /* standard deviation */

      /* Update interation counter */
      iter ++;

      /* Measure mean and standard deviation of non-flagged samples */
      smf_stats1D( rms_amp, 1, ngoodbolo, NULL, 0, 0, &m, &sig, &ngood,
                   status );

      msgOutiff( MSG__DEBUG, "", FUNC_NAME
                 ": iter %zu mean=%lf sig=%lf ngood=%zu", status,
                 iter, m, sig, ngood );

      /* Flag new outliers */
      new = 0;
      for( i=0; i<ngood; i++ ) {
        if( (rms_amp[i]!=VAL__BADD) && ((rms_amp[i]-m) > sig*thresh) ) {
          new = 1;
          rms_amp[i] = VAL__BADD;
          ngood--;
        }
      }

      /* Converged if no new values flagged */
      if( !new ) converge = 1;

      /* Trap huge numbers of iterations */
      if( iter > 50 ) {
        *status = SAI__ERROR;
        errRep( "", FUNC_NAME ": more than 50 iterations!", status );
      }

      /* If we have less than 10% of the original modes (or 2,
         whichever is larger), generate bad status */
      if( (ngood <= 2) || (ngood < 0.1*ngoodbolo) ) {
        *status = SAI__ERROR;
        errRepf( "", FUNC_NAME ": only %zu of %zu modes remain!", status,
                 ngoodbolo, ngood );
      }
    }

    msgOutiff( MSG__VERB, "", FUNC_NAME
               ": after %zu clipping iterations, will remove "
               "%zu / %zu components...", status, iter, ngoodbolo - ngood,
               ngoodbolo );

    /* Now that we know which modes to remove, call the parallel routine
       for doing the hard work */

    for( ii=0; ii<nw; ii++ ) {
      pdata = job_data + ii;
      pdata->operation = 3;
      pdata->rms_amp = rms_amp;
      pdata->ijob = smf_add_job( wf, SMF__REPORT_JOB, pdata, smfPCAParallel,
                                 NULL, status );
    }

    /* Wait until all of the submitted jobs have completed */
    smf_wait( wf, status );

    if( rms_amp ) rms_amp = astFree( rms_amp );
  }

  /* Returning components? ---------------------------------------------------*/
  if( (*status==SAI__OK) && components ) {
    dim_t dims[3];
    int lbnd[3];
    smfHead *hdr=NULL;

    dims[0] = ngoodbolo;
    dims[1] = 1;
    lbnd[0] = 0;
    lbnd[1] = 0;

    if( data->isTordered ) { /* T is 3rd axis in data if time-ordered */
      dims[2] = data->dims[2];
      lbnd[2] = data->lbnd[2];
    } else {                 /* T is 1st axis in data if bolo-ordered */
      dims[2] = data->dims[0];
      lbnd[2] = data->lbnd[0];
    }

    /* Copy the header if one was supplied with the input data. This
       will at least give us a sensible time axis and allow us to run
       components through sc2fft if stored to disk, although the
       bolometer axes will be misleading. Should probably strip off
       the time axis properly, e.g. code that was removed from
       smf_model_createHdr.c in commit
       2c342cd4f7d9ab33e4fea5bc250eaae9804a229f
    */
    if( data->hdr ) {
      hdr = smf_deepcopy_smfHead( data->hdr, status );
    }

    *components = smf_construct_smfData( NULL, NULL, hdr, NULL, NULL,
                                         SMF__DOUBLE, NULL, NULL,
                                         SMF__QFAM_TSERIES, NULL,
                                         1, dims, lbnd, 3, 0, 0, NULL,
                                         NULL, status );

    if( *status == SAI__OK ) {
      (*components)->pntr[0] = comp;
      comp = NULL;  /* Set to NULL to avoid freeing before exiting */
    }
  }

  /* Returning amplitudes? */
  if( (*status==SAI__OK) && amplitudes ) {
    dim_t dims[3];
    int lbnd[3];

    smf_qual_t *q=NULL;   /* Quality array that just maps SMF__Q_BADB */

    if( qua ) {
      q = astCalloc( nbolo*ngoodbolo, sizeof(*q), 1 );

      if( *status == SAI__OK ) {
        for( i=0; i<nbolo; i++ ) {        /* bolometer */
          for( j=0; j<ngoodbolo; j++ ) {  /* component amplitude */
            q[i*abstride + j*acompstride] = qua[i*bstride]&SMF__Q_BADB;
          }
        }
      }
    }

    if( data->isTordered ) { /* if time-ordered bolos first 2 dims */
      dims[0] = data->dims[0];
      dims[1] = data->dims[1];
      lbnd[0] = data->lbnd[0];
      lbnd[1] = data->lbnd[1];
    } else {                 /* if bolo-ordered bolos last 2 dims */
      dims[0] = data->dims[1];
      dims[1] = data->dims[2];
      lbnd[0] = data->lbnd[1];
      lbnd[1] = data->lbnd[2];
    }

    /* last dimension enumerates component */
    dims[2] = ngoodbolo;
    lbnd[2] = 0;


    *amplitudes = smf_construct_smfData( NULL, NULL, NULL, NULL, NULL,
                                         SMF__DOUBLE, NULL, q,
                                         SMF__QFAM_TSERIES, NULL,
                                         1, dims, lbnd, 3, 0, 0, NULL,
                                         NULL, status );
    if( *status==SAI__OK ) {
      (*amplitudes)->pntr[0] = amp;
      amp = NULL;  /* Set to NULL to avoid freeing before exiting */
    }
  }

  /* Clean up */
 CLEANUP:
  if( amp ) amp = astFree( amp );
  if( comp ) comp = astFree( comp );
  if( goodbolo ) goodbolo = astFree( goodbolo );
  if( cov ) gsl_matrix_free( cov );
  if( s ) gsl_vector_free( s );
  if( v ) gsl_matrix_free( v );
  if( work ) gsl_vector_free( work );

  if( job_data ) {
    for( ii=0; ii<nw; ii++ ) {
      pdata = job_data + ii;
      if( pdata->goodbolo ) pdata->goodbolo = astFree( pdata->goodbolo );
      if( pdata->covwork ) pdata->covwork = astFree( pdata->covwork );
    }
    job_data = astFree(job_data);
  }
}