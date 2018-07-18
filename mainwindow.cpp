#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    model = new LogModel(this);
    ui->setupUi(this);
    ui->tableView->setModel(model);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionLoad_Noise_log_triggered()
{
    loadLogs(false);
}

void MainWindow::on_actionLoad_Signal_log_triggered()
{
    loadLogs(true);
}

void MainWindow::loadLogs(bool signal)
{
    const QString DEFAULT_DIR_KEY("default_dir");

    QSettings settings;

    QString selectedFile = QFileDialog::getOpenFileName(
            this, QString("Select a %1 logfile").arg(signal ? "signal" : "noise"),
                settings.value(DEFAULT_DIR_KEY).toString());

    if(!selectedFile.isEmpty())
    {
        QDir currentDir;
        settings.setValue(DEFAULT_DIR_KEY,
                            currentDir.absoluteFilePath(selectedFile));
        model->loadLog(selectedFile, signal);
    }
}

void MainWindow::on_actionClear_triggered()
{
    model->clear();
}
