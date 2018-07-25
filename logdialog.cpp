#include "logdialog.h"
#include "ui_logdialog.h"
#include <QDebug>
#include <QFileDialog>
#include <QSettings>

LogDialog::LogDialog(QWidget *parent, CANMessage *pmsg) :
    QDialog(parent),
    ui(new Ui::LogDialog)
{
    ui->setupUi(this);

    _pmsg = pmsg;

    setWindowTitle(QString("%1:%2").arg(_pmsg->can).arg(_pmsg->id, 3, 16, QChar('0')));

    ui->lineCAN->setText(_pmsg->can);
    ui->lineID->setText(QString("%1").arg(_pmsg->id, 3, 16, QChar('0')));
    ui->lineMask->setText(toHex(_pmsg->chbits, _pmsg->length));
    ui->textNote->setText(_pmsg->note);

    ui->tableWidget->setColumnCount(END);
    ui->tableWidget->setRowCount(_pmsg->changeLog.size());
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    QStringList headers;
    headers.append(QString("Time"));
    headers.append(QString("Data(hex)"));
    headers.append(QString("Masked Data(hex)"));
    headers.append(QString("Note"));
    ui->tableWidget->setHorizontalHeaderLabels(headers);

    int i = 0;
    foreach(const MessageLog &item, _pmsg->changeLog)
    {
        QTableWidgetItem *timeItem = new QTableWidgetItem(QString("%1.%2")
                                                          .arg(item.sec, 10, 10, QChar('0'))
                                                          .arg(item.usec, 6, 10, QChar('0')));
        timeItem->setFlags(timeItem->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget->setItem(i, TIME, timeItem);
        QTableWidgetItem *dataItem = new QTableWidgetItem(toHex(item.data, _pmsg->length));
        dataItem->setFlags(dataItem->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget->setItem(i, DATA, dataItem);
        QTableWidgetItem *maskedItem = new QTableWidgetItem(toHex(item.data & _pmsg->chbits, _pmsg->length));
        maskedItem->setFlags(maskedItem->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget->setItem(i, MASKED, maskedItem);
        QTableWidgetItem *noteItem = new QTableWidgetItem(item.note);
        ui->tableWidget->setItem(i, NOTE, noteItem);
        i++;
    }
    _inited = true;
}

LogDialog::~LogDialog()
{
    delete ui;
}

void LogDialog::on_textNote_textChanged()
{
    _pmsg->note = ui->textNote->toPlainText();
}

void LogDialog::on_tableWidget_itemChanged(QTableWidgetItem *item)
{
    if(!_inited) return;
    if(item->column() == NOTE)
    {
        int ix = item->row();
        int i = 0;
        QMutableLinkedListIterator<MessageLog> it(_pmsg->changeLog);
        while(it.hasNext())
        {
            MessageLog &log = it.next();
            if(i == ix)
            {
                log.note = item->text();
                break;
            }
            i++;
        }
    }
}

void LogDialog::on_btnSave_clicked()
{
    const QString DEFAULT_DIR_KEY("default_dir");

    QSettings settings;

    QString selectedFile = QFileDialog::getSaveFileName(
            this, QString("Select am outputfile"),
                settings.value(DEFAULT_DIR_KEY).toString()
                + QString("/%1-%2.txt").arg(_pmsg->can).arg(_pmsg->id, 3, 16, QChar('0')),
                "Text files (*.txt)");

    if(!selectedFile.isEmpty())
    {
        settings.setValue(DEFAULT_DIR_KEY,
                            QFileInfo(selectedFile).absolutePath());

        QFile file(selectedFile);
        if(!file.open(QIODevice::WriteOnly | QFile::Truncate))
            return;
        QTextStream out(&file);
        out << "CAN bus: " << _pmsg->can
            << "  ID: " << QString("%1").arg(_pmsg->id, 3, 16, QChar('0')) << endl;
        out << "Mask: " << toHex(_pmsg->bitmask, _pmsg->length) << endl;
        out << "Changing bits: " << toHex(_pmsg->chbits, _pmsg->length) << endl << endl;
        out << _pmsg->note << endl << endl;

        foreach(const MessageLog &item, _pmsg->changeLog)
        {
            out << QString("%1.%2")
                   .arg(item.sec, 10, 10, QChar('0'))
                   .arg(item.usec, 6, 10, QChar('0')) << ";"
                << toHex(item.data, _pmsg->length) << ";"
                << toHex(item.data & _pmsg->chbits, _pmsg->length) << ";"
                << item.note << endl;
        }
    }
}
