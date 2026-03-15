/****************************************************************************
** Meta object code from reading C++ file 'SSMReader.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/SSMReader.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'SSMReader.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SSMReader_t {
    QByteArrayData data[11];
    char stringdata0[86];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SSMReader_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SSMReader_t qt_meta_stringdata_SSMReader = {
    {
QT_MOC_LITERAL(0, 0, 9), // "SSMReader"
QT_MOC_LITERAL(1, 10, 9), // "newSample"
QT_MOC_LITERAL(2, 20, 0), // ""
QT_MOC_LITERAL(3, 21, 10), // "inputLevel"
QT_MOC_LITERAL(4, 32, 6), // "seeing"
QT_MOC_LITERAL(5, 39, 7), // "rawLine"
QT_MOC_LITERAL(6, 47, 5), // "token"
QT_MOC_LITERAL(7, 53, 9), // "connected"
QT_MOC_LITERAL(8, 63, 12), // "disconnected"
QT_MOC_LITERAL(9, 76, 5), // "error"
QT_MOC_LITERAL(10, 82, 3) // "msg"

    },
    "SSMReader\0newSample\0\0inputLevel\0seeing\0"
    "rawLine\0token\0connected\0disconnected\0"
    "error\0msg"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SSMReader[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   39,    2, 0x06 /* Public */,
       5,    1,   44,    2, 0x06 /* Public */,
       7,    0,   47,    2, 0x06 /* Public */,
       8,    0,   48,    2, 0x06 /* Public */,
       9,    1,   49,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Double, QMetaType::Double,    3,    4,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   10,

       0        // eod
};

void SSMReader::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SSMReader *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->newSample((*reinterpret_cast< double(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2]))); break;
        case 1: _t->rawLine((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 2: _t->connected(); break;
        case 3: _t->disconnected(); break;
        case 4: _t->error((*reinterpret_cast< QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SSMReader::*)(double , double );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SSMReader::newSample)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (SSMReader::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SSMReader::rawLine)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (SSMReader::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SSMReader::connected)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (SSMReader::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SSMReader::disconnected)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (SSMReader::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SSMReader::error)) {
                *result = 4;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject SSMReader::staticMetaObject = { {
    QMetaObject::SuperData::link<QThread::staticMetaObject>(),
    qt_meta_stringdata_SSMReader.data,
    qt_meta_data_SSMReader,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *SSMReader::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SSMReader::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SSMReader.stringdata0))
        return static_cast<void*>(this);
    return QThread::qt_metacast(_clname);
}

int SSMReader::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void SSMReader::newSample(double _t1, double _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void SSMReader::rawLine(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void SSMReader::connected()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void SSMReader::disconnected()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void SSMReader::error(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
