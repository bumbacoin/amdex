#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "walletmodel.h"
#include "bitcoinunits.h"
#include "optionsmodel.h"
#include "transactiontablemodel.h"
#include "transactionfilterproxy.h"
#include "guiutil.h"
#include "guiconstants.h"
#include "askpassphrasedialog.h"
#include "main.h"
#include "wallet.h"
#include "init.h"
#include "base58.h"
#include "clientmodel.h"
#include "bitcoinrpc.h"
#include "util.h"

#include <sstream>
#include <string>

#include <QAbstractItemDelegate>
#include <QPainter>

#define DECORATION_SIZE 42
#define NUM_ITEMS 5

using namespace json_spirit;

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate(): QAbstractItemDelegate(), unit(BitcoinUnits::BTC)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        QRect mainRect = option.rect;
        painter->setBackground(QBrush(QColor(255,255,255),Qt::SolidPattern));
        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2*ypad)/2;
        QRect amountRect(mainRect.left() + xspace, mainRect.top()+ypad, mainRect.width() - xspace, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top()+ypad+halfheight, mainRect.width() - xspace, halfheight);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = option.palette.color(QPalette::Text);
        if(qVariantCanConvert<QColor>(value))
        {
            foreground = qvariant_cast<QColor>(value);
        }
foreground = QColor(167,169,169);
        painter->setPen(foreground);
        painter->drawText(addressRect, Qt::AlignLeft|Qt::AlignVCenter, address);

        if(amount < 0)
        {
            foreground = option.palette.color(QPalette::Text);
            foreground = QColor(217,119,119);
        }
        else if(!confirmed)
        {
            foreground = option.palette.color(QPalette::Text);
            foreground = QColor(167,169,169);
        }
        else
        {
            foreground = option.palette.color(QPalette::Text);
            foreground = QColor(131,189,177);
        }
        painter->setPen(foreground);
        QString amountText = BitcoinUnits::formatWithUnit(unit, amount, true);
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignVCenter, amountText);

        painter->setPen(option.palette.color(QPalette::Text));
        painter->drawText(amountRect, Qt::AlignLeft|Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->setPen(QColor(39,39,39));
        painter->drawLine(QPoint(mainRect.left(), mainRect.bottom()), QPoint(mainRect.right(), mainRect.bottom()));
        painter->drawLine(QPoint(amountRect.left()-6, mainRect.top()), QPoint(amountRect.left()-6, mainRect.bottom()));
        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }

    int unit;

};
#include "overviewpage.moc"

OverviewPage::OverviewPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverviewPage),
    currentBalance(-1),
    currentStake(0),
    currentUnconfirmedBalance(-1),
    currentImmatureBalance(-1),
    txdelegate(new TxViewDelegate()),
    filter(0)
{
    ui->setupUi(this);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));

	
    // init "out of sync" warning labels
    ui->labelWalletStatus->setText("(" + tr("Out of sync with the Amsterdex blockchain") + ")");
    ui->labelTransactionsStatus->setText("(" + tr("Out of sync with the Amsterdex blockchain") + ")");
	ui->labelNetworkStatus->setText("(" + tr("Out of sync with the Amsterdex blockchain") + ")");
	
    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);
}

void OverviewPage::handleTransactionClicked(const QModelIndex &index)
{
    if(filter)
        emit transactionClicked(filter->mapToSource(index));
}

OverviewPage::~OverviewPage()
{
    delete ui;
}


void OverviewPage::setBalance(qint64 balance, qint64 stake, qint64 unconfirmedBalance, qint64 immatureBalance)
{
    int unit = model->getOptionsModel()->getDisplayUnit();
    currentBalance = balance;
    currentStake = stake;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    ui->labelBalance->setText(BitcoinUnits::formatWithUnit(unit, balance));
    ui->labelStake->setText(BitcoinUnits::formatWithUnit(unit, stake));
    ui->labelUnconfirmed->setText(BitcoinUnits::formatWithUnit(unit, unconfirmedBalance));
    ui->labelImmature->setText(BitcoinUnits::formatWithUnit(unit, immatureBalance));
	ui->labelTotal->setText(BitcoinUnits::formatWithUnit(unit, balance + stake + unconfirmedBalance + immatureBalance));

    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = immatureBalance != 0;
    ui->labelImmature->setVisible(showImmature);
    ui->labelImmatureText->setVisible(showImmature);
}

