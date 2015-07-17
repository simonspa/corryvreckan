using namespace std;

void display(){
	
	gROOT->ProcessLine(".x lhcbStyle.C");
	gStyle->SetPalette(1);
	gStyle->SetPadGridX(1);
	gStyle->SetPadGridY(1);
 
	//string filename = "../thresholdScans/SET14.dat";
//	string filename = "/home/vertextb/uASIC/DAQ/software/data/001ec0db94b1/equalization/20150513_202456/scan.dat"; //SET9
//	string filename = "/home/vertextb/uASIC/DAQ/software/data/001ec0db2e5e/equalization/20150515_100657/scan.dat"; //SET10
//	string filename = "/home/vertextb/uASIC/DAQ/software/data/001ec0dbabb9/equalization/20150516_112028/scan.dat"; //SET12	
//	string filename = "/home/vertextb/uASIC/DAQ/software/data/001ec0db5ca6/equalization/20150517_105158/scan.dat"; //SET15
//	string filename = "/home/vertextb/uASIC/DAQ/software/data/001ec0dafabc/equalization/20150518_094238/scan.dat"; //SET11
//	string filename = "/home/vertextb/uASIC/DAQ/software/data/001ec0db5bfb/equalization/20150518_134703/scan.dat"; //SET14
//	string filename = "/home/vertextb/uASIC/DAQ/software/data/001ec0db2082/equalization/20150519_160810/scan.dat"; //SET13
 	string filename = "/home/vertextb/uASIC/DAQ/software/data/001ec0db2082/equalization/20150527_155526/scan.dat"; //SET13 part 2

	TNtuple thlScan("thlScan","thlScan","thl:negPixels:negCounts:posPixels:posCounts");
	thlScan.ReadFile(filename.c_str());
	thlScan.Draw("negPixels:thl>>thlScanPlot(900,600,1500)","","prof,goff");
	TH1F* thlScanPlot = (TH1F*)gDirectory->Get("thlScanPlot");

	TCanvas* canv = new TCanvas();
	thlScanPlot->GetYaxis()->SetTitle("Pixels");
	thlScanPlot->GetXaxis()->SetTitle("THL");
	thlScanPlot->DrawCopy("p");
	
	return;
	
}
