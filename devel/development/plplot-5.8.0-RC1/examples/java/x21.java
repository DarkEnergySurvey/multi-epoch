//---------------------------------------------------------------------------//
// $Id: x21.java 6815 2006-07-19 07:13:56Z andrewross $
//---------------------------------------------------------------------------//
//
//---------------------------------------------------------------------------//
// Copyright (C) 2006  Andrew Ross
// Copyright (C) 2004  Alan W. Irwin
//
// This file is part of PLplot.
//
// PLplot is free software; you can redistribute it and/or modify
// it under the terms of the GNU Library General Public License as published by
// the Free Software Foundation; version 2 of the License.
//
// PLplot is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with PLplot; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
//---------------------------------------------------------------------------//
//
//---------------------------------------------------------------------------//
// Implementation of PLplot example 21 in Java.
//---------------------------------------------------------------------------//

package plplot.examples;

import plplot.core.*;

import java.lang.Math;
import java.util.Random;

class x21 {

    // Class data
    PLStream pls = new PLStream();

    double xm, xM, ym, yM;

    // Options data structure definition.
    static int pts = 500;
    static int xp = 25;
    static int yp = 20;
    static int nl = 15;
    static int knn_order = 20;
    static double threshold = 1.001;
    static double wmin = -1e3;
    static int randn = 0;
    static int rosen = 0;

    //  static PLOptionTable options[];


    // PLOptionTable options[] = {
    //   {
    //     "npts",
    //     NULL,
    //     NULL,
    //     &pts,
    //     PL_OPT_INT,
    //     "-npts points",
    //     "Specify number of random points to generate [500]" },
    //   {
    //     "randn",
    //     NULL,
    //     NULL,
    //     &randn,
    //     PL_OPT_BOOL,
    //     "-randn",
    //     "Normal instead of uniform sampling -- the effective \n\
    // \t\t\t  number of points will be smaller than the specified." },
    //   {
    //     "rosen",
    //     NULL,
    //     NULL,
    //     &rosen,
    //     PL_OPT_BOOL,
    //     "-rosen",
    //     "Generate points from the Rosenbrock function."},
    //   {
    //     "nx",
    //     NULL,
    //     NULL,
    //     &xp,
    //     PL_OPT_INT,
    //     "-nx points",
    //     "Specify grid x dimension [25]" },
    //   {
    //     "ny",
    //     NULL,
    //     NULL,
    //     &yp,
    //     PL_OPT_INT,
    //     "-ny points",
    //     "Specify grid y dimension [20]" },
    //   {
    //     "nlevel",
    //     NULL,
    //     NULL,
    //     &nl,
    //     PL_OPT_INT,
    //     "-nlevel ",
    //     "Specify number of contour levels [15]" },
    //   {
    //     "knn_order",
    //     NULL,
    //     NULL,
    //     &knn_order,
    //     PL_OPT_INT,
    //     "-knn_order order",
    //     "Specify the number of neighbors [20]" },
    //   {
    //     "threshold",
    //     NULL,
    //     NULL,
    //     &threshold,
    //     PL_OPT_FLOAT,
    //     "-threshold float",
    //     "Specify what a thin triangle is [1. < [1.001] < 2.]" },

    //   {
    //     NULL,                       /* option */
    //     NULL,                       /* handler */
    //     NULL,                       /* client data */
    //     NULL,                       /* address of variable to set */
    //     0,                          /* mode flag */
    //     NULL,                       /* short syntax */
    //     NULL }                      /* long syntax */
    // };

