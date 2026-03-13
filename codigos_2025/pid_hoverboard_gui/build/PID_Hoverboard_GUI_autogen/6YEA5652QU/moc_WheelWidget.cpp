/****************************************************************************
** Meta object code from reading C++ file 'WheelWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../include/WheelWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'WheelWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_WheelWidget_t {
    QByteArrayData data[11];
    char stringdata0[146];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_WheelWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_WheelWidget_t qt_meta_stringdata_WheelWidget = {
    {
QT_MOC_LITERAL(0, 0, 11), // "WheelWidget"
QT_MOC_LITERAL(1, 12, 10), // "regulation"
QT_MOC_LITERAL(2, 23, 0), // ""
QT_MOC_LITERAL(3, 24, 3), // "val"
QT_MOC_LITERAL(4, 28, 11), // "onWheelData"
QT_MOC_LITERAL(5, 40, 9), // "speedByte"
QT_MOC_LITERAL(6, 50, 20), // "onTargetSpeedChanged"
QT_MOC_LITERAL(7, 71, 19), // "onPropFactorChanged"
QT_MOC_LITERAL(8, 91, 18), // "onIntFactorChanged"
QT_MOC_LITERAL(9, 110, 19), // "onDiffFactorChanged"
QT_MOC_LITERAL(10, 130, 15) // "onDataFrequency"

    },
    "WheelWidget\0regulation\0\0val\0onWheelData\0"
    "speedByte\0onTargetSpeedChanged\0"
    "onPropFactorChanged\0onIntFactorChanged\0"
    "onDiffFactorChanged\0onDataFrequency"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_WheelWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   49,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    1,   52,    2, 0x0a /* Public */,
       6,    1,   55,    2, 0x0a /* Public */,
       7,    1,   58,    2, 0x0a /* Public */,
       8,    1,   61,    2, 0x0a /* Public */,
       9,    1,   64,    2, 0x0a /* Public */,
      10,    1,   67,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::UChar,    3,

 // slots: parameters
    QMetaType::Void, QMetaType::UChar,    5,
    QMetaType::Void, QMetaType::Double,    3,
    QMetaType::Void, QMetaType::Double,    3,
    QMetaType::Void, QMetaType::Double,    3,
    QMetaType::Void, QMetaType::Double,    3,
    QMetaType::Void, QMetaType::Double,    3,

       0        // eod
};

void WheelWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<WheelWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->regulation((*reinterpret_cast< quint8(*)>(_a[1]))); break;
        case 1: _t->onWheelData((*reinterpret_cast< quint8(*)>(_a[1]))); break;
        case 2: _t->onTargetSpeedChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 3: _t->onPropFactorChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 4: _t->onIntFactorChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 5: _t->onDiffFactorChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 6: _t->onDataFrequency((*reinterpret_cast< double(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (WheelWidget::*)(quint8 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&WheelWidget::regulation)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject WheelWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_WheelWidget.data,
    qt_meta_data_WheelWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *WheelWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *WheelWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_WheelWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int WheelWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void WheelWidget::regulation(quint8 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
