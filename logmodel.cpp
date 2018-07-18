#include "logmodel.h"
#include <QFile>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QDebug>
#include "logdialog.h"

const QRegularExpression logReg("^\\((\\d+).(\\d+)\\)\\s(\\w+)\\s([0-9a-f]+)#([0-9a-f]*)$", QRegularExpression::CaseInsensitiveOption);
enum Columns { CAN = 0, ID = 1, DATA = 2, MASK = 3, CHBITS = 4, CHCNT = 5, END = 6 };

LogModel::LogModel(QObject *parent)
    :QAbstractTableModel(parent)
{
    _signal = false;
}

int LogModel::rowCount(const QModelIndex&) const
{
    return _msgs.size();
}

int LogModel::columnCount(const QModelIndex&) const
{
    return END;
}

QString toBin(const QByteArray &ba)
{
    QString res;
    res.fill('0', ba.size() * 9);
    int cnt = 0;
    for(int i = 0; i < ba.size(); i++)
    {
        const quint8 b = ba[i];
        for(int i2 = 7; i2 >= 0; i2--)
        {
            if(b & (1 << i2))
            {
                res[cnt] = '1';
            }
            cnt++;
        }
        res[cnt++] = ' ';
    }
    return res;
}

QVariant LogModel::data(const QModelIndex &index, int role) const
{
    if(role == Qt::DisplayRole)
    {
        const CANMessage msg = _msgs[index.row()];
        switch(index.column())
        {
        case CAN:
            return msg.can;
        case ID:
            return msg.id;
        case DATA:
            return QString(msg.data.toHex());
        case MASK:
            return toBin(msg.bitmask);
        case CHBITS:
            return toBin(msg.chbits);
        case CHCNT:
            return QString::number(msg.changeLog.size(), 10);
        default:
            return QVariant();
        }
    }
    else if(role == Qt::ForegroundRole)
    {
        const CANMessage msg = _msgs[index.row()];
        if(index.column() == 1)
        {
            switch(msg.status)
            {
            case CANMessage::New:
                return QBrush(Qt::red);
            case CANMessage::Changes:
                return QBrush(Qt::blue);
            default:
                return QBrush(Qt::black);
            }
        }
        return QBrush(Qt::black);
    }
    else if(role == Qt::ToolTipRole)
    {
        if(index.column() == 5)
        {
            const CANMessage msg = _msgs[index.row()];
            QString res;
            for(int i = 0; i < msg.changeLog.size(); i++)
            {
                res += QString("(%1.%2) %3").arg(msg.changeLog[i].sec).arg(msg.changeLog[i].usec).arg(QString(msg.changeLog[i].data.toHex()));
                if(i < (msg.changeLog.size() - 1)) res += "\n";
            }
            return res;
        }
    }
    return QVariant();
}

QVariant LogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        if(role == Qt::DisplayRole)
        {
            switch(section)
            {
            case CAN:
                return QString("can");
            case ID:
                return QString("id");
            case DATA:
                return QString("data(hex)");
            case MASK:
                return QString("mask(bin)");
            case CHBITS:
                return QString("changing bits(bin)");
            case CHCNT:
                return QString("ch");
            default:
                return QVariant();
            }
        }
    }
    return QVariant();
}

void LogModel::loadLog(QString fname)
{
    QFile file(fname);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    while(!file.atEnd())
    {
        QByteArray line = file.readLine();
        QRegularExpressionMatch match = logReg.match(line);
        if(match.hasMatch())
        {
            QString sec = match.captured(1);
            QString usec = match.captured(2);
            QString can = match.captured(3);
            QString id = match.captured(4);
            QString sdata = match.captured(5);

            QByteArray data = QByteArray::fromHex(sdata.toUtf8());

            procMessage(sec, usec, can, id, data, false);
        }
        int percent = (file.pos() * 99 / file.size()) + 1;
        emit progressValue(percent);
    }
    emit layoutChanged();
    for(int i = 0; i < _msgs.size(); i++)
    {
        applyMask(i, false);
    }
    emit dataChanged(createIndex(0, 0), createIndex(_msgs.size() - 1, END - 1));
}

void LogModel::clear()
{
    _msgs.clear();
    emit layoutChanged();
}

void LogModel::onDoubleClicked(const QModelIndex &index)
{
    if(index.column() == CHCNT)
    {
        const CANMessage msg = _msgs[index.row()];

        LogDialog dlg(NULL, QString("%1:%2").arg(msg.can).arg(msg.id), msg.changeLog, msg.chbits);
        dlg.exec();
    }

}

void LogModel::procMessage(const QString &sec, const QString &usec, const QString &can, const QString &id, const QByteArray &data, bool update)
{
    bool found = false;
    for(int i = 0; i < _msgs.size(); i++)
    {
        const CANMessage msg = _msgs[i];
        if((msg.can == can) && (msg.id == id))
        {
            // match
            found = true;
            if(msg.status == CANMessage::New) _msgs[i].status = CANMessage::None;

            if(_signal)
            {
                // signal log
                if(msg.log.empty() || (msg.log.last().data != data))
                {
                    MessageLog log(sec, usec, data);
                    _msgs[i].log.append(log);
                }
            }
            else
            {
                // noise log
                QByteArray newmask = _msgs[i].bitmask;
                for(int i2 = 0; i2 < _msgs[i].bitmask.size(); i2++)
                {
                    newmask[i2] = (quint8)_msgs[i].bitmask[i2] & ~((quint8)_msgs[i].data[i2] ^ (quint8)data[i2]);
                }
                if(newmask != _msgs[i].bitmask)
                {
                    _msgs[i].bitmask = newmask;
                    if(update)
                    {
                        emit dataChanged(createIndex(i, MASK), createIndex(i, MASK));
                        applyMask(i, update);
                    }
                }
            }
            if(_msgs[i].data != data)
            {
                _msgs[i].data = data;
                if(update) emit dataChanged(createIndex(i, DATA), createIndex(i, DATA));
            }
            break;
        }
    }
    if(!found)
    {
        CANMessage msg(can, id, data);
        if(_signal)
        {
            msg.status = CANMessage::New;
            MessageLog log(sec, usec, data);
            msg.log.append(log);
        }
        _msgs.append(msg);
        if(update) emit layoutChanged();
    }
}

void LogModel::applyMask(int ix, bool update)
{
    const CANMessage msg = _msgs[ix];

    _msgs[ix].chbits.fill(0, msg.data.size());
    _msgs[ix].changeLog.clear();
    _msgs[ix].status = CANMessage::None;

    if(msg.log.empty()) return;

    QByteArray lastData = msg.log[0].data;
    for(int i = 1; i < msg.log.size(); i++)
    {
        const MessageLog log = msg.log[i];

        quint8 bytech;
        bool changed = false;
        for(int i2 = 0; i2 < log.data.size(); i2++)
        {
            bytech = (((quint8)lastData[i2] ^ (quint8)log.data[i2]) & (quint8)msg.bitmask[i2]);
            if(bytech)
            {
                changed = true;
                _msgs[ix].chbits[i2] = (quint8)_msgs[ix].chbits[i2] | bytech;
            }
        }
        if(changed)
        {
            _msgs[ix].status = CANMessage::Changes;
            MessageLog chlog(log.sec, log.usec, log.data);
            _msgs[ix].changeLog.append(chlog);
        }
        lastData = log.data;
    }
    if(update) emit dataChanged(createIndex(ix, CHBITS), createIndex(ix, CHCNT));
}
