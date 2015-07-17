# Makefile depends on ROOT and Boost. 
# Define the variables ROOTSYS and BOOSTROOT in your environment.

BOOSTROOT=${PWD}

ROOTCFLAGS   := $(shell root-config --cflags)
ROOTLIBS     := $(shell root-config --glibs)

CXXFLAGS      = -Wall -fPIC -g -W -m64
CXXFLAGS     += $(ROOTCFLAGS) 
CXXFLAGS     += -I$(BOOSTROOT)

LIBS          = $(ROOTLIBS) -lGenVector -lMinuit 

SRCDIR        = ${PWD}/src/

BINDIR        = ${PWD}/bin/

CC            = g++ 

OBJS = $(BINDIR)Main.o $(BINDIR)Parameters.o $(BINDIR)Amalgamation.o  $(BINDIR)VetraCluster.o $(BINDIR)SciFiCluster.o $(BINDIR)VetraNtupleClass.o $(BINDIR)TestBeamEvent.o $(BINDIR)SummaryDisplayer.o $(BINDIR)Analysis.o $(BINDIR)RawHitMapMaker.o $(BINDIR)ClusterMaker.o $(BINDIR)NClustersMonitor.o $(BINDIR)EventDisplay.o $(BINDIR)PatternRecognition.o $(BINDIR)KDTree.o $(BINDIR)kdtree2.o $(BINDIR)PulseShapeAnalyser.o $(BINDIR)LanGau.o $(BINDIR)CommonTools.o $(BINDIR)TrackFitter.o $(BINDIR)Alignment.o $(BINDIR)EtaDistribution.o $(BINDIR)TestBeamTrack.o $(BINDIR)EfficiencyCalculator.o $(BINDIR)TDCManager.o $(BINDIR)SciFiClusterMaker.o $(BINDIR)TrackTimestamper.o $(BINDIR)VetraMapping.o $(BINDIR)VetraNoise.o $(BINDIR)FEI4ClusterMaker.o $(BINDIR)ResidualPlotter.o $(BINDIR)AkraiaApodosi.o $(BINDIR)TimepixCalibration.o $(BINDIR)NewVetraClusterMaker.o $(BINDIR)FileReader.o $(BINDIR)OverlapPolygon.o $(BINDIR)Cell.o $(BINDIR)CellAutomata.o $(BINDIR)TestBeamMillepede.o $(BINDIR)ClicpixAnalysis.o


DATAOBJS = ${SRCDIR}TestBeamEvent.h ${SRCDIR}TestBeamEventElement.h ${SRCDIR}RowColumnEntry.h ${SRCDIR}FEI4RowColumnEntry.h ${SRCDIR}TestBeamDataSummary.h ${SRCDIR}TDCFrame.h ${SRCDIR}TDCTrigger.h ${SRCDIR}SystemOfUnits.h ${SRCDIR}PhysicalConstants.h

GUIOBJS = $(BINDIR)MainGUI.o ${BINDIR}TestBeamMoniGUI.o $(BINDIR)TestBeamEvent.o $(BINDIR)Amalgamation.o $(BINDIR)CommonTools.o $(BINDIR)TDCManager.o $(BINDIR)TrackTimestamper.o $(BINDIR)VetraMapping.o $(BINDIR)VetraNtupleClass.o 

GUIDATAOBJS = $(DATAOBJS)

#-------------------------------------------------------
all:				tpanal
#-------------------------------------------------------

tpanal :			$(OBJS) $(BINDIR)EventDict.o
				@echo "Linking executable ..."
#				@${CC} $(CXXFLAGS) $(LIBS) -o $(BINDIR)tpanal $(OBJS) $(BINDIR)EventDict.o
				@${CC} $(CXXFLAGS) -o $(BINDIR)tpanal $(OBJS) $(BINDIR)EventDict.o $(LIBS)
				@echo "Done."
