#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "walletmodel.h"
#include "bitcoinunits.h"
#include "optionsmodel.h"
#include "transactiontablemodel.h"
#include "transactionfilterproxy.h"
#include "guiutil.h"
#include "guiconstants.h"

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QDesktopServices>
#include <QUrl>

#define DECORATION_SIZE 64
#define NUM_ITEMS 3

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

        painter->setPen(foreground);
        painter->drawText(addressRect, Qt::AlignLeft|Qt::AlignVCenter, address);

        if(amount < 0)
        {
            foreground = COLOR_NEGATIVE;
        }
        else if(!confirmed)
        {
            foreground = COLOR_UNCONFIRMED;
        }
        else
        {
            foreground = option.palette.color(QPalette::Text);
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
    ui->labelWalletStatus->setText("(" + tr("Out of sync") + ")");
    ui->labelTransactionsStatus->setText("(" + tr("Out of sync") + ")");

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

void OverviewPage::setBalance(qint64 balance, qint64 unconfirmedBalance, qint64 immatureBalance)
{
    int unit = model->getOptionsModel()->getDisplayUnit();
    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    ui->labelBalance->setText(BitcoinUnits::formatWithUnit(unit, balance));
    ui->labelUnconfirmed->setText(BitcoinUnits::formatWithUnit(unit, unconfirmedBalance));
    ui->labelImmature->setText(BitcoinUnits::formatWithUnit(unit, immatureBalance));

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
        filter->sort(TransactionTableModel::Status, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter);
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance());
        connect(model, SIGNAL(balanceChanged(qint64, qint64, qint64)), this, SLOT(setBalance(qint64, qint64, qint64)));

        setNumTransactions(model->getNumTransactions());
        connect(model, SIGNAL(numTransactionsChanged(int)), this, SLOT(setNumTransactions(int)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
    }

    // update the display unit, to not use the default ("BIC")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if(model && model->getOptionsModel())
    {
        if(currentBalance != -1)
            setBalance(currentBalance, currentUnconfirmedBalance, currentImmatureBalance);

        // Update txdelegate->unit with the current unit
        txdelegate->unit = model->getOptionsModel()->getDisplayUnit();

        ui->listTransactions->update();
    }
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->labelWalletStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
}

// -------------------------
// BITCOIN TALK BTN
// -------------------------
// BTN PRESS CTRL FUNCTION
void OverviewPage::on_bct_btn_pressed()
{
    QPixmap pix(":/images/res/images/btctalkr.png");
    this->ui->bct_btn->setIcon(pix);
}
// BTN RELEASE CTRL FUNCTION
void OverviewPage::on_bct_btn_released()
{
    QPixmap pix(":/images/res/images/btctalk.png");
    this->ui->bct_btn->setIcon(pix);
}
// BTN ACTION
void OverviewPage::on_bct_btn_clicked()
{
    QString link="https://bitcointalk.org/index.php?topic=1223493.0";
    QDesktopServices::openUrl(QUrl(link));
}

// -------------------------
// CRYPTOPIA BTN
// -------------------------
// BTN PRESS CTRL FUNCTION
void OverviewPage::on_ex_btn_pressed()
{
    QPixmap pix(":/images/res/images/topiabtnr.png");
    this->ui->ex_btn->setIcon(pix);
}
// BTN RELEASE CTRL FUNCTION
void OverviewPage::on_ex_btn_released()
{
    QPixmap pix(":/images/res/images/topiabtn.png");
    this->ui->ex_btn->setIcon(pix);
}
// BTN ACTION
void OverviewPage::on_ex_btn_clicked()
{
    QString link="https://www.cryptopia.co.nz/";
    QDesktopServices::openUrl(QUrl(link));
}

// -------------------------
// CURRENT LOGO BTN
// -------------------------
// BTN PRESS CTRL FUNCTION
void OverviewPage::on_current_btn_pressed()
{
    QPixmap pix(":/icons/res/icons/nbic_2.png");
    this->ui->current_btn->setIcon(pix);
}
// BTN RELEASE CTRL FUNCTION
void OverviewPage::on_current_btn_released()
{
    QPixmap pix(":/icons/res/icons/nbic.png");
    this->ui->current_btn->setIcon(pix);
}
// BTN ACTION
void OverviewPage::on_current_btn_clicked()
{
    QPixmap pix(":/images/res/images/Binary_Banner.png");
    this->ui->label_walletart->setPixmap(pix);
}


// -------------------------
// INFO BTN
// -------------------------
// BTN PRESS CTRL FUNCTION
void OverviewPage::on_info_btn_pressed()
{
    QPixmap pix(":/icons/res/icons/ibic.png");
    this->ui->info_btn->setIcon(pix);
}
// BTN RELEASE CTRL FUNCTION
void OverviewPage::on_info_btn_released()
{
    QPixmap pix(":/icons/res/icons/ibic_2.png");
    this->ui->info_btn->setIcon(pix);
}
// BTN ACTION
void OverviewPage::on_info_btn_clicked()
{
    QPixmap pix(":/images/res/images/Binary_Bannerinfo1.png");
    this->ui->label_walletart->setPixmap(pix);
}

// -------------------------
// OLD BTN
// -------------------------
// BTN PRESS CTRL FUNCTION
void OverviewPage::on_old_btn_pressed()
{
    QPixmap pix(":/icons/res/icons/bic_2.png");
    this->ui->old_btn->setIcon(pix);
}
// BTN RELEASE CTRL FUNCTION
void OverviewPage::on_old_btn_released()
{
    QPixmap pix(":/icons/res/icons/bic.png");
    this->ui->old_btn->setIcon(pix);
}
// BTN ACTION
void OverviewPage::on_old_btn_clicked()
{
    QPixmap pix(":/images/res/images/Binary_BannerOLD1.png");
    this->ui->label_walletart->setPixmap(pix);
}

// -------------------------
// CRYPTOCOIN TALK BTN
// -------------------------
// BTN PRESS CTRL FUNCTION
void OverviewPage::on_cctbtn_pressed()
{
    QPixmap pix(":/images/res/images/cctr.png");
    this->ui->cctbtn->setIcon(pix);
}
// BTN RELEASS CTRL FUNCTION
void OverviewPage::on_cctbtn_released()
{
    QPixmap pix(":/images/res/images/cct.png");
    this->ui->cctbtn->setIcon(pix);
}
// BTN ACTION
void OverviewPage::on_cctbtn_clicked()
{
    QString link="https://cryptocointalk.com/topic/42780-ann-binarybic-scrypt-15-sec-blocks-84m-coins-binarycoin-revived/";
    QDesktopServices::openUrl(QUrl(link));
}

// -------------------------
// C-CEX BTN
// -------------------------
// BTN PRESS CTRL FUNCTION
void OverviewPage::on_ccexbtn_pressed()
{
    QPixmap pix(":/images/res/images/ccexbtnr.png");
    this->ui->ccexbtn->setIcon(pix);
}
// BTN RELEASS CTRL FUNCTION
void OverviewPage::on_ccexbtn_released()
{
    QPixmap pix(":/images/res/images/ccexbtn.png");
    this->ui->ccexbtn->setIcon(pix);
}
// BTN ACTION
void OverviewPage::on_ccexbtn_clicked()
{
    QString link="https://c-cex.com/?p=bic-btc";
    QDesktopServices::openUrl(QUrl(link));
}
