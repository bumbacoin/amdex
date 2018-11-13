#include "posstats.h"
#include "ui_posstats.h"
#include "clientmodel.h"
#include "main.h"
#include "bitcoinrpc.h"
#include "util.h"
double GetPoSKernelPS(const CBlockIndex* pindex);

using namespace json_spirit;

PosStats::PosStats(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PosStats)
{
    ui->setupUi(this);    
    if(GetBoolArg("-chart", true))
    {
		QFont label = font();
        ui->diffplot_weight->addGraph();
		ui->diffplot_weight->setBackground(QBrush(QColor(255,255,255)));
        ui->diffplot_weight->xAxis->setLabel("Height");
        ui->diffplot_weight->yAxis->setLabel("Network Weight");
        ui->diffplot_weight->graph(0)->setPen(QPen(QColor(131,189,177), 3, Qt::SolidLine, Qt::SquareCap, Qt::BevelJoin));
		ui->diffplot_weight->graph(0)->setBrush(QBrush(QColor(131,189,177,25)));
        ui->diffplot_weight->xAxis->setTickLabelColor(QColor(32,43,53));
        ui->diffplot_weight->xAxis->setBasePen(QPen(QColor(212,211,211)));
        ui->diffplot_weight->xAxis->setLabelColor(QColor(131,189,177));
        ui->diffplot_weight->xAxis->setTickPen(QPen(QColor(212,211,211)));
        ui->diffplot_weight->xAxis->setSubTickPen(QPen(QColor(212,211,211)));
        ui->diffplot_weight->yAxis->setTickLabelColor(QColor(32,43,53));
        ui->diffplot_weight->yAxis->setBasePen(QPen(QColor(212,211,211)));
        ui->diffplot_weight->yAxis->setLabelColor(QColor(131,189,177));
        ui->diffplot_weight->yAxis->setTickPen(QPen(QColor(212,211,211)));
        ui->diffplot_weight->yAxis->setSubTickPen(QPen(QColor(212,211,211)));
        ui->diffplot_weight->yAxis->grid()->setPen(QPen(QColor(QColor(212,211,211)), 1, Qt::DotLine));		
        ui->diffplot_weight->xAxis->grid()->setPen(QPen(QColor(QColor(212,211,211)), 1, Qt::DotLine));
		ui->diffplot_weight->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
        ui->diffplot_weight->graph(0)->setLineStyle(QCPGraph::lsLine); 
        ui->diffplot_weight->xAxis->setLabelFont(label);
        ui->diffplot_weight->yAxis->setLabelFont(label);
		
		ui->diffplot_pos->addGraph();
		ui->diffplot_pos->setBackground(QBrush(QColor(255,255,255)));
		ui->diffplot_pos->xAxis->setLabel(tr("Height"));
		ui->diffplot_pos->yAxis->setLabel(tr("Difficulty"));
        ui->diffplot_pos->graph(0)->setPen(QPen(QColor(131,189,177), 3, Qt::SolidLine, Qt::SquareCap, Qt::BevelJoin));
		ui->diffplot_pos->graph(0)->setBrush(QBrush(QColor(131,189,177,25)));
        ui->diffplot_pos->xAxis->setTickLabelColor(QColor(32,43,53));
        ui->diffplot_pos->xAxis->setBasePen(QPen(QColor(212,211,211)));
        ui->diffplot_pos->xAxis->setLabelColor(QColor(131,189,177));
        ui->diffplot_pos->xAxis->setTickPen(QPen(QColor(212,211,211)));
        ui->diffplot_pos->xAxis->setSubTickPen(QPen(QColor(212,211,211)));
        ui->diffplot_pos->yAxis->setTickLabelColor(QColor(32,43,53));
        ui->diffplot_pos->yAxis->setBasePen(QPen(QColor(212,211,211)));
        ui->diffplot_pos->yAxis->setLabelColor(QColor(131,189,177));
        ui->diffplot_pos->yAxis->setTickPen(QPen(QColor(212,211,211)));
        ui->diffplot_pos->yAxis->setSubTickPen(QPen(QColor(212,211,211)));
        ui->diffplot_pos->yAxis->grid()->setPen(QPen(QColor(QColor(212,211,211)), 1, Qt::DotLine));		
        ui->diffplot_pos->xAxis->grid()->setPen(QPen(QColor(QColor(212,211,211)), 1, Qt::DotLine));
		ui->diffplot_pos->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
        ui->diffplot_pos->graph(0)->setLineStyle(QCPGraph::lsLine);    
		ui->diffplot_pos->xAxis->setLabelFont(label);
		ui->diffplot_pos->yAxis->setLabelFont(label);
    }
}

