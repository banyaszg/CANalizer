#include "logdialog.h"
#include "ui_logdialog.h"

QByteArray maskByteArray(const QByteArray &data, const QByteArray &mask)
{
    QByteArray res;
    res.fill(0, data.size());

    for(int i = 0; i < data.size(); i++)
    {
        res[i] = data[i] & mask[i];
    }

    return res;
}

LogDialog::LogDialog(QWidget *parent, const QString &title, const QVector<MessageLog> &log, const QByteArray &mask) :
    QDialog(parent),
    ui(new Ui::LogDialog)
{
    ui->setupUi(this);
    setWindowTitle(title);
    ui->tableWidget->setColumnCount(3);
    ui->tableWidget->setRowCount(log.size());
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    QStringList headers;
    headers.append(QString("time"));
    headers.append(QString("data(hex)"));
    headers.append(QString("masked data(hex)"));
    ui->tableWidget->setHorizontalHeaderLabels(headers);

    for(int i = 0; i < log.size(); i++)
    {
        const MessageLog &item = log[i];
        QTableWidgetItem *timeItem = new QTableWidgetItem(QString("%1.%2").arg(item.sec).arg(item.usec));
        ui->tableWidget->setItem(i, 0, timeItem);
        QTableWidgetItem *dataItem = new QTableWidgetItem(QString(item.data.toHex()));
        ui->tableWidget->setItem(i, 1, dataItem);
        QTableWidgetItem *maskedItem = new QTableWidgetItem(QString(maskByteArray(item.data, mask).toHex()));
        ui->tableWidget->setItem(i, 2, maskedItem);
    }
}

LogDialog::~LogDialog()
{
    delete ui;
}
