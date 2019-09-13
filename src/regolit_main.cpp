#define _XOPEN_SOURCE 700
#define __STDC_FORMAT_MACROS
#include<iostream>
#include<cstdlib>
#include<cmath>
#include<cctype>
#include<cstring>
#include<ctime>
#include<sstream>
#include<fstream>
#include<string>
#include<sys/time.h>
#include<sys/resource.h>
#include<sys/types.h>
#include<inttypes.h>
#include<dirent.h>
#include<vector>
#include "../include/regolit_main.hpp"
#include "../include/log.hpp"
#include "../include/utility.hpp"
#include "../include/histogram.hpp"
#include "../include/impactor.hpp"
#include "../include/layer.hpp"
#include "../include/crater.hpp"
#include "../include/subsurf_column.hpp"
#include "../include/grid.hpp"
#include "../include/utility.hpp"
#include "../include/tests.hpp"


//////////////////////////////
// DECLARE INPUT PARAMETERS //
//////////////////////////////
// Simulation parameters:
double regionWidth; // km
double resolution; // 1/km
double endTime; // Ma
double printTimeStep; // Time step for printing data in Ma.
double initialThickness; // Initial thickness of subsurface layer.
bool isEmplaceEjecta; // Should emplace ejecta? Computationally extensive.
bool isEmplaceSecondaries; // Should emplace ejecta? Computationally extensive.
bool runTests; // Should emplace ejecta? Computationally extensive.

// Crater formation variables:
double depthToDiameter; // Dimensionless ratio, crater depth to diameter
double rimToDiameter; // Dimensionless ratio, rim height to diameter
double outerRimExponent; // The exponent of the rim height decrease power law
double numberOfZModelShells; // Number of shells in z model (for ejecta calc.)
int craterProfile; // Chosen crater profile
int ejectaSpread; // The spread of the ejecta in crater radii
double slope_secondaries; // The spread of the ejecta in crater radii

// Impactor distribution variables:
double slope_b; // Slope of the impactor CDF
double moonEarthFluxRatio; // Moon-earth impactor flux ratio due to cross-section
double minimumImpactorDiameter; // The smallest impactor in the distribution
double fluxConstant_c; // The Flux of impactors > 1 m, Ma^-1 m^-2
double impactorDensity; // The impactor density in kg m^-3
double meanImpactVelocity; // The impact velocity, kg s^-1

// Surface physical properties:
double g;
double k1;
double Ybar;
double mu;
double targetDensity;

/////////////
//Functions//
/////////////
// Read config file:
std::vector<var> readConfig(){
  std::ifstream configFile;
  configFile.open("./config/config.cfg");
  std::vector<var> varList; // A varlist vector to store variables.
  var tempVar; // Temporary var struct used as buffer.
  addLogEntry("Initializing config file");

  if (!configFile){
    addLogEntry("Cannot read config file.");
    exit(EXIT_FAILURE);
  }

  // Count how many variables in config file:
  std::string line;
  while (std::getline(configFile, line)) {
    // Check if line is commented out:
    if (line[0] == '/' && line[1] == '/')
      continue;

    // Check if line is empty:
    if (line.empty())
      continue;

    std::istringstream iss(line);
    std::string name; double value;
    if (!(iss >> name >> value)){
      addLogEntry("Cannot read variable from config file.");
      exit(EXIT_FAILURE);
    }

    tempVar.name = name;
    tempVar.value = value;
    varList.push_back(tempVar);
  }

  return varList;
}

// Get variable from list:
double setVariable(std::vector<var> varList, std::string varName){
  size_t i;

  for (i = 0; i < varList.size(); i++) {
    if (varList[i].name == varName){
      char logEntry[100];
      sprintf(logEntry, "Getting variable %s with value %f.", varList[i].name.c_str(), varList[i].value);
      addLogEntry(logEntry);
      return varList[i].value;
    }
  }
// If variable was not found:
return -1;
}

