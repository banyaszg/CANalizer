#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "logmodel.h"

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
    void on_actionLoad_Noise_log_triggered();
    void on_actionLoad_Signal_log_triggered();

    void on_actionClear_triggered();

protected:
    void loadLogs(bool signal);

private:
    Ui::MainWindow *ui;
    LogModel *model;
};

#endif // MAINWINDOW_H
