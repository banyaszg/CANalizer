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

    connect(ui->tableView, &QTableView::doubleClicked, model, &LogModel::onDoubleClicked);

    progressBar = new QProgressBar(this);
    progressBar->setMaximumHeight(16);
    progressBar->setMaximumWidth(200);
    progressBar->setTextVisible(false);
    progressBar->setMinimum(0);
    progressBar->setMaximum(100);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionClear_triggered()
{
    model->clear();
}

void MainWindow::on_actionLoad_triggered()
{
    const QString DEFAULT_DIR_KEY("default_dir");

    QSettings settings;

    QString selectedFile = QFileDialog::getOpenFileName(
            this, QString("Select a logfile"),
                settings.value(DEFAULT_DIR_KEY).toString());

    if(!selectedFile.isEmpty())
    {
        QDir currentDir;
        settings.setValue(DEFAULT_DIR_KEY,
                            currentDir.absoluteFilePath(selectedFile));

        statusBar()->addPermanentWidget(progressBar, 0);
        statusBar()->showMessage(QString("Loading"));
        connect(model, &LogModel::progressValue, progressBar, &QProgressBar::setValue);

        model->loadLog(selectedFile);

        disconnect(progressBar);
        statusBar()->clearMessage();
        statusBar()->removeWidget(progressBar);
    }
}

void MainWindow::on_actionSignal_toggled(bool arg1)
{
    model->setSignalState(arg1);
}
