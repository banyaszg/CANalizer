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
    res.fill('0', ba.size() * 9 - 1);
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
        if(i < (ba.size() - 1)) res[cnt++] = ' ';
    }
    return res;
}

QVariant LogModel::data(const QModelIndex &index, int role) const
{
    if(role == Qt::DisplayRole)
    {
        const CANMessage &msg = _msgs[index.row()];
        switch(index.column())
        {
        case CAN:
            return msg.can;
        case ID:
            return QString("%1").arg(msg.id, 3, 16, QChar('0'));
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
        const CANMessage &msg = _msgs[index.row()];
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
//    else if(role == Qt::ToolTipRole)
//    {
//        if(index.column() == CHCNT)
//        {
//            const CANMessage msg = _msgs[index.row()];
//            QString res;
//            foreach(const MessageLog &item, msg.changeLog)
//            {
//                res += QString("(%1.%2) %3\n").arg(item.sec).arg(item.usec).arg(QString(item.data.toHex()));
//            }
//            return res;
//        }
//    }
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
            QString ssec = match.captured(1);
            QString susec = match.captured(2);
            QString can = match.captured(3);
            QString sid = match.captured(4);
            QString sdata = match.captured(5);

            QByteArray data = QByteArray::fromHex(sdata.toUtf8());

            procMessage(ssec.toULong(), susec.toUInt(), can, sid.toUInt(nullptr, 16), data, false);
        }
        int percent = (file.pos() * 99 / file.size()) + 1;
        emit progressValue(percent);
    }
    emit layoutChanged();
//    emit dataChanged(createIndex(0, 0), createIndex(_msgs.size() - 1, END - 1));
}

void LogModel::clearAll()
{
    _msgs.clear();
    emit layoutChanged();
}

void LogModel::clearStatus()
{
    for(int i = 0; i < _msgs.size(); i++)
    {
        _msgs[i].status = CANMessage::None;
    }
    emit dataChanged(createIndex(0, 0), createIndex(_msgs.size() - 1, END - 1));
}

void LogModel::onDoubleClicked(const QModelIndex &index)
{
    if(index.column() == CHCNT)
    {
        const CANMessage &msg = _msgs[index.row()];

        LogDialog dlg(NULL, QString("%1:%2").arg(msg.can).arg(msg.id, 3, 16, QChar('0')), msg.changeLog, msg.chbits);
        dlg.exec();
    }

}

void LogModel::procMessage(quint64 sec, quint32 usec, const QString &can, quint16 id, const QByteArray &data, bool update)
{
    bool found = false;
    for(int i = 0; i < _msgs.size(); i++)
    {
        const CANMessage &msg = _msgs[i];
        if((msg.can.isEmpty() || (msg.can == can)) && (msg.id == id))
        {
            // match
            found = true;
            if(msg.data != data)
            {
                if(_logChange)
                {
                    quint8 bytech;
                    bool changed = false;
                    for(int i2 = 0; i2 < msg.data.size(); i2++)
                    {
                        bytech = (((quint8)msg.data[i2] ^ (quint8)data[i2]) & (quint8)msg.bitmask[i2]);
                        if(bytech)
                        {
                            changed = true;
                            _msgs[i].chbits[i2] = (quint8)msg.chbits[i2] | bytech;
                        }
                    }
                    if(changed)
                    {
                        _msgs[i].status = CANMessage::Changes;
                        MessageLog chlog(sec, usec, data);
                        _msgs[i].changeLog.append(chlog);
                    }
                }
                else if(_genMask)
                {
                    // noise log
                    QByteArray newmask = msg.bitmask;
                    for(int i2 = 0; i2 < msg.bitmask.size(); i2++)
                    {
                        newmask[i2] = (quint8)msg.bitmask[i2] & ~((quint8)msg.data[i2] ^ (quint8)data[i2]);
                    }
                    if(newmask != _msgs[i].bitmask)
                    {
                        _msgs[i].bitmask = newmask;
                    }
                }
                _msgs[i].data = data;
                if(update) emit dataChanged(createIndex(i, 0), createIndex(i, END - 1));
            }
            break;
        }
    }
    if((!found) && (!_filtering))
    {
        CANMessage msg(can, id, data);
        msg.status = CANMessage::New;
        _msgs.append(msg);
        if(update) emit layoutChanged();
    }
}
