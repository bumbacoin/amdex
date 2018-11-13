#include "amdexshares.h"
#include "ui_amdexshares.h"

AmdexSharesDialog::AmdexSharesDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AmdexSharesDialog)
{
    ui->setupUi(this);
}

AmdexSharesDialog::~AmdexSharesDialog()
{
    delete ui;
}

void AmdexSharesDialog::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
        case QEvent::LanguageChange:
            ui->retranslateUi(this);
            break;
        default:
            break;
    }
}
