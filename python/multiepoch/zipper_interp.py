#!/usr/bin/env python

import numpy as np

# TODO:
# Add logger to print

def zipper_inter_rows(image,mask,**kwargs):

    """
    Performs zipper row interpolation.
    Extracted from Gary Berstein's row_interp.py inside pixcorrect
    """
    
    BADPIX_INTERP = kwargs.get('BADPIX_INTERP',None)

    print 'Interpolating along rows'

    interpolate = np.array(mask & interp_mask, dtype=bool)
    # Make arrays noting where a run of bad pixels starts or ends
    # Then make arrays has_?? which says whether left side is valid
    # and an array with the value just to the left/right of the run.
    work = np.array(interpolate)
    work[:,1:] = np.logical_and(interpolate[:,1:], ~interpolate[:,:-1])
    ystart,xstart = np.where(work)
    
    work = np.array(interpolate)
    work[:,:-1] = np.logical_and(interpolate[:,:-1], ~interpolate[:,1:])
    yend, xend = np.where(work)
    xend = xend + 1   # Make the value one-past-end
    
    # If we've done this correctly, every run has a start and an end.
    if not np.all(ystart==yend):
        print "Logic problem, ystart and yend not equal."
        return 1
    
    # Narrow our list to runs of the desired length range
    use = xend-xstart >= min_cols
    if max_cols is not None:
        use = np.logical_and(xend-xstart<=max_cols, use)
    xstart = xstart[use]
    xend   = xend[use]
    ystart = ystart[use]

    # Now determine which runs have valid data at left/right
    xleft    = np.maximum(0, xstart-1)
    has_left = ~np.array(mask[ystart,xleft] & invalid_mask, dtype=bool)
    has_left = np.logical_and(xstart>=1,has_left)
    left_value = image[ystart,xleft]
    
    xright = np.minimum(work.shape[1]-1, xend)
    has_right = ~np.array(mask[ystart,xright] & invalid_mask, dtype=bool)
    has_right = np.logical_and(xend<work.shape[1],has_right)
    right_value = image[ystart,xright]
        
    # Assign right-side value to runs having just right data
    for run in np.where(np.logical_and(~has_left,has_right))[0]:
        image[ystart[run],xstart[run]:xend[run]] = right_value[run]
        if BADPIX_INTERP:
            mask[ystart[run],xstart[run]:xend[run]] |= BADPIX_INTERP
    # Assign left-side value to runs having just left data
    for run in np.where(np.logical_and(has_left,~has_right))[0]:
        image[ystart[run],xstart[run]:xend[run]] = left_value[run]
        if BADPIX_INTERP:
            mask[ystart[run],xstart[run]:xend[run]] |= BADPIX_INTERP

    # Assign mean of left and right to runs having both sides
    for run in np.where(np.logical_and(has_left,has_right))[0]:
        image[ystart[run],xstart[run]:xend[run]] = \
          0.5*(left_value[run]+right_value[run])
        if BADPIX_INTERP:
            mask[ystart[run],xstart[run]:xend[run]] |= BADPIX_INTERP

    # Move this to the outside
    # Add to image history
    #image['HISTORY'] =time.asctime(time.localtime()) + \
    #                   ' row_interp over mask 0x{:04X}'.format(interp_mask)
    
    print 'Finished interpolating rows'
    if BADPIX_INTERP:
        return image,mask
    else:
        return image

def zipper_inter_cols(image,mask,interp_mask,**kwargs):

    BADPIX_INTERP = kwargs.get('BADPIX_INTERP',None)
    
    """
    Performs zipper column interpolation 
    Extracted from Gary Berstein coadd-prepare
    """

    print 'Interpolating along rows'
    interpolate = np.array(mask & interp_mask, dtype=bool)

    # Identify column runs to interpolate, start by marking beginnings of runs
    work = np.array(interpolate)
    work[1:,:] = np.logical_and(interpolate[1:,:], ~interpolate[:-1,:])
    xstart,ystart = np.where(work.T)

    # Now ends of runs
    work = np.array(interpolate)
    work[:-1,:] = np.logical_and(interpolate[:-1,:], ~interpolate[1:,:])
    xend, yend = np.where(work.T)
    yend = yend + 1   # Make the value one-past-end
    
    # If we've done this correctly, every run has a start and an end, on same col
    if not np.all(xstart==xend):
        print "Logic problem, xstart and xend not equal."
        print xstart,xend ###
        return 1

    # Narrow our list to runs of the desired length range and
    # not touching the edges
    use = yend-ystart >= min_cols
    if max_cols is not None:
        use = np.logical_and(yend-ystart<=max_cols, use)
    use = np.logical_and(ystart>0, use)
    use = np.logical_and(yend<interpolate.shape[0], use)
    ystart = ystart[use]
    yend   = yend[use]
    xstart = xstart[use]

    # Assign mean of top and bottom to runs
    for run in range(len(xstart)):
        image[ystart[run]:yend[run],xstart[run]] = \
          0.5*(image[ystart[run]-1,xstart[run]] +
               image[yend[run],xstart[run]])
        if BADPIX_INTERP:
            mask[ystart[run]:yend[run],xstart[run]] |= BADPIX_INTERP
        
    if BADPIX_INTERP:
        return image,mask
    else:
        return image
