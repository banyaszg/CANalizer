#ifndef LOGMODEL_H
#define LOGMODEL_H

#include <QAbstractTableModel>
#include <QLinkedList>

class MessageLog
{
public:
    MessageLog() {}
    MessageLog(quint64 sec, quint32 usec, const QByteArray &data)
    {
        this->sec = sec;
        this->usec = usec;
        this->data = data;
    }

    quint64 sec;
    quint32 usec;
    QByteArray data;
};

class CANMessage
{
public:
    enum Status { None, New, Changes };

    CANMessage() { status = None; }
    CANMessage(const QString &can, quint16 id, const QByteArray &data)
    {
        this->can = can;
        this->id = id;
        this->data = data;
        status = None;
        bitmask.fill(0xff, data.size());
        chbits.fill(0, data.size());
    }

    QString can;
    quint16 id;
    Status status;
    QByteArray data;
    QByteArray bitmask;
    QByteArray chbits;
    QLinkedList<MessageLog> changeLog;
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

    void loadLog(QString fname);
    void clearAll();
    void clearStatus();

    void setLogChange(bool val) { _logChange = val; if(val) { clearStatus(); _genMask = false; } }
    bool logChange() { return _logChange; }
    void setGenMask(bool val) { _genMask = val; if(val) { clearStatus(); _logChange = false; } }
    bool genMask() { return _genMask; }
    void setFiltering(bool val) { _filtering = val; }
    bool filtering() { return _filtering; }
    void procMessage(quint64 sec, quint32 usec, const QString &can, quint16 id, const QByteArray &data, bool update = true);

signals:
    void progressValue(int);

public slots:
    void onDoubleClicked(const QModelIndex &index);

protected:
    void applyMask(int ix, bool update = true);

protected:
    bool _logChange = false;
    bool _genMask = false;
    bool _filtering = false;
    QVector<CANMessage> _msgs;
};

#endif // LOGMODEL_H
