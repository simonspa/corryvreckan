using namespace std;

std::string makestring(double voltage){
  std::ostringstream strs;
  strs << voltage;
  std::string legs = strs.str();
  return legs;
}

void display(){
  
  gROOT->ProcessLine(".x lhcbStyle.C");
  gStyle->SetPalette(1);
	
	string histoDir = "../monitoring/";
	string maskDir = "../masks/";

	int polarity = 0; // 0 = negative, 1 = positive
	
	vector<double> files;
//	files.push_back(10032.); //SET9
//	files.push_back(10320.); //SET10
//	files.push_back(10494.); //SET12
//	files.push_back(10677.); //SET15
//	files.push_back(10894.); //SET11
//	files.push_back(10905.); //SET14
	files.push_back(11043.); //SET13

	for(int ifiles=0;ifiles<files.size();ifiles++){
		string filename = histoDir + "histograms" + makestring(files[ifiles]) + ".root";
		TFile* file = new TFile(filename.c_str(),"OPEN");
		string histoname = "/tpanal/ClicpixAnalysis/hHitPixels";
		TH2F* hitPixels = gDirectory->Get(histoname.c_str());

		double hotPixelValue = getHotPixelValue(hitPixels,polarity);
	
		string maskFileName = maskDir+"maskedPixels"+makestring(files[ifiles])+".dat";
		ofstream maskFile;
		maskFile.open(maskFileName.c_str());
		maskFile<<"== masked pixel file for run "<<makestring(files[ifiles])<<" ==\n";
		
		TH1F* pixelHitHisto = new TH1F("pixelHitHisto","pixelHitHisto",1000,0,40000);
		int nHotPixels = 0;
		for(int col=0;col<64;col++){
			// Check if column should be masked
			if(polarity == 0 && col > 59){ maskFile<<"c "<<0<<" "<<col<<"\n"; continue; }
			if(polarity == 1 && col < 60){ maskFile<<"c "<<0<<" "<<col<<"\n"; continue; }
			// Check if pixel should be masked
			for(int row=0;row<64;row++){
				if( (hitPixels->GetBinContent(hitPixels->GetXaxis()->FindBin(col),hitPixels->GetYaxis()->FindBin(row))) >= hotPixelValue){
					nHotPixels++;
//					cout<<"maskPixel(par,"<<row<<","<<col<<");"<<endl;
					maskFile<<"p "<<row<<" "<<col<<"\n";
				}
				pixelHitHisto->Fill( hitPixels->GetBinContent(hitPixels->GetXaxis()->FindBin(col),hitPixels->GetYaxis()->FindBin(row)) );
			}
		}
		maskFile.close();

		cout<<"Hot pixel value is "<<hotPixelValue<<endl;
		cout<<"Number of hot pixels: "<<nHotPixels<<endl;
	
		pixelHitHisto->GetXaxis()->SetTitle("Number of hits");
		pixelHitHisto->Draw();
	
//		TCanvas* canv = new TCanvas();
//		hitPixels->Scale(1./20.);
//		hitPixels->GetXaxis()->SetTitle("Column");
//		hitPixels->GetYaxis()->SetTitle("Row");
//		hitPixels->SetMaximum(pixelHitHisto->GetMean(1)+5.*pixelHitHisto->GetRMS(1));
//		hitPixels->DrawCopy("colz");
	}
}

double getHotPixelValue(TH2F* map, int polarity){
	TH1F* pixelHitHistoTemp = new TH1F("pixelHitHistoTemp","pixelHitHistoTemp",1000,0,40000);
	for(int col=0;col<64;col++){
		// Check if column should be ignored
		if(polarity == 0 && col > 59) continue;
		if(polarity == 1 && col < 60) continue;
		for(int row=0;row<64;row++){
			pixelHitHistoTemp->Fill( map->GetBinContent(map->GetXaxis()->FindBin(col),map->GetYaxis()->FindBin(row)) );
		}
	}
	pixelHitHistoTemp->Fit("gaus","0q");
	TF1* fit = pixelHitHistoTemp->GetFunction("gaus");
	cout<<"Mean "<<fit->GetParameter(1)<<endl;
	cout<<"RMS "<<fit->GetParameter(2)<<endl;
	//double value = fit->GetParameter(1) + 5.*fit->GetParameter(2); //Gaussian fit 
	double value = 2500.; //TEMP 
	delete pixelHitHistoTemp;
	return value;

}