void PosStats::updatePlot()
{
    static int64_t lastUpdate = 0;
    // Double Check to make sure we don't try to update the plot when it is disabled
    if(!GetBoolArg("-chart", true)) { return; }
    if (GetTime() - lastUpdate < 600) { return; } // This is just so it doesn't redraw rapidly during syncing

    if(fDebug) { printf("Plot: Getting Ready: pindexBest: %p\n", pindexBest); }
    	
		bool fProofOfStake = (nBestHeight > 1);
    if (fProofOfStake)
        ui->diffplot_weight->yAxis->setLabel("Network Weight");
		else
        ui->diffplot_weight->yAxis->setLabel("Network Weight");

    int numLookBack = 1000;
    double diffMax = 0;
    const CBlockIndex* pindex = GetLastBlockIndex(pindexBest, fProofOfStake);
    int height = pindex->nHeight;
    int xStart = std::max<int>(height-numLookBack, 0) + 1;
    int xEnd = height;

    // Start at the end and walk backwards
    int i = numLookBack-1;
    int x = xEnd;

    // This should be a noop if the size is already 2000
    vX.resize(numLookBack);
    vY.resize(numLookBack);

    if(fDebug) {
        if(height != pindex->nHeight) {
            printf("Plot: Warning: nBestHeight and pindexBest->nHeight don't match: %d:%d:\n", height, pindex->nHeight);
        }
    }

    if(fDebug) { printf("Plot: Reading blockchain\n"); }

    const CBlockIndex* itr = pindex;
    while(i >= 0 && itr != NULL)
    {
        if(fDebug) { printf("Plot: Processing block: %d - pprev: %p\n", itr->nHeight, itr->pprev); }
        vX[i] = itr->nHeight;
        if (itr->nHeight < xStart) {
        	xStart = itr->nHeight;
        }
        vY[i] = fProofOfStake ? GetPoSKernelPS(itr) : GetDifficulty(itr);
        diffMax = std::max<double>(diffMax, vY[i]);

        itr = GetLastBlockIndex(itr->pprev, fProofOfStake);
        i--;
        x--;
    }

    if(fDebug) { printf("Plot: Drawing plot\n"); }

    ui->diffplot_weight->graph(0)->setData(vX, vY);

    // set axes ranges, so we see all data:
    ui->diffplot_weight->xAxis->setRange((double)xStart, (double)xEnd);
    ui->diffplot_weight->yAxis->setRange(0, diffMax+(diffMax/10));

    ui->diffplot_weight->xAxis->setAutoSubTicks(false);
    ui->diffplot_weight->yAxis->setAutoSubTicks(false);
    ui->diffplot_weight->xAxis->setSubTickCount(0);
    ui->diffplot_weight->yAxis->setSubTickCount(0);

    ui->diffplot_weight->replot();
	
	    diffMax = 0;

    // Start at the end and walk backwards
    i = numLookBack-1;
    x = xEnd;
	// This should be a noop if the size is already 2000
    vX3.resize(numLookBack);
    vY3.resize(numLookBack);

    CBlockIndex* itr3 = pindex;

    while(i >= 0 && itr3 != NULL)
    {
        vX3[i] = itr3->nHeight;
        vY3[i] = fProofOfStake ? GetDifficulty(itr3) : GetPoSKernelPS(itr3);
        diffMax = std::max<double>(diffMax, vY3[i]);

        itr3 = GetLastBlockIndex(itr3->pprev, fProofOfStake);
        i--;
        x--;
    }

    ui->diffplot_pos->graph(0)->setData(vX3, vY3);

    // set axes ranges, so we see all data:
    ui->diffplot_pos->xAxis->setRange((double)xStart, (double)xEnd);
    ui->diffplot_pos->yAxis->setRange(0, diffMax+(diffMax/10));

    ui->diffplot_pos->xAxis->setAutoSubTicks(false);
    ui->diffplot_pos->yAxis->setAutoSubTicks(false);
    ui->diffplot_pos->xAxis->setSubTickCount(0);
    ui->diffplot_pos->yAxis->setSubTickCount(0);

    ui->diffplot_pos->replot();

    

    if(fDebug) { printf("Plot: Done!\n"); }
    	
    lastUpdate = GetTime();
}

void PosStats::setStrength(double strength)
{
    QString strFormat;
    if (strength == 0)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 0;
    }
	 else if(strength < 0.01)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 1;
    }
	else if(strength < 0.02)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 2;
    }
	else if(strength < 0.03)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 3;
    }
	else if(strength < 0.04)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 4;
    }
	 else if(strength < 0.05)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 5;
    }
	else if(strength < 0.06)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 6;
    }
	else if(strength < 0.07)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 7;
    }
	else if(strength < 0.08)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 8;
    }
	else if(strength < 0.09)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 9;
    }
    else if(strength < 0.1)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 10;
    }
	else if(strength < 0.11)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 11;
    }
	else if(strength < 0.12)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 12;
    }
	else if(strength < 0.13)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 13;
    }
	else if(strength < 0.14)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 14;
    }
	else if(strength < 0.15)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 15;
    }
	else if(strength < 0.16)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 1;
    }
	else if(strength < 0.16)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 17;
    }
	else if(strength < 0.18)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 18;
    }
	else if(strength < 0.19)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 19;
    }
    else if (strength < 0.2)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 20;
    }
	else if(strength < 0.25)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 25;
    }
    else if (strength < 0.3)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 25;
    }
	else if(strength < 0.35)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 35;
    }
    else if (strength < 0.4)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 40;
    }
	else if(strength < 0.45)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 45;
    }
    else if (strength < 0.5)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 50;
    }
	else if(strength < 0.55)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 55;
    }
    else if (strength < 0.6)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 60;
    }
	else if(strength < 0.65)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 65;
    }
    else if (strength < 0.7)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 70;
    }
	else if(strength < 0.75)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 75;
    }
    else if (strength < 0.8)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 80;
    }
    else if (strength < 0.9)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 90;
    }
    else if (strength <= 1.0)
    {
        strFormat = "My Estimated Network Weight";
        currentStrength = 100;
    }
    else
    {
        strFormat = "Error!";
    }
    ui->strengthBar->setValue(currentStrength);
    ui->strengthBar->setFormat(strFormat);
    ui->strengthBar->setTextVisible(true);
}

void PosStats::setModel(ClientModel *model)
{
    //updateStatistics();
    this->model = model;
}

PosStats::~PosStats()
{
    delete ui;
}
