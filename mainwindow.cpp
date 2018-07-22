#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QSettings>
#include <QCanBus>
#include <QCanBusFrame>
#include "capturedialog.h"
#include <QDebug>

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

    liveUpdate = false;
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

void MainWindow::on_actionExit_triggered()
{
    qApp->exit();
}

void MainWindow::on_actionStartCapture_triggered()
{
    const QString DEFAULT_CANPLUGIN_KEY("can_plugin");
    const QString DEFAULT_CANIF_KEY("can_interface");
    const QString DEFAULT_LIVEUPDATE_KEY("live_update");

    QSettings settings;
    CaptureDialog dlg(settings.value(DEFAULT_CANPLUGIN_KEY).toString(),
                      settings.value(DEFAULT_CANIF_KEY).toString(),
                      settings.value(DEFAULT_LIVEUPDATE_KEY).toBool());
    if(dlg.exec() == QDialog::Accepted)
    {
        settings.setValue(DEFAULT_CANPLUGIN_KEY, dlg.plugin());
        settings.setValue(DEFAULT_CANIF_KEY, dlg.interface());
        settings.setValue(DEFAULT_LIVEUPDATE_KEY, dlg.live());
        canInterface = dlg.interface();
        liveUpdate = dlg.live();

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

    if(!liveUpdate)
    {
        model->applyMaskAll();
    }

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
        model->procMessage(QString::number(frame.timeStamp().seconds(), 10),
                           QString::number(frame.timeStamp().microSeconds(), 10),
                           canInterface, QString::number(frame.frameId(), 16),
                           frame.payload(), liveUpdate);
    }
}
