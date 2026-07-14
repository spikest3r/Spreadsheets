#include "tablemodel.h"

// Save table

QDataStream& operator<<(QDataStream& ds, const Cell& cell) {
    ds << cell.value << cell.cellColor.rgba() << cell.textColor.rgba() << cell.fontFlags << cell.styled;
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

QDataStream& operator<<(QDataStream& ds, const Script& script) {
    ds << script.name << script.code;
    return ds;
}

bool TableModel::saveToFile(const QString& path, const QVector<Script>& scripts) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    QDataStream ds(&file);
    ds.setVersion(QDataStream::Qt_6_0);
    // Header
    ds << (quint32)0x54424C58; // magic "TBLX"
    ds << (quint32)2;          // version
    // Edit stack
    ds << editStack;
    // Scripts (v2+)
    ds << scripts;
    return ds.status() == QDataStream::Ok;
}

// Load table

QDataStream& operator>>(QDataStream& ds, Cell& cell) {
    QRgb cellRgba, textRgba;
    ds >> cell.value >> cellRgba >> textRgba >> cell.fontFlags >> cell.styled;
    cell.cellColor = QColor::fromRgba(cellRgba);
    cell.textColor = QColor::fromRgba(textRgba);
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

QDataStream& operator>>(QDataStream& ds, Script& script) {
    ds >> script.name >> script.code;
    return ds;
}

bool TableModel::loadFromFile(const QString& path, QVector<Script>& scripts) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;
    QDataStream ds(&file);
    ds.setVersion(QDataStream::Qt_6_0);
    // Validate header
    quint32 magic, version;
    ds >> magic >> version;
    if (magic != 0x54424C58) return false;
    if (version != 1 && version != 2) return false;

    // Load stack
    QVector<EditOperation> loadedStack;
    ds >> loadedStack;
    if (ds.status() != QDataStream::Ok) return false;

    // Load scripts — only present in v2+
    QVector<Script> loadedScripts;
    if (version >= 2) {
        ds >> loadedScripts;
        if (ds.status() != QDataStream::Ok) return false;
    }

    // Reset state
    data_.clear();
    editStack.clear();
    redoStack.clear();

    // Replay
    for (const EditOperation& op : loadedStack) {
        std::visit([&](auto&& o) {
            using T = std::decay_t<decltype(o)>;
            if constexpr (std::is_same_v<T, CellEdit>) {
                applyCell(o.cell, o.after);
            } else if constexpr (std::is_same_v<T, RangeEdit>) {
                applyRange(o.topLeft, o.bottomRight, o.after);
            }
        }, op);
    }
    editStack = loadedStack;
    scripts = loadedScripts; // empty for v1 files, populated for v2

    return true;
}
