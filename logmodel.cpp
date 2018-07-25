#include "logmodel.h"
#include <QFile>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QDebug>
#include "logdialog.h"

const QRegularExpression logReg("^\\((\\d+).(\\d+)\\)\\s(\\w+)\\s([0-9a-f]+)#([0-9a-f]*)$", QRegularExpression::CaseInsensitiveOption);

CANMessage::CANMessage(const QString &can, quint32 id, const QByteArray &data)
{
    status = None;
    this->can = can;
    this->id = id;
    setLength(data.length());
    this->data = 0;
    for(int i = 0; i < length; i++)
    {
        this->data <<= 8;
        this->data |= (quint8)data[i];
    }
}

void CANMessage::setLength(quint8 len)
{
    if(length == len) return;
    if(len > 8) return;

    length = len;
    mask = 0;
    for(int i = 0; i < length; i++)
    {
        mask <<= 8;
        mask |= 0xff;
    }
    data = 0;
    bitmask = mask;
    chbits = 0;
    changeLog.clear();
}

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

QVariant LogModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) return QVariant();

    if((role == Qt::DisplayRole) || (role == Qt::EditRole))
    {
        const CANMessage &msg = _msgs[index.row()];
        switch(index.column())
        {
        case CAN:
            return msg.can;
        case ID:
            return QString("%1").arg(msg.id, 3, 16, QChar('0'));
        case DATA:
            return toHex(msg.data, msg.length);
        case BITMASK:
            if(role == Qt::EditRole) return QString("%1").arg(msg.bitmask, msg.length * 2, 16, QChar('0'));
            return toHex(msg.bitmask, msg.length);
        case CHBITS:
            if(role == Qt::EditRole) return QString("%1").arg(msg.chbits, msg.length * 2, 16, QChar('0'));
            return toHex(msg.chbits, msg.length);
        case CHCNT:
            return QString::number(msg.changeLog.size(), 10);
        case NOTE:
        {
            if(role == Qt::EditRole) return msg.note;
            QStringList lines = msg.note.split("\n", QString::SkipEmptyParts);
            if(lines.size() <= 1) return msg.note;
            return lines[0];
        }
        default:
            return QVariant();
        }
    }
    else if(role == Qt::ForegroundRole)
    {
        const CANMessage &msg = _msgs[index.row()];
        if(index.column() == ID)
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
        const CANMessage &msg = _msgs[index.row()];
        switch(index.column())
        {
        case CAN:
            return msg.can;
        case ID:
            return QString("%1").arg(msg.id, 3, 16, QChar('0'));
        case DATA:
            return toBin(msg.data, msg.length);
        case BITMASK:
            return toBin(msg.bitmask, msg.length);
        case CHBITS:
            return toBin(msg.chbits, msg.length);
//        case CHCNT:
//        {
//            QString res;
//            foreach(const MessageLog &item, msg.changeLog)
//            {
//                res += QString("(%1.%2) %3\n").arg(item.sec, 10, 10, QChar('0'))
//                        .arg(item.usec, 6, 10, QChar('0'))
//                        .arg(item.data, msg.length * 2, 16, QChar('0'));
//            }
//            return res;
//        }
        case NOTE:
            return msg.note;
        default:
            return QVariant();
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
                return QString("CAN");
            case ID:
                return QString("ID");
            case DATA:
                return QString("data(hex)");
            case BITMASK:
                return QString("mask(bin)");
            case CHBITS:
                return QString("changing bits(bin)");
            case CHCNT:
                return QString("ch");
            case NOTE:
                return QString("note");
            default:
                return QVariant();
            }
        }
    }
    return QVariant();
}

