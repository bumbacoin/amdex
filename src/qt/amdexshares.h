#ifndef AMDEXSHARES_H
#define AMDEXSHARES_H

#include <QWidget>

namespace Ui {
    class AmdexSharesDialog;
}

class AmdexSharesDialog : public QWidget
{
        Q_OBJECT

    public:
        explicit AmdexSharesDialog(QWidget *parent = 0);
        ~AmdexSharesDialog();

    protected:
        void changeEvent(QEvent *e);

    private:
        Ui::AmdexSharesDialog *ui;
};

#endif
