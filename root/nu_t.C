void nu_t(void) 
{
	char str[256];
	TText *txt;

	gROOT->SetStyle("Plain");
	gStyle->SetOptStat(0);
	
	TFile f7("danss_data_000700.root");
	TFile f9("danss_data_000900.root");
	
	TH1D *h7t = f7.Get("hT1");
	TH1D *h9t = f9.Get("hT1");
	TH1D *hdiff = h9t->Clone();
	
	h7t->Sumw2();
	h9t->Sumw2();
	
	h7t->Scale(6.683);
	
	h7t->SetLineColor(kRed);
	h9t->SetLineColor(kBlue);
	hdiff->SetLineColor(kGreen);

	h9t->SetTitle("Time between the first and the second events;us");

	h9t->DrawCopy();
	h7t->DrawCopy("same");
	hdiff->Add(h7t, -1);
	hdiff->DrawCopy("same");

	TLegend	*lg = new TLegend(0.6, 0.7, 0.98, 0.9);
	lg->SetFillColor(kWhite);
	lg->AddEntry(h9t, "March 4-14, ~8.5 days, Power ~70%", "l");
	lg->AddEntry(h7t, "March 2, OFF (normalized by time)", "l");
	lg->AddEntry(hdiff, "Difference", "l");
	lg->Draw();
	
	sprintf(str, "Events %d", (int)hdiff->Integral(5, 60));
	txt = new TText();
	txt->SetTextColor(kGreen);
	txt->DrawTextNDC(0.35, 0.85, str);
	
	gPad->Update();
	f7.Close();
	f9.Close();
}

void nu_e2s(void) 
{
	char str[256];
	TText *txt;

	gROOT->SetStyle("Plain");
	gStyle->SetOptStat(0);
	
	TFile f7("danss_data_000700.root");
	TFile f9("danss_data_000900.root");
	
	TH2D *h7e = f7.Get("hESEP2");
	TH2D *h9e = f9.Get("hESEP2");
	
	TH1D *h7es = h7e->ProjectionX("h7es");
	TH1D *h9es = h9e->ProjectionX("h9es");
	
	TH1D *hdiff = h9es->Clone();
	
	h7es->Sumw2();
	h9es->Sumw2();
	
	h7es->Scale(6.683);
	
	h7es->SetLineColor(kRed);
	h9es->SetLineColor(kBlue);
	hdiff->SetLineColor(kGreen);

	h9es->SetTitle("\"Neutron\" capture energy;MeV");

	h9es->DrawCopy();
	h7es->DrawCopy("same");
	hdiff->Add(h7es, -1);
	hdiff->DrawCopy("same");

	TLegend	*lg = new TLegend(0.6, 0.7, 0.98, 0.9);
	lg->SetFillColor(kWhite);
	lg->AddEntry(h9es, "March 4-14, ~8.5 days, Power ~70%", "l");
	lg->AddEntry(h7es, "March 2, OFF (normalized by time)", "l");
	lg->AddEntry(hdiff, "Difference", "l");
	lg->Draw();
	
	sprintf(str, "Events %d", hdiff->Integral(3, 35));
	txt = new TText();
	txt->SetTextColor(kGreen);
	txt->DrawTextNDC(0.35, 0.85, str);
	
	gPad->Update();
	f7.Close();
	f9.Close();
}

void nu_diff2X(char *hname, char *title)
{
	char str[256];
	TText *txt;

	gROOT->SetStyle("Plain");
	gStyle->SetOptStat(0);
	
	TFile f7("danss_data_000700.root");
	TFile f9("danss_data_000900.root");
	
	TH2D *h7e = f7.Get(hname);
	TH2D *h9e = f9.Get(hname);

	if (!h7e || !h9e) {
		printf("%s not found\n", hname);
		return;
	}

	if (strcmp(h7e->ClassName(), "TH2D") || strcmp(h9e->ClassName(), "TH2D")) {
		printf("%s is not TH2D\n", hname);
		return;
	}

	TH1D *h7es = h7e->ProjectionX("h7es");
	TH1D *h9es = h9e->ProjectionX("h9es");
	
	TH1D *hdiff = h9es->Clone();
	
	h7es->Sumw2();
	h9es->Sumw2();
	
	h7es->Scale(6.683);
	
	h7es->SetLineColor(kRed);
	h9es->SetLineColor(kBlue);
	hdiff->SetLineColor(kGreen);

	h9es->SetTitle(title);

	h9es->DrawCopy();
	h7es->DrawCopy("same");
	hdiff->Add(h7es, -1);
	hdiff->DrawCopy("same");

	TLegend	*lg = new TLegend(0.6, 0.7, 0.98, 0.9);
	lg->SetFillColor(kWhite);
	lg->AddEntry(h9es, "March 4-14, ~8.5 days, Power ~70%", "l");
	lg->AddEntry(h7es, "March 2, OFF (normalized by time)", "l");
	lg->AddEntry(hdiff, "Difference", "l");
	lg->Draw();
	
	sprintf(str, "Events %d", hdiff->Integral());
	txt = new TText();
	txt->SetTextColor(kGreen);
	txt->DrawTextNDC(0.35, 0.85, str);
	
	gPad->Update();
	f7.Close();
	f9.Close();

}

