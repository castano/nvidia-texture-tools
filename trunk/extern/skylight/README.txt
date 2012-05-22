The files in this archive are a sample implementation of the analytical 
skylight model presented in the SIGGRAPH 2012 paper


           "An Analytic Model for Full Spectral Sky-Dome Radiance"

                                    by 

                       Lukas Hosek and Alexander Wilkie
                Charles University in Prague, Czech Republic


                        Version: 1.0, May 11th, 2012


Please visit http://cgg.mff.cuni.cz/projects/SkylightModelling/ to check if
an updated version of this code has been published!


This archive contains the following files:

README.txt                    This file.

ArHosekSkyModel.h             Header file for the reference functions. Their
                              usage is explained there, and sample code for 
                              calling them is given.

ArHosekSkyModel.c             Implementation of the functions.

ArHosekSkyModelData.h         Coefficient data.

Please note that the source files are in C99, so that when e.g. compiling this 
code with gcc, you have to add the "-std=c99" or "-std=gnu99" flags.