#-------------------------------------------------------
tpmon :				$(GUIOBJS) $(BINDIR)GuiDict.o
				@echo "Linking GUI executable ..."
				@${CC} $(CXXFLAGS) -o $(BINDIR)tpmon $(GUIOBJS) $(BINDIR)GuiDict.o $(LIBS)
				@echo "Done."
#-------------------------------------------------------
$(BINDIR)EventDict.o :		${DATAOBJS}
				@echo "Generating dictionary ..."
				@rm -f ${SRCDIR}EventDict.C ${SRCDIR}EventDict.h
			        @rootcint ${SRCDIR}EventDict.C -c ${DATAOBJS}
				@${CC} $(CXXFLAGS) -c ${SRCDIR}EventDict.C -o $(BINDIR)EventDict.o
$(BINDIR)GuiDict.o :		${SRCDIR}TestBeamMoniGUI.h ${GUIDATAOBJS}
				@echo "Generating GUI dictionary ..."
				@rm -f ${SRCDIR}GuiDict.C ${SRCDIR}GuiDict.h 
			        @rootcint ${SRCDIR}GuiDict.C -c ${SRCDIR}TestBeamMoniGUI.h ${GUIDATAOBJS}
				@${CC} $(CXXFLAGS) -c ${SRCDIR}GuiDict.C -o $(BINDIR)GuiDict.o
#-------------------------------------------------------
$(BINDIR)Main.o : 		${SRCDIR}Main.C
				@mkdir -p bin
				@echo Compiling components...
				@@${CC} $(CXXFLAGS) -c ${SRCDIR}Main.C -o $(BINDIR)Main.o

$(BINDIR)Amalgamation.o : 	${SRCDIR}Amalgamation.C ${SRCDIR}Amalgamation.h
				@echo Amalgamation
				@${CC} $(CXXFLAGS) -c ${SRCDIR}Amalgamation.C -o $(BINDIR)Amalgamation.o

$(BINDIR)Parameters.o : 	${SRCDIR}Parameters.C ${SRCDIR}Parameters.h
				@echo Parameters
				@${CC} $(CXXFLAGS) -c ${SRCDIR}Parameters.C -o $(BINDIR)Parameters.o

$(BINDIR)TestBeamEvent.o : 	${SRCDIR}TestBeamEvent.C ${SRCDIR}TestBeamEvent.h
				@echo TestBeamEvent
				@${CC} $(CXXFLAGS) -c ${SRCDIR}TestBeamEvent.C -o $(BINDIR)TestBeamEvent.o

$(BINDIR)LanGau.o : 		${SRCDIR}LanGau.C ${SRCDIR}LanGau.h
				@echo LanGau
				@${CC} $(CXXFLAGS) -c ${SRCDIR}LanGau.C -o $(BINDIR)LanGau.o

$(BINDIR)CommonTools.o : 	${SRCDIR}CommonTools.C ${SRCDIR}CommonTools.h
				@echo CommonTools
				@${CC} $(CXXFLAGS) -c ${SRCDIR}CommonTools.C -o $(BINDIR)CommonTools.o

$(BINDIR)Analysis.o : 		${SRCDIR}Analysis.C ${SRCDIR}Analysis.h ${SRCDIR}Clipboard.h ${SRCDIR}Algorithm.h
				@echo Analysis
				@${CC} $(CXXFLAGS) -c ${SRCDIR}Analysis.C -o $(BINDIR)Analysis.o

$(BINDIR)Algorithm.o : 		${SRCDIR}Algorithm.C ${SRCDIR}Algorithm.h
				@echo Algorithm
				@${CC} $(CXXFLAGS) -c ${SRCDIR}Algorithm.C -o $(BINDIR)Algorithm.o

$(BINDIR)EtaDistribution.o : 	${SRCDIR}EtaDistribution.C ${SRCDIR}EtaDistribution.h
				@echo EtaDistribution
				@${CC} $(CXXFLAGS) -c ${SRCDIR}EtaDistribution.C -o $(BINDIR)EtaDistribution.o

