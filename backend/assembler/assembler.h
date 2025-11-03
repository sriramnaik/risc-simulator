

#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "../vm_asm_mw.h"
#include "../vm/registers.h"
#include <QObject>
#include <QVector>
// #include <variant>

class Assembler : public QObject
{
    Q_OBJECT

public:
    explicit Assembler(RegisterFile* regs, QObject* parent = nullptr);
    AssembledProgram assemble(const std::string &filename);


private:
    RegisterFile* registers_;
signals:
    void errorsAvailable(const QVector<std::string> &errors);
};


#endif // ASSEMBLER_H