void OverviewPage::setNumTransactions(int count)
{
    ui->labelNumTransactions->setText(QLocale::system().toString(count));
}

int heightPrevious = -1;
int connectionPrevious = -1;
int volumePrevious = -1;
double netPawratePrevious = -1;
double hardnessPrevious = -1;
double hardnessPrevious2 = -1;
int stakeminPrevious = -1;
int stakemaxPrevious = -1;
QString stakecPrevious = "";
QString percPrevious = "";

void OverviewPage::updateOverview()
{
	int unit = model->getOptionsModel()->getDisplayUnit();
    double pHardness2 = GetDifficulty(GetLastBlockIndex(pindexBest, true));
    int nHeight = pindexBest->nHeight;
    uint64_t nNetworkWeight = GetPoSKernelPS();
    double volume = pindexBest->nMoneySupply;
    int peers = this->model2->getNumConnections();
    QString height = QString::number(nHeight);
    QString stakemax = QString::number(nNetworkWeight);
    QString hardness2 = QString::number(pHardness2, 'f', 8);
	QString Qlpawrate = model2->getLastBlockDate().toString();
    QString QPeers = QString::number(peers);
    QString qVolume = BitcoinUnits::formatWithUnit(unit, volume);
	
    if(nHeight > heightPrevious)
    {
        ui->heightBox->setText("" + height + "");
    } else {
    ui->heightBox->setText(height);
    }

    if(0 > stakemaxPrevious)
    {
        ui->maxBox->setText("" + stakemax + "");
    } else {
    ui->maxBox->setText(stakemax);
    }

    if(pHardness2 > hardnessPrevious2)
    {
        ui->diffBox2->setText("" + hardness2 + "");
    } else if(pHardness2 < hardnessPrevious2) {
        ui->diffBox2->setText("" + hardness2 + "");
    } else {
        ui->diffBox2->setText(hardness2);
    }
    	
	if(Qlpawrate != pawratePrevious)
    {
        ui->localBox->setText("" + Qlpawrate + "");
    } else {
    ui->localBox->setText(Qlpawrate);
    }

    if(peers > connectionPrevious)
    {
        ui->connectionBox->setText("" + QPeers + "");             
    } else if(peers < connectionPrevious) {
        ui->connectionBox->setText("" + QPeers + "");        
    } else {
        ui->connectionBox->setText(QPeers);  
    }

    if(volume > volumePrevious)
    {
        ui->volumeBox->setText("" + qVolume + "");
    } else if(volume < volumePrevious) {
        ui->volumeBox->setText("" + qVolume + "");
    } else {
        ui->volumeBox->setText(qVolume);
    }
    updatePrevious(nHeight, nNetworkWeight, pHardness2, Qlpawrate, peers, volume);
}

void OverviewPage::updatePrevious(int nHeight, int nNetworkWeight, double pHardness2, QString Qlpawrate, int peers, double volume)
{
    heightPrevious = nHeight;	
    stakemaxPrevious = nNetworkWeight;
    hardnessPrevious2 = pHardness2;
	pawratePrevious = Qlpawrate;
    connectionPrevious = peers;
    volumePrevious = volume;
}


void OverviewPage::setModel(WalletModel *model)
{
    this->model = model;
    if(model && model->getOptionsModel())
    {
        // Set up transaction list
        filter = new TransactionFilterProxy();
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->sort(TransactionTableModel::Date, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter);
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getStake(), model->getUnconfirmedBalance(), model->getImmatureBalance());
        connect(model, SIGNAL(balanceChanged(qint64, qint64, qint64, qint64)), this, SLOT(setBalance(qint64, qint64, qint64, qint64)));

        setNumTransactions(model->getNumTransactions());
        connect(model, SIGNAL(numTransactionsChanged(int)), this, SLOT(setNumTransactions(int)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
    }

    // update the display unit, to not use the default ("420")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if(model && model->getOptionsModel())
    {
        if(currentBalance != -1)
            setBalance(currentBalance, model->getStake(), currentUnconfirmedBalance, currentImmatureBalance);

        // Update txdelegate->unit with the current unit
        txdelegate->unit = model->getOptionsModel()->getDisplayUnit();

        ui->listTransactions->update();
    }
}

void OverviewPage::setModel2(ClientModel *model2)
{
    //updateStatistics();
    this->model2 = model2;
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->labelWalletStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
	ui->labelNetworkStatus->setVisible(fShow);
}