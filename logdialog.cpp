#include "logdialog.h"
#include "ui_logdialog.h"

LogDialog::LogDialog(QWidget *parent, const QString &can, quint16 id, quint8 length, const QLinkedList<MessageLog> &log, quint64 mask) :
    QDialog(parent),
    ui(new Ui::LogDialog)
{
    ui->setupUi(this);
    setWindowTitle(QString("%1:%2").arg(can).arg(id, 3, 16, QChar('0')));
    ui->lineCAN->setText(can);
    ui->lineID->setText(QString("%1").arg(id, 3, 16, QChar('0')));
    ui->lineMask->setText(toHex(mask, length));
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
        QTableWidgetItem *dataItem = new QTableWidgetItem(toHex(item.data, length));
        ui->tableWidget->setItem(i, 1, dataItem);
        QTableWidgetItem *maskedItem = new QTableWidgetItem(toHex(item.data & mask, length));
        ui->tableWidget->setItem(i, 2, maskedItem);
        i++;
    }
}

LogDialog::~LogDialog()
{
    delete ui;
}
