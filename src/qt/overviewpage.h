#ifndef OVERVIEWPAGE_H
#define OVERVIEWPAGE_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

namespace Ui {
    class OverviewPage;
}
class WalletModel;
class TxViewDelegate;
class TransactionFilterProxy;

/** Overview ("home") page widget */
class OverviewPage : public QWidget
{
    Q_OBJECT

public:
    explicit OverviewPage(QWidget *parent = 0);
    ~OverviewPage();

    void setModel(WalletModel *model);
    void showOutOfSyncWarning(bool fShow);

public slots:
    void setBalance(qint64 balance, qint64 unconfirmedBalance, qint64 immatureBalance);
    void setNumTransactions(int count);

signals:
    void transactionClicked(const QModelIndex &index);

private:
    Ui::OverviewPage *ui;
    WalletModel *model;
    qint64 currentBalance;
    qint64 currentUnconfirmedBalance;
    qint64 currentImmatureBalance;

    TxViewDelegate *txdelegate;
    TransactionFilterProxy *filter;

private slots:
    void updateDisplayUnit();
    void handleTransactionClicked(const QModelIndex &index);
    void on_bct_btn_pressed();
    void on_bct_btn_released();
    void on_ex_btn_pressed();
    void on_ex_btn_released();
    void on_current_btn_pressed();
    void on_current_btn_released();
    void on_info_btn_pressed();
    void on_info_btn_released();
    void on_old_btn_pressed();
    void on_old_btn_released();
    void on_old_btn_clicked();
    void on_current_btn_clicked();
    void on_bct_btn_clicked();
    void on_ex_btn_clicked();
    void on_info_btn_clicked();
};

#endif // OVERVIEWPAGE_H
