#include "tablemodel.h"

// Save table

QDataStream& operator<<(QDataStream& ds, const Cell& cell) {
    ds << cell.value << cell.cellColor << cell.textColor << cell.fontFlags << cell.styled;
    return ds;
}

QDataStream& operator<<(QDataStream& ds, const EditOperation& op) {
    std::visit([&](auto&& o) {
        using T = std::decay_t<decltype(o)>;
        if constexpr (std::is_same_v<T, CellEdit>) {
            ds << (qint8)0;
            ds << o.cell << o.before << o.after;
        } else if constexpr (std::is_same_v<T, RangeEdit>) {
            ds << (qint8)1;
            ds << o.topLeft << o.bottomRight << o.before << o.after;
        }
    }, op);
    return ds;
}

bool TableModel::saveToFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;

    QDataStream ds(&file);
    ds.setVersion(QDataStream::Qt_6_0);

    // Header
    ds << (quint32)0x54424C58; // magic "TBLX"
    ds << (quint32)1;          // version

    // Edit stack
    ds << editStack;

    return ds.status() == QDataStream::Ok;
}

// Load table

QDataStream& operator>>(QDataStream& ds, Cell& cell) {
    ds >> cell.value >> cell.cellColor >> cell.textColor >> cell.fontFlags >> cell.styled;
    return ds;
}

QDataStream& operator>>(QDataStream& ds, EditOperation& op) {
    qint8 tag;
    ds >> tag;
    if (tag == 0) {
        CellEdit o;
        ds >> o.cell >> o.before >> o.after;
        op = o;
    } else {
        RangeEdit o;
        ds >> o.topLeft >> o.bottomRight >> o.before >> o.after;
        op = o;
    }
    return ds;
}

bool TableModel::loadFromFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QDataStream ds(&file);
    ds.setVersion(QDataStream::Qt_6_0);

    // Validate header
    quint32 magic, version;
    ds >> magic >> version;
    if (magic != 0x54424C58) return false;
    if (version != 1) return false;

    // Load stack
    QVector<EditOperation> loadedStack;
    ds >> loadedStack;
    if (ds.status() != QDataStream::Ok) return false;

    // Reset state
    data_.clear();
    editStack.clear();
    redoStack.clear();

    // Replay
    for (const EditOperation& op : loadedStack) {
        std::visit([&](auto&& o) {
            using T = std::decay_t<decltype(o)>;
            if constexpr (std::is_same_v<T, CellEdit>) {
                applyCell(o.cell, o. after);
            } else if constexpr (std::is_same_v<T, RangeEdit>) {
                applyRange(o.topLeft, o.bottomRight, o.after);
            }
        }, op);
    }

    editStack = loadedStack;
    return true;
}
