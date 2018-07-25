#ifndef LOGDIALOG_H
#define LOGDIALOG_H

#include <QDialog>
#include "logmodel.h"

namespace Ui {
class LogDialog;
}

class QTableWidgetItem;

class LogDialog : public QDialog
{
    Q_OBJECT

    enum Columns { TIME = 0, DATA = 1, MASKED = 2, NOTE = 3, END = 4 };

public:
    explicit LogDialog(QWidget *parent, CANMessage *pmsg);
    ~LogDialog();

private slots:
    void on_textNote_textChanged();

    void on_tableWidget_itemChanged(QTableWidgetItem *item);

protected:
    CANMessage *_pmsg = nullptr;

private:
    Ui::LogDialog *ui;
    bool _inited = false;
};

#endif // LOGDIALOG_H