Qt::ItemFlags LogModel::flags(const QModelIndex &index) const
{
    if(!index.isValid()) return Qt::ItemIsEnabled;

    if((index.column() == CAN) || (index.column() == ID)
            || (index.column() == BITMASK) || (index.column() == NOTE))
    {
        return (Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
    }
    return (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

bool LogModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(!index.isValid()) return false;

    if(role == Qt::EditRole)
    {
        if(index.column() == BITMASK)
        {
            QString sval = value.toString();

            quint64 newMask= sval.toULongLong(nullptr, 16);
            quint8 len = sval.length() / 2;

            // no modification
            if((_msgs[index.row()].bitmask == newMask) && (_msgs[index.row()].length == len)) return false;

            _msgs[index.row()].setLength(len);
            _msgs[index.row()].bitmask = newMask;
            if((_msgs[index.row()].chbits & newMask) != _msgs[index.row()].chbits)
            {
                _msgs[index.row()].chbits = 0;
                _msgs[index.row()].changeLog.clear();
            }
            emit dataChanged(createIndex(index.row(), 0), createIndex(index.row(), END - 1));
            return true;
        }
        else if(index.column() == CAN)
        {
            QString newCan = value.toString();
            // no modification
            if(_msgs[index.row()].can == newCan) return false;

            quint32 id = _msgs[index.row()].id;
            for(int i = 0; i < _msgs.size(); i++)
            {
                if((i != index.row()) && (_msgs[i].can == newCan) && (_msgs[i].id == id))
                    return false;
            }
            _msgs[index.row()].can = newCan;
            _msgs[index.row()].setLength(0);
            emit dataChanged(createIndex(index.row(), 0), createIndex(index.row(), END - 1));
            return true;
        }
        else if(index.column() == ID)
        {
            quint32 newID = value.toString().toUInt(nullptr, 16);
            // no modification
            if(_msgs[index.row()].id == newID) return false;

            QString can = _msgs[index.row()].can;
            for(int i = 0; i < _msgs.size(); i++)
            {
                if((i != index.row()) && (_msgs[i].can == can) && (_msgs[i].id == newID))
                    return false;
            }
            _msgs[index.row()].id = newID;
            _msgs[index.row()].setLength(0);
            emit dataChanged(createIndex(index.row(), 0), createIndex(index.row(), END - 1));
            return true;
        }
        else if(index.column() == NOTE)
        {
            _msgs[index.row()].note = value.toString();
            emit dataChanged(index, index);
        }
    }

    return false;
}

bool LogModel::insertRows(int row, int count, const QModelIndex&)
{
    beginInsertRows(QModelIndex(), row, row + count - 1);

    for(int i = 0; i < count; i++)
    {
        _msgs.insert(row, CANMessage());
    }

    endInsertRows();
    return true;
}

bool LogModel::removeRows(int row, int count, const QModelIndex&)
{
    beginRemoveRows(QModelIndex(), row, row + count - 1);

    _msgs.remove(row, count);

    endRemoveRows();
    return true;
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
    emit dataChanged(createIndex(0, 0), createIndex(_msgs.size() - 1, END - 1));
}

void LogModel::clearAll()
{
    beginRemoveRows(QModelIndex(), 0, _msgs.size() - 1);
    _msgs.clear();
    endRemoveRows();
}

void LogModel::clearStatus()
{
    for(int i = 0; i < _msgs.size(); i++)
    {
        _msgs[i].status = CANMessage::None;
    }
    emit dataChanged(createIndex(0, 0), createIndex(_msgs.size() - 1, END - 1));
}

void LogModel::clearMasks()
{
    for(int i = 0; i < _msgs.size(); i++)
    {
        _msgs[i].bitmask = _msgs[i].mask;
    }
    emit dataChanged(createIndex(0, 0), createIndex(_msgs.size() - 1, END - 1));
}

void LogModel::clearChanges()
{
    for(int i = 0; i < _msgs.size(); i++)
    {
        _msgs[i].chbits = 0;
        _msgs[i].changeLog.clear();
    }
    emit dataChanged(createIndex(0, 0), createIndex(_msgs.size() - 1, END - 1));
}

void LogModel::onDoubleClicked(const QModelIndex &index)
{
    if(!index.isValid()) return;

    if(index.column() == CHCNT)
    {
        LogDialog dlg(NULL, &_msgs[index.row()]);
        dlg.exec();
        emit dataChanged(createIndex(index.row(), 0), createIndex(index.row(), END - 1));
    }
}

void LogModel::procMessage(quint64 sec, quint32 usec, const QString &can, quint32 id, const QByteArray &data, bool update)
{
    bool found = false;
    for(int i = 0; i < _msgs.size(); i++)
    {
        const CANMessage &msg = _msgs[i];
        if((msg.can.isEmpty() || (msg.can == can)) && (msg.id == id))
        {
            if(msg.can.isEmpty()) _msgs[i].can = can;
            if(msg.length != data.length())
            {
                _msgs[i].setLength(data.length());
            }

            quint64 bdata = 0;
            for(int i = 0; i < data.length(); i++)
            {
                bdata <<= 8;
                bdata |= (quint8)data[i];
            }

            // match
            found = true;
            if(msg.data != bdata)
            {
                if(_logChange)
                {
                    quint64 change = ((msg.data ^ bdata) & msg.bitmask);
                    if(change > 0)
                    {
                        _msgs[i].chbits |= change;
                        _msgs[i].status = CANMessage::Changes;
                        MessageLog chlog(sec, usec, bdata);
                        _msgs[i].changeLog.append(chlog);
                    }
                }
                else if(_genMask)
                {
                    // noise log
                    _msgs[i].bitmask &= ~(msg.data ^ bdata);
                }
                _msgs[i].data = bdata;
                if(update) emit dataChanged(createIndex(i, 0), createIndex(i, END - 1));
            }
            break;
        }
    }
    if((!found) && (!_filtering))
    {
        CANMessage msg(can, id, data);
        msg.status = CANMessage::New;
        beginInsertRows(QModelIndex(), _msgs.size(), _msgs.size());
        _msgs.append(msg);
        endInsertRows();
    }
}

QString toHex(quint64 value, quint8 length)
{
    QString res = QString("%1").arg(value, length * 2, 16, QChar('0'));
    for(int i = (length - 1); i > 0; i--)
    {
        res.insert(i * 2, ' ');
    }
    return res;
}

QString toBin(quint64 value, quint8 length)
{
    QString res = QString("%1").arg(value, length * 8, 2, QChar('0'));
    for(int i = (length - 1); i > 0; i--)
    {
        res.insert(i * 8, ' ');
    }
    return res;
}