    x21( String[] args ) {
	double x[], y[], z[], clev[];
	double xg[], yg[], zg[][];
	double xg0[][], yg0[][];
	double zmin, zmax, lzm[], lzM[];
	long ct;
	int i, j, k;
	int alg;
	String ylab, xlab;
	String title[] = {"Cubic Spline Approximation",
			 "Delaunay Linear Interpolation",
			 "Natural Neighbors Interpolation",
			 "KNN Inv. Distance Weighted",
			 "3NN Linear Interpolation",
			 "4NN Around Inv. Dist. Weighted"};
	
	double opt[] = {0., 0., 0., 0., 0., 0.};
	
	xm = ym = -0.2;
	xM = yM = 0.8;
	
	// plplot initialization
	
	// Parse and process command line arguments.
	// pls.MergeOpts(options, "x22c options", NULL);
	pls.parseopts( args, PLStream.PL_PARSE_FULL | PLStream.PL_PARSE_NOPROGRAM );
	
	opt[2] = wmin;
	opt[3] = (double) knn_order;
	opt[4] = threshold;
	
	// Initialize PLplot.
	pls.init();
	
	x = new double[pts];
	y = new double[pts];
	z = new double[pts];

	create_data(x, y, z); /* the sampled data */
	zmin = z[0];
	zmax = z[0];
	for (i=1; i<pts; i++) {
	    if (z[i] > zmax)
		zmax = z[i];
	    if (z[i] < zmin)
		zmin = z[i];
	}
	
	xg = new double[xp];
	yg = new double[yp];

	create_grid(xg, yg); /* grid the data at */
	zg = new double[xp][yp]; /* the output grided data */

	xg0 = new double[xp][yp];
	yg0 = new double[xp][yp];

	for (i=0;i<xp;i++) {
	    for (j=0;j<yp;j++) {
		xg0[i][j] = xg[i];
		yg0[i][j] = yg[j];
	    }
	}
	
	clev = new double[nl];
	
	xlab = new String("Npts="+pts+" gridx="+xp+" gridy="+yp);
	pls.col0(1);
	pls.env(xm, xM, ym, yM, 2, 0);
	pls.col0(15);
	pls.lab(xlab, "", "The original data");
	pls.col0(2);
	pls.poin(x, y, 5);
	pls.adv(0);
	
	pls.ssub(3,2);
	
	for (k=0; k<2; k++) {
	    pls.adv(0);
	    for (alg=1; alg<7; alg++) {
		
		ct = System.currentTimeMillis();
		pls.griddata(x, y, z, xg, yg, zg, alg, opt[alg-1]);
		xlab = new String("time="+(System.currentTimeMillis() - ct)/1000+" ms");
		ylab = new String("opt="+opt[alg-1]);
		
		/* - CSA can generate NaNs (only interpolates?!).
		 * - DTLI and NNI can generate NaNs for points outside the 
		 *      convex hull of the data points.
		 * - NNLI can generate NaNs if a sufficiently thick triangle 
		 *      is not found
		 *
		 * PLplot should be NaN/Inf aware, but changing it now is 
		 * quite a job... so, instead of not plotting the NaN 
		 * regions, a weighted average over the neighbors is done.
		 */
		
		if (alg == PLStream.GRID_CSA || alg == PLStream.GRID_DTLI || 
		    alg == PLStream.GRID_NNLI || alg == PLStream.GRID_NNI) {
		    int ii, jj;
		    double dist, d;
		    
		    for (i=0; i<xp; i++) {
			for (j=0; j<yp; j++) {
			    if (Double.isNaN(zg[i][j])) { /* average (IDW) over the 8 neighbors */
				
				zg[i][j] = 0.; dist = 0.;
				
				for (ii=i-1; ii<=i+1 && ii<xp; ii++) {
				    for (jj=j-1; jj<=j+1 && jj<yp; jj++) {
					if (ii >= 0 && jj >= 0 && !Double.isNaN(zg[ii][jj])) {
					    d = (Math.abs(ii-i) + Math.abs(jj-j)) == 1 ? 1. : 1.4142;
					    zg[i][j] += zg[ii][jj]/(d*d);
					    dist += d;
					}
				    }
				}
				if (dist != 0.)
				    zg[i][j] /= dist;
				else
				    zg[i][j] = zmin;
				
			    }
			}
		    }
		}
		
		lzm = new double[1];
		lzM = new double[1];
		pls.minMax2dGrid(zg, lzM, lzm);

		pls.col0(1);
		pls.adv(alg);
		
		if (k == 0) {
		    
		    lzm[0] = Math.min(lzm[0], zmin);
		    lzM[0] = Math.max(lzM[0], zmax);
		    for (i=0; i<nl; i++)
			clev[i] = lzm[0] + (lzM[0]-lzm[0])/(nl-1)*i;
		    
		    pls.env0(xm, xM, ym, yM, 2, 0);
		    pls.col0(15);
		    pls.lab(xlab, ylab, title[alg-1]);
		    pls.shades(zg, xm, xM, ym, yM,
			       clev, 1, 0, 1, true, xg0, yg0);
		    pls.col0(2);
		} else {
		    
		    for (i=0; i<nl; i++)
			clev[i] = lzm[0] + (lzM[0]-lzm[0])/(nl-1)*i;
		    
		    cmap1_init();
		    pls.vpor(0.0, 1.0, 0.0, 0.9);
		    pls.wind(-1.0, 1.0, -1.0, 1.5);
		    /*
		     * For the comparison to be fair, all plots should have the
		     * same z values, but to get the max/min of the data 
		     * generated by all algorithms would imply two passes. 
		     * Keep it simple.
		     *
		     * plw3d(1., 1., 1., xm, xM, ym, yM, zmin, zmax, 30, -60);
		     */
		    
		    pls.w3d(1., 1., 1., xm, xM, ym, yM, lzm[0], lzM[0], 30, -60);
		    pls.box3("bnstu", ylab, 0.0, 0,
			     "bnstu", xlab, 0.0, 0,
			     "bcdmnstuv", "", 0.0, 4);
		    pls.col0(15);
		    pls.lab("", "", title[alg-1]);
		    pls.plot3dc(xg, yg, zg, PLStream.DRAW_LINEXY | 
				PLStream.MAG_COLOR | PLStream.BASE_CONT, 
				clev);
		}
	    }
	}

	pls.end();
	
    }

