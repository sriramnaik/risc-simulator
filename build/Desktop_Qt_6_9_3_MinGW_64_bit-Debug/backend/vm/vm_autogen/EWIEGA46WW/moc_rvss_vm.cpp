/****************************************************************************
** Meta object code from reading C++ file 'rvss_vm.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../../backend/vm/rvss_vm.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'rvss_vm.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.9.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN6RVSSVME_t {};
} // unnamed namespace

template <> constexpr inline auto RVSSVM::qt_create_metaobjectdata<qt_meta_tag_ZN6RVSSVME_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "RVSSVM",
        "gprUpdated",
        "",
        "index",
        "value",
        "csrUpdated",
        "fprUpdated",
        "memoryUpdated",
        "address",
        "QList<quint8>",
        "data",
        "vmError",
        "msg",
        "syscallOutput",
        "statusChanged"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'gprUpdated'
        QtMocHelpers::SignalData<void(int, quint64)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { QMetaType::ULongLong, 4 },
        }}),
        // Signal 'csrUpdated'
        QtMocHelpers::SignalData<void(int, quint64)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { QMetaType::ULongLong, 4 },
        }}),
        // Signal 'fprUpdated'
        QtMocHelpers::SignalData<void(int, quint64)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { QMetaType::ULongLong, 4 },
        }}),
        // Signal 'memoryUpdated'
        QtMocHelpers::SignalData<void(quint64, QVector<quint8>)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::ULongLong, 8 }, { 0x80000000 | 9, 10 },
        }}),
        // Signal 'vmError'
        QtMocHelpers::SignalData<void(const QString &)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 12 },
        }}),
        // Signal 'syscallOutput'
        QtMocHelpers::SignalData<void(const QString &)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 12 },
        }}),
        // Signal 'statusChanged'
        QtMocHelpers::SignalData<void(const QString &)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 12 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<RVSSVM, qt_meta_tag_ZN6RVSSVME_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject RVSSVM::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6RVSSVME_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6RVSSVME_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN6RVSSVME_t>.metaTypes,
    nullptr
} };

void RVSSVM::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<RVSSVM *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->gprUpdated((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<quint64>>(_a[2]))); break;
        case 1: _t->csrUpdated((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<quint64>>(_a[2]))); break;
        case 2: _t->fprUpdated((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<quint64>>(_a[2]))); break;
        case 3: _t->memoryUpdated((*reinterpret_cast< std::add_pointer_t<quint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QList<quint8>>>(_a[2]))); break;
        case 4: _t->vmError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->syscallOutput((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 6: _t->statusChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 1:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<quint8> >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (RVSSVM::*)(int , quint64 )>(_a, &RVSSVM::gprUpdated, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (RVSSVM::*)(int , quint64 )>(_a, &RVSSVM::csrUpdated, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (RVSSVM::*)(int , quint64 )>(_a, &RVSSVM::fprUpdated, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (RVSSVM::*)(quint64 , QVector<quint8> )>(_a, &RVSSVM::memoryUpdated, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (RVSSVM::*)(const QString & )>(_a, &RVSSVM::vmError, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (RVSSVM::*)(const QString & )>(_a, &RVSSVM::syscallOutput, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (RVSSVM::*)(const QString & )>(_a, &RVSSVM::statusChanged, 6))
            return;
    }
}

const QMetaObject *RVSSVM::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *RVSSVM::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6RVSSVME_t>.strings))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "VmBase"))
        return static_cast< VmBase*>(this);
    return QObject::qt_metacast(_clname);
}

int RVSSVM::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void RVSSVM::gprUpdated(int _t1, quint64 _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2);
}

// SIGNAL 1
void RVSSVM::csrUpdated(int _t1, quint64 _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}

// SIGNAL 2
void RVSSVM::fprUpdated(int _t1, quint64 _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2);
}

// SIGNAL 3
void RVSSVM::memoryUpdated(quint64 _t1, QVector<quint8> _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1, _t2);
}

// SIGNAL 4
void RVSSVM::vmError(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void RVSSVM::syscallOutput(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void RVSSVM::statusChanged(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1);
}
QT_WARNING_POP
