#ifndef POSSTATS_H
#define POSSTATS_H

#include <QWidget>

//#include "walletmodel.h"

namespace Ui {
	class PosStats;
}
class ClientModel;

class PosStats : public QWidget
{
    Q_OBJECT

public:
    explicit PosStats(QWidget *parent = 0);
    ~PosStats();
    
    void setModel(ClientModel *model);
    void updatePlot();
	void setStrength(double strength);

private slots:

private:
    Ui::PosStats *ui;
    ClientModel *model;
	
	QVector<double> vX;
	QVector<double> vY;
	
	QVector<double> vX3;
	QVector<double> vY3;
	
	double currentStrength;
};

#endif 