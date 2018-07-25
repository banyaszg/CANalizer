#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QSettings>
#include <QCanBus>
#include <QCanBusFrame>
#include "capturedialog.h"
#include <QShortcut>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    model = new LogModel(this);
    ui->setupUi(this);

    proxymodel = new QSortFilterProxyModel(this);
    proxymodel->setSourceModel(model);
    ui->tableView->setModel(proxymodel);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableView->sortByColumn(1, Qt::AscendingOrder);

    connect(ui->tableView, &QTableView::doubleClicked, this, &MainWindow::onDoubleClicked);

    progressBar = new QProgressBar(this);
    progressBar->setMaximumHeight(16);
    progressBar->setMaximumWidth(200);
    progressBar->setTextVisible(false);
    progressBar->setMinimum(0);
    progressBar->setMaximum(100);

    QShortcut* del = new QShortcut(QKeySequence(Qt::Key_Delete), ui->tableView);
    connect(del, SIGNAL(activated()), this, SLOT(on_actionRemoveIDs_triggered()));
    QShortcut* ins = new QShortcut(QKeySequence(Qt::Key_Insert), ui->tableView);
    connect(ins, SIGNAL(activated()), this, SLOT(on_actionAddID_triggered()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionClearAll_triggered()
{
    model->clearAll();
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
        settings.setValue(DEFAULT_DIR_KEY,
                            QFileInfo(selectedFile).absolutePath());

        statusBar()->addPermanentWidget(progressBar, 0);
        statusBar()->showMessage(QString("Loading"));
        connect(model, &LogModel::progressValue, progressBar, &QProgressBar::setValue);

        model->loadLog(selectedFile);

        disconnect(progressBar);
        statusBar()->clearMessage();
        statusBar()->removeWidget(progressBar);
    }
}

void MainWindow::on_actionChanges_toggled(bool arg1)
{
    if(arg1) ui->actionGenMask->setChecked(false);
    model->setLogChange(arg1);
}

void MainWindow::on_actionExit_triggered()
{
    qApp->exit();
}

void MainWindow::on_actionStartCapture_triggered()
{
    const QString DEFAULT_CANPLUGIN_KEY("can_plugin");
    const QString DEFAULT_CANIF_KEY("can_interface");

    QSettings settings;
    CaptureDialog dlg(settings.value(DEFAULT_CANPLUGIN_KEY).toString(),
                      settings.value(DEFAULT_CANIF_KEY).toString());
    if(dlg.exec() == QDialog::Accepted)
    {
        settings.setValue(DEFAULT_CANPLUGIN_KEY, dlg.plugin());
        settings.setValue(DEFAULT_CANIF_KEY, dlg.interface());
        canInterface = dlg.interface();

        QString errorString;
        canDevice = QCanBus::instance()->createDevice(dlg.plugin(), dlg.interface(), &errorString);
        if(!canDevice)
        {
            statusBar()->showMessage(tr("Error creating device '%1': '%2'")
                              .arg(dlg.plugin()).arg(errorString));
            canInterface.clear();
            return;
        }

        connect(canDevice, &QCanBusDevice::errorOccurred, this, &MainWindow::errorOccurred);
        connect(canDevice, &QCanBusDevice::framesReceived, this, &MainWindow::framesReceived);

        if(!canDevice->connectDevice()) {
            statusBar()->showMessage(tr("Connection error: %1").arg(canDevice->errorString()));
            delete canDevice;
            canDevice = nullptr;
            canInterface.clear();
            return;
        }
        ui->actionLoad->setEnabled(false);
        ui->actionStartCapture->setEnabled(false);
        ui->actionStopCapture->setEnabled(true);

        statusBar()->showMessage(tr("Connected to %1").arg(dlg.interface()));
    }
}

void MainWindow::on_actionStopCapture_triggered()
{
    if(!canDevice) return;

    canDevice->disconnectDevice();
    delete canDevice;
    canDevice = nullptr;

    ui->actionLoad->setEnabled(true);
    ui->actionStartCapture->setEnabled(true);
    ui->actionStopCapture->setEnabled(false);

    statusBar()->showMessage(tr("Disconnected"));
}

void MainWindow::errorOccurred(QCanBusDevice::CanBusError error) const
{
    switch (error) {
    case QCanBusDevice::ReadError:
    case QCanBusDevice::WriteError:
    case QCanBusDevice::ConnectionError:
    case QCanBusDevice::ConfigurationError:
    case QCanBusDevice::UnknownError:
        qWarning() << canDevice->errorString();
    default:
        break;
    }
}

void MainWindow::framesReceived()
{
    if(!canDevice) return;

    while(canDevice->framesAvailable())
    {
        const QCanBusFrame frame = canDevice->readFrame();
        model->procMessage(frame.timeStamp().seconds(), frame.timeStamp().microSeconds(),
                    canInterface, frame.frameId(),
                    frame.payload());
    }
}

void MainWindow::on_actionGenMask_toggled(bool arg1)
{
    if(arg1) ui->actionChanges->setChecked(false);
    model->setGenMask(arg1);
}

void MainWindow::on_actionClearStatus_triggered()
{
    model->clearStatus();
}

void MainWindow::on_actionClearMasks_triggered()
{
    model->clearMasks();
}

void MainWindow::on_actionClearChanges_triggered()
{
    model->clearChanges();
}

void MainWindow::on_actionFiltering_toggled(bool arg1)
{
    model->setFiltering(arg1);
}

void MainWindow::on_actionAddID_triggered()
{
    proxymodel->insertRow(proxymodel->rowCount());
}

void MainWindow::on_actionRemoveIDs_triggered()
{
    QModelIndexList indexes =  ui->tableView->selectionModel()->selectedRows();
    int countRow = indexes.count();

    for(int i = (countRow - 1); i >= 0; i--)
    {
        proxymodel->removeRow(indexes[i].row(), QModelIndex());
    }
}

void MainWindow::onDoubleClicked(const QModelIndex &index)
{
    model->onDoubleClicked(proxymodel->mapToSource(index));
}

void MainWindow::on_actionRemoveStatic_triggered()
{
    model->removeZeros();
}

void MainWindow::on_actionSaveFrames_triggered()
{
    const QString DEFAULT_DIR_KEY("default_dir");

    QSettings settings;

    QString selectedFile = QFileDialog::getSaveFileName(
            this, QString("Select am outputfile"),
                settings.value(DEFAULT_DIR_KEY).toString(),
                "Text files (*.txt)");

    if(!selectedFile.isEmpty())
    {
        settings.setValue(DEFAULT_DIR_KEY,
                            QFileInfo(selectedFile).absolutePath());

        QFile file(selectedFile);
        if(!file.open(QIODevice::WriteOnly | QFile::Truncate))
            return;
        QTextStream out(&file);
        model->saveAllIDs(out);
    }
}
