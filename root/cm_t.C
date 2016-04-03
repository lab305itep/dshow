void cm_t(void)
{
	gROOT->SetStyle("Plain");
	gStyle->SetOptStat(10);
	TFile fa("danss_data_001303-1.root");
	TFile fb("danss_data_001303.root");
	TFile fc("danss_data_001403-1.root");
	TFile fd("danss_data_001403.root");
	
	TH1D *hta = fa.Get("hT1");
	TH1D *htb = fb.Get("hT1");
	TH1D *htc = fc.Get("hT1");
	TH1D *htd = fd.Get("hT1");
	
	hta->SetLineColor(kRed);
	htb->SetLineColor(kBlack);
	htc->SetLineColor(kBlue);
	htd->SetLineColor(kGreen);
	
	htc->SetTitle("Time between the first and the second events;us");
	
	htc->DrawCopy();
	htd->DrawCopy("same");
	hta->DrawCopy("same");
	htb->DrawCopy("same");

	TLegend	*lg = new TLegend(0.7, 0.75, 0.95, 0.9);
	lg->SetFillColor(kWhite);
	lg->AddEntry(htc, "Cm, 1 MeV second event cut", "l");
	lg->AddEntry(htd, "Cm, 3 MeV second event cut", "l");
	lg->AddEntry(hta, "No Cm, 1 MeV second event cut", "l");
	lg->AddEntry(htb, "No Cm, 3 MeV second event cut", "l");
	lg->Draw();
	
	gPad->Update();
	fa.Close();
	fb.Close();
	fc.Close();
	fd.Close();
}

void cm_e2(void)
{
	gROOT->SetStyle("Plain");
	gStyle->SetOptStat(10);
	TFile fa("danss_data_001303-1.root");
	TFile fc("danss_data_001403-1.root");
	
	TH2D *hta = fa.Get("hESEP2");
	TH2D *htc = fc.Get("hESEP2");
	
	TCanvas *cv = new TCanvas("CV", "Neutron capture energy", 1600, 800);
	cv->Divide(2, 1);

	hta->SetTitle("No Cm;SiPM, MeV;PMT, MeV");
	htc->SetTitle("Cm;SiPM, MeV;PMT, MeV");
	
	cv->cd(1);
	hta->DrawCopy("color");
	cv->cd(2);
	htc->DrawCopy("color");

	cv->Update();
	fa.Close();
	fc.Close();
}

void cm_e2s(void)
{
	gROOT->SetStyle("Plain");
	gStyle->SetOptStat(10);
	TFile fa("danss_data_001303-1.root");
	TFile fc("danss_data_001403-1.root");
	
	TH2D *ha = fa.Get("hESEP2");
	TH2D *hc = fc.Get("hESEP2");
	TH1D *hta = ha->ProjectionX("__NOCM");
	TH1D *htc = hc->ProjectionX("__CM");
	
	htc->SetTitle("Neutron capture energy;SiPM, MeV");
	
	hta->SetLineColor(kRed);
	htc->SetLineColor(kBlue);
	htc->SetMarkerStyle(21);

	htc->DrawCopy();
	hta->DrawCopy("same");

	hta->Sumw2();
	htc->Sumw2();
	htc->Add(hta, -1.2);
	htc->DrawCopy("same");

	TLegend	*lg = new TLegend(0.7, 0.8, 0.95, 0.9);
	lg->SetFillColor(kWhite);
	lg->AddEntry(htc, "Cm", "l");
	lg->AddEntry(hta, "No Cm", "l");
	lg->AddEntry(htc, "Background subtracted", "p");
	lg->Draw();

	gPad->Update();
	fa.Close();
	fc.Close();
}

void cm_m(void)
{
	gROOT->SetStyle("Plain");
	gStyle->SetOptStat(100);
	TFile fb("danss_data_001303.root");
	TFile fd("danss_data_001403.root");
	
	TH1D *htb = fb.Get("hM");
	TH1D *htd = fd.Get("hM");
	
	htd->SetTitle("Events in 50 us");
	
	htb->SetBinContent(1, 0);
	htd->SetBinContent(1, 0);

	htb->SetLineColor(kRed);
	htd->SetLineColor(kBlue);
	
	htd->DrawCopy();
	htb->DrawCopy("same");

	TLegend	*lg = new TLegend(0.7, 0.8, 0.95, 0.9);
	lg->SetFillColor(kWhite);
	lg->AddEntry(htd, "Cm", "l");
	lg->AddEntry(htb, "No Cm", "l");
	lg->Draw();

	gPad->Update();
	fb.Close();
	fd.Close();
}

void cm_xyz(void)
{
	gROOT->SetStyle("Plain");
	gStyle->SetOptStat(100);
	TFile fd("danss_data_001403.root");
	
	TH2D *hxz = fd.Get("hXZ2");
	TH2D *hyz = fd.Get("hYZ2");
	TH2D *mxz = fd.Get("mXZ2");
	TH2D *myz = fd.Get("mYZ2");

	TCanvas *cv = new TCanvas("CV", "Neutron capture energy", 1600, 1600);
	cv->Divide(2, 2);

	cv->cd(1);
	hyz->DrawCopy("colorZ");
	cv->cd(2);
	hxz->DrawCopy("colorZ");
	cv->cd(3);
	myz->DrawCopy("colorZ");
	cv->cd(4);
	mxz->DrawCopy("colorZ");

	cv->Update();
	fd.Close();
}

