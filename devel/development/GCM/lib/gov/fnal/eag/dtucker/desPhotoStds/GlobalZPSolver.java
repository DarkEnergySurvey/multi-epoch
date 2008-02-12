package gov.fnal.eag.dtucker.desPhotoStds;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.util.StringTokenizer;
import java.util.Arrays;
import java.util.TreeMap;
import java.util.TreeSet;

import nom.tam.fits.Fits;
import nom.tam.util.BufferedFile;

import cern.colt.list.DoubleArrayList;
import cern.colt.matrix.DoubleMatrix2D;
import cern.colt.matrix.impl.DenseDoubleMatrix2D;
import cern.colt.matrix.impl.SparseDoubleMatrix2D;
import cern.colt.matrix.linalg.Algebra;


/**
 * This class defines methods for solving the region-to-region 
 * zeropoint offsets for a set of overlapping regions using the 
 * methodology of Glazebrook et al. (1994).  The zeropoint offsets 
 * are solved for by inverting an NxN matrix, where N=the number 
 * of regions.  At least one of the regions needs to be set as a 
 * reference or calibration region; the zeropoint offsets for all 
 * reference/calibration regions are fixed to a value of 0. 
 * 
 * NOTE:  the "solve" method in this class performs a sanity check 
 * to identify any disconnected "islands" of overlapping regions 
 * that contain no calibrated reference region; regions in any 
 * such disconnected "islands" are marked as uncalibrateable and 
 * their zeropoint offsets are set to "-9999".
 * 
 * (We use the term "region" throughout to emphasize that the code 
 * can deal with generic overlapping frames of no particular shape 
 * or size.)  
 * 
 * @author dtucker
 *
 */
public class GlobalZPSolver {
	
	//Instance variables dealing with this run of the Global Calibrations Module
	private Date date = new Date();
	
	//Instance variables dealing with the observed data to be calibrated
	private String starMatchFileName  = null;
	private String outputFileName = null;
	
	//General instance variables...
	private String[] filterList = {"u", "g", "r", "i", "z"};
	private int verbose = 0;
	
