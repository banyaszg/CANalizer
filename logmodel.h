#ifndef LOGMODEL_H
#define LOGMODEL_H

#include <QAbstractTableModel>
#include <QStyledItemDelegate>

class MessageLog
{
public:
    MessageLog() {}
    MessageLog(const QString &sec, const QString &usec, const QByteArray &data)
    {
        this->sec = sec;
        this->usec = usec;
        this->data = data;
    }

    QString sec;
    QString usec;
    QByteArray data;
};

class CANMessage
{
public:
    enum Status { None, New, Changes };

    CANMessage() { status = None; }
    CANMessage(const QString &can, const QString &id, const QByteArray &data)
    {
        this->can = can;
        this->id = id;
        this->data = data;
        status = None;
        bitmask.fill(0xff, data.size());
        bitch.fill(0, data.size());
    }

    QString can;
    QString id;
    Status status;
    QByteArray data;
    QByteArray bitmask;
    QByteArray bitch;
    QList<MessageLog> log;
};

class LogModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    LogModel(QObject *parent);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void loadLog(QString fname, bool signal);
    void clear();

protected:
    QList<CANMessage> _msgs;
};

#endif // LOGMODEL_H