$(BINDIR)TestBeamTrack.o : 	${SRCDIR}TestBeamTrack.C ${SRCDIR}TestBeamTrack.h
				@echo TestBeamTrack
				@${CC} $(CXXFLAGS) -c ${SRCDIR}TestBeamTrack.C -o $(BINDIR)TestBeamTrack.o

$(BINDIR)VetraNtupleClass.o : 	${SRCDIR}VetraNtupleClass.C ${SRCDIR}VetraNtupleClass.h
				@echo VetraNtupleClass
				@${CC} $(CXXFLAGS) -c ${SRCDIR}VetraNtupleClass.C -o $(BINDIR)VetraNtupleClass.o

$(BINDIR)RawHitMapMaker.o : 	${SRCDIR}RawHitMapMaker.C ${SRCDIR}RawHitMapMaker.h
				@echo RawHitMapMaker
				@${CC} $(CXXFLAGS) -c ${SRCDIR}RawHitMapMaker.C -o $(BINDIR)RawHitMapMaker.o

$(BINDIR)SummaryDisplayer.o : 	${SRCDIR}SummaryDisplayer.C ${SRCDIR}SummaryDisplayer.h
				@echo SummaryDisplayer
				@${CC} $(CXXFLAGS) -c ${SRCDIR}SummaryDisplayer.C -o $(BINDIR)SummaryDisplayer.o

$(BINDIR)EventDisplay.o : 	${SRCDIR}EventDisplay.C ${SRCDIR}EventDisplay.h
				@echo EventDisplay
				@${CC} $(CXXFLAGS) -c ${SRCDIR}EventDisplay.C -o $(BINDIR)EventDisplay.o

$(BINDIR)ClusterMaker.o : 	${SRCDIR}ClusterMaker.C ${SRCDIR}ClusterMaker.h
				@echo ClusterMaker
				@${CC} $(CXXFLAGS) -c ${SRCDIR}ClusterMaker.C -o $(BINDIR)ClusterMaker.o

$(BINDIR)VetraCluster.o : 	${SRCDIR}VetraCluster.C ${SRCDIR}VetraCluster.h
				@echo VetraCluster
				@${CC} $(CXXFLAGS) -c ${SRCDIR}VetraCluster.C -o $(BINDIR)VetraCluster.o

$(BINDIR)SciFiCluster.o : 	${SRCDIR}SciFiCluster.C ${SRCDIR}SciFiCluster.h
				@echo SciFiCluster
				@${CC} $(CXXFLAGS) -c ${SRCDIR}SciFiCluster.C -o $(BINDIR)SciFiCluster.o

$(BINDIR)NClustersMonitor.o : 	${SRCDIR}NClustersMonitor.C ${SRCDIR}NClustersMonitor.h
				@echo NClustersMonitor
				@${CC} $(CXXFLAGS) -c ${SRCDIR}NClustersMonitor.C -o $(BINDIR)NClustersMonitor.o

$(BINDIR)KDTree.o : 		${SRCDIR}KDTree.C ${SRCDIR}KDTree.h
				@echo KDTree
				@${CC} $(CXXFLAGS) -c ${SRCDIR}KDTree.C -o $(BINDIR)KDTree.o

$(BINDIR)kdtree2.o : 		${SRCDIR}kdtree2.C ${SRCDIR}kdtree2.h
				@echo kdtree2
				@${CC} $(CXXFLAGS) -c ${SRCDIR}kdtree2.C -o $(BINDIR)kdtree2.o

$(BINDIR)PatternRecognition.o : 	${SRCDIR}PatternRecognition.C ${SRCDIR}PatternRecognition.h
				@echo PatternRecognition
				@${CC} $(CXXFLAGS) -c ${SRCDIR}PatternRecognition.C -o $(BINDIR)PatternRecognition.o

