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

LogDialog::LogDialog(QWidget *parent, const QString &title, const QLinkedList<MessageLog> &log, const QByteArray &mask) :
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

    int i = 0;
    foreach(const MessageLog &item, log)
    {
        QTableWidgetItem *timeItem = new QTableWidgetItem(QString("%1.%2")
                                                          .arg(item.sec, 10, 10, QChar('0'))
                                                          .arg(item.usec, 6, 10, QChar('0')));
        ui->tableWidget->setItem(i, 0, timeItem);
        QTableWidgetItem *dataItem = new QTableWidgetItem(QString(item.data.toHex()));
        ui->tableWidget->setItem(i, 1, dataItem);
        QTableWidgetItem *maskedItem = new QTableWidgetItem(QString(maskByteArray(item.data, mask).toHex()));
        ui->tableWidget->setItem(i, 2, maskedItem);
        i++;
    }
}

LogDialog::~LogDialog()
{
    delete ui;
}
