#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProgressBar>
#include "logmodel.h"
#include <QCanBusDevice>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionClearAll_triggered();
    void on_actionLoad_triggered();
    void on_actionChanges_toggled(bool arg1);
    void on_actionExit_triggered();
    void on_actionStartCapture_triggered();
    void on_actionStopCapture_triggered();
    void errorOccurred(QCanBusDevice::CanBusError) const;
    void framesReceived();

    void on_actionGenMask_toggled(bool arg1);

    void on_actionClearStatus_triggered();

private:
    Ui::MainWindow *ui;
    LogModel *model = nullptr;
    QProgressBar *progressBar = nullptr;
    QCanBusDevice *canDevice = nullptr;
    QString canInterface;
    bool liveUpdate;
};

#endif // MAINWINDOW_H