    void cmap1_init() {

	double i[] = new double[2];
	double h[] = new double[2];
	double l[] = new double[2];
	double s[] = new double[2];

	i[0] = 0.0;           /* left boundary */
	i[1] = 1.0;           /* right boundary */

	h[0] = 240; /* blue . green . yellow . */
	h[1] = 0;   /* . red */

	l[0] = 0.6;
	l[1] = 0.6;

	s[0] = 0.8;
	s[1] = 0.8;

	pls.scmap1n(256);
	pls.scmap1l(false, i, h, l, s);
    }

    void create_grid(double xx[], double yy[]) {

	int i;
	int px = xx.length;
	int py = yy.length;

	for (i=0; i<px; i++)
	    xx[i] = xm + (xM-xm)*i/(px-1.);

	for (i=0; i<py; i++)
	    yy[i] = ym + (yM-ym)*i/(py-1.);
    }

    void create_data(double x[], double y[], double z[]) {
	int i;
	double r;
	double xt, yt;
	Random rnd = new Random();
	int pts = x.length;

	for(i=0; i<pts; i++) {
	    xt = rnd.nextDouble();
	    yt = rnd.nextDouble();
	    if (randn == 0) {
		x[i] = xt + xm;
		y[i] = yt + ym;
	    } else { /* std=1, meaning that many points are outside the plot range */
		x[i] = Math.sqrt(-2.*Math.log(xt)) * Math.cos(2.*Math.PI*yt) + xm;
		y[i] = Math.sqrt(-2.*Math.log(xt)) * Math.sin(2.*Math.PI*yt) + ym;
	    }
	    if (rosen == 0) {
		r = Math.sqrt((x[i]) * (x[i]) + (y[i]) * (y[i]));
		z[i] = Math.exp(-r * r) * Math.cos(2.0 * Math.PI * r);
	    } else {
		z[i] = Math.log(Math.pow((1. - x[i]), 2.) + 100. * Math.pow((y[i] - Math.pow(x[i], 2.)), 2.));
	    }
	}
    }

    public static void main( String[] args ) {
	new x21( args );
    }

}

//---------------------------------------------------------------------------//
//                              End of x21.java
//---------------------------------------------------------------------------//


