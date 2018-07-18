#include "logmodel.h"
#include <QFile>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QDebug>

const QRegularExpression logReg("^\\((\\d+).(\\d+)\\)\\s(\\w+)\\s([0-9a-f]+)#([0-9a-f]*)$", QRegularExpression::CaseInsensitiveOption);
const int columns = 6;

LogModel::LogModel(QObject *parent)
    :QAbstractTableModel(parent)
{
}

int LogModel::rowCount(const QModelIndex&) const
{
    return _msgs.size();
}

int LogModel::columnCount(const QModelIndex&) const
{
    return columns;
}

QString toBin(const QByteArray &ba)
{
    QString res;
    res.fill('0', ba.size() * 9);
    int cnt = 0;
    for(int i = 0; i < ba.size(); i++)
    {
        const char b = ba[i];
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
        case 0:
            return msg.can;
        case 1:
            return msg.id;
        case 2:
            return QString(msg.data.toHex());
        case 3:
            return toBin(msg.bitmask);
        case 4:
            return toBin(msg.bitch);
        case 5:
            return QString::number(msg.log.size(), 10);
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
            for(int i = 0; i < msg.log.size(); i++)
            {
                res += QString("(%1.%2) %3\n").arg(msg.log[i].sec).arg(msg.log[i].usec).arg(QString(msg.log[i].data.toHex()));
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
            case 0:
                return QString("can");
            case 1:
                return QString("id");
            case 2:
                return QString("data(hex)");
            case 3:
                return QString("mask(bin)");
            case 4:
                return QString("bit changes(bin)");
            case 5:
                return QString("changes");
            default:
                return QVariant();
            }
        }
    }
    return QVariant();
}

void LogModel::loadLog(QString fname, bool signal)
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

            bool found = false;
            for(int i = 0; i < _msgs.size(); i++)
            {
                const CANMessage msg = _msgs[i];
                if((msg.can == can) && (msg.id == id))
                {
                    // match
                    found = true;
                    if(signal)
                    {
                        // signal log
                        quint8 bytech;
                        bool changed = false;
                        for(int i2 = 0; i2 < msg.data.size(); i2++)
                        {
                            bytech = (((char)msg.data[i2] ^ (char)data[i2]) & (char)msg.bitmask[i2]);
                            if(bytech)
                            {
                                changed = true;
                                _msgs[i].bitch[i2] = (char)msg.bitch[i2] | bytech;
                            }
                        }
                        if(changed)
                        {
                            _msgs[i].status = CANMessage::Changes;
                            MessageLog log(sec, usec, data);
                            _msgs[i].log.append(log);
                        }
                    }
                    else
                    {
                        // noise log
                        for(int i2 = 0; i2 < msg.data.size(); i2++)
                        {
                            _msgs[i].bitmask[i2] = (char)msg.bitmask[i2] & ~((char)msg.data[i2] ^ (char)data[i2]);
                        }
                    }
                    _msgs[i].data = data;
                    break;
                }
                emit dataChanged(createIndex(i, 0), createIndex(i, columns));
            }
            if(!found)
            {
                CANMessage msg(can, id, data);
                if(signal) msg.status = CANMessage::New;
                _msgs.append(msg);
                emit layoutChanged();
            }
        }
    }
}

void LogModel::clear()
{
    _msgs.clear();
    emit layoutChanged();
}
