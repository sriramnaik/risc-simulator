// instructioninfo.h
#ifndef INSTRUCTIONINFOH_H
#define INSTRUCTIONINFOH_H

#include <QString>
#include <QMap>
#include <QStringList>

struct InstructionInfo {
    QString mnemonic;       // e.g., "ld"
    QString syntax;         // e.g., "ld rd, offset(rs1)"
    QString description;    // Brief description
    QString category;       // e.g., "Load/Store", "Arithmetic", "Branch"
    QStringList examples;   // e.g., ["ld x1, 0(x2)", "ld a0, 8(sp)"]
    QString format;         // e.g., "I-Type"

    InstructionInfo() = default;

    InstructionInfo(const QString& mn, const QString& syn, const QString& desc,
                    const QString& cat, const QStringList& ex = QStringList(),
                    const QString& fmt = "")
        : mnemonic(mn), syntax(syn), description(desc),
        category(cat), examples(ex), format(fmt) {}
};

class InstructionDatabase {
public:
    static InstructionDatabase& instance() {
        static InstructionDatabase db;
        return db;
    }

    void initialize();
    const InstructionInfo* getInfo(const QString& mnemonic) const;
    QStringList getAllMnemonics() const;
    QStringList getCompletions(const QString& prefix) const;
    QVector<InstructionInfo> searchInstructions(const QString& prefix) const;

private:
    InstructionDatabase() { initialize(); }
    QMap<QString, InstructionInfo> instructions_;

    void addInstruction(const InstructionInfo& info);
};

#endif
