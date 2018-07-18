#ifndef LOGDIALOG_H
#define LOGDIALOG_H

#include <QDialog>
#include "logmodel.h"

namespace Ui {
class LogDialog;
}

class LogDialog : public QDialog
{
    Q_OBJECT
    friend class LogModel;

public:
    explicit LogDialog(QWidget *parent, const QString &title, const QVector<MessageLog> &log, const QByteArray &mask);
    ~LogDialog();

private:
    Ui::LogDialog *ui;
};

#endif // LOGDIALOG_H
