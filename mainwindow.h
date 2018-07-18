#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProgressBar>
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
    void on_actionClear_triggered();
    void on_actionLoad_triggered();

    void on_actionSignal_toggled(bool arg1);

private:
    Ui::MainWindow *ui;
    LogModel *model;
    QProgressBar *progressBar;
};

#endif // MAINWINDOW_H
