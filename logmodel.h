#ifndef LOGMODEL_H
#define LOGMODEL_H

#include <QAbstractTableModel>
#include <QLinkedList>
#include <QTextStream>

class MessageLog
{
public:
    MessageLog() {}
    MessageLog(quint64 sec, quint32 usec, quint64 data)
    {
        this->sec = sec;
        this->usec = usec;
        this->data = data;
    }

    quint64 sec = 0;
    quint32 usec = 0;
    quint64 data = 0;
    QString note;
};

class CANMessage
{
public:
    enum Status { None, New, Changes };

    CANMessage() { status = None; }
    CANMessage(const QString &can, quint32 id, const QByteArray &data);

    void setLength(quint8 len);
    void save(QTextStream &out);

    QString can;
    quint32 id = 0;
    Status status;
    quint64 data = 0;
    quint8 length = 0;
    quint64 mask = 0;
    quint64 bitmask = 0;
    quint64 chbits = 0;
    QLinkedList<MessageLog> changeLog;
    QString note;
};

class LogModel : public QAbstractTableModel
{
    Q_OBJECT

    enum Columns { CAN = 0, ID = 1, DATA = 2, BITMASK = 3, CHBITS = 4, CHCNT = 5, NOTE = 6, END = 7 };

public:
    LogModel(QObject *parent);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    void loadLog(QString fname);
    void clearAll();
    void clearStatus();
    void clearMasks();
    void clearChanges();
    void removeZeros();

    void setLogChange(bool val) { _logChange = val; if(val) { clearStatus(); _genMask = false; } }
    bool logChange() { return _logChange; }
    void setGenMask(bool val) { _genMask = val; if(val) { clearStatus(); _logChange = false; } }
    bool genMask() { return _genMask; }
    void setFiltering(bool val) { _filtering = val; }
    bool filtering() { return _filtering; }
    void procMessage(quint64 sec, quint32 usec, const QString &can, quint32 id, const QByteArray &data, bool update = true);

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

QString toHex(quint64 value, quint8 length);
QString toBin(quint64 value, quint8 length);

#endif // LOGMODEL_H
