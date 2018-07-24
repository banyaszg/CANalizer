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
