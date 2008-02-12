package gov.fnal.eag.dtucker.desPhotoStds;

import gov.fnal.eag.dtucker.desPhotoStds.GlobalZPSolverPrep;
import java.sql.SQLException;
import java.util.Date;


/**
 * GlobalZPSolverPrepRun.java   The DES Collaboration  2007
 * This class instantiates an object of the GlobalZPSolverPrep class,
 * sets values for certain GlobalZPSolverPrep parameters, and invokes the 
 * GlobalZPSolverPrep "solve" method.  The GlobalZPSolverPrep "solve" method
 * creates an ASCII file containing a table of the unique star matches  
 * from all the overlapping tiles in a given coadd.  This file is input
 * for the GlobalZPSolverRun command, which solves for region-to-region
 * zeropoint offsets for a set of overlapping regions.  
 * 
 * @author dtucker
 */
public class GlobalZPSolverPrepRun {


    public static void main (String[] args) {

    	System.out.println("GlobalZPSolverPrepRun");
    	
        System.out.print("arglist:  ");
        for (int i=0; i< args.length; i++) {
            System.out.print(args[i] + " ");
        }
        System.out.print("\n");
        
        GlobalZPSolverPrep gzpsPrep = new GlobalZPSolverPrep();

        // Process any arguments passed to the main method.
        // Assume they are ordered "url dbName username password filter magLo magHi 
        //    imageType imageNameFilter runiddesc outputFileName referenceImageID".
        if (args.length > 0) {
            String url = args[0];
            gzpsPrep.setUrl(url);   
            System.out.println("url="+url);
        } 
        if (args.length > 1) {
            String dbName = args[1];
            gzpsPrep.setDbName(dbName);   
            System.out.println("dbName="+dbName);
        } 
        if (args.length > 2) {
            String user = args[2];
            gzpsPrep.setUser(user);   
            System.out.println(user);
        } 
        if (args.length > 3) {
            String passwd = args[3];
            gzpsPrep.setPasswd(passwd);   
            System.out.println(passwd);
        }
        if (args.length > 4) {
            String filter = args[4];
            gzpsPrep.setFilter(filter);
            System.out.println("filter="+filter);
        } 
        if (args.length > 5) {
            double magLo = Double.parseDouble(args[5]);
            gzpsPrep.setMagLo(magLo);
            System.out.println("magLo="+magLo);
        }        
        if (args.length > 6) {
            double magHi = Double.parseDouble(args[6]);
            gzpsPrep.setMagHi(magHi);
            System.out.println("magHi="+magHi);
        } 
        if (args.length > 7) {
        		String imageType = args[7];
        		gzpsPrep.setImageType(imageType);   
        		System.out.println("imageType="+imageType);
        } 
        if (args.length > 8) {
    			String imageNameFilter = args[8];
    			gzpsPrep.setImageNameFilter(imageNameFilter);   
    			System.out.println("imageNameFilter="+imageNameFilter);
        } 
        if (args.length > 9) {
            String runiddesc = args[9];
            gzpsPrep.setRuniddesc(runiddesc);   
            System.out.println("runiddesc="+runiddesc);
        } 
        if (args.length > 10) {
            String outputFileName = args[10];
            gzpsPrep.setOutputFileName(outputFileName);   
            System.out.println("outputFileName="+outputFileName);
        } 
        if (args.length > 11) {
            int referenceImageID = Integer.parseInt(args[11]);
            gzpsPrep.setReferenceImageID(referenceImageID);   
            System.out.println("referenceImageID="+referenceImageID);
        } 

        gzpsPrep.setSqlDriver("oracle.jdbc.driver.OracleDriver");

        Date date = new Date();
        gzpsPrep.setDate(date);
        
        try {
            gzpsPrep.solve();
        } catch (ClassNotFoundException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } catch (SQLException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } catch (Exception e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }

    }

}