$(BINDIR)TrackFitter.o: ${SRCDIR}TrackFitter.C ${SRCDIR}TrackFitter.h
				@echo TrackFitter
				@${CC} $(CXXFLAGS) -c ${SRCDIR}TrackFitter.C -o $(BINDIR)TrackFitter.o

$(BINDIR)ResidualPlotter.o: ${SRCDIR}ResidualPlotter.C ${SRCDIR}ResidualPlotter.h
				@echo ResidualPlotter
				@${CC} $(CXXFLAGS) -c ${SRCDIR}ResidualPlotter.C -o $(BINDIR)ResidualPlotter.o

$(BINDIR)AkraiaApodosi.o: ${SRCDIR}AkraiaApodosi.C ${SRCDIR}AkraiaApodosi.h
				@echo AkraiaApodosi 
				@${CC} $(CXXFLAGS) -c ${SRCDIR}AkraiaApodosi.C -o $(BINDIR)AkraiaApodosi.o

$(BINDIR)PulseShapeAnalyser.o : ${SRCDIR}PulseShapeAnalyser.C ${SRCDIR}PulseShapeAnalyser.h
				@echo PulseShapeAnalyser
				@${CC} $(CXXFLAGS) -c ${SRCDIR}PulseShapeAnalyser.C -o $(BINDIR)PulseShapeAnalyser.o

$(BINDIR)Alignment.o : 		${SRCDIR}Alignment.C ${SRCDIR}Alignment.h
				@echo Alignment
				@${CC} $(CXXFLAGS) -c ${SRCDIR}Alignment.C -o $(BINDIR)Alignment.o

$(BINDIR)EfficiencyCalculator.o : ${SRCDIR}EfficiencyCalculator.C ${SRCDIR}EfficiencyCalculator.h
				@echo EfficiencyCalculator
				@${CC} $(CXXFLAGS) -c ${SRCDIR}EfficiencyCalculator.C -o $(BINDIR)EfficiencyCalculator.o

$(BINDIR)TDCManager.o : 	${SRCDIR}TDCManager.C ${SRCDIR}TDCManager.h ${SRCDIR}TDCFrame.h ${SRCDIR}TDCTrigger.h
				@echo TDCManager
				@${CC} $(CXXFLAGS) -c ${SRCDIR}TDCManager.C -o $(BINDIR)TDCManager.o

$(BINDIR)NewVetraClusterMaker.o : 	${SRCDIR}NewVetraClusterMaker.C ${SRCDIR}NewVetraClusterMaker.h
				@echo NewVetraClusterMaker
				@${CC} $(CXXFLAGS) -c ${SRCDIR}NewVetraClusterMaker.C -o $(BINDIR)NewVetraClusterMaker.o

$(BINDIR)SciFiClusterMaker.o : 	${SRCDIR}SciFiClusterMaker.C ${SRCDIR}SciFiClusterMaker.h
				@echo SciFiClusterMaker
				@${CC} $(CXXFLAGS) -c ${SRCDIR}SciFiClusterMaker.C -o $(BINDIR)SciFiClusterMaker.o

$(BINDIR)TrackTimestamper.o : ${SRCDIR}TrackTimestamper.C ${SRCDIR}TrackTimestamper.h
				@echo TrackTimestamper
				@${CC} $(CXXFLAGS) -c ${SRCDIR}TrackTimestamper.C -o $(BINDIR)TrackTimestamper.o

$(BINDIR)VetraMapping.o : 	${SRCDIR}VetraMapping.C ${SRCDIR}VetraMapping.h
				@echo VetraMapping
				@${CC} $(CXXFLAGS) -c ${SRCDIR}VetraMapping.C -o $(BINDIR)VetraMapping.o