////////////////
// START MAIN //
////////////////
int main() {
  // Prepare directories:
  DIR * outputdir = opendir("./output");
  if (outputdir){
    closedir(outputdir);
  }
  else {
    system("mkdir ./output");
  }

  DIR * logdir = opendir("./log");
  if (logdir){
    closedir(logdir);
  }
  else {
    system("mkdir ./log");
  }

  // Create log:
  createLogFile("log/log.txt");
  // Read config:
  std::vector<var> varList = readConfig();

  // Set variables from parameters:
  // Simulation variables:
  regionWidth = setVariable(varList, "regionWidth");
  resolution = setVariable(varList, "resolution");
  endTime = setVariable(varList, "endTime");
  printTimeStep = setVariable(varList, "printTimeStep");
  initialThickness = setVariable(varList, "initialThickness");
  isEmplaceEjecta = setVariable(varList, "isEmplaceEjecta");
  isEmplaceSecondaries = setVariable(varList, "isEmplaceSecondaries");
  runTests = setVariable(varList, "runTests");

  // Crater formation variables:
  depthToDiameter = setVariable(varList, "depthToDiameter");
  rimToDiameter = setVariable(varList, "rimToDiameter");
  outerRimExponent = setVariable(varList, "outerRimExponent");
  numberOfZModelShells = setVariable(varList, "numberOfZModelShells");
  craterProfile = (int) setVariable(varList, "craterProfile");
  ejectaSpread = (int) setVariable(varList, "ejectaSpread");
  slope_secondaries = setVariable(varList, "slope_secondaries");

  // Impactor distribution variables:
  slope_b = setVariable(varList, "slope_b");
  moonEarthFluxRatio = setVariable(varList, "moonEarthFluxRatio");
  minimumImpactorDiameter = setVariable(varList, "minimumImpactorDiameter");
  fluxConstant_c = setVariable(varList, "fluxConstant_c");
  impactorDensity = setVariable(varList, "impactorDensity");
  meanImpactVelocity = setVariable(varList, "meanImpactVelocity");

  // Surface physical properties:
  g = setVariable(varList, "g");
  k1 = setVariable(varList, "k1");
  Ybar = setVariable(varList, "Ybar");
  mu = setVariable(varList, "mu");
  targetDensity = setVariable(varList, "targetDensity");

	// Initialize the random number generator seed:
	srand48((long) get_time());

  ///////////////
  // Run tests //
  ///////////////
  if (runTests) {
    // If tests did not pass:
    if (tests())
      std::cout << "All tests passed successfully." << std::endl;
    else
      std::cout << "Not all tests passed successfully." << std::endl;

    return 0;
  }

  ////////////////////////////////////////////
  ////////////////////////////////////////////
  // Declare crater related varialbes (DO NO TOUCH!):
  double xGhost; // Ghost craters coordinates
  double yGhost; // Ghost craters coordinates
  int printIndex = 1; // The index to add to the end of the output matrix file (zmat_1.txt).
  ////////////////////////////////////////////
  ////////////////////////////////////////////

  ////////////////////////////
  // Simulation parameters  //
  ////////////////////////////
  // Generate grid:
  Grid grid(regionWidth, resolution);

  // Total number of craters to be created:
	long totalNumberOfImpactors = ceil(fluxConstant_c * pow(minimumImpactorDiameter,-slope_b) * endTime * grid.area * moonEarthFluxRatio); // total number of impactors to generate larger than minimumDiameter: N/At = cD^-b.
  long numberOfCratersInTimestep = ceil(fluxConstant_c * pow(minimumImpactorDiameter,-slope_b) * printTimeStep * grid.area * moonEarthFluxRatio); // number of impactors to generate larger than minimumDiameter: N/At = cD^-b in some time interval.

  // Craters and impactors histograms:
  Histogram cratersHistogram(minimumImpactorDiameter * 10, regionWidth, 12); // Crater histogram from 10*minimumImpactorDiameter to regionWidth meters
  Histogram impactorsHistogram(minimumImpactorDiameter, 1e4, 12); // Impactor histogram from 0 to 10 km

  //////////////////////
  // Start simulation //
  //////////////////////
  addLogEntry("Starting simulation:");
  char logEntry[50];
  sprintf(logEntry, "Number of craters in simulation: %ld.", totalNumberOfImpactors);
  addLogEntry(logEntry);
  totalNumberOfImpactors = numberOfCratersInTimestep;
	for (long i = 0; i < totalNumberOfImpactors; i++){
    // Randomize a new impactor:
    // Impactor impactor;
    //
    // // Add diameter to crater histogram:
    // impactorsHistogram.add(2 * impactor.radius);
    // // Form a crater:
    // Crater crater(impactor);
    // // Add diameter to crater histogram:
    // cratersHistogram.add(2 * crater.finalRadius);
    Impactor impactor;

    // Add diameter to crater histogram:
    impactorsHistogram.add(2 * impactor.radius);
    // Form a crater:
    Crater crater(impactor);
    // Add diameter to crater histogram:
    cratersHistogram.add(2 * crater.finalRadius);

    // Form a crater on the grid:
    grid.formCrater(crater);
    if (crater.finalRadius > 2 * resolution && isEmplaceEjecta){
        grid.emplaceEjecta(crater);
    }

    // Ghost cratering:
    // If the crater exceeds the grid, wrap around it by creating a ghost crater.
    // If a corner:
    if ( (fabs(crater.xPosition) > regionWidth/2 - crater.finalRadius) && (fabs(crater.yPosition) > regionWidth/2 - crater.finalRadius) ){
      // Calculate ghost crater position as sgn(x) * (region_width - x);
      xGhost = (-crater.xPosition/fabs(crater.xPosition)) * (regionWidth - crater.xPosition * (crater.xPosition/fabs(crater.xPosition)));
      yGhost = (-crater.yPosition/fabs(crater.yPosition)) * (regionWidth - crater.yPosition * (crater.yPosition/fabs(crater.yPosition)));

      Crater ghost1 = Crater(impactor, xGhost, yGhost);
      Crater ghost2 = Crater(impactor, xGhost, crater.yPosition);
      Crater ghost3 = Crater(impactor, crater.xPosition, yGhost);
      grid.formCrater(ghost1);
      grid.emplaceEjecta(ghost1);
      grid.formCrater(ghost2);
      grid.emplaceEjecta(ghost2);
      grid.formCrater(ghost3);
      grid.emplaceEjecta(ghost3);
    }

    // If a side:
    if ( (fabs(crater.xPosition) > regionWidth/2 - crater.finalRadius/2) && (fabs(crater.yPosition) < regionWidth/2 - crater.finalRadius/2)){
      // Calculate ghost crater position as sgn(x) * (region_width - x);
      xGhost = (-crater.xPosition/fabs(crater.xPosition)) * (regionWidth - crater.xPosition * (crater.xPosition/fabs(crater.xPosition)));
      Crater ghost2 = Crater(impactor, xGhost, crater.yPosition);

      // Create one ghost crater:
      grid.formCrater(ghost2);
      grid.emplaceEjecta(ghost2);
    }

    // If another side:
    if ( (fabs(crater.xPosition) < regionWidth/2 - crater.finalRadius/2) && (fabs(crater.yPosition) > regionWidth/2 - crater.finalRadius/2)){
      // Calculate ghost crater position as sgn(x) * (region_width - x);
      yGhost = (-crater.yPosition/fabs(crater.yPosition)) * (regionWidth - crater.yPosition * (crater.yPosition/fabs(crater.yPosition)));
      Crater ghost3 = Crater(impactor, crater.xPosition, yGhost);

      // Create one ghost crater:
      grid.formCrater(ghost3);
      grid.emplaceEjecta(ghost3);
    }

    // Secondary craters:
    if (isEmplaceSecondaries) {
      // Form a secondary crater within 2 crater diameters from the primary:
      if (crater.numberOfSecondaries > 0){
        char logEntry[50];
        sprintf(logEntry, "Primary diameter: %f. Number of secondaries: %d.", 2*crater.finalRadius, crater.numberOfSecondaries);
        addLogEntry(logEntry);
      }
      for (long j = 0; j < crater.numberOfSecondaries; j++){
        double secondaryXPosition = randU(crater.xPosition - 4 * crater.finalRadius, crater.xPosition + 4 * crater.finalRadius);
        double secondaryYPosition = randU(crater.yPosition - 4 * crater.finalRadius, crater.yPosition + 4 * crater.finalRadius);
        double secondaryRadius = resolution * pow(randU(0,1), -1/slope_secondaries); // Set impactor radius from the cumulative distribution, meters
        Crater secondaryCrater(secondaryXPosition, secondaryYPosition, secondaryRadius);
        grid.formCrater(secondaryCrater);
      }
    }

    // Print progress to a file:
    if (i%numberOfCratersInTimestep == 0){
      // Pring z matrix:
      grid.printSurface(printIndex);

      // Print to log:
      char logEntry[100];
      sprintf(logEntry, "Progress: %0.2f%%.",(double) i/(double) totalNumberOfImpactors * 100);
      addLogEntry(logEntry);
      printIndex++;
    }

	 }

  // Print craters histogram to file:
  addLogEntry("Printing crater histogram file.");
  cratersHistogram.print("./output/craters_histogram.txt");
  impactorsHistogram.print("./output/impactor_histogram.txt");
  grid.printSurface(0);
  grid.printGrid(0);
	// Free memory
  addLogEntry("Freeing memory.");
  addLogEntry("Simulation has ended.");
}