void nu_diff2Y(char *hname, char *title)
{
	char str[256];
	TText *txt;

	gROOT->SetStyle("Plain");
	gStyle->SetOptStat(0);
	
	TFile f7("danss_data_000700.root");
	TFile f9("danss_data_000900.root");
	
	TH2D *h7e = f7.Get(hname);
	TH2D *h9e = f9.Get(hname);

	if (!h7e || !h9e) {
		printf("%s not found\n", hname);
		return;
	}

	if (strcmp(h7e->ClassName(), "TH2D") || strcmp(h9e->ClassName(), "TH2D")) {
		printf("%s is not TH2D\n", hname);
		return;
	}

	TH1D *h7es = h7e->ProjectionY("h7es");
	TH1D *h9es = h9e->ProjectionY("h9es");
	
	TH1D *hdiff = h9es->Clone();
	
	h7es->Sumw2();
	h9es->Sumw2();
	
	h7es->Scale(6.683);
	
	h7es->SetLineColor(kRed);
	h9es->SetLineColor(kBlue);
	hdiff->SetLineColor(kGreen);

	h9es->SetTitle(title);

	h9es->DrawCopy();
	h7es->DrawCopy("same");
	hdiff->Add(h7es, -1);
	hdiff->DrawCopy("same");

	TLegend	*lg = new TLegend(0.6, 0.7, 0.98, 0.9);
	lg->SetFillColor(kWhite);
	lg->AddEntry(h9es, "March 4-14, ~8.5 days, Power ~70%", "l");
	lg->AddEntry(h7es, "March 2, OFF (normalized by time)", "l");
	lg->AddEntry(hdiff, "Difference", "l");
	lg->Draw();
	
	sprintf(str, "Events %d", hdiff->Integral());
	txt = new TText();
	txt->SetTextColor(kGreen);
	txt->DrawTextNDC(0.35, 0.85, str);
	
	gPad->Update();
	f7.Close();
	f9.Close();

}

void nu_diff(char *hname, char *title)
{
	char str[256];
	TText *txt;

	gROOT->SetStyle("Plain");
	gStyle->SetOptStat(0);
	
	TFile f7("danss_data_000700.root");
	TFile f9("danss_data_000900.root");
	
	TH1D *h7es = f7.Get(hname);
	TH1D *h9es = f9.Get(hname);

	if (!h7es || !h9es) {
		printf("%s not found\n", hname);
		return;
	}

	TH1D *hdiff = h9es->Clone();
	
	h7es->Sumw2();
	h9es->Sumw2();
	
	h7es->Scale(6.683);
	
	h7es->SetLineColor(kRed);
	h9es->SetLineColor(kBlue);
	hdiff->SetLineColor(kGreen);

	h9es->SetTitle(title);

	h9es->DrawCopy();
	h7es->DrawCopy("same");
	hdiff->Add(h7es, -1);
	hdiff->DrawCopy("same");

	TLegend	*lg = new TLegend(0.6, 0.7, 0.98, 0.9);
	lg->SetFillColor(kWhite);
	lg->AddEntry(h9es, "March 4-14, ~8.5 days, Power ~70%", "l");
	lg->AddEntry(h7es, "March 2, OFF (normalized by time)", "l");
	lg->AddEntry(hdiff, "Difference", "l");
	lg->Draw();
	
	sprintf(str, "Events %d", hdiff->Integral());
	txt = new TText();
	txt->SetTextColor(kGreen);
	txt->DrawTextNDC(0.35, 0.85, str);
	
	gPad->Update();
	f7.Close();
	f9.Close();

}