	// The method "solve" is the heart of this class...
	public void solve () throws Exception {
	
		System.out.println("\n\nGlobalZPSolver");
		System.out.println("Start Time: \t" + (new Date()).toString());

		if (verbose > 0) {
			System.out.println("");
			System.out.println("The beginning...");
			System.out.println("");
		}
		
		//Open the file starMatchFileName and prepare to read it in.  This file 
		// contains all the unique star matches in all the overlapping regions...
		File starMatchFile = new File(starMatchFileName);
		if (starMatchFile.exists() == false || starMatchFile.canRead() == false) {
			System.out.println(starMatchFileName + " either does not exist or cannot be read");
			return;
		}
		FileReader fileReader = new FileReader(starMatchFile);
		BufferedReader reader = new BufferedReader(fileReader);
		
		//Instantiate ArrayLists to contain the region id's, the matched star 
		// info, and the raw magnitude offsets in each overlap...
		ArrayList regionidList = new ArrayList();
		ArrayList matchedStarList = new ArrayList();
		ArrayList regionRawOffsetsList = new ArrayList();
		int regionRawOffsetsListSize = 0;
		
		//Instantiate a TreeSet to hold a list of all the unique overlapping 
		// region pairs, a TreeMap to connect a regionPair with with the list
		// of delta_mags for matching stars in the overlap, and a TreeMap to
		// connect a region with its calibration flag (1=use this region as 
		// a zeropoint reference, 0=calculate the zeropoint offset for this 
		// region, -1=uncalibrateable).
		Set regionPairSet = new TreeSet();		
		Map regionPairMap = new TreeMap();
		Map regionCalFlagMap = new TreeMap();
		
		Map regionRAMap = new TreeMap();
		Map regionDecMap = new TreeMap();
		
		int iLine = 0;
		String line = null;
		
		//Read in the contents of the file starMatchFileName...
		while ((line = reader.readLine()) != null) {
			
			if (line.length() == 0) {
				System.out.println("line \'" + line + "\' has zero length...  skipping...");
				continue;
			}
			
			if (line.charAt(0) == '#') {
				System.out.println("line \'" + line + "\' commented out...  skipping...");
				continue;
			}
			
			StringTokenizer st = new StringTokenizer(line);
			int nTokens = st.countTokens();
			if (nTokens != 19) {
				System.out.println("line \'" + line + "\' does not contain 19 fields...  skipping...");
				continue;
			}
			
			// Create and fill a MatchedStar object...
			// (Since switching to using Sets and Maps -- see below -- 
			//  we actually no longer use the MatchedStar class for 
			//  anything in particular, but let's keep it for now.)
			MatchedStar matchedStar = new MatchedStar();
			
			int regionid1 = Integer.parseInt(st.nextToken());
			matchedStar.setRegionid1(regionid1);
			double raCenDeg1 = Double.parseDouble(st.nextToken());
			matchedStar.setRegionRaCenDeg1(raCenDeg1);
			double decCenDeg1 = Double.parseDouble(st.nextToken());
			matchedStar.setRegionDecCenDeg1(decCenDeg1);
			int regionCalFlag1 = Integer.parseInt(st.nextToken());
			matchedStar.setRegionCalFlag1(regionCalFlag1);
			matchedStar.setStarid1(Integer.parseInt(st.nextToken()));
			matchedStar.setRaDeg1(Double.parseDouble(st.nextToken()));
			matchedStar.setDecDeg1(Double.parseDouble(st.nextToken()));
			double mag1 = Double.parseDouble(st.nextToken());
			matchedStar.setMag1(mag1);
			matchedStar.setMagErr1(Double.parseDouble(st.nextToken()));

			int regionid2 = Integer.parseInt(st.nextToken());
			matchedStar.setRegionid2(regionid2);
			double raCenDeg2 = Double.parseDouble(st.nextToken());
			matchedStar.setRegionRaCenDeg2(raCenDeg2);
			double decCenDeg2 = Double.parseDouble(st.nextToken());
			matchedStar.setRegionDecCenDeg2(decCenDeg2);
			int regionCalFlag2 = Integer.parseInt(st.nextToken());
			matchedStar.setRegionCalFlag2(regionCalFlag2);
			matchedStar.setStarid2(Integer.parseInt(st.nextToken()));
			matchedStar.setRaDeg2(Double.parseDouble(st.nextToken()));
			matchedStar.setDecDeg2(Double.parseDouble(st.nextToken()));
			double mag2 = Double.parseDouble(st.nextToken());
			matchedStar.setMag2(mag2);
			matchedStar.setMagErr2(Double.parseDouble(st.nextToken()));

			matchedStar.setSepArcsec12(Double.parseDouble(st.nextToken()));
			
			// Add the MatchedStarObject to the matchedStar ArrayList...
			matchedStarList.add(matchedStar);
			
			double dmag = mag1-mag2;

			//If we haven't encountered this region before, add its id to the 
			// regionidList, add its regionCalFlag to the regionCalFlagMap, 
			// and add its RA and DEC respectively to the regionRAMap and 
			// regionDecMap.  (Recall that a map is like a dictionary, in 
			// that it links a key (word) to a value (definition), so the 
			// regionCalFlagMap will link a regionid with its regionCalFlag.)
			// For now, we will set the regionCalFlag to either a value of
			// 1 (reference/calibrated region) or a value of 0 (uncalibrated
			// region).  Later, we will determine whether any of the 
			// uncalibrated regions are actually uncalibrateable (and thus
			// should have their regionCalFlag set to -1).
			if (regionidList.contains(regionid1) == false) {
				regionidList.add(new Integer(regionid1));
				regionCalFlagMap.put(regionid1,(Integer) regionCalFlag1);
				regionRAMap.put(regionid1,(Double) raCenDeg1);
				regionDecMap.put(regionid1,(Double) decCenDeg1);
			}
			if (regionidList.contains(regionid2) == false) {
				regionidList.add(new Integer(regionid2));
				regionCalFlagMap.put(regionid2,(Integer) regionCalFlag2);
				regionRAMap.put(regionid2,(Double) raCenDeg2);
				regionDecMap.put(regionid2,(Double) decCenDeg2);
			}

			//Create a regionPairName and add it to the regionPairSet...
			// (Recall that a Set is a Collection that forbids duplicate 
			// entries, so the regionPairSet will contain a collection of 
			// unique regionPairNames.) 
			String regionPairName = regionid1 + " " + regionid2;
			regionPairSet.add(regionPairName);			
			
			//If the regionPairMap does not contain a key with value 
			// regionPairName, create a new RegionRawOffsets object and 
			// add it to the regionRawOffsetsList; otherwise, update the 
			// already existing RegionRawOffsets object within the 
			// regionRawOffsetsList for that regionPairName.  What we 
			// are doing here is collecting all the delta_mags for each 
			// overlap region together and linking this collection with 
			// the unique regionPairName...
			if (regionPairMap.containsKey(regionPairName) == false) {
				regionPairMap.put(regionPairName, (Integer) regionRawOffsetsListSize);
				RegionRawOffsets rro = new RegionRawOffsets();
				rro.setRegionid1(regionid1);
				rro.setRegionid2(regionid2);
				DoubleArrayList dmagList = new DoubleArrayList();
				dmagList.add(new Double(dmag));
				rro.setDmagList(dmagList);
				regionRawOffsetsList.add(rro);
				regionRawOffsetsListSize++;	
			} else {
				int kk = (Integer) regionPairMap.get(regionPairName);
				RegionRawOffsets rro = (RegionRawOffsets) regionRawOffsetsList.get(kk);
				DoubleArrayList dmagList = rro.getDmagList();
				dmagList.add(new Double(dmag));
				rro.setDmagList(dmagList);		
			}
			
			//Output lines to standard output, but be reasonable in the number 
			// of lines output, esp. for large files...
			if (verbose > 0) {
				if (    (iLine <     100) || 
						(iLine <    1000 && iLine%100  == 0) || 
						(iLine <   10000 && iLine%1000 == 0) || 
						(iLine%10000 == 0) 
						) {
					System.out.println("line " + iLine + ":  " + line);
				}
			}
			
			iLine++;
			
		}

		reader.close();
		int matchedStarListSize = matchedStarList.size();

		if (verbose > 0) {
			System.out.println("There are " + matchedStarListSize + " entries in matchedStarList.");
		}
				
		if (verbose > 0) {
			System.out.println("There are " + regionRawOffsetsListSize + " entries in regionRawOffsetsList.");
		}
		
		if (verbose > 1) {
			System.out.println("TreeMap regionPairMap contains " + regionPairMap.size() + " entries");
			System.out.println("output of TreeMap map:");
			Iterator iter = regionPairSet.iterator();
			while (iter.hasNext()) {
				String regionPairName = (String) iter.next();
				System.out.println(regionPairName +  " " + regionPairMap.get(regionPairName));
			}
		}

		//Sort regionidList...
		Collections.sort(regionidList);	
		int regionidListSize = regionidList.size();
		if (verbose > 1) {
			for (int ii=0; ii<regionidListSize; ii++) {
				System.out.println(ii + " " + Integer.parseInt(regionidList.get(ii).toString()));
			}
		}

				
		//Loop through all the regionRawOffsets objects on the 
		// regionRawOffsetsList, and calculate the median, mean, std error 
		// of the dmagList for each region1-region2 overlap region...
		for (int kk=0; kk<regionRawOffsetsListSize; kk++) {
			RegionRawOffsets rro = (RegionRawOffsets) regionRawOffsetsList.get(kk);
			int regionid1 = rro.getRegionid1();
			int regionid2 = rro.getRegionid2();
			DoubleArrayList dmagList = rro.getDmagList();
			if (dmagList != null) { 
				int dmagListSize = dmagList.size();
				double [] dmagArray = new double[dmagListSize];
				for (int mm=0; mm< dmagListSize; mm++) {
					dmagArray[mm] = dmagList.get(mm);
				}
				//Calculate median...
				Arrays.sort(dmagArray);
				int result = dmagArray.length % 2;
				double median = 0;
				if (result == 0) {
					int rightNumber = dmagArray.length / 2;
					int leftNumber = rightNumber - 1;
					median = (dmagArray[rightNumber] + dmagArray[leftNumber]) / 2;
				} else {
					median = dmagArray[dmagArray.length / 2];
				}
				//Calculate mean and error in the mean...
				double sum = 0;
				double sum2 = 0.;
				double mean = -1000;
				double sdMean = -1000;
				for (int mm=0; mm< dmagListSize; mm++) {
					sum = sum + dmagArray[mm];
					sum2 = sum2 + dmagArray[mm]*dmagArray[mm];
				}
				int ntot = dmagArray.length;
				mean = sum / ntot;
				if (ntot > 1) {
					double var = sum2/ntot - mean*mean;
					double ratio = (double) ntot / (double) (ntot-1);
					double sampleVar = ratio*var;
					sdMean = Math.sqrt(sampleVar/ntot);
				}
				rro.setDmagListMean(mean);
				rro.setDmagListSDMean(sdMean);
				rro.setDmagListMedian(median);
				rro.setDmagListSize(ntot);
				if (verbose > 0 && kk % 100 == 0) {
					System.out.println(regionid1 + "\t" + regionid2 + "\t" + dmagList.size() + "\t" + mean + "\t"+  sdMean + "\t" + median);
				} else if (verbose > 1) {
					System.out.println(regionid1 + "\t" + regionid2 + "\t" + dmagList.size() + "\t" + mean + "\t"+  sdMean + "\t" + median);
				}
				
			}
		
		}	

		
		//Remove overlapping region pairs with fewer than 5 matched stars...
		if (verbose > 0) {
			System.out.println("regionRawOffsetsListSize before culling:  " + regionRawOffsetsListSize);
		}
		for (int kk=0; kk<regionRawOffsetsListSize; kk++) {
			int kkinv = regionRawOffsetsListSize-kk-1;
			RegionRawOffsets rro = (RegionRawOffsets) regionRawOffsetsList.get(kkinv);
			int regionid1 = rro.getRegionid1();
			int regionid2 = rro.getRegionid2();
			int dmagListSize = rro.getDmagListSize();
			double dmagListMean = rro.getDmagListMean();
			double dmagListSDMean = rro.getDmagListSDMean();
			double dmagListMedian = rro.getDmagListMedian();
			if (dmagListSize < 5) {
				regionRawOffsetsList.remove(kkinv);
			}
		}				
		regionRawOffsetsListSize = regionRawOffsetsList.size();
		if (verbose > 0) {
			System.out.println("regionRawOffsetsListSize after culling:  " + regionRawOffsetsListSize);
		}
		
		if (verbose > 0) {
			for (int ii=0; ii<regionidListSize; ii++) {
				int regionid = (Integer) regionidList.get(ii);
				System.out.println(ii + " " + regionid + " " + 
						regionCalFlagMap.get((Integer) regionid));
			}
		}
		
		
		//Initialize some variables that will be used in characterizing the least
		// squares solution...
		double chi2 = -1;
		double rms  = -1;
		int dof     = -1;
		
		//Instantiate arrays that will be used in setting up matrix equation 
		// AA*XX=BB...
		double[] array1d = new double[regionidListSize];
		double[][] array2d = new double[regionidListSize][regionidListSize];
		double[][] dmagArray2d = new double[regionidListSize][regionidListSize];
		int[][] overlapArray2d = new int[regionidListSize][regionidListSize];
		
		//Instantiate matrices to be used in solving matrix equation AA*XX=BB...
		DoubleMatrix2D AA = null;
		DoubleMatrix2D BB = null;
		DoubleMatrix2D XX = null;
		DoubleMatrix2D AAinv = null;
		DoubleMatrix2D II = null;
		
		//Initialize arrays...
		for (int ii=0; ii<regionidListSize; ii++) {
			array1d[ii] = 0.;
			for (int jj=0; jj<regionidListSize; jj++) {
				array2d[ii][jj] = 0.;
				overlapArray2d[ii][jj] = 0;
				dmagArray2d[ii][jj] = 0.;
			}
		}
		
		//Populate arrays...
		for (int kk=0; kk<regionRawOffsetsListSize; kk++) {
			RegionRawOffsets rro = (RegionRawOffsets) regionRawOffsetsList.get(kk);
			int regionid1 = rro.getRegionid1();
			int regionid2 = rro.getRegionid2();
			//We'll use the median delta_mag for now.  Maybe later we will
			// switch to a weighted mean...
			double dmagListMedian = rro.getDmagListMedian();
			int ii = regionidList.indexOf((Integer) regionid1);
			int jj = regionidList.indexOf((Integer) regionid2);
			//We have reversed the signs of the AA matrix entries relative
			// to what is given in Glazebrook et al. (1994).  In the 
			// Glazebrook et al. convention, we would do the following:
			//   array2d[ii][ii]--
			//   array2d[ii][ii]--
			//   array2d[ii][jj] = 1;
			//   array2d[jj][ii] = 1;
			array2d[ii][ii]++;
			array2d[jj][jj]++;
			array2d[ii][jj] = -1;
			array2d[jj][ii] = -1;
			array1d[ii] = array1d[ii] + dmagListMedian;
			array1d[jj] = array1d[jj] - dmagListMedian;
			overlapArray2d[ii][jj] = 1;
			overlapArray2d[jj][ii] = 1;
			dmagArray2d[ii][jj] =  dmagListMedian;
			dmagArray2d[jj][ii] = -dmagListMedian;
		}		

		
		//Identify any disconnected "islands" of overlapping regions that 
		// contain no reference or calibrated region.  Regions in any such 
		// "islands" are uncalibrateable and would yield a singular 
		// (uninvertable) matrix.  The method for identifying these 
		// "islands" is based upon the Huchra & Gellar (1982) group-finding 
		// algorithm.

		// Instantiate some variables that will be used in the group-finding
		// algorithm.  The regionGroupFlag array indicates which regions have
		// already been "grouped".  The iadd array contains a list of regions
		// (actually, a list of indices representing the regions) in a
		// particular group/"island".  i and j are just indices.
		int [] regionGroupFlag = new int[regionidListSize];
		int [] iadd = new int[regionidListSize];
		int i;
		int j;

		// Initialize regionGroupFlag and iadd arrays...
		for (i=0; i<regionidListSize; i++) {
			regionGroupFlag[i] = 0;
			iadd[i]  = -1;
		}

		// ngrp is the group or "island" number; i0 is a 
		//  reference value for the index i.
		int ngrp = 0;
		int i0 = 0;

		// Continue looping until all groups/"islands" have 
		//  been identified.  Huchra & Geller (1982) required
		//  that a group have at least 3 members; here, we 
		//  allow "groups" or "islands" containing as few as
		//  a single member.
		while (true) {
			
			// Find a non-grouped region (a "seed" region)...
			for (i=i0; i<regionidListSize; i++) {
				if (regionGroupFlag[i] == 0) {
					break;
				}
			}
			
			if (i < regionidListSize) {
						
				// Here, we have found a seed region and
				// find all the other regions that are
				// part of this group/island.
				i0 = i;
				int n = 0;
				int m = 0;
				iadd[m] = i;
				regionGroupFlag[i] = 1;
						
				// Find all the companions to the original
				// "seed" region, then to its "friends," then
				// to each of their "friends," etc.
				while (m < regionidListSize && iadd[m] != -1) {

					i = iadd[m];

					// Find ALL "friends" of region "i=iadd[m],"
					// looking only at previously un-grouped
					// regions ("regionGroupFlag[j] = 0")...
					for (j=0; j<regionidListSize; j++) {
						if (regionGroupFlag[j] == 0) {
							// If regions i and j overlap, 
							// they are part of the same
							// group, so append j to the
							// list of members of this
							// group and flag it as 
							// "grouped"...
							if (overlapArray2d[i][j] != 0) {
								n++;
								iadd[n] = j;
								regionGroupFlag[j]  = 1;
							}
						}
					}
					
					m++;
				
				}
				
				// Does this group include a calibrated (i.e. 
				//  "reference") region?  
				int calibrateableFlag = -1;
				for (n=0; n<m; n++) {
					i = iadd[n];
					int regionid = ((Integer) regionidList.get(i)).intValue();
					int regionCalFlag = ((Integer)regionCalFlagMap.get((Integer) regionid)).intValue();
					if (regionCalFlag == 1) {
						calibrateableFlag = 1;
					}
				}

				// If appropriate, reset regionCalFlagMap for the regions 
				//  in this group.  Choices are:
				//    * regionCalFlag = 1  if the region is a calibrated
				//                         (or "reference") region; the
				//                         zeropoint offsets for these
				//                         regions will be fixed to a value
				//                         of zero and these regions will 
				//                         be used to calibrate uncalibrated
				//                         images in the same group
				//    * regionCalFlag = 0  if the region is not calibrated
				//                         but the group contains at least  
				//                         one calibrated (or "reference") 
				//                         region; we will solve for the 
				//                         zeropoint offsets to calibrate 
				//                         such regions
				//    * regionCalFlag = -1 if the region is not calibrated
				//                         and the group contains no 
				//                         calibrated (or "reference")
				//                         regions; we cannot solve for
				//                         the zeropoint offsets for 
				//                         such regions.  It is 
				//                         uncalibrateable.
				for (n=0; n<m; n++) {
					i = iadd[n];
					int regionid = ((Integer) regionidList.get(i)).intValue();
					
					int regionCalFlag0 = ((Integer)regionCalFlagMap.get((Integer) regionid)).intValue();
					int regionCalFlag  = regionCalFlag0;
					
					if (regionCalFlag == 0 && calibrateableFlag == -1) {
						regionCalFlag = -1;
						regionCalFlagMap.put(regionid, (Integer) regionCalFlag);
					}
					
					if (verbose > 0) {
						System.out.println(i + 
								" \t Region: "  + regionid + 
								" \t RA:  "     + regionRAMap.get((Integer) regionid) +
								" \t DEC: "     + regionDecMap.get((Integer) regionid) + 
								" \t Group: "   + ngrp + 
								" \t Calibrateable: " + calibrateableFlag + 
								" \t CalFlagOrig: " + regionCalFlag0 + 
								" \t CalFlagNew: " + regionCalFlag);
					}
				
				}
				
				//Reset iadd array for next group...
				for (n=0; n<m; n++) {
					iadd[n] = -1;
				}
				
				ngrp++;
				
			} else {
				
				// We did not find any more non-grouped
				//  regions to form the basis of a
				// group/island, so we break out of the 
				// while loop... 
				break;
				
			}
			
		}
		
		
		//Reset values for regions with calibrated zeropoints
		// (i.e., these are the "reference" regions for which  
		// we fix the zeropoint offsets to a value of 0) and 
		// for regions that are uncalibrateable (i.e., we fix
		// the zeropoint offsets for these to a value of 
		// -9999).
		for (int ii=0; ii<regionidListSize; ii++) {
			int region = ((Integer) regionidList.get(ii)).intValue();
			int regionCalFlag = ((Integer) regionCalFlagMap.get((Integer) region)).intValue();
			if (regionCalFlag == 1) {
				for (int jj=0; jj<regionidListSize; jj++) {
					array2d[ii][jj] = 0.;
				}
				array2d[ii][ii] = 1;
				array1d[ii] = 0.;
			} else if (regionCalFlag == -1) {
				for (int jj=0; jj<regionidListSize; jj++) {
					array2d[ii][jj] = 0.;
				}
				array2d[ii][ii] = 1;
				array1d[ii] = -9999.;
				
			}
		}
		
		
		//Now convert the arrays into matrices.
		AA = new SparseDoubleMatrix2D(regionidListSize,regionidListSize);
		BB = new DenseDoubleMatrix2D(regionidListSize,1);
		for (int ii=0; ii<regionidListSize; ii++) {
			BB.set(ii,0,array1d[ii]);
			for (int jj=0; jj<regionidListSize; jj++) {
				AA.set(ii,jj,array2d[ii][jj]);
			}
		}
		
		//QA:  create and output a FITS image file of the matrix AA.
		if (verbose > 0) {
			if (regionidListSize < 100) {
				System.out.println("Matrix AA: \n" + AA.toString());
				System.out.println("");
			}
			Fits f = new Fits();
			double[][] dimg = new double[regionidListSize][regionidListSize];
			for (int ii=0; ii<regionidListSize; ii++) {
				for (int jj=0; jj<regionidListSize; jj++) {
					dimg[ii][jj] = AA.get(ii,jj);
				}
			}				
			f.addHDU(Fits.makeHDU(dimg));
			BufferedFile bf = new BufferedFile("matrixAA.fits", "rw");
			f.write(bf);
			bf.flush();
			bf.close();
		}

		//QA:  create and output a FITS image file of the matrix BB.
		if (verbose > 0) {
			if (regionidListSize < 100) {
				System.out.println("Matrix BB: \n" + BB.toString());
				System.out.println("");
			}
			Fits f = new Fits();
			double[][] dimg = new double[regionidListSize][1];
			for (int ii=0; ii<regionidListSize; ii++) {
				dimg[ii][0] = BB.get(ii,0);
			}
			f.addHDU(Fits.makeHDU(dimg));
			BufferedFile bf = new BufferedFile("matrixBB.fits", "rw");
			f.write(bf);
			bf.flush();
			bf.close();
		}
		
		//Perform inversion of matrix AA.
		if (verbose > 0) {
			System.out.println("start " + regionidListSize + "x" + regionidListSize + " matrix inversion");
			System.out.println("");
		}
		Algebra alg = new Algebra();
		Date date1 = new Date();
		AAinv = alg.inverse(AA);
		Date date2 = new Date();
		double timeSec = (date2.getTime()-date1.getTime())/1000.;
		if (verbose > 0) {
			DecimalFormat formatter = new DecimalFormat("0.##");
			System.out.println(regionidListSize + "x" + regionidListSize + " matrix inversion finished in " + 
					formatter.format(timeSec) + " sec\n");
		}

		//QA:  create and output FITS image files of the inverted matrix AAinv 
		//     and of the identity matrix II found by multiplying AA by AAinv.		
		if (verbose > 0) {
			II = alg.mult(AA,AAinv);
			if (regionidListSize < 100) {
				System.out.println("Matrix AAinv: \n" + AAinv.toString());
				System.out.println("");
				System.out.println("Identity Matrix: \n" + II.toString());
				System.out.println("");
			}
			Fits f = new Fits();
			double[][] dimg = new double[regionidListSize][regionidListSize];
			for (int ii=0; ii<regionidListSize; ii++) {
				for (int jj=0; jj<regionidListSize; jj++) {
					dimg[ii][jj] = AAinv.get(ii,jj);
				}
			}
			f.addHDU(Fits.makeHDU(dimg));	
			BufferedFile bf = new BufferedFile("matrixAAinv.fits", "rw");
			f.write(bf);
			bf.flush();
			bf.close();

			for (int ii=0; ii<regionidListSize; ii++) {
				for (int jj=0; jj<regionidListSize; jj++) {
					dimg[ii][jj] = II.get(ii,jj);
				}
			}
			f.addHDU(Fits.makeHDU(dimg));		
			bf = new BufferedFile("matrixII.fits", "rw");
			f.write(bf);
			bf.flush();
			bf.close();

		}

		//Multiply AAinv by BB to obtain the fit values for the zeropoints of each 
		// region.
		XX = alg.mult(AAinv,BB);
		
		//QA:  create and output a FITS image file of the matrix XX.
		if (verbose > 0) {
			System.out.println("Matrix XX: \n" + XX.toString());
			System.out.println("");
			Fits f = new Fits();
			double[][] dimg = new double[regionidListSize][1];
			for (int ii=0; ii<regionidListSize; ii++) {
				dimg[ii][0] = XX.get(ii,0);
			}
			f.addHDU(Fits.makeHDU(dimg));
			BufferedFile bf = new BufferedFile("matrixXX.fits", "rw");
			f.write(bf);
			bf.flush();
			bf.close();
		}
		
		//Estimate rms of solution and rms of each region's zeropoint.
		// Output each region's zeropoint and zeropoint rms to outputFileName.
		File outputFile = new File(outputFileName);
		FileWriter writer = new FileWriter(outputFile);
		writer.write("#To correct the mags in each regionid, *subtract* the following zeropoints.\n");
		writer.write("#regionid     zeropoint     rms \n");
		double rmsAll = -1.;
		double sumAll = 0.;
		double nAll   = 0.;
		for (int ii=0; ii<regionidListSize; ii++) {
			int regionid1 = Integer.parseInt(regionidList.get(ii).toString());
			int regionCalFlag1 = ((Integer) regionCalFlagMap.get((Integer) regionid1)).intValue();
			double zp1 = XX.get(ii,0);
			double rms1 = -1.;
			double sum1 = 0.;
			int n = 0;
			//Neither estimate rms of an uncalibrateable region's zeropoint, nor
			// include its contribution to the rms of the solution as a whole...
			if (regionCalFlag1 != -1) {
				for (int jj=0; jj<regionidListSize; jj++) {
					if (ii != jj) {
						if (overlapArray2d[ii][jj] == 1) {
							int regionid2 = Integer.parseInt(regionidList.get(jj).toString());
							int regionCalFlag2 = ((Integer) regionCalFlagMap.get((Integer) regionid2)).intValue();
							//Ignore contributions from any adjoining uncalibrateable 
							// regions (there shouldn't be any, but just in case...):
							if (regionCalFlag2 == -1) {continue;}
							double zp2 = XX.get(jj,0);
							double dmagListMedian = dmagArray2d[ii][jj];
							//Under the Glazebrook et al. (1994) sign 
							// conventions, we would subtract zp2 from
							// zp1, not the reverse.
							sum1 = sum1 + (dmagListMedian-(zp1-zp2))*(dmagListMedian-(zp1-zp2));
							n++;
							sumAll = sumAll + (dmagListMedian-(zp1-zp2))*(dmagListMedian-(zp1-zp2));
							nAll++;
						}
					}
				}
				if (n > 0) {
					rms1 = sum1/n;
					rms1 = Math.sqrt(rms1);
				}
			}
			
			//This formatted input requireds java 1.5 or higher...
			String outputLine = String.format("%1$-10d   %2$8.3f   %3$8.3f \n", regionid1, zp1, rms1);
			if (verbose > 0) {
				System.out.print(outputLine);
			}
			writer.write(outputLine);
		}
		if (nAll > 0 ) {
			rmsAll = sumAll/nAll;
			rmsAll = Math.sqrt(rmsAll);
		}
		String outputLine = String.format("rms of solution: %1$8.3f \n", rmsAll);
		if (verbose > 0) {
			System.out.println(outputLine);
		}
		outputLine = "#\n#" + outputLine;
		writer.write(outputLine);
		writer.close();

		if (verbose > 0) {
			System.out.println("\nResults have been written to the file " + outputFileName + "\n");
		}

		if (verbose > 0) {
			System.out.println("End Time: \t" + (new Date()).toString());
			System.out.println("\nThat's all, folks!\n");
		}     

		return;
		
	}
	
	
	//Getters and Setters for private variables...
	
	public String getStarMatchFileName() {
		return starMatchFileName;
	}
	
	public void setStarMatchFileName(String starMatchFileName) {
		this.starMatchFileName = starMatchFileName;
	}
	
	public String getOutputFileName() {
		return outputFileName;
	}
	
	public void setOutputFileName(String outputFileName) {
		this.outputFileName = outputFileName;
	}
	
	public int getVerbose() {
		return verbose;
	}
	
	public void setVerbose(int verbose) {
		this.verbose = verbose;
	}
	
	public Date getDate() {
		return date;
	}
	
	public void setDate(Date date) {
		this.date = date;
	}
	
	public String[] getFilterList() {
		return filterList;
	}
	
	public void setFilterList(String[] filterList) {
		this.filterList = filterList;
	}
	
}
