#ifndef CAPTUREDIALOG_H
#define CAPTUREDIALOG_H

#include <QDialog>

namespace Ui {
class CaptureDialog;
}

class CaptureDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CaptureDialog(const QString &plugin, const QString &interface, bool live, QWidget *parent = 0);
    ~CaptureDialog();

    QString plugin();
    QString interface();
    bool live();

private slots:
    void on_selectPlugin_currentTextChanged(const QString &arg1);

    void on_selectInterface_currentTextChanged(const QString &currentText);

    void on_btnOk_clicked();

    void on_btnCancel_clicked();

private:
    Ui::CaptureDialog *ui;
};

#endif // CAPTUREDIALOG_H
