#!/usr/bin/env python

import numpy as np

# TODO:
# Add logger to print

DEFAULT_MINCOLS = 1     # Narrowest feature to interpolate
DEFAULT_MAXCOLS = None  # Widest feature to interpolate.  None means no limit.

def zipper_interp(image,mask,interp_mask,axis=1,**kwargs):

    """
    Calls either zipper_interp_rows (axis=1) or zipper_interp_cols (axis=2)
    """
    if axis == 1:
        return zipper_interp_rows(image,mask,interp_mask,**kwargs)
    elif axis == 2:
        return zipper_interp_cols(image,mask,interp_mask,**kwargs)
    else:
        raise ValueError("ERROR: Need to specify axis as axis=1 or axis=2")
        
def zipper_interp_rows(image,mask,interp_mask,**kwargs):

    """
    Performs zipper row interpolation.
    Extracted from Gary Berstein's row_interp.py inside pixcorrect

    Interpolate over selected pixels by inserting average of pixels to left and right
    of any bunch of adjacent selected pixels.  If the interpolation region touches an
    edge, or the adjacent pixel has flags marking it as invalid, than the value at
    other border is used for interpolation.  No interpolation is done if both
    boundary pixels are invalid.

    Returns the 'image' back and if 'BADPIX_INTERP' is not None
    it returns a tuple with image,mask
    
    :Postional parameters:
       'image': the 2D numpy array input image
       'mask':  the 2D numpy array input image
       'interp_mask': Mask bits that will trigger interpolation

    :Optional parameters (passed as **kwargs)
       'BADPIX_INTERP': bit value to assign to interpolated pixels (off by default)
       'min_cols': Minimum width of region to be interpolated.
       'max_cols': Maximum width of region to be interpolated.
       'invalid_mask': Mask bits invalidating a pixel as interpolation source.
       'logger' : Logger object for logging info

    """

    # Extract kwargs for optional params
    BADPIX_INTERP = kwargs.get('BADPIX_INTERP',None)
    invalid_mask  = kwargs.get('invalid_mask',0)
    min_cols = kwargs.get('DEFAULT_MINCOLS',DEFAULT_MINCOLS)
    max_cols = kwargs.get('DEFAULT_MAXCOLS',DEFAULT_MAXCOLS)
    logger   = kwargs.get('logger',None)
    
    msg = 'Zipper interpolation along rows'
    if logger:logger.info(msg)
    else: print "#",msg

    # Find the pixels to work with
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
        print ystart,yend ###
        return 1
    
    # Narrow our list to runs of the desired length range
    # not touching the edges
    use = xend-xstart >= min_cols
    if max_cols is not None:
        use = np.logical_and(xend-xstart<=max_cols, use)
    use = np.logical_and(xstart>0, use)
    use = np.logical_and(xend<interpolate.shape[0], use)
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

    if BADPIX_INTERP:
        return image,mask
    else:
        return image

def zipper_interp_cols(image,mask,interp_mask,**kwargs):

    """
    Performs zipper column interpolation 
    Extracted and adapted from Gary Berstein in coadd-prepare

    Interpolate over selected pixels by inserting average of pixels to
    top and bottom of any bunch of adjacent selected pixels. For
    column interpolation we do not attempt to determine invalid
    pixels, as it is done for row_interp. The column interpolation is
    meant for coadded images, which do not have a bit to flag
    'invalid_mask.'

    Returns the 'image' back and if 'BADPIX_INTERP' is not None
    it returns a tuple with image,mask
        
    :Postional parameters:
       'image': the 2D numpy array input image
       'mask':  the 2D numpy array input image
       'interp_mask': Mask bits that will trigger interpolation

    :Optional parameters (passed as **kwargs)
       'BADPIX_INTERP': bit value to assign to interpolated pixels (off by default)
       'min_cols': Minimum width of region to be interpolated.
       'max_cols': Maximum width of region to be interpolated.
       'logger' : Logger object for logging info



    """

    # Extract kwargs for optional params
    BADPIX_INTERP = kwargs.get('BADPIX_INTERP',None)
    min_cols = kwargs.get('DEFAULT_MINCOLS',DEFAULT_MINCOLS)
    max_cols = kwargs.get('DEFAULT_MAXCOLS',DEFAULT_MAXCOLS)
    logger   = kwargs.get('logger',None)
    
    msg = 'Zipper interpolation along columns'
    if logger:logger.info(msg)
    else: print "#",msg

    # Find the pixels to work with
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

    # Change the bit in the mask to reflect the pixel was interpolated
    if BADPIX_INTERP:
        return image,mask
    else:
        return image
