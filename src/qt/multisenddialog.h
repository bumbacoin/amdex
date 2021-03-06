#ifndef CHARITYDIALOG_H
#define CHARITYDIALOG_H

#include <QDialog>

namespace Ui {
class MultisendDialog;
}
class WalletModel;
class QLineEdit;
class MultisendDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MultisendDialog(QWidget *parent = 0);
    ~MultisendDialog();

    void setModel(WalletModel *model);
	void setAddress(const QString &address);
	void setAddress(const QString &address, QLineEdit *addrEdit);
private slots:
	void on_viewButton_clicked();
    void on_addButton_clicked();
    void on_deleteButton_clicked();
	void on_activateButton_clicked();
	void on_disableButton_clicked();
	void on_addressBookButton_clicked();

	
private:
    Ui::MultisendDialog *ui;
	WalletModel *model;
};

#endif // CHARITYDIALOG_H