$(BINDIR)VetraNoise.o : 	${SRCDIR}VetraNoise.C ${SRCDIR}VetraNoise.h
				@echo VetraNoise
				@${CC} $(CXXFLAGS) -c ${SRCDIR}VetraNoise.C -o $(BINDIR)VetraNoise.o

$(BINDIR)FEI4ClusterMaker.o :  ${SRCDIR}FEI4ClusterMaker.C ${SRCDIR}FEI4ClusterMaker.h
				@echo FEI4ClusterMaker
				@${CC} $(CXXFLAGS) -c ${SRCDIR}FEI4ClusterMaker.C -o $(BINDIR)FEI4ClusterMaker.o

$(BINDIR)TimepixCalibration.o :  ${SRCDIR}TimepixCalibration.C ${SRCDIR}TimepixCalibration.h
				@echo TimepixCalibration
				@${CC} $(CXXFLAGS) -c ${SRCDIR}TimepixCalibration.C -o $(BINDIR)TimepixCalibration.o

$(BINDIR)FileReader.o :  ${SRCDIR}FileReader.C ${SRCDIR}FileReader.h
				@echo FileReader
				@${CC} $(CXXFLAGS) -c ${SRCDIR}FileReader.C -o $(BINDIR)FileReader.o

$(BINDIR)OverlapPolygon.o :  ${SRCDIR}OverlapPolygon.C ${SRCDIR}OverlapPolygon.h
				@echo OverlapPolygon
				@${CC} $(CXXFLAGS) -c ${SRCDIR}OverlapPolygon.C -o $(BINDIR)OverlapPolygon.o

$(BINDIR)Cell.o :  ${SRCDIR}Cell.C ${SRCDIR}Cell.h
				@echo Cell
				@${CC} $(CXXFLAGS) -c ${SRCDIR}Cell.C -o $(BINDIR)Cell.o

$(BINDIR)CellAutomata.o :  ${SRCDIR}CellAutomata.C ${SRCDIR}CellAutomata.h
				@echo CellAutomata
				@${CC} $(CXXFLAGS) -c ${SRCDIR}CellAutomata.C -o $(BINDIR)CellAutomata.o

$(BINDIR)ClicpixAnalysis.o :	${SRCDIR}ClicpixAnalysis.C ${SRCDIR}ClicpixAnalysis.h
				@echo ClicpixAnalysis
				@${CC} $(CXXFLAGS) -c ${SRCDIR}ClicpixAnalysis.C -o $(BINDIR)ClicpixAnalysis.o

#-------------------------------------------------------
$(BINDIR)MainGUI.o : 		${SRCDIR}MainGUI.C
				@mkdir -p bin
				@echo Compiling GUI...
				@@${CC} $(CXXFLAGS) -c ${SRCDIR}MainGUI.C -o $(BINDIR)MainGUI.o

$(BINDIR)TestBeamMoniGUI.o :	${SRCDIR}TestBeamMoniGUI.C ${SRCDIR}TestBeamMoniGUI.h
				@echo TestBeamMoniGUI
				@${CC} $(CXXFLAGS) -c ${SRCDIR}TestBeamMoniGUI.C -o $(BINDIR)TestBeamMoniGUI.o

$(BINDIR)TestBeamMillepede.o :	${SRCDIR}TestBeamMillepede.cpp ${SRCDIR}TestBeamMillepede.h
				@echo TestBeamMillepede
				@${CC} $(CXXFLAGS) -c ${SRCDIR}TestBeamMillepede.cpp -o $(BINDIR)TestBeamMillepede.o
#-------------------------------------------------------
clean:
		@rm -f bin/tpanal bin/tpmon ${SRCDIR}._* ${SRCDIR}*~ core* $(BINDIR)/*.o $(BINDIR)/*.so ._*
		@rm -f ${SRCDIR}EventDict.C ${SRCDIR}EventDict.h
		@rm -f ${SRCDIR}GuiDict.C ${SRCDIR}GuiDict.h
		@echo cleaned
# DO NOT DELETE
