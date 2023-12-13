
#include "console_window.h"
#include "qapplication.h"
#include "qevent.h"
#include <QClipboard>
#include <QScrollBar>

Console::Console(QWidget *parent) :
    QTextEdit(parent)
{
    QSizePolicy qsp = QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    qsp.setVerticalStretch(4);
    setSizePolicy(qsp);
    setAcceptRichText(false);
//    setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
//    get

//    setSizeAdjustPolicy()
//    document()->setMaximumBlockCount(100);
    QPalette p = palette();
    p.setColor(QPalette::Base, Qt::black);
    p.setColor(QPalette::Text, Qt::green);
    setPalette(p);
}

void Console::putData(const QByteArray &data)
{

    QScrollBar *bar = verticalScrollBar();
    bar->setValue(bar->maximum());
    QTextCursor cursor(textCursor());
    cursor.movePosition(QTextCursor::End);
    setTextCursor(cursor);
    insertPlainText(data);

//    QScrollBar *bar = verticalScrollBar();
//    bar->setValue(bar->maximum());
}

void Console::setLocalEchoEnabled(bool set)
{
    m_localEchoEnabled = set;
}

void Console::keyPressEventProxy(QKeyEvent *e){
    keyPressEvent(e);
}

void Console::keyPressEvent(QKeyEvent *e)
{
    QTextCursor cursor(textCursor());
    switch (e->key()) {
    case Qt::Key_Paste:
//        putData(QApplication::clipboard()->text().toLocal8Bit());
        emit getData(QApplication::clipboard()->text().toLocal8Bit());
    case Qt::Key_Backspace:
        cursor.movePosition(QTextCursor::End);
        setTextCursor(cursor);
        textCursor().deletePreviousChar();
        emit getData(e->text().toLocal8Bit());
        break;
//    case Qt::Key_Left:
//    case Qt::Key_Right:
    case Qt::Key_Up:
        emit getData(QString("\e[A").toLocal8Bit());
        break;
    case Qt::Key_Down:
        emit getData(QString("\e[B").toLocal8Bit());
        break;
    default:
//        if (false)
//            msg = QTextEdit::keyPressEvent(e);
//            QTextEdit::keyPressEvent(e);
//            text
        if(e->key() == Qt::Key_V && e->modifiers().testFlag(Qt::ControlModifier)){
//            putData("pasting");
            emit getData(QApplication::clipboard()->text().toLocal8Bit());
            return;
        }
        else if(e->key() == Qt::Key_L && e->modifiers().testFlag(Qt::ControlModifier)){
            clear();
        }
        emit getData(e->text().toLocal8Bit());
    }
}

//void Console::mousePressEvent(QMouseEvent *e)
//{
//    Q_UNUSED(e);
//    setFocus();
//}

//void Console::mouseDoubleClickEvent(QMouseEvent *e)
//{
////    Q_UNUSED(e);
//}

//void Console::contextMenuEvent(QContextMenuEvent *e)
//{
//    QMenu* menu = createStandardContextMenu();
//    menu->removeAction(QAction::);
//}


