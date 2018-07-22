#include "capturedialog.h"
#include "ui_capturedialog.h"
#include <QtSerialBus/QCanBus>

CaptureDialog::CaptureDialog(const QString &plugin, const QString &interface, bool live, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CaptureDialog)
{
    ui->setupUi(this);
    ui->selectPlugin->addItems(QCanBus::instance()->plugins());
    ui->selectPlugin->setCurrentText(plugin);
    ui->selectInterface->setCurrentText(interface);
    ui->checkLive->setChecked(live);
}

CaptureDialog::~CaptureDialog()
{
    delete ui;
}

QString CaptureDialog::plugin()
{
    return ui->selectPlugin->currentText();
}

QString CaptureDialog::interface()
{
    return ui->selectInterface->currentText();
}

bool CaptureDialog::live()
{
    return (ui->checkLive->checkState() == Qt::Checked);
}

void CaptureDialog::on_selectPlugin_currentTextChanged(const QString &arg1)
{
    ui->selectInterface->clear();
    QList<QCanBusDeviceInfo> ifs = QCanBus::instance()->availableDevices(arg1);
    for(const QCanBusDeviceInfo &info : qAsConst(ifs))
        ui->selectInterface->addItem(info.name());
}

void CaptureDialog::on_selectInterface_currentTextChanged(const QString &currentText)
{
    ui->btnOk->setEnabled(!currentText.isEmpty());
}

void CaptureDialog::on_btnOk_clicked()
{
    accept();
}

void CaptureDialog::on_btnCancel_clicked()
{
    reject();